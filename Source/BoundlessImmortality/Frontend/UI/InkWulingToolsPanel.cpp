#include "InkWulingToolsPanel.h"
#include "Frontend/InkFrontendWidget.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"

namespace {
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(K,V)?*V:nullptr;}
FString Str(const TSharedPtr<FJsonObject>& O,const TCHAR* K){FString V;if(O)O->TryGetStringField(K,V);return V;}
double Num(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;if(O)O->TryGetNumberField(K,V);return V;}
FString Json(const TSharedPtr<FJsonObject>& O){FString S;if(O)FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&S));return S;}
FString CatalogIdentity(const TSharedPtr<FJsonObject>& O,const FString& Kind)
{
    if(Kind==TEXT("recipes"))return Str(O,TEXT("recipe_id"));
    if(Kind==TEXT("sources"))return Str(O,TEXT("source_id"))+TEXT("|")+Str(O,TEXT("item_id"));
    if(Kind==TEXT("buildings"))return Str(O,TEXT("building_id"))+FString::Printf(TEXT("|%.0f"),Num(O,TEXT("level")));
    if(Kind==TEXT("shop"))return Str(O,TEXT("sku"));
    if(Kind==TEXT("market"))return Str(O,TEXT("id"));
    return Str(O,TEXT("item_id"));
}
FString ShopCosts(const TSharedPtr<FJsonObject>& O)
{
    FString Text=FString::Printf(TEXT("%d灵石"),int32(Num(O,TEXT("stone_cost"))));
    static const TMap<FString,FString> Names={{TEXT("MAT_HERB_B"),TEXT("灵草")},{TEXT("MAT_IRON_B"),TEXT("玄铁")},{TEXT("MAT_PAGE"),TEXT("残卷")},{TEXT("MAT_BREAK"),TEXT("破境石")},{TEXT("MAT_CORE"),TEXT("灵核")},{TEXT("STAT_CONTRIB"),TEXT("贡献")}};
    for(int32 I=1;I<=2;++I)
    {
        const FString Item=Str(O,*FString::Printf(TEXT("extra_item_%d"),I));const int32 Count=Num(O,*FString::Printf(TEXT("extra_count_%d"),I));
        if(Count>0)Text+=FString::Printf(TEXT(" · %s×%d"),*Names.FindRef(Item),Count);
    }
    return Text;
}
FString RenderStateKey(const TSharedPtr<FJsonObject>& S,bool Busy)
{
    const auto C=Obj(S,TEXT("catalog")),T=Obj(S,TEXT("tribulation")),R=Obj(S,TEXT("race"));const FString Phase=Str(T,TEXT("status")),Race=Str(R,TEXT("status"));
    const bool Live=Phase==TEXT("Prepare")||Phase==TEXT("Trial")||Phase==TEXT("Reconcile")||Race==TEXT("Ready")||Race==TEXT("Running")||Race==TEXT("Frozen");
    return FString::Printf(TEXT("%.0f|%s|%.0f|%.0f|%d|%s|%s|%.0f"),Num(S,TEXT("version")),*Str(C,TEXT("kind")),Num(C,TEXT("offset")),Num(T,TEXT("helper_count")),Busy,*Phase,*Race,Live?FMath::FloorToDouble(Num(S,TEXT("server_utc"))):0);
}
const TCHAR* Routes[]={TEXT("WulingCraft"),TEXT("WulingSources"),TEXT("WulingBreakthrough"),TEXT("WulingEquipment"),TEXT("WulingConstruction"),TEXT("WulingActivity"),TEXT("WulingRace"),TEXT("WulingShop"),TEXT("WulingMarket")};
const TCHAR* Titles[]={TEXT("物品炼制"),TEXT("物品来源"),TEXT("突破与天劫"),TEXT("装备与功法"),TEXT("武灵建设"),TEXT("签到与开服"),TEXT("灵脉竞采"),TEXT("原型商店"),TEXT("原型寄售")};
UWulingPrototypeSubsystem* Service(const UUserWidget* W){return W->GetGameInstance()?W->GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;}
}
FText UInkWulingToolsPanel::PanelTitle()const
{for(int32 I=0;I<9;++I)if(PageId==Routes[I])return FText::FromString(Titles[I]);return FText::FromString(TEXT("武灵功能"));}
FString UInkWulingToolsPanel::CatalogKind()const
{
    if(PageId==TEXT("WulingCraft"))return TEXT("recipes");if(PageId==TEXT("WulingSources"))return TEXT("sources");
    if(PageId==TEXT("WulingEquipment"))return SelectedEntry.StartsWith(TEXT("methods"))?TEXT("methods"):(SelectedEntry==TEXT("props")?TEXT("props"):TEXT("equipment"));
    if(PageId==TEXT("WulingConstruction"))return TEXT("buildings");if(PageId==TEXT("WulingShop"))return TEXT("shop");
    if(PageId==TEXT("WulingMarket"))return SelectedEntry==TEXT("sell")?TEXT("items"):TEXT("market");return TEXT("items");
}
FString UInkWulingToolsPanel::SelectionJson()const
{
    TArray<TSharedPtr<FJsonValue>> Rows;for(const auto& P:MaterialSelection){auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("item_id"),P.Key);O->SetNumberField(TEXT("count"),P.Value);Rows.Add(MakeShared<FJsonValueObject>(O));}
    FString S;FJsonSerializer::Serialize(Rows,TJsonWriterFactory<>::Create(&S));return S;
}
void UInkWulingToolsPanel::BuildPanelContent()
{
    bBuilding=true;
    const float OldCatalogOffset=CatalogScroll?CatalogScroll->GetScrollOffset():0;
    const float OldDetailOffset=DetailScroll?DetailScroll->GetScrollOffset():0;
    CatalogScroll=nullptr;DetailScroll=nullptr;DisplayRows.Reset();
    auto* W=Service(this);const auto State=W?W->Snapshot():nullptr;
    const bool Active=FrontendOwner&&FrontendOwner->CurrentPage==PageId;
    const bool Ready=Active&&W&&W->IsEnabled()&&W->IsConnected()&&!W->IsBusy();
    const float Gap=16,Hit=88;
    Label(BodyCanvas,TEXT("WulingToolStatus"),W?W->GetStatus():TEXT("服务未连接"),0,4,BodyWidth-180,80,24);
    Action(BodyCanvas,TEXT("ToolNavigation"),bShowNavigation?TEXT("收起目录"):TEXT("玩法目录"),BodyWidth-168,0,168,Hit);
    float Top=104;
    auto ScrollArea=[&](const TCHAR* Name,float X,float Y,float Width,float Height,float ContentHeight,TObjectPtr<UScrollBox>& Saved,float Offset)
    {
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),Name);
        Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6));
        auto* ToolScrollLayout=BodyCanvas->AddChildToCanvas(Scroll);ToolScrollLayout->SetPosition({X,Y});ToolScrollLayout->SetSize({Width,Height});
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Width-10);Size->SetHeightOverride(ContentHeight);Scroll->AddChild(Size);
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(Canvas);Saved=Scroll;Scroll->SetScrollOffset(Offset);return Canvas;
    };
    if(bShowNavigation)
    {
        const int32 Columns=bPortraitLayout?2:3;const float CW=(BodyWidth-10-(Columns-1)*12)/Columns;
        auto* Menu=ScrollArea(TEXT("WulingNavigationScroll"),0,Top,BodyWidth,FMath::Max(88.f,BodyHeight-Top),FMath::DivideAndRoundUp(9,Columns)*100,CatalogScroll,0);
        for(int32 I=0;I<9;++I)Action(Menu,TEXT("ToolRoute_")+FString::FromInt(I),Titles[I],(I%Columns)*(CW+12),(I/Columns)*100,CW,Hit,false,false,PageId==Routes[I]);
        RenderKey=RenderStateKey(State,!Ready)+(Active?TEXT("active"):TEXT("inactive"));bBuilding=false;return;
    }
    const auto Catalog=Obj(State,TEXT("catalog"));const FString Kind=CatalogKind();
    if(Ready&&PageId!=TEXT("WulingActivity")&&PageId!=TEXT("WulingRace")&&(Str(Catalog,TEXT("kind"))!=Kind||int32(Num(Catalog,TEXT("offset")))!=Offset))W->RequestCatalog(Kind,Offset,bOwnedOnly||PageId==TEXT("WulingBreakthrough")||(PageId==TEXT("WulingMarket")&&SelectedEntry==TEXT("sell")));
    if(PageId==TEXT("WulingActivity"))
    {
        const auto A=Obj(State,TEXT("activities")),L=Obj(State,TEXT("launch")),Personal=Obj(L,TEXT("personal"));
        const TCHAR* Tabs[]={TEXT("overview"),TEXT("tasks"),TEXT("guild"),TEXT("rank")};const TCHAR* Names[]={TEXT("活动与签到"),TEXT("七项任务"),TEXT("共同目标"),TEXT("贡献荣誉榜")};
        for(int32 I=0;I<4;++I)Action(BodyCanvas,TEXT("ToolActivityTab_")+FString(Tabs[I]),Names[I],(I%(bPortraitLayout?2:4))*(BodyWidth+12)/(bPortraitLayout?2:4),Top+(bPortraitLayout?I/2:0)*100,(BodyWidth-(bPortraitLayout?12:36))/(bPortraitLayout?2:4),88,false,false,ActivityTab==Tabs[I]);
        FString Detail;
        if(ActivityTab==TEXT("tasks"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;const TMap<FString,FString> Conditions={{TEXT("join_and_free_response"),TEXT("正式加入并免费响应")},{TEXT("complete_world_rounds"),TEXT("完整参与世界历练")},{TEXT("correct_guard_response"),TEXT("正确护阵")},{TEXT("correct_break_response"),TEXT("正确破障")},{TEXT("route_vote_in_distinct_rounds"),TEXT("不同轮次路线选择")}};
            if(L&&L->TryGetArrayField(TEXT("tasks"),Rows))for(const auto& V:*Rows){auto O=V->AsObject();bool Confirmed=false;O->TryGetBoolField(TEXT("confirmed"),Confirmed);Detail+=FString::Printf(TEXT("%s %s · %.0f/%.0f · %.0f修为%s\n"),*Str(O,TEXT("task_id")),*Conditions.FindRef(Str(O,TEXT("condition_type"))),Num(O,TEXT("progress")),Num(O,TEXT("target")),Num(O,TEXT("reward_count")),Confirmed?TEXT(" · 已结算"):TEXT(""));}
            else Detail=TEXT("尚未开启青云初启，开启后显示真实任务记录。");
        }
        else if(ActivityTab==TEXT("guild"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Completed=nullptr;const int32 Count=L&&L->TryGetArrayField(TEXT("complete_rounds"),Completed)?Completed->Num():0;
            Detail=FString::Printf(TEXT("主播和正式成员完整且免费参与：%d 轮\n1轮：20灵石 / 6破境石\n3轮：40灵石 / 12破境石\n6轮：60灵石 / 18破境石\n12轮：80灵石 / 24破境石\n最终20分钟：正式同房成员修为、剑伤+10%%，主播防御+10%%"),Count);
        }
        else if(ActivityTab==TEXT("rank"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;Detail=TEXT("贡献不按礼物计分 · 无数值排行奖励\n");
            if(L&&L->TryGetArrayField(TEXT("leaderboard"),Rows)&&Rows->Num())for(int32 I=RankOffset;I<FMath::Min(Rows->Num(),RankOffset+8);++I){auto O=(*Rows)[I]->AsObject();Detail+=FString::Printf(TEXT("第%.0f名 · %s · %.0f分\n"),Num(O,TEXT("rank")),*Str(O,TEXT("name")),Num(O,TEXT("score")));}else Detail+=TEXT("暂无满足30分且完整参与2轮的成员");
        }
        else Detail=FString::Printf(TEXT("服务日期：%s · 本月签到 %.0f 日\n%s\n开服剩余 %.0f 分钟 · 个人贡献 %.0f / 完整轮次 %.0f\n七任务全成获得青云踏飞雁，退出武灵仍保留原期限。\n观众弹幕：展示青云踏飞雁 / 隐藏荣誉称号"),*Str(A,TEXT("day")),Num(A,TEXT("month_sign_count")),*Str(A,TEXT("verse")),FMath::Max(0.,(Num(L,TEXT("end"))-Num(State,TEXT("server_utc")))/60),Num(Personal,TEXT("score")),Num(Personal,TEXT("complete_count")));

        TArray<FString> ActivityLines;Detail.ParseIntoArrayLines(ActivityLines,false);int32 ActivityLineCount=0;const int32 ActivityCapacity=FMath::Max(8,int32((BodyWidth-34)/26));for(const FString& Line:ActivityLines)ActivityLineCount+=FMath::Max(1,FMath::DivideAndRoundUp(Line.Len(),ActivityCapacity));
        const float ActivityTextHeight=FMath::Max(320.f,ActivityLineCount*38.f+24);
        const float DetailTop=Top+(bPortraitLayout?208:100),Footer=BodyHeight-88;
        auto* ActivityCanvas=ScrollArea(TEXT("WulingActivityScroll"),0,DetailTop,BodyWidth,FMath::Max(88.f,Footer-DetailTop-12),ActivityTextHeight+(ActivityTab==TEXT("rank")?112:12),DetailScroll,OldDetailOffset);
        Label(ActivityCanvas,TEXT("WulingActivityData"),Detail,8,8,BodyWidth-34,ActivityTextHeight,26);
        if(ActivityTab==TEXT("rank")){Action(ActivityCanvas,TEXT("ToolRankPrevious"),TEXT("上一页"),0,ActivityTextHeight+12,(BodyWidth-12)/2,Hit);Action(ActivityCanvas,TEXT("ToolRankNext"),TEXT("下一页"),(BodyWidth+12)/2,ActivityTextHeight+12,(BodyWidth-32)/2,Hit);}
        bool Signed=false;if(A)A->TryGetBoolField(TEXT("signed"),Signed);
        Action(BodyCanvas,TEXT("ToolSign"),Signed?TEXT("今日已签到"):TEXT("签到记录"),0,Footer,(BodyWidth-12)/2,Hit,true)->SetIsEnabled(Ready&&!Signed);
        Action(BodyCanvas,TEXT("ToolLaunch"),TEXT("开启青云初启"),(BodyWidth+12)/2,Footer,(BodyWidth-12)/2,Hit,true)->SetIsEnabled(Ready&&!L);
    }
    else if(PageId==TEXT("WulingRace"))
    {
        const auto R=Obj(State,TEXT("race")),L=Obj(State,TEXT("lease"));
        const float LW=bPortraitLayout?BodyWidth:(BodyWidth-Gap)*.48f,RX=bPortraitLayout?0:LW+Gap,RW=BodyWidth-RX;
        const float Footer=BodyHeight-88,AreaH=Footer-Top-12;
        auto* Summary=ScrollArea(TEXT("WulingRaceSummaryScroll"),0,Top,LW,AreaH,bPortraitLayout?832:320,DetailScroll,OldDetailOffset);
        Label(Summary,TEXT("WulingRaceData"),FString::Printf(TEXT("阶段：%s\n共同600 / 主播200 / 成员200\n挑战期：第2至8分钟\n主播：挑战第90 / 270秒前后10秒\n成员：挑战第60 / 240秒起20秒\n开采权剩余 %.0f 分钟 · 在线 %.0f 秒"),*Str(R,TEXT("status")),FMath::Max(0.,Num(L,TEXT("end"))-Num(State,TEXT("server_utc")))/60,Num(L,TEXT("online_seconds"))),0,0,LW-16,300,24);
        auto* Choices=bPortraitLayout?Summary:ScrollArea(TEXT("WulingRaceChoicesScroll"),RX,Top,RW,AreaH,512,CatalogScroll,OldCatalogOffset);
        const float ChoiceTop=bPortraitLayout?320:0,CW=(RW-26)/2;
        const TCHAR* Nodes[]={TEXT("NODE_QI"),TEXT("NODE_STONE"),TEXT("NODE_HERB"),TEXT("NODE_IRON"),TEXT("NODE_PAGE")};const TCHAR* Names[]={TEXT("灵泉"),TEXT("灵石矿"),TEXT("灵草园"),TEXT("玄铁脉"),TEXT("残卷遗址")};
        for(int32 I=0;I<5;++I)Action(Choices,TEXT("ToolQueue_")+FString(Nodes[I]),Names[I],(I%2)*(CW+12),ChoiceTop+(I/2)*100,CW,Hit)->SetIsEnabled(Ready);
        Action(Choices,TEXT("ToolRaceAnchor"),TEXT("主播时机"),0,ChoiceTop+312,CW,Hit)->SetIsEnabled(Ready);
        Action(Choices,TEXT("ToolRaceCancel"),TEXT("取消排队"),CW+12,ChoiceTop+312,CW,Hit)->SetIsEnabled(Ready);
        Action(BodyCanvas,TEXT("ToolRaceGuard"),TEXT("共同护阵"),0,Footer,(BodyWidth-12)/2,Hit,true)->SetIsEnabled(Ready);
        Action(BodyCanvas,TEXT("ToolRaceBreak"),TEXT("共同破障"),(BodyWidth+12)/2,Footer,(BodyWidth-12)/2,Hit)->SetIsEnabled(Ready);
    }
    else
    {
        const bool BreakPage=PageId==TEXT("WulingBreakthrough");
        if(PageId==TEXT("WulingEquipment"))
        {
            const float CW=(BodyWidth-24)/3;
            const TCHAR* Views[]={TEXT("equipment"),TEXT("methods"),TEXT("props")};const TCHAR* Names[]={TEXT("装备法宝"),TEXT("功法"),TEXT("道具")};
            for(int I=0;I<3;++I)Action(BodyCanvas,TEXT("ToolView_")+FString(Views[I]),Names[I],I*(CW+12),Top,CW,Hit,false,false,Kind==Views[I]);Top+=100;
        }
        else if(PageId==TEXT("WulingMarket"))
        {
            Action(BodyCanvas,TEXT("ToolMarketBrowse"),TEXT("浏览购买"),0,Top,(BodyWidth-12)/2,Hit,false,false,Kind==TEXT("market"));
            Action(BodyCanvas,TEXT("ToolMarketSell"),TEXT("出售原料"),(BodyWidth+12)/2,Top,(BodyWidth-12)/2,Hit,false,false,Kind==TEXT("items"));Top+=100;
        }
        const TArray<TSharedPtr<FJsonValue>>* FreshRows=nullptr;
        if(SelectedData&&Str(Catalog,TEXT("kind"))==Kind&&Catalog->TryGetArrayField(TEXT("rows"),FreshRows))for(const auto& Value:*FreshRows){const auto Fresh=Value->AsObject();if(Fresh&&CatalogIdentity(Fresh,Kind)==CatalogIdentity(SelectedData,Kind)){SelectedData=Fresh;break;}}
        const float Footer=BodyHeight-88,ListW=bPortraitLayout?BodyWidth:(BodyWidth-Gap)*(Kind==TEXT("shop")?.64f:.52f);
        const float DetailX=bPortraitLayout?0:ListW+Gap,DetailW=BodyWidth-DetailX;
        const float DetailH=bPortraitLayout?FMath::Clamp((Footer-Top)*.34f,120.f,236.f):Footer-Top-12,ListTop=bPortraitLayout?Top+DetailH+16:Top;
        const float ListH=FMath::Max(88.f,Footer-ListTop-12);
        FString Details=TEXT("选择条目后查看条件、消耗与当前可执行操作。");
        FString Primary=TEXT("确认执行");
        if(BreakPage)
        {
            const auto T=Obj(State,TEXT("tribulation")),Q=Obj(State,TEXT("breakthrough_quote"));
            Details=FString::Printf(TEXT("突破准备 · 已选 %d 类材料\n确定品质 Q%.0f · %s\n助力 %.0f 人 · %s\n成功率 %.0f%% · 试炼剩余 %.0f 秒\n失败最多扣1500修为，材料保留；连续失败公开保底。"),MaterialSelection.Num(),Num(Q,TEXT("q")),*Str(Obj(Q,TEXT("band")),TEXT("name")),Num(T,TEXT("helper_count")),*Str(T,TEXT("status")),(T?Num(T,TEXT("probability_basis_points")):Num(Q,TEXT("probability_basis_points")))/100,FMath::Max(0.,Num(T,TEXT("deadline"))-Num(State,TEXT("server_utc"))));
        }
        else if(SelectedData)
        {
            Details=Str(SelectedData,TEXT("name"))+TEXT("\n");const TArray<TSharedPtr<FJsonValue>>* Inputs=nullptr;
            if(SelectedData->TryGetArrayField(TEXT("ingredients"),Inputs))
            {
                Details+=FString::Printf(TEXT("本次炼制一份 · %.0f 灵石\n所需材料\n"),Num(SelectedData,TEXT("stone_cost")));
                for(const auto& V:*Inputs){auto O=V->AsObject();Details+=Str(O,TEXT("name"))+FString::Printf(TEXT(" ×%.0f\n"),Num(O,TEXT("count")));}Primary=TEXT("确认炼制");
            }
            else if(Kind==TEXT("sources")){const auto Source=Obj(SelectedData,TEXT("source"));Details+=TEXT("可达来源：")+Str(Source,TEXT("region"))+TEXT(" · ")+Str(Source,TEXT("name"))+FString::Printf(TEXT("\n每份需要 %.0f 完整轮次\n"),Num(SelectedData,TEXT("required_full_rounds")))+TEXT("开放条件：")+Str(Source,TEXT("unlock_condition"))+TEXT("\n配方来源可查阅说明；固定奖励可设为专注目标。");Primary=TEXT("设为专注目标");}
            else if(Kind==TEXT("shop")){Details+=TEXT("本次精确消耗：")+ShopCosts(SelectedData)+FString::Printf(TEXT("\n获得 %.0f 份 · 每日上限 %.0f\n"),Num(SelectedData,TEXT("output_count")),Num(SelectedData,TEXT("daily_limit")))+TEXT("余额、购买资格和限额以服务确认结果为准。");Primary=TEXT("确认购买一份");}
            else if(Kind==TEXT("buildings")){Details+=FString::Printf(TEXT("设施等级 %.0f → %.0f\n公库资材 %.0f\n公库材料：%s ×%.0f\n%s ×%.0f\n仅主播可建设，服务核验当前等级与公库库存。"),Num(SelectedData,TEXT("prerequisite_level")),Num(SelectedData,TEXT("level")),Num(SelectedData,TEXT("supply_cost")),*Str(SelectedData,TEXT("material_1")),Num(SelectedData,TEXT("material_1_count")),*Str(SelectedData,TEXT("material_2")),Num(SelectedData,TEXT("material_2_count")));Details+=TEXT("\n建设收益：")+Str(SelectedData,TEXT("benefit"))+TEXT("\n开放条件：")+Str(SelectedData,TEXT("unlock"));Primary=TEXT("确认建设");}
            else if(Kind==TEXT("market")){bool Own=false;SelectedData->TryGetBoolField(TEXT("is_own"),Own);Details+=FString::Printf(TEXT("数量 %.0f · 单价 %.0f 灵石\n成交总价 %.0f 灵石\n出售到账扣除5%%手续费；期限不延长。"),Num(SelectedData,TEXT("count")),Num(SelectedData,TEXT("unit_price")),Num(SelectedData,TEXT("count"))*Num(SelectedData,TEXT("unit_price")));const int32 Total=int32(Num(SelectedData,TEXT("count"))*Num(SelectedData,TEXT("unit_price"))),Tax=(Total+19)/20;Details+=FString::Printf(TEXT("\n卖方手续费 %d · 实收 %d 灵石"),Tax,Total-Tax);Primary=Own?TEXT("撤回自己的寄售"):TEXT("确认购买寄售");}
            else if(PageId==TEXT("WulingMarket")){Details+=FString::Printf(TEXT("上架数量 %d · 单价 %d 灵石\n总价 %d 灵石 · 成交扣5%%手续费\n可用库存 %.0f"),ListingQuantity,ListingPrice,ListingQuantity*ListingPrice,Num(SelectedData,TEXT("owned_count")));const int32 Total=ListingQuantity*ListingPrice,Tax=(Total+19)/20;Details+=FString::Printf(TEXT("\n上架立即扣费 1 灵石\n成交手续费 %d · 成交实收 %d 灵石"),Tax,Total-Tax);Primary=TEXT("确认上架");}
            else
            {
                Details+=FString::Printf(TEXT("可用库存 %.0f\n"),Num(SelectedData,TEXT("owned_count")));
                const auto Loadout=Obj(State,TEXT("loadout"));
                if(Kind==TEXT("equipment"))
                {
                    const FString EquipmentSlot=Str(SelectedData,TEXT("slot")),Current=EquipmentSlot.IsEmpty()?Str(Loadout,TEXT("fabao")):Str(Obj(Loadout,TEXT("equipment")),*EquipmentSlot);
                    Details+=TEXT("当前槽位：")+(Current.IsEmpty()?TEXT("空槽"):Current)+TEXT("\n");
                    const TMap<FString,FString> Stats={{TEXT("health_add"),TEXT("血量")},{TEXT("defense_add"),TEXT("防御")},{TEXT("attack_add"),TEXT("攻击")},{TEXT("move_speed_add"),TEXT("移动速度")}};
                    if(!EquipmentSlot.IsEmpty())Details+=Stats.FindRef(Str(SelectedData,TEXT("stat")))+FString::Printf(TEXT(" +%.0f%%\n"),Num(SelectedData,TEXT("additive_value"))*100);
                    else Details+=Str(SelectedData,TEXT("description"))+FString::Printf(TEXT("\n冷却 %.0f 秒"),Num(SelectedData,TEXT("cooldown_seconds")));
                }
                else if(Kind==TEXT("methods"))Details+=TEXT("当前主修：")+Str(Obj(Loadout,TEXT("method_names")),*Str(Loadout,TEXT("main")))+TEXT("\n主修效果：")+Str(SelectedData,TEXT("main_mechanic"))+TEXT("\n辅修效果：")+Str(SelectedData,TEXT("support_mechanic"))+TEXT("\n")+Str(SelectedData,TEXT("stacking"));
                else Details+=Str(SelectedData,TEXT("description"));
                Primary=Kind==TEXT("equipment")?TEXT("确认装备"):Kind==TEXT("methods")?TEXT("研习所选功法"):TEXT("使用所选道具");
            }
        }
        Details+=FString::Printf(TEXT("\n第 %d 页 · 目录共 %.0f 项"),Offset/8+1,Num(Catalog,TEXT("total")));
        Details.ReplaceInline(TEXT("生命"),TEXT("血量"));
        TArray<FString> DetailLines;Details.ParseIntoArrayLines(DetailLines,false);int32 LineCount=0;const int32 LineCapacity=FMath::Max(8,int32((DetailW-34)/26));for(const FString& Line:DetailLines)LineCount+=FMath::Max(1,FMath::DivideAndRoundUp(Line.Len(),LineCapacity));
        const float TextHeight=FMath::Max(164.f,LineCount*38.f+24);
        const float ExtraHeight=BreakPage||PageId==TEXT("WulingEquipment")||PageId==TEXT("WulingMarket")?212.f:0.f;
        auto* DetailsCanvas=ScrollArea(TEXT("WulingDetailsScroll"),DetailX,Top,DetailW,DetailH,TextHeight+ExtraHeight+12,DetailScroll,OldDetailOffset);
        Label(DetailsCanvas,BreakPage?TEXT("WulingBreakData"):TEXT("WulingSelectedDetails"),Details,8,0,DetailW-34,TextHeight,26);
        if(BreakPage)
        {
            Action(DetailsCanvas,TEXT("ToolClearSelection"),TEXT("清空材料"),0,TextHeight+12,DetailW-16,Hit);
            Action(DetailsCanvas,TEXT("ToolReworkMode"),bRework?TEXT("重炼模式"):TEXT("突破模式"),0,TextHeight+112,DetailW-16,Hit,false,false,bRework);
        }
        else if(PageId==TEXT("WulingEquipment"))
        {
            if(Kind==TEXT("methods")){Action(DetailsCanvas,TEXT("ToolSetMain"),TEXT("设为主修"),0,TextHeight+12,DetailW-16,Hit)->SetIsEnabled(Ready&&SelectedData);Action(DetailsCanvas,TEXT("ToolAddSupport"),TEXT("添加辅修"),0,TextHeight+112,DetailW-16,Hit)->SetIsEnabled(Ready&&SelectedData);}
            else{Action(DetailsCanvas,TEXT("ToolFabao"),TEXT("使用已装备法宝"),0,TextHeight+12,DetailW-16,Hit)->SetIsEnabled(Ready);Action(DetailsCanvas,TEXT("ToolMethod"),TEXT("施展主修功法"),0,TextHeight+112,DetailW-16,Hit)->SetIsEnabled(Ready);}
        }
        else if(PageId==TEXT("WulingMarket")&&Kind==TEXT("items"))
        {
            Action(DetailsCanvas,TEXT("ToolQuantity"),FString::Printf(TEXT("数量 %d ＋"),ListingQuantity),0,TextHeight+12,DetailW-16,Hit);
            Action(DetailsCanvas,TEXT("ToolPrice"),FString::Printf(TEXT("单价 %d ＋"),ListingPrice),0,TextHeight+112,DetailW-16,Hit);
        }
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        const bool HasCatalog=Str(Catalog,TEXT("kind"))==Kind&&int32(Num(Catalog,TEXT("offset")))==Offset&&Catalog&&Catalog->TryGetArrayField(TEXT("rows"),Rows);
        const int32 Count=HasCatalog?Rows->Num():0,Columns=Kind==TEXT("shop")?(bPortraitLayout?2:3):1;
        const float RowH=Kind==TEXT("shop")?300:128,CW=(ListW-10-(Columns-1)*12)/Columns;
        auto* List=ScrollArea(TEXT("WulingCatalogScroll"),0,ListTop,ListW,ListH,FMath::Max(132.f,FMath::DivideAndRoundUp(Count,Columns)*RowH)+100,CatalogScroll,OldCatalogOffset);
        if(!HasCatalog||Count==0)Label(List,TEXT("WulingCatalogEmpty"),HasCatalog?TEXT("当前没有可用条目"):TEXT("目录待同步，请稍候"),8,8,ListW-34,112,26);
        for(int32 I=0;I<Count;++I)
        {
            auto O=(*Rows)[I]->AsObject();if(!O)continue;DisplayRows.Add(O);const FString Id=TEXT("ToolRow_")+FString::FromInt(DisplayRows.Num()-1);
            const float X=(I%Columns)*(CW+12),Y=(I/Columns)*RowH;
            if(Kind==TEXT("shop"))
            {
                const FString Item=Str(O,TEXT("output_item"));const int32 Tile=Item==TEXT("ITEM_HEAL")?10:Item==TEXT("ITEM_SHIELD")||Item.Contains(TEXT("PAGE"))||Item.Contains(TEXT("BOOK"))?9:8;
                AtlasIcon(List,NAME_None,Tile,X+8,Y+8,64);
                Label(List,NAME_None,Str(O,TEXT("name")),X+80,Y+8,CW-88,76,26);
                Label(List,NAME_None,ShopCosts(O)+FString::Printf(TEXT("\n每日上限 %.0f"),Num(O,TEXT("daily_limit"))),X+8,Y+88,CW-16,104,24);
                Action(List,Id,TEXT("选择商品"),X,Y+200,CW,Hit,false,false,SelectedData&&Str(SelectedData,TEXT("sku"))==Str(O,TEXT("sku")))->SetIsEnabled(Ready);
            }
            else
            {
                FString Caption=Str(O,TEXT("name"));if(Caption.IsEmpty())Caption=Str(O,TEXT("item_id"));
                const bool Selected=(SelectedData&&CatalogIdentity(SelectedData,Kind)==CatalogIdentity(O,Kind))||(BreakPage&&MaterialSelection.Contains(Str(O,TEXT("item_id"))));
                Action(List,Id,Caption,X,Y,CW,Hit,false,false,Selected)->SetIsEnabled(Ready);
                FString Hint=Kind==TEXT("sources")?Str(Obj(O,TEXT("source")),TEXT("name")):Kind==TEXT("buildings")?FString::Printf(TEXT("目标等级 %.0f · 资材 %.0f"),Num(O,TEXT("level")),Num(O,TEXT("supply_cost"))):Kind==TEXT("recipes")?FString::Printf(TEXT("炼制消耗 %.0f 灵石"),Num(O,TEXT("stone_cost"))):FString::Printf(TEXT("可用 %.0f"),Num(O,TEXT("owned_count")));
                if(BreakPage)Hint+=FString::Printf(TEXT(" · 已选 %d"),MaterialSelection.FindRef(Str(O,TEXT("item_id"))));
                Label(List,NAME_None,Hint,X+8,Y+92,CW-16,32,24);
            }
        }
        Action(List,TEXT("ToolPrevious"),TEXT("上一页"),0,FMath::Max(132.f,FMath::DivideAndRoundUp(Count,Columns)*RowH),(ListW-22)/2,Hit)->SetIsEnabled(Offset>0);
        Action(List,TEXT("ToolNext"),TEXT("下一页"),(ListW+2)/2,FMath::Max(132.f,FMath::DivideAndRoundUp(Count,Columns)*RowH),(ListW-22)/2,Hit)->SetIsEnabled(HasCatalog&&Offset+8<int32(Num(Catalog,TEXT("total"))));
        if(BreakPage)
        {
            Action(BodyCanvas,TEXT("ToolPrepare"),bRework?TEXT("确认重炼"):TEXT("准备突破"),0,Footer,(BodyWidth-12)*.6f,Hit,true)->SetIsEnabled(Ready&&!MaterialSelection.IsEmpty());
            Action(BodyCanvas,TEXT("ToolCancelBreak"),Str(Obj(State,TEXT("tribulation")),TEXT("status"))==TEXT("Reconcile")?TEXT("查询对账"):TEXT("取消准备"),(BodyWidth-12)*.6f+12,Footer,(BodyWidth-12)*.4f,Hit)->SetIsEnabled(Ready);
        }
        else Action(BodyCanvas,TEXT("ToolConfirm"),Primary,0,Footer,BodyWidth,Hit,true)->SetIsEnabled(Ready&&SelectedData);
    }
    RenderKey=RenderStateKey(State,!Ready)+(Active?TEXT("active"):TEXT("inactive"));bBuilding=false;
}
void UInkWulingToolsPanel::RefreshLiveData()
{
    if(bBuilding||!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;auto* W=Service(this);if(!W)return;const auto S=W->Snapshot();
    const FString Key=RenderStateKey(S,!W->IsEnabled()||!W->IsConnected()||W->IsBusy())+TEXT("active");
    if(Key!=RenderKey)RebuildPanel();
}
void UInkWulingToolsPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("ToolNavigation")){bShowNavigation=!bShowNavigation;RebuildPanel();return;}
    auto* W=Service(this);if(!W)return;
    if(Id.StartsWith(TEXT("ToolRoute_"))){const int32 I=FCString::Atoi(*Id.Mid(10));if(I>=0&&I<9){bShowNavigation=false;Open(Routes[I]);}return;}
    if(Id==TEXT("ToolPrevious")||Id==TEXT("ToolNext")){Offset=FMath::Max(0,Offset+(Id==TEXT("ToolNext")?8:-8));W->RequestCatalog(CatalogKind(),Offset,PageId==TEXT("WulingBreakthrough"));RebuildPanel();return;}
    if(Id.StartsWith(TEXT("ToolView_"))){SelectedEntry=Id.Mid(9);SelectedData.Reset();Offset=0;RebuildPanel();return;}
    if(Id==TEXT("ToolRankPrevious")||Id==TEXT("ToolRankNext")){RankOffset=FMath::Max(0,RankOffset+(Id==TEXT("ToolRankNext")?8:-8));RebuildPanel();return;}
    if(Id.StartsWith(TEXT("ToolActivityTab_"))){ActivityTab=Id.Mid(16);RebuildPanel();return;}
    if(Id==TEXT("ToolSign")){W->SignIn();return;}if(Id==TEXT("ToolLaunch")){W->StartLaunchEvent();return;}
    if(Id==TEXT("ToolPrepare")){if(bRework)W->SubmitPrototypeCommand(TEXT("ReworkQuality"),TEXT("{\"selection\":")+SelectionJson()+TEXT("}"));else W->PrepareBreakthrough(SelectionJson());return;}
    if(Id==TEXT("ToolReworkMode")){bRework=!bRework;MaterialSelection.Reset();RebuildPanel();return;}
    if(Id==TEXT("ToolCancelBreak")){if(Str(Obj(W->Snapshot(),TEXT("tribulation")),TEXT("status"))==TEXT("Reconcile")){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("attempt_id"),Str(Obj(W->Snapshot(),TEXT("tribulation")),TEXT("id")));W->SubmitPrototypeCommand(TEXT("ReconcileBreakthrough"),Json(A));}else W->CancelBreakthrough();return;}
    if(Id==TEXT("ToolClearSelection")){MaterialSelection.Reset();RebuildPanel();return;}
    if(Id==TEXT("ToolFabao")){W->SubmitPrototypeCommand(TEXT("ActivateFabao"),TEXT("{}"));return;}if(Id==TEXT("ToolMethod")){W->SubmitPrototypeCommand(TEXT("ActivateMethod"),TEXT("{}"));return;}
    if(Id==TEXT("ToolConfirm")){ExecuteSelected();return;}
    if(Id==TEXT("ToolMarketBrowse")||Id==TEXT("ToolMarketSell")){SelectedEntry=Id==TEXT("ToolMarketSell")?TEXT("sell"):TEXT("");Offset=0;SelectedData.Reset();RebuildPanel();return;}
    if(Id==TEXT("ToolQuantity")){const int32 Maximum=FMath::Clamp(int32(Num(SelectedData,TEXT("owned_count"))),1,100);ListingQuantity=ListingQuantity%Maximum+1;RebuildPanel();return;}
    if(Id==TEXT("ToolPrice")){ListingPrice=ListingPrice>=(Str(SelectedData,TEXT("item_id"))==TEXT("MAT_IRON_T")?18:12)?(Str(SelectedData,TEXT("item_id"))==TEXT("MAT_IRON_T")?6:4):ListingPrice+1;RebuildPanel();return;}
    if(Id==TEXT("ToolSetMain")||Id==TEXT("ToolAddSupport"))
    {
        if(!SelectedData)return;auto A=MakeShared<FJsonObject>();auto Loadout=Obj(W->Snapshot(),TEXT("loadout"));const FString Method=Str(SelectedData,TEXT("method_id"));A->SetStringField(TEXT("main"),Id==TEXT("ToolSetMain")?Method:Str(Loadout,TEXT("main")));
        TArray<TSharedPtr<FJsonValue>> Supports;const TArray<TSharedPtr<FJsonValue>>* Old=nullptr;if(Loadout&&Loadout->TryGetArrayField(TEXT("supports"),Old))for(const auto& V:*Old)if(V->AsString()!=Method)Supports.Add(V);
        if(Id==TEXT("ToolAddSupport"))Supports.Add(MakeShared<FJsonValueString>(Method));A->SetArrayField(TEXT("supports"),Supports);W->SubmitPrototypeCommand(TEXT("ConfigureMethods"),Json(A));return;
    }
    if(Id.StartsWith(TEXT("ToolQueue_"))){W->QueueRace(Id.Mid(10));return;}
    if(Id==TEXT("ToolRaceCancel")){W->SubmitPrototypeCommand(TEXT("CancelRaceQueue"),TEXT("{}"));return;}
    if(Id.StartsWith(TEXT("ToolRace")))
    {auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("race_id"),Str(Obj(W->Snapshot(),TEXT("race")),TEXT("id")));A->SetStringField(TEXT("kind"),Id==TEXT("ToolRaceAnchor")?TEXT("anchor"):TEXT("common"));A->SetStringField(TEXT("action"),Id==TEXT("ToolRaceGuard")?TEXT("护阵"):TEXT("破障"));W->SubmitPrototypeCommand(TEXT("SubmitRaceAction"),Json(A));return;}
    if(Id.StartsWith(TEXT("ToolRow_")))
    {
        const int32 I=FCString::Atoi(*Id.Mid(8));if(!DisplayRows.IsValidIndex(I))return;auto O=DisplayRows[I];auto A=MakeShared<FJsonObject>();const FString Kind=CatalogKind();
        if(PageId==TEXT("WulingBreakthrough")){const FString Item=Str(O,TEXT("item_id"));if(Item.StartsWith(TEXT("MAT_BR"))&&MaterialSelection.FindRef(Item)<Num(O,TEXT("owned_count")))++MaterialSelection.FindOrAdd(Item);W->SubmitPrototypeCommand(bRework?TEXT("QuoteReworkQuality"):TEXT("QuoteBreakthrough"),TEXT("{\"selection\":")+SelectionJson()+TEXT("}"));RebuildPanel();return;}
        SelectedData=O;ListingQuantity=1;ListingPrice=Str(O,TEXT("item_id"))==TEXT("MAT_IRON_T")?6:4;RebuildPanel();return;
    }
    Super::HandlePanelAction(Id);
}
void UInkWulingToolsPanel::ExecuteSelected()
{
    auto* W=Service(this);if(!W||!SelectedData)return;const auto O=SelectedData;const FString Kind=CatalogKind();auto A=MakeShared<FJsonObject>();
    if(Kind==TEXT("recipes")){W->Craft(Str(O,TEXT("recipe_id")));return;}
    if(Kind==TEXT("sources")){if(Str(O,TEXT("method"))!=TEXT("fixed_selected_reward")){FeedbackMessage(TEXT("该来源为配方或既有机制，请查看具体说明"));return;}A->SetStringField(TEXT("source_id"),Str(O,TEXT("source_id")));A->SetStringField(TEXT("item_id"),Str(O,TEXT("item_id")));W->SubmitPrototypeCommand(TEXT("ChooseFocus"),Json(A));return;}
    if(Kind==TEXT("equipment")||Kind==TEXT("methods")||Kind==TEXT("props")){A->SetStringField(TEXT("item_id"),Str(O,TEXT("item_id")));W->SubmitPrototypeCommand(Kind==TEXT("equipment")?TEXT("Equip"):(Kind==TEXT("methods")?TEXT("LearnMethod"):TEXT("UseProp")),Json(A));return;}
    if(Kind==TEXT("buildings")){A->SetStringField(TEXT("building_id"),Str(O,TEXT("building_id")));A->SetNumberField(TEXT("target_level"),Num(O,TEXT("level")));W->SubmitPrototypeCommand(TEXT("Build"),Json(A));return;}
    if(Kind==TEXT("shop")){A->SetStringField(TEXT("sku"),Str(O,TEXT("sku")));W->SubmitPrototypeCommand(TEXT("Purchase"),Json(A));return;}
    if(Kind==TEXT("items")&&PageId==TEXT("WulingMarket")){A->SetStringField(TEXT("item_id"),Str(O,TEXT("item_id")));A->SetNumberField(TEXT("count"),ListingQuantity);A->SetNumberField(TEXT("unit_price"),ListingPrice);W->SubmitPrototypeCommand(TEXT("CreateListing"),Json(A));return;}
    if(Kind==TEXT("market")){bool Own=false;O->TryGetBoolField(TEXT("is_own"),Own);A->SetStringField(TEXT("listing_id"),Str(O,TEXT("id")));A->SetNumberField(TEXT("listing_version"),Num(O,TEXT("version")));W->SubmitPrototypeCommand(Own?TEXT("CancelListing"):TEXT("BuyListing"),Json(A));}
}
