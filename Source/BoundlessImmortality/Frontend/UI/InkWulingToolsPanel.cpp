#include "InkWulingToolsPanel.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreMaterialBinding.h"
#include "Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h"
#include "Frontend/UiRollout20261024/Player/InkPlayerMaterialSlotWidget.h"
#include "Frontend/UiRollout20261024/LogicJourney/InkRolloutMaterialSlotWidget.h"
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
#include "Frontend/InkPresentationStyle.h"

namespace {
constexpr int32 BreakthroughMaterialCountLimit=100;
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(K,V)?*V:nullptr;}
FString Str(const TSharedPtr<FJsonObject>& O,const TCHAR* K){FString V;if(O)O->TryGetStringField(K,V);return V;}
double Num(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;if(O)O->TryGetNumberField(K,V);return V;}
bool HasNum(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;return O&&O->TryGetNumberField(K,V);}
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
const TCHAR* Titles[]={TEXT("物品炼制"),TEXT("物品来源"),TEXT("突破与天劫"),TEXT("装备与功法"),TEXT("武灵建设"),TEXT("签到与开服"),TEXT("灵脉竞采"),TEXT("武灵商店"),TEXT("武灵寄售")};
UWulingPrototypeSubsystem* Service(const UUserWidget* W){return W->GetGameInstance()?W->GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;}
FString ServiceDisplayStatus(const UWulingPrototypeSubsystem* W)
{
    if(!W)return TEXT("武灵服务暂不可用");
    if(!W->IsEnabled())return TEXT("武灵服务尚未启用");
    if(!W->IsConnected())return TEXT("武灵服务未连接");
    return W->GetStatus();
}
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
    if(PageId==TEXT("WulingBreakthrough")&&!RolloutMaterialBinding)
    {
        RolloutMaterialBinding=NewObject<UInkCoreMaterialBinding>(this);
        RolloutMaterialBinding->Configure(FrontendOwner,PageId,bRework);
        RolloutMaterialBinding->OnFeedback.AddUniqueDynamic(this,&UInkWulingToolsPanel::HandleRolloutMaterialFeedback);
    }
    const float OldCatalogOffset=CatalogScroll?CatalogScroll->GetScrollOffset():0;
    const float OldDetailOffset=DetailScroll?DetailScroll->GetScrollOffset():0;
    CatalogScroll=nullptr;DetailScroll=nullptr;DisplayRows.Reset();
    auto* W=Service(this);const auto State=W?W->Snapshot():nullptr;
    RolloutSignInDay=Str(Obj(State,TEXT("activities")),TEXT("day"));
    const bool Active=FrontendOwner&&FrontendOwner->CurrentPage==PageId;
    const bool Ready=Active&&W&&W->IsEnabled()&&W->IsConnected()&&!W->IsBusy();
    const float Gap=16,Hit=88;
    Surface(BodyCanvas,TEXT("WulingToolStatusPaper"),InkPresentationStyle::ESurface::FeatureCard,0,0,BodyWidth-180,80);
    Label(BodyCanvas,TEXT("WulingToolStatus"),ServiceDisplayStatus(W),0,4,BodyWidth-180,80,24);
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
        const TCHAR* Tabs[]={TEXT("overview"),TEXT("tasks"),TEXT("guild"),TEXT("rank")};const TCHAR* Names[]={TEXT("活动与签到"),TEXT("个人任务"),TEXT("共同目标"),TEXT("贡献荣誉榜")};
        Surface(BodyCanvas,TEXT("WulingActivityHeadingPaper"),InkPresentationStyle::ESurface::FeatureHeading,0,Top,BodyWidth,88);
        for(int32 I=0;I<4;++I)Action(BodyCanvas,TEXT("ToolActivityTab_")+FString(Tabs[I]),Names[I],(I%(bPortraitLayout?2:4))*(BodyWidth+12)/(bPortraitLayout?2:4),Top+(bPortraitLayout?I/2:0)*100,(BodyWidth-(bPortraitLayout?12:36))/(bPortraitLayout?2:4),88,false,false,ActivityTab==Tabs[I]);
        FString Detail;
        if(ActivityTab==TEXT("tasks"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;const TMap<FString,FString> Conditions={{TEXT("join_and_free_response"),TEXT("正式加入并免费响应")},{TEXT("complete_world_rounds"),TEXT("完整参与世界历练")},{TEXT("correct_guard_response"),TEXT("正确护阵")},{TEXT("correct_break_response"),TEXT("正确破障")},{TEXT("route_vote_in_distinct_rounds"),TEXT("不同轮次路线选择")}};
            if(L&&L->TryGetArrayField(TEXT("tasks"),Rows))for(const auto& V:*Rows){auto O=V->AsObject();bool Confirmed=false;const bool HasConfirmed=O&&O->TryGetBoolField(TEXT("confirmed"),Confirmed);const FString Progress=HasNum(O,TEXT("progress"))&&HasNum(O,TEXT("target"))?FString::Printf(TEXT("%.0f/%.0f"),Num(O,TEXT("progress")),Num(O,TEXT("target"))):TEXT("进度待服务提供");const auto Permissions=Obj(L,TEXT("permissions"));bool RewardsAllowed=false;const bool HasRewardPermission=Permissions&&Permissions->TryGetBoolField(TEXT("task_numeric_rewards"),RewardsAllowed);const FString Reward=HasRewardPermission&&RewardsAllowed&&HasNum(O,TEXT("reward_count"))?FString::Printf(TEXT(" · 服务奖励 %.0f"),Num(O,TEXT("reward_count"))):TEXT(" · 奖励资料待服务提供");const FString StateText=HasConfirmed&&Confirmed?TEXT(" · 已结算"):HasConfirmed?TEXT(" · 待服务确认"):TEXT(" · 状态待服务提供");const FString Condition=Conditions.FindRef(Str(O,TEXT("condition_type")));Detail+=FString::Printf(TEXT("%s %s · %s%s%s\n"),*Str(O,TEXT("task_id")),Condition.IsEmpty()?TEXT("任务条件待服务提供"):*Condition,*Progress,*Reward,*StateText);}
            else Detail=TEXT("尚未开启青云初启，开启后显示真实任务记录。");
        }
        else if(ActivityTab==TEXT("guild"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Completed=nullptr;const bool HasCompleted=L&&L->TryGetArrayField(TEXT("complete_rounds"),Completed)&&Completed;
            Detail=HasCompleted?FString::Printf(TEXT("服务端确认完整共同轮次：%d\n里程碑与奖励（服务配置）\n"),Completed->Num()):TEXT("完整共同轮次：待服务提供\n里程碑与奖励（服务配置）\n");
            const TArray<TSharedPtr<FJsonValue>>* Milestones=nullptr;
            const TArray<TSharedPtr<FJsonValue>>* Granted=nullptr;
            const auto Permissions=Obj(L,TEXT("permissions"));bool NumericRewardsEnabled=false,NumericBuffEnabled=false;
            const bool HasNumericRewardsPermission=Permissions&&Permissions->TryGetBoolField(TEXT("guild_numeric_rewards"),NumericRewardsEnabled);
            const bool HasNumericBuffPermission=Permissions&&Permissions->TryGetBoolField(TEXT("guild_numeric_buff"),NumericBuffEnabled);
            const FString NumericRewardsState=!HasNumericRewardsPermission?FString(TEXT("未提供")):(NumericRewardsEnabled?FString(TEXT("启用")):FString(TEXT("未启用")));
            const FString NumericBuffState=!HasNumericBuffPermission?FString(TEXT("未提供")):(NumericBuffEnabled?FString(TEXT("启用")):FString(TEXT("未启用")));
            Detail+=TEXT("服务权限：数值奖励 ")+NumericRewardsState+TEXT(" · 组织增益 ")+NumericBuffState+TEXT("\n");
            const bool HasMilestones=L&&L->TryGetArrayField(TEXT("guild_milestones"),Milestones)&&Milestones;
            const bool HasGranted=L&&L->TryGetArrayField(TEXT("milestones"),Granted)&&Granted;
            if(HasMilestones)
            {
                if(Milestones->IsEmpty())Detail+=TEXT("服务已返回配置，但没有里程碑条目。\n");
                for(const auto& Value:*Milestones)
                {
                    const auto Row=Value.IsValid()?Value->AsObject():nullptr;if(!Row)continue;
                    const FString Tier=Str(Row,TEXT("tier_id"));bool bFinalBuff=false;
                    const bool HasFinalBuff=Row->TryGetBoolField(TEXT("grant_final_buff"),bFinalBuff);
                    bool bCompleted=false;
                    if(HasGranted&&Granted)for(const auto& GrantedValue:*Granted)if(GrantedValue.IsValid()&&GrantedValue->AsString()==Tier){bCompleted=true;break;}
                    const FString Threshold=Row->HasField(TEXT("complete_rounds"))?FString::FromInt(int32(Num(Row,TEXT("complete_rounds")))):TEXT("待服务提供");
                    const FString StoneConfig=Row->HasField(TEXT("anchor_stone"))?FString::FromInt(int32(Num(Row,TEXT("anchor_stone")))):TEXT("待服务提供");
                    const FString MaterialConfig=Row->HasField(TEXT("anchor_break_material"))?FString::FromInt(int32(Num(Row,TEXT("anchor_break_material")))):TEXT("待服务提供");
                    const FString NumericRewardOutcome=!HasNumericRewardsPermission?FString(TEXT("奖励权限未知，不能确认发放")):!NumericRewardsEnabled?FString(TEXT("奖励未启用，配置不代表发放")):bCompleted?FString(TEXT("奖励权限已启用，发放以服务收据为准")):FString(TEXT("达标后按服务权限处理"));
                    const FString Stone=!HasNumericRewardsPermission?StoneConfig+TEXT("（权限未知）"):NumericRewardsEnabled?StoneConfig:FString(TEXT("不发放"));
                    const FString Material=!HasNumericRewardsPermission?MaterialConfig+TEXT("（权限未知）"):NumericRewardsEnabled?MaterialConfig:FString(TEXT("不发放"));
                    const FString Completion=HasGranted?(bCompleted?FString(TEXT("已达成")):FString(TEXT("未达成"))):FString(TEXT("达成状态待服务提供"));
                    FString FinalBuffState=TEXT("待服务提供");
                    if(!HasFinalBuff)FinalBuffState=TEXT("触发配置待服务提供");
                    else if(!bFinalBuff)FinalBuffState=TEXT("配置不触发");
                    else if(!HasNumericBuffPermission)FinalBuffState=TEXT("增益权限待服务提供");
                    else if(!NumericBuffEnabled)FinalBuffState=TEXT("增益未启用，不会发放");
                    else if(!bCompleted)FinalBuffState=TEXT("增益权限已启用，待达标");
                    else if(L&&L->HasField(TEXT("buff_start"))&&L->HasField(TEXT("buff_end")))FinalBuffState=FString::Printf(TEXT("已触发，%.0f–%.0f"),Num(L,TEXT("buff_start")),Num(L,TEXT("buff_end")));
                    else FinalBuffState=TEXT("配置与权限已启用，服务未提供触发时间");
                    Detail+=FString::Printf(TEXT("%s · %s轮 · %s · %s · 配置奖励 %s灵石/%s破境石；%s · 最终增益：%s\n"),*Tier,*Threshold,*Str(Row,TEXT("guild_visual")),*Completion,*Stone,*Material,*NumericRewardOutcome,*FinalBuffState);
                }
            }
            else Detail+=TEXT("服务未提供 guild_milestones 配置。\n");
            if(L&&L->HasField(TEXT("milestones"))&&!HasGranted)Detail+=TEXT("达成里程碑状态结构无效。\n");
            else if(!HasGranted)Detail+=TEXT("服务未提供达成里程碑状态。\n");
            if(L&&L->HasField(TEXT("buff_start"))&&L->HasField(TEXT("buff_end")))Detail+=FString::Printf(TEXT("最终增益服务时间：开始 %.0f · 结束 %.0f\n"),Num(L,TEXT("buff_start")),Num(L,TEXT("buff_end")));
            else Detail+=TEXT("最终增益时间：服务尚未提供 buff_start/buff_end。\n");
        }
        else if(ActivityTab==TEXT("rank"))
        {
            const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;Detail=TEXT("贡献不按礼物计分 · 无数值排行奖励\n");
            if(L&&L->TryGetArrayField(TEXT("leaderboard"),Rows)&&Rows->Num())for(int32 I=RankOffset;I<FMath::Min(Rows->Num(),RankOffset+8);++I){auto O=(*Rows)[I]->AsObject();const FString Rank=HasNum(O,TEXT("rank"))?FString::Printf(TEXT("第%.0f名"),Num(O,TEXT("rank"))):TEXT("名次待服务提供");const FString Score=HasNum(O,TEXT("score"))?FString::Printf(TEXT("%.0f分"),Num(O,TEXT("score"))):TEXT("贡献待服务提供");Detail+=FString::Printf(TEXT("%s · %s · %s\n"),*Rank,*Str(O,TEXT("name")),*Score);}else Detail+=TEXT("暂无荣誉榜条目；入榜资格由服务端确认。");
        }
        else
        {
            const FString Day=Str(A,TEXT("day")).IsEmpty()?TEXT("待服务提供"):Str(A,TEXT("day"));
            const FString SignCount=HasNum(A,TEXT("month_sign_count"))?FString::Printf(TEXT("%.0f"),Num(A,TEXT("month_sign_count"))):TEXT("待服务提供");
            const FString Remaining=HasNum(L,TEXT("end"))&&HasNum(State,TEXT("server_utc"))?FString::Printf(TEXT("%.0f 分钟"),FMath::Max(0.,(Num(L,TEXT("end"))-Num(State,TEXT("server_utc")))/60)):TEXT("待服务时间同步");
            const FString Score=HasNum(Personal,TEXT("score"))?FString::Printf(TEXT("%.0f"),Num(Personal,TEXT("score"))):TEXT("待服务提供");
            const FString Complete=HasNum(Personal,TEXT("complete_count"))?FString::Printf(TEXT("%.0f"),Num(Personal,TEXT("complete_count"))):TEXT("待服务提供");
            Detail=FString::Printf(TEXT("服务日期：%s · 本月签到 %s 日\n%s\n开服剩余 %s · 个人贡献 %s / 完整轮次 %s\n称号与活动期限以服务快照为准。"),*Day,*SignCount,*Str(A,TEXT("verse")),*Remaining,*Score,*Complete);
        }

        TArray<FString> ActivityLines;Detail.ParseIntoArrayLines(ActivityLines,false);int32 ActivityLineCount=0;const int32 ActivityCapacity=FMath::Max(8,int32((BodyWidth-34)/26));for(const FString& Line:ActivityLines)ActivityLineCount+=FMath::Max(1,FMath::DivideAndRoundUp(Line.Len(),ActivityCapacity));
        const float ActivityTextHeight=FMath::Max(320.f,ActivityLineCount*38.f+24);
        const float DetailTop=Top+(bPortraitLayout?208:100),Footer=BodyHeight-88;
        auto* ActivityCanvas=ScrollArea(TEXT("WulingActivityScroll"),0,DetailTop,BodyWidth,FMath::Max(88.f,Footer-DetailTop-12),ActivityTextHeight+(ActivityTab==TEXT("rank")?112:12),DetailScroll,OldDetailOffset);
        Surface(ActivityCanvas,TEXT("WulingActivityDataPaper"),InkPresentationStyle::ESurface::FeatureCard,0,0,BodyWidth-10,ActivityTextHeight+12);
        Surface(ActivityCanvas,TEXT("WulingActivityDataHeading"),InkPresentationStyle::ESurface::FeatureHeading,8,8,BodyWidth-26,52);
        Label(ActivityCanvas,TEXT("WulingActivityData"),Detail,8,8,BodyWidth-34,ActivityTextHeight,26);
        if(ActivityTab==TEXT("rank")){Action(ActivityCanvas,TEXT("ToolRankPrevious"),TEXT("上一页"),0,ActivityTextHeight+12,(BodyWidth-12)/2,Hit);Action(ActivityCanvas,TEXT("ToolRankNext"),TEXT("下一页"),(BodyWidth+12)/2,ActivityTextHeight+12,(BodyWidth-32)/2,Hit);}
        bool Signed=false;const bool HasSigned=A&&A->TryGetBoolField(TEXT("signed"),Signed);
        Action(BodyCanvas,TEXT("ToolSign"),HasSigned&&Signed?TEXT("今日已签到"):TEXT("签到记录"),0,Footer,(BodyWidth-12)/2,Hit,true)->SetIsEnabled(Ready&&HasSigned&&!Signed);
        const auto Identity=Obj(State,TEXT("identity")),Guild=Obj(State,TEXT("guild")),Manifest=Obj(State,TEXT("manifest"));
        const auto* LaunchValue=State?State->Values.Find(TEXT("launch")):nullptr;
        const bool KnownLaunchAbsent=LaunchValue&&LaunchValue->IsValid()&&(*LaunchValue)->Type==EJson::Null;
        const bool CanStart=Ready&&KnownLaunchAbsent&&Str(Manifest,TEXT("profile"))==TEXT("prototype_full")&&
            Str(Identity,TEXT("role"))==TEXT("anchor")&&!Str(Identity,TEXT("user")).IsEmpty()&&
            !Str(Guild,TEXT("id")).IsEmpty()&&Str(Identity,TEXT("user"))==Str(Guild,TEXT("anchor"))&&
            HasNum(State,TEXT("server_utc"))&&HasNum(Manifest,TEXT("window_end"))&&Num(Manifest,TEXT("window_end"))>Num(State,TEXT("server_utc"));
        Action(BodyCanvas,TEXT("ToolLaunch"),TEXT("开启青云初启"),(BodyWidth+12)/2,Footer,(BodyWidth-12)/2,Hit,true)->SetIsEnabled(CanStart);
    }
    else if(PageId==TEXT("WulingRace"))
    {
        const auto R=Obj(State,TEXT("race")),L=Obj(State,TEXT("lease")),Queue=Obj(State,TEXT("race_queue")),Identity=Obj(State,TEXT("identity")),Guild=Obj(State,TEXT("guild"));
        const TArray<TSharedPtr<FJsonValue>>* Teams=nullptr;if(R)R->TryGetArrayField(TEXT("teams"),Teams);
        TSharedPtr<FJsonObject> Team;
        if(Teams)for(const auto& Value:*Teams)
        {
            const auto Candidate=Value.IsValid()?Value->AsObject():nullptr;
            if(Candidate&&Str(Candidate,TEXT("room"))==Str(Identity,TEXT("room"))&&(Str(Candidate,TEXT("anchor"))==Str(Identity,TEXT("user"))||([&](){const TArray<TSharedPtr<FJsonValue>>* Reps=nullptr;if(!Candidate->TryGetArrayField(TEXT("representatives"),Reps)||!Reps)return false;for(const auto& Rep:*Reps)if(Rep.IsValid()&&Rep->AsString()==Str(Identity,TEXT("user")))return true;return false;})())){Team=Candidate;break;}
        }
        const FString RaceStatus=Str(R,TEXT("status")),Role=Str(Identity,TEXT("role")),User=Str(Identity,TEXT("user"));
        const double ServerNow=Num(State,TEXT("server_utc")),RaceStart=Num(R,TEXT("start")),Elapsed=ServerNow-RaceStart-120;
        const auto Counts=Obj(State,TEXT("counts"));bool ActivityBusy=false;const bool HasActivityBusy=State&&State->TryGetBoolField(TEXT("activity_busy"),ActivityBusy);
        bool HeartbeatActive=false;const bool HasHeartbeat=Identity&&Identity->TryGetBoolField(TEXT("active"),HeartbeatActive);
        bool TeamEligible=false;const bool HasTeamEligibility=Team&&Team->TryGetBoolField(TEXT("eligible"),TeamEligible);
        const bool HasLeaseField=State&&State->HasField(TEXT("lease"));
        const TArray<TSharedPtr<FJsonValue>>* AllowedNodes=nullptr;const bool HasAllowedNodes=State&&State->TryGetArrayField(TEXT("race_allowed_nodes"),AllowedNodes);
        FString QueueReason;
        if(!Ready)QueueReason=TEXT("服务未就绪");
        else if(Role!=TEXT("anchor"))QueueReason=TEXT("仅主播可排队");
        else if(!Guild)QueueReason=TEXT("尚未加入武灵");
        else if(Queue&&Str(Queue,TEXT("status"))==TEXT("Queued"))QueueReason=TEXT("已在排队");
        else if(RaceStatus==TEXT("Ready")||RaceStatus==TEXT("Running")||RaceStatus==TEXT("Frozen"))QueueReason=TEXT("已有竞赛进行中");
        else if(HasActivityBusy&&ActivityBusy)QueueReason=TEXT("服务确认当前有其他活动占用");
        else if(!HasActivityBusy)QueueReason=TEXT("服务未提供 activity_busy，无法确认活动互斥条件");
        else if(!HasLeaseField)QueueReason=TEXT("服务未提供开采权状态");
        else if(Str(L,TEXT("status"))==TEXT("Active")&&Num(L,TEXT("end"))>ServerNow)QueueReason=TEXT("当前存在有效开采权");
        else if(!Counts||!Counts->HasField(TEXT("participating_members")))QueueReason=TEXT("服务未提供房间正式成员在线人数");
        else if(Num(Counts,TEXT("participating_members"))<=0)QueueReason=TEXT("当前房间没有在线正式成员");
        else if(!HasAllowedNodes)QueueReason=TEXT("服务未提供允许的节点列表");
        else if(!AllowedNodes||AllowedNodes->IsEmpty())QueueReason=TEXT("服务允许节点列表为空");
        auto NodeAllowed=[&](const FString& Node){if(!AllowedNodes)return false;for(const auto& V:*AllowedNodes)if(V.IsValid()&&V->AsString()==Node)return true;return false;};
        FString CancelReason;
        if(!Ready)CancelReason=TEXT("服务未就绪");else if(Role!=TEXT("anchor"))CancelReason=TEXT("仅主播可取消排队");else if(!Queue)CancelReason=TEXT("服务未提供排队状态");else if(Str(Queue,TEXT("status"))!=TEXT("Queued"))CancelReason=TEXT("当前没有可取消的排队");
        auto RaceActionReason=[&](int32 ActionKind,const FString& RequiredAction)->FString
        {
            if(!Ready)return TEXT("服务未就绪");
            if(RaceStatus!=TEXT("Running"))return TEXT("竞赛尚未进入 Running 阶段");
            if(!Team)return TEXT("服务未提供当前房间的队伍/代表资格");
            if(!HasTeamEligibility)return TEXT("服务未提供队伍 eligible 字段");
            if(!TeamEligible)return TEXT("服务确认本队不具备提交资格");
            if(!HasHeartbeat)return TEXT("服务未提供当前会话 active/heartbeat 资格");
            if(!HeartbeatActive)return TEXT("服务确认当前会话不活跃");
            if(ActionKind==1&&Role!=TEXT("anchor"))return TEXT("主播时机仅主播可提交");
            if(ActionKind==2&&Role!=TEXT("viewer"))return TEXT("代表响应仅正式成员可提交");
            if(ActionKind!=1&&User!=Str(Team,TEXT("anchor")))
            {
                const TArray<TSharedPtr<FJsonValue>>* Reps=nullptr;bool IsRepresentative=false;
                if(Team->TryGetArrayField(TEXT("representatives"),Reps)&&Reps)for(const auto& V:*Reps)if(V.IsValid()&&V->AsString()==User){IsRepresentative=true;break;}
                if(!IsRepresentative)return TEXT("当前用户不是本队主播或代表成员");
            }
            if(!State||!State->HasField(TEXT("server_utc"))||!R->HasField(TEXT("start")))return TEXT("服务未提供服务时钟或竞赛开始时间");
            if(ActionKind==1)
            {
                const bool Open=FMath::Abs(Elapsed-90)<=10||FMath::Abs(Elapsed-270)<=10;
                return Open?FString():FString(TEXT("当前不在主播时机窗口（开赛后第90/270秒±10秒）"));
            }
            if(ActionKind==2)
            {
                const bool Open=(Elapsed>=60&&Elapsed<80)||(Elapsed>=240&&Elapsed<260);
                return Open?FString():FString(TEXT("当前不在代表响应窗口（挑战开始后第60–80或240–260秒）"));
            }
            if(Elapsed<0||Elapsed>=360)return TEXT("当前不在第2至8分钟挑战期");
            const int32 Slot=FMath::Min(5,int32(Elapsed/60));const FString Expected=Slot%2==0?TEXT("护阵"):TEXT("破障");
            return RequiredAction==Expected?FString():FString::Printf(TEXT("当前轮次要求：%s"),*Expected);
        };
        const float LW=bPortraitLayout?BodyWidth:(BodyWidth-Gap)*.48f,RX=bPortraitLayout?0:LW+Gap,RW=BodyWidth-RX;
        const float Footer=BodyHeight-88,AreaH=Footer-Top-12;
        auto* Summary=ScrollArea(TEXT("WulingRaceSummaryScroll"),0,Top,LW,AreaH,bPortraitLayout?1060:320,DetailScroll,OldDetailOffset);
        const FString QueueStatus=Queue?Str(Queue,TEXT("status")):TEXT("服务未提供");
        const FString RaceActionStatus=RaceActionReason(0,TEXT("护阵"));
        Label(Summary,TEXT("WulingRaceData"),FString::Printf(TEXT("阶段：%s\n排队：%s\n排队条件：%s\n原型固定计分：共同任务上限600 / 主播时机上限200 / 代表响应上限200\n挑战期：第2至8分钟\n共同任务槽：每60秒交替护阵/破障\n主播时机：挑战开始后第90 / 270秒前后10秒\n代表窗口：挑战开始后第60–80 / 240–260秒\n开采权剩余 %.0f 分钟 · 在线 %.0f 秒\n共同任务：%s"),*(RaceStatus.IsEmpty()?FString(TEXT("服务未提供")):RaceStatus),*QueueStatus,*(QueueReason.IsEmpty()?FString(TEXT("服务条件通过；节点仍需在允许列表中")):QueueReason),FMath::Max(0.,Num(L,TEXT("end"))-ServerNow)/60,Num(L,TEXT("online_seconds")),*(RaceActionStatus.IsEmpty()?FString(TEXT("可提交")):RaceActionStatus)),0,0,LW-16,520,24);
        auto* Choices=bPortraitLayout?Summary:ScrollArea(TEXT("WulingRaceChoicesScroll"),RX,Top,RW,AreaH,760,CatalogScroll,OldCatalogOffset);
        const float ChoiceTop=bPortraitLayout?320:0,CW=(RW-26)/2;
        const TCHAR* Nodes[]={TEXT("NODE_QI"),TEXT("NODE_STONE"),TEXT("NODE_HERB"),TEXT("NODE_IRON"),TEXT("NODE_PAGE")};const TCHAR* Names[]={TEXT("灵泉"),TEXT("灵石矿"),TEXT("灵草园"),TEXT("玄铁脉"),TEXT("残卷遗址")};
        for(int32 I=0;I<5;++I){const FString Node=Nodes[I];const float Y=ChoiceTop+(I/2)*136;auto* Button=Action(Choices,TEXT("ToolQueue_")+Node,Names[I],(I%2)*(CW+12),Y,CW,Hit);const bool Enabled=QueueReason.IsEmpty()&&NodeAllowed(Node);Button->SetIsEnabled(Enabled);if(!Enabled)Label(Choices,FName(*(TEXT("RaceQueueReason_")+Node)),QueueReason.IsEmpty()?TEXT("服务未确认此节点可用"):QueueReason,(I%2)*(CW+12),Y+90,CW,42,18);}
        const FString AnchorReason=RaceActionReason(1,TEXT(""));auto* AnchorButton=Action(Choices,TEXT("ToolRaceAnchor"),TEXT("主播时机"),0,ChoiceTop+432,CW,Hit);AnchorButton->SetIsEnabled(AnchorReason.IsEmpty());if(!AnchorReason.IsEmpty())Label(Choices,TEXT("RaceAnchorReason"),AnchorReason,0,ChoiceTop+522,CW,48,18);
        const FString RepresentativeReason=RaceActionReason(2,TEXT(""));auto* RepresentativeButton=Action(Choices,TEXT("ToolRaceRepresentative"),TEXT("代表限时响应"),0,ChoiceTop+568,CW,Hit,true);RepresentativeButton->SetIsEnabled(RepresentativeReason.IsEmpty());if(!RepresentativeReason.IsEmpty())Label(Choices,TEXT("RaceRepresentativeReason"),RepresentativeReason,0,ChoiceTop+658,CW,48,18);
        auto* CancelButton=Action(Choices,TEXT("ToolRaceCancel"),TEXT("取消排队"),CW+12,ChoiceTop+432,CW,Hit);CancelButton->SetIsEnabled(CancelReason.IsEmpty());if(!CancelReason.IsEmpty())Label(Choices,TEXT("RaceCancelReason"),CancelReason,CW+12,ChoiceTop+522,CW,48,18);
        const FString GuardReason=RaceActionReason(0,TEXT("护阵")),BreakReason=RaceActionReason(0,TEXT("破障"));
        auto* GuardButton=Action(BodyCanvas,TEXT("ToolRaceGuard"),TEXT("共同护阵"),0,Footer,(BodyWidth-12)/2,Hit,true);GuardButton->SetIsEnabled(GuardReason.IsEmpty());
        auto* BreakButton=Action(BodyCanvas,TEXT("ToolRaceBreak"),TEXT("共同破障"),(BodyWidth+12)/2,Footer,(BodyWidth-12)/2,Hit);BreakButton->SetIsEnabled(BreakReason.IsEmpty());
        if(!GuardReason.IsEmpty())Label(BodyCanvas,TEXT("RaceGuardReason"),GuardReason,0,Footer-Hit-38,(BodyWidth-12)/2,36,18);
        if(!BreakReason.IsEmpty())Label(BodyCanvas,TEXT("RaceBreakReason"),BreakReason,(BodyWidth+12)/2,Footer-Hit-38,(BodyWidth-12)/2,36,18);
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
        bool BreakSubmissionReady=false;
        bool CanCancelPrepare=false,CanReconcile=false;
        FString BreakCancelReason;
        if(BreakPage)
        {
            const auto T=Obj(State,TEXT("tribulation")),Q=Obj(State,TEXT("breakthrough_quote"));
            const FString AttemptStatus=Str(T,TEXT("status"));double TrialStart=0;
            const bool HasTrialStart=T&&T->TryGetNumberField(TEXT("trial_start"),TrialStart);
            CanCancelPrepare=AttemptStatus==TEXT("Prepare")&&HasTrialStart&&State&&State->HasField(TEXT("server_utc"))&&Num(State,TEXT("server_utc"))<TrialStart;
            CanReconcile=AttemptStatus==TEXT("Reconcile")&&!Str(T,TEXT("id")).IsEmpty();
            BreakCancelReason=CanCancelPrepare?FString():CanReconcile?FString():AttemptStatus==TEXT("Prepare")?FString(TEXT("服务未提供有效 trial_start/server_utc，无法确认仍可取消")):AttemptStatus==TEXT("Reconcile")?FString(TEXT("服务未提供可对账的 attempt id")):FString(TEXT("仅 Prepare 且试炼开始前可取消；仅 Reconcile 可查询对账"));
            const auto Stage=Obj(Q,TEXT("stage"));
            const FString StageName=Str(Stage,TEXT("name"));
            const auto Growth=Obj(State,TEXT("growth"));
            const FString CurrentStage=Str(Q,TEXT("current_stage")).IsEmpty()?Str(Growth,TEXT("name")):Str(Q,TEXT("current_stage"));
            const FString TargetStage=Str(Q,TEXT("target_stage")).IsEmpty()?Str(Stage,TEXT("target_stage")):Str(Q,TEXT("target_stage"));
            const FString CurrentStageDisplay=CurrentStage.IsEmpty()?FString(TEXT("境界信息待同步")):CurrentStage;
            const FString TargetStageDisplay=TargetStage.IsEmpty()?FString(TEXT("目标境界待同步")):TargetStage;
            const FString QualityBand=Str(Q,TEXT("quality_band")).IsEmpty()?Str(Obj(Q,TEXT("band")),TEXT("name")):Str(Q,TEXT("quality_band"));
            const bool HasQ=Q&&Q->HasField(TEXT("q"));
            const bool HasQuality=Q&&(Q->HasField(TEXT("quality"))||Q->HasField(TEXT("band"))||Q->HasField(TEXT("quality_band")));
            const bool HasGrowth=Q&&Q->HasField(TEXT("growth_min"))&&Q->HasField(TEXT("growth_max"));
            const bool HasNormalizedQ=Q&&Q->HasField(TEXT("q_normalized"));
            const auto Readiness=Obj(Q,TEXT("readiness"));
            bool ReadyByService=false;
            const bool HasReadiness=Q&&Q->TryGetBoolField(TEXT("readiness"),ReadyByService)?true:(Readiness&&Readiness->TryGetBoolField(TEXT("ready"),ReadyByService));
            const TArray<TSharedPtr<FJsonValue>>* Slots=nullptr;
            const bool HasSlots=Q&&Q->TryGetArrayField(TEXT("slots"),Slots)&&Slots->Num()>0;
            bool SlotFieldsComplete=HasSlots;
            if(HasSlots)for(const auto& Value:*Slots)
            {
                const auto SlotObject=Value->AsObject();
                const bool HasCategory=!Str(SlotObject,TEXT("category")).IsEmpty()||!Str(SlotObject,TEXT("slot_category")).IsEmpty();
                const bool HasItem=!Str(SlotObject,TEXT("item_id")).IsEmpty()||!Str(SlotObject,TEXT("name")).IsEmpty()||!Str(SlotObject,TEXT("item_name")).IsEmpty();
                const bool HasCount=SlotObject&&(SlotObject->HasField(TEXT("count"))||SlotObject->HasField(TEXT("quantity")));
                SlotFieldsComplete=SlotFieldsComplete&&HasCategory&&HasItem&&HasCount;
            }
            // A quote is authoritative only when it echoes the exact submitted material set.
            // Compare item_id/count pairs independent of JSON array order and reject duplicates.
            const TArray<TSharedPtr<FJsonValue>>* QuotedSelection=nullptr;
            const bool HasQuotedSelection=Q&&Q->TryGetArrayField(TEXT("selection"),QuotedSelection);
            bool SelectionMatches=HasQuotedSelection;
            TMap<FString,int32> QuotedCounts;
            if(HasQuotedSelection)for(const auto& Value:*QuotedSelection)
            {
                const auto Entry=Value->AsObject();double Count=0;FString Item;
                if(!Entry||!Entry->TryGetStringField(TEXT("item_id"),Item)||Item.IsEmpty()||!Entry->TryGetNumberField(TEXT("count"),Count)||Count<1||Count>BreakthroughMaterialCountLimit||FMath::FloorToDouble(Count)!=Count||QuotedCounts.Contains(Item))
                {SelectionMatches=false;break;}
                QuotedCounts.Add(Item,int32(Count));
            }
            if(SelectionMatches&&QuotedCounts.Num()!=MaterialSelection.Num())SelectionMatches=false;
            if(SelectionMatches)for(const auto& Pair:MaterialSelection)if(QuotedCounts.FindRef(Pair.Key)!=Pair.Value){SelectionMatches=false;break;}
            bool SlotSelectionMatches=true;FString SlotSelectionReason;
            TMap<FString,int32> SlotItemCounts;
            if(HasSlots)for(const auto& Value:*Slots)
            {
                const auto SlotObject=Value->AsObject();if(!SlotObject){SlotSelectionMatches=false;SlotSelectionReason=TEXT("slots 条目不是对象");continue;}
                const FString Item=Str(SlotObject,TEXT("item_id"));
                if(Item.IsEmpty()){SlotSelectionMatches=false;SlotSelectionReason=TEXT("slots 条目缺少 item_id");continue;}
                double Count=0;
                if(!SlotObject->TryGetNumberField(SlotObject->HasField(TEXT("count"))?TEXT("count"):TEXT("quantity"),Count)||Count<1||Count>BreakthroughMaterialCountLimit||FMath::FloorToDouble(Count)!=Count){SlotSelectionMatches=false;SlotSelectionReason=TEXT("slots 条目缺少合法 count/quantity");continue;}
                SlotItemCounts.FindOrAdd(Item)+=int32(Count);
            }
            if(!MaterialSelection.IsEmpty())
            {
                if(!HasSlots){SlotSelectionMatches=false;SlotSelectionReason=TEXT("服务报价缺少 slots");}
                if(SlotItemCounts.Num()!=MaterialSelection.Num()){SlotSelectionMatches=false;if(SlotSelectionReason.IsEmpty())SlotSelectionReason=TEXT("slots 与当前材料种类不一致");}
                if(SlotSelectionMatches)for(const auto& Pair:MaterialSelection)if(SlotItemCounts.FindRef(Pair.Key)!=Pair.Value){SlotSelectionMatches=false;SlotSelectionReason=TEXT("slots item_id/count 汇总与当前选择不一致");break;}
            }
            // For normal breakthrough, a categorized slot quote must contain exactly one core.
            // Rework follows the service's old_core rule: zero cores is valid; one is an upgrade
            // candidate, with the service remaining authoritative for core_score and Q.
            bool HasSlotCategories=false;int32 QuotedCoreCount=0;bool SlotCategoriesValid=HasSlots;
            if(HasSlots)for(const auto& Value:*Slots)
            {
                const auto SlotObject=Value->AsObject();if(!SlotObject)continue;
                FString Category=Str(SlotObject,TEXT("category"));if(Category.IsEmpty())Category=Str(SlotObject,TEXT("slot_category"));
                if(Category.IsEmpty())continue;
                HasSlotCategories=true;double Count=0;
                if(!SlotObject->TryGetNumberField(SlotObject->HasField(TEXT("count"))?TEXT("count"):TEXT("quantity"),Count)||Count<0||FMath::FloorToDouble(Count)!=Count){SlotCategoriesValid=false;continue;}
                if(Category.Equals(TEXT("core"),ESearchCase::IgnoreCase))QuotedCoreCount+=int32(Count);
            }
            const bool IsReworkQuote=Str(Q,TEXT("kind"))==TEXT("rework");
            const bool CoreSlotsValid=!HasSlotCategories||(IsReworkQuote?(SlotCategoriesValid&&QuotedCoreCount<=1):(SlotCategoriesValid&&QuotedCoreCount==1));
            const TArray<TSharedPtr<FJsonValue>>* Sources=nullptr;
            const bool HasSources=Q&&Q->TryGetArrayField(TEXT("sources"),Sources)&&Sources->Num()>0;
            const bool HasChance=!bRework&&((T&&T->HasField(TEXT("tribulation_chance")))||(Q&&Q->HasField(TEXT("tribulation_chance")))||(T&&T->HasField(TEXT("probability_basis_points")))||(Q&&Q->HasField(TEXT("probability_basis_points"))));
            const bool HasStage=!CurrentStage.IsEmpty()&&!TargetStage.IsEmpty();
            const bool QuoteKindMatches=bRework?IsReworkQuote:Str(Q,TEXT("kind"))!=TEXT("rework");
            const bool BreakDisplayComplete=HasStage&&HasQ&&HasNormalizedQ&&HasQuality&&HasGrowth&&HasReadiness&&HasSlots&&SlotFieldsComplete&&HasSources&&HasChance&&ReadyByService&&SelectionMatches&&SlotSelectionMatches&&CoreSlotsValid&&QuoteKindMatches;
            BreakSubmissionReady=InkCore::EvaluateQuote(State,MaterialSelection,bRework).CanSubmit;

            Details.Reset();
            Details+=TEXT("当前境界：")+CurrentStageDisplay+TEXT("\n目标境界：")+TargetStageDisplay+TEXT("\n");
            if(!StageName.IsEmpty())Details+=TEXT("服务突破阶段：")+StageName+TEXT("\n");
            if(HasQ)Details+=FString::Printf(TEXT("服务报价 Q：%.0f\n"),Num(Q,TEXT("q")));
            else Details+=TEXT("服务报价尚未同步\n");
            if(HasQuality)Details+=TEXT("品质档：")+(QualityBand.IsEmpty()?Str(Q,TEXT("quality")):QualityBand)+TEXT("\n");
            else Details+=TEXT("品质信息待同步\n");
            if(HasGrowth)Details+=FString::Printf(TEXT("成长预估：%.0f – %.0f\n"),Num(Q,TEXT("growth_min")),Num(Q,TEXT("growth_max")));
            else Details+=TEXT("成长预估待同步\n");

            if(Q&&Q->TryGetArrayField(TEXT("slots"),Slots)&&Slots->Num())
            {
                Details+=TEXT("材料槽位\n");
                for(const auto& Value:*Slots)
                {
                    const auto SlotObject=Value->AsObject();
                    const FString Category=Str(SlotObject,TEXT("category")).IsEmpty()?Str(SlotObject,TEXT("slot_category")):Str(SlotObject,TEXT("category"));
                    const FString CategoryDisplay=Category.IsEmpty()?FString(TEXT("材料类别待同步")):Category;
                    const FString ItemName=Str(SlotObject,TEXT("name")).IsEmpty()?Str(SlotObject,TEXT("item_name")):Str(SlotObject,TEXT("name"));
                    const int32 Count=SlotObject&&SlotObject->HasField(TEXT("count"))?int32(Num(SlotObject,TEXT("count"))):int32(Num(SlotObject,TEXT("quantity")));
                    Details+=FString::Printf(TEXT("%s：%s ×%d\n"),*CategoryDisplay,*(ItemName.IsEmpty()?Str(SlotObject,TEXT("item_id")):ItemName),Count);
                }
            }
            else if(HasQuotedSelection&&!QuotedCounts.IsEmpty())
            {
                Details+=TEXT("报价材料（材料类别待同步）\n");
                const auto CatalogState=Obj(State,TEXT("catalog"));
                const TArray<TSharedPtr<FJsonValue>>* CatalogRows=nullptr;
                if(CatalogState&&CatalogState->TryGetArrayField(TEXT("rows"),CatalogRows))
                {
                    for(const auto& Quoted:QuotedCounts)
                    {
                        FString ItemName=Quoted.Key;double OwnedCount=0;bool HasOwnedCount=false;
                        for(const auto& RowValue:*CatalogRows)
                        {
                            const auto Row=RowValue->AsObject();
                            if(Str(Row,TEXT("item_id"))==Quoted.Key)
                            {
                                ItemName=Str(Row,TEXT("name")).IsEmpty()?Quoted.Key:Str(Row,TEXT("name"));
                                HasOwnedCount=Row&&Row->TryGetNumberField(TEXT("owned_count"),OwnedCount);
                                break;
                            }
                        }
                        Details+=FString::Printf(TEXT("材料类别待同步：%s ×%d"),*ItemName,Quoted.Value);
                        Details+=HasOwnedCount?FString::Printf(TEXT(" · 目录库存 %.0f\n"),OwnedCount):FString(TEXT(" · 库存待同步\n"));
                    }
                }
                else Details+=TEXT("材料名称与库存待同步\n");
            }
            else if(MaterialSelection.IsEmpty())Details+=TEXT("材料要求待服务报价\n");
            else Details+=TEXT("材料数量与类别待同步\n");

            if(HasSources)
            {
                Details+=TEXT("材料来源\n");
                for(const auto& Value:*Sources){const auto Source=Value->AsObject();Details+=Str(Source,TEXT("name")).IsEmpty()?Str(Source,TEXT("source_id")):Str(Source,TEXT("name"));Details+=TEXT("\n");}
            }
            else Details+=TEXT("材料来源待同步\n");

            Details+=HasNormalizedQ?FString::Printf(TEXT("标准化评分 Q：%.0f\n"),Num(Q,TEXT("q_normalized"))):TEXT("标准化评分待同步\n");
            const bool HasTribulationChance=(T&&T->HasField(TEXT("tribulation_chance")))||(Q&&Q->HasField(TEXT("tribulation_chance")));
            if(bRework)Details+=TEXT("重炼概率：服务未提供\n");
            else if(HasTribulationChance)Details+=FString::Printf(TEXT("天劫成功率：%.0f%%\n"),T&&T->HasField(TEXT("tribulation_chance"))?Num(T,TEXT("tribulation_chance")):Num(Q,TEXT("tribulation_chance")));
            else if(HasChance)
            {
                const double ChanceBasisPoints=T&&T->HasField(TEXT("probability_basis_points"))?Num(T,TEXT("probability_basis_points")):Num(Q,TEXT("probability_basis_points"));
                Details+=FString::Printf(TEXT("天劫成功率（服务端）：%.0f%%\n"),ChanceBasisPoints/100.0);
            }
            else Details+=TEXT("天劫成功率待同步\n");

            const FString ServiceQuoteStatus=Str(Q,TEXT("status"));
            Details+=TEXT("报价状态：")+(ServiceQuoteStatus.IsEmpty()?TEXT("待同步"):ServiceQuoteStatus)+TEXT("\n");
            if(T)Details+=FString::Printf(TEXT("天劫服务状态：%s · 助力 %.0f 人 · 剩余 %.0f 秒\n"),*Str(T,TEXT("status")),Num(T,TEXT("helper_count")),FMath::Max(0.,Num(T,TEXT("deadline"))-Num(State,TEXT("server_utc"))));
            else Details+=TEXT("天劫状态待同步\n");
            const FString ReadinessStatus=!HasReadiness?FString(TEXT("服务未提供；提交时校验")):(ReadyByService?FString(TEXT("服务准备度标记通过；仍需提交校验")):Str(Readiness,TEXT("reason")));
            Details+=TEXT("准备度：")+ReadinessStatus+TEXT("\n");
            Details+=TEXT("取消/对账：")+(BreakCancelReason.IsEmpty()?(CanReconcile?FString(TEXT("可查询对账")):FString(TEXT("可在试炼开始前取消"))):BreakCancelReason)+TEXT("\n");
            if(!BreakDisplayComplete)Details+=TEXT("部分展示资料未提供；保留未知值，提交时由服务校验。\n");
            if(!BreakSubmissionReady)Details+=InkCore::EvaluateQuote(State,MaterialSelection,bRework).Reason+TEXT("\n");
            if(!HasQuotedSelection)Details+=TEXT("提交门控：服务报价缺少 selection，无法核对材料。\n");
            else if(!SelectionMatches)Details+=TEXT("提交门控：报价 selection 与当前材料选择不一致或结构无效，请重新报价。\n");
            if(HasSlots&&!SlotSelectionMatches)Details+=TEXT("展示资料：")+(SlotSelectionReason.IsEmpty()?FString(TEXT("slots item_id/count 汇总与当前选择不一致")):SlotSelectionReason)+TEXT("。\n");
            if(HasSlotCategories&&!CoreSlotsValid)Details+=TEXT("展示资料：材料槽位与选择不一致，分类资料待核对；实际材料资格由服务提交时重验。\n");
            if(bRework&&!IsReworkQuote)Details+=TEXT("提交门控：重炼需要服务明确返回 kind=rework 的报价。\n");
            if(!bRework&&IsReworkQuote)Details+=TEXT("提交门控：当前为普通突破模式，不能使用重炼报价。\n");
            const bool HasQuoteData=Q&&(Q->HasField(TEXT("q"))||Q->HasField(TEXT("slots"))||Q->HasField(TEXT("selection")));
            if(!HasQuoteData)
            {
                Details=TEXT("突破报价尚未同步。\n境界、材料资格与成功率以服务报价为准。");
                if(!W||!W->IsEnabled())Details+=TEXT("\n武灵服务尚未启用。");
                else if(!W->IsConnected())Details+=TEXT("\n武灵服务未连接。");
                else Details+=TEXT("\n正在读取突破报价。");
            }
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
            else if(Kind==TEXT("market")){bool Own=false;SelectedData->TryGetBoolField(TEXT("is_own"),Own);Details+=FString::Printf(TEXT("数量 %.0f · 单价 %.0f 灵石\n成交总价 %.0f 灵石\n手续费与实际到账以成交记录为准。"),Num(SelectedData,TEXT("count")),Num(SelectedData,TEXT("unit_price")),Num(SelectedData,TEXT("count"))*Num(SelectedData,TEXT("unit_price")));Primary=Own?TEXT("撤回自己的寄售"):TEXT("确认购买寄售");}
            else if(PageId==TEXT("WulingMarket")){Details+=FString::Printf(TEXT("上架数量 %d · 单价 %d 灵石\n标价合计 %d 灵石\n可用库存 %.0f\n费用与实际到账以成交记录为准。"),ListingQuantity,ListingPrice,ListingQuantity*ListingPrice,Num(SelectedData,TEXT("owned_count")));Primary=TEXT("确认上架");}
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
        double CatalogTotal=0;
        if(Catalog&&Str(Catalog,TEXT("kind"))==Kind&&int32(Num(Catalog,TEXT("offset")))==Offset&&Catalog->TryGetNumberField(TEXT("total"),CatalogTotal))
            Details+=FString::Printf(TEXT("\n第 %d 页 · 目录共 %.0f 项"),Offset/8+1,CatalogTotal);
        Details.ReplaceInline(TEXT("生命"),TEXT("血量"));
        TArray<FString> DetailLines;Details.ParseIntoArrayLines(DetailLines,false);int32 LineCount=0;const int32 LineCapacity=FMath::Max(8,int32((DetailW-34)/26));for(const FString& Line:DetailLines)LineCount+=FMath::Max(1,FMath::DivideAndRoundUp(Line.Len(),LineCapacity));
        const float TextHeight=FMath::Max(164.f,LineCount*38.f+24);
        const float DetailsTextTop=BreakPage?80.f:12.f;
        const float ExtraHeight=BreakPage?288.f:(PageId==TEXT("WulingEquipment")||PageId==TEXT("WulingMarket")?212.f:0.f);
        auto* DetailsCanvas=ScrollArea(TEXT("WulingDetailsScroll"),DetailX,Top,DetailW,DetailH,DetailsTextTop+TextHeight+ExtraHeight+12,DetailScroll,OldDetailOffset);
        Surface(DetailsCanvas,TEXT("WulingDetailsPaper"),InkPresentationStyle::ESurface::FeatureCard,0,0,DetailW-10,DetailsTextTop+TextHeight+ExtraHeight+12);
        if(BreakPage)
        {
            Surface(DetailsCanvas,TEXT("WulingDetailsHeading"),InkPresentationStyle::ESurface::FeatureHeading,8,8,DetailW-26,64);
            Label(DetailsCanvas,TEXT("WulingBreakHeading"),TEXT("突破准备"),16,18,DetailW-50,44,25);
        }
        Label(DetailsCanvas,BreakPage?TEXT("WulingBreakData"):TEXT("WulingSelectedDetails"),Details,16,DetailsTextTop,DetailW-50,TextHeight,25);
        if(BreakPage)
        {
            Action(DetailsCanvas,TEXT("ToolClearSelection"),TEXT("清空材料"),0,DetailsTextTop+TextHeight+12,DetailW-16,Hit);
            Action(DetailsCanvas,TEXT("ToolReworkMode"),bRework?TEXT("重炼模式"):TEXT("突破模式"),0,DetailsTextTop+TextHeight+112,DetailW-16,Hit,false,false,bRework);
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
        const float RowH=Kind==TEXT("shop")?300:BreakPage?188:128,CW=(ListW-10-(Columns-1)*12)/Columns;
        auto* List=ScrollArea(TEXT("WulingCatalogScroll"),0,ListTop,ListW,ListH,FMath::Max(132.f,FMath::DivideAndRoundUp(Count,Columns)*RowH)+100,CatalogScroll,OldCatalogOffset);
        if(!HasCatalog||Count==0)Label(List,TEXT("WulingCatalogEmpty"),HasCatalog?TEXT("当前没有可用条目"):BreakPage?TEXT("材料目录尚未同步"):TEXT("目录尚未同步"),8,8,ListW-34,112,26);
        for(int32 I=0;I<Count;++I)
        {
            auto O=(*Rows)[I]->AsObject();if(!O)continue;DisplayRows.Add(O);const FString Id=TEXT("ToolRow_")+FString::FromInt(DisplayRows.Num()-1);
            const float X=(I%Columns)*(CW+12),Y=(I/Columns)*RowH;
            if(Kind==TEXT("shop"))
            {
                Surface(List,NAME_None,InkPresentationStyle::ESurface::FeatureCard,X,Y,CW,RowH-12);
                const FString Item=Str(O,TEXT("output_item"));const int32 Tile=Item==TEXT("ITEM_HEAL")?10:Item==TEXT("ITEM_SHIELD")||Item.Contains(TEXT("PAGE"))||Item.Contains(TEXT("BOOK"))?9:8;
                AtlasIcon(List,NAME_None,Tile,X+8,Y+8,64);
                Label(List,NAME_None,Str(O,TEXT("name")),X+80,Y+8,CW-88,76,26);
                Label(List,NAME_None,ShopCosts(O)+FString::Printf(TEXT("\n每日上限 %.0f"),Num(O,TEXT("daily_limit"))),X+8,Y+88,CW-16,104,24);
                Action(List,Id,TEXT("选择商品"),X,Y+200,CW,Hit,false,false,SelectedData&&Str(SelectedData,TEXT("sku"))==Str(O,TEXT("sku")))->SetIsEnabled(Ready);
            }
            else if(BreakPage&&RolloutMaterialBinding)
            {
                auto* Material=CreateWidget<UInkRolloutMaterialSlotWidget>(GetOwningPlayer());
                Material->Rename(*FString::Printf(TEXT("RolloutMaterial_%d"),I),WidgetTree);
                List->AddChildToCanvas(Material)->SetOffsets(FMargin(X,Y,CW,176));
                RolloutMaterialBinding->BindSlot(Material,FName(*Str(O,TEXT("item_id"))),FName(*Str(O,TEXT("item_id"))),nullptr,UInkRolloutMaterialSlotWidget::MaterialFrame());
            }
            else
            {
                Surface(List,NAME_None,InkPresentationStyle::ESurface::FeatureCard,X,Y,CW,RowH-12);
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
            Action(BodyCanvas,TEXT("ToolPrepare"),bRework?TEXT("确认重炼"):TEXT("准备突破"),0,Footer,(BodyWidth-12)*.6f,Hit,true)->SetIsEnabled(Ready&&RolloutMaterialBinding&&RolloutMaterialBinding->CanSubmit()&&BreakSubmissionReady);
            auto* CancelAction=Action(BodyCanvas,TEXT("ToolCancelBreak"),CanReconcile?TEXT("查询对账"):TEXT("取消准备"),(BodyWidth-12)*.6f+12,Footer,(BodyWidth-12)*.4f,Hit);CancelAction->SetIsEnabled(Ready&&BreakCancelReason.IsEmpty());
        }
        else Action(BodyCanvas,TEXT("ToolConfirm"),Primary,0,Footer,BodyWidth,Hit,true)->SetIsEnabled(Ready&&SelectedData);
    }
    RenderKey=RenderStateKey(State,!Ready)+(Active?TEXT("active"):TEXT("inactive"));bBuilding=false;
}
void UInkWulingToolsPanel::HandleRolloutMaterialFeedback(const FString& Message)
{
    if(RolloutMaterialBinding)MaterialSelection=RolloutMaterialBinding->GetSelection();
    FeedbackMessage(Message);
}
void UInkWulingToolsPanel::RefreshLiveData()
{
    if(RolloutMaterialBinding)RolloutMaterialBinding->Refresh();
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
    if(Id==TEXT("ToolSign")){HandleAction(TEXT("SignInClaim"));return;}
    if(Id==TEXT("ToolLaunch")){InkJourneyAdapter::FRequest Request;Request.Action=InkJourneyAdapter::EAction::StartLaunch;Request.bConfirmed=true;FString ActionFeedback;InkJourneyAdapter::Submit(W,IsRolloutPageActive(),Request,ActionFeedback);FeedbackMessage(ActionFeedback);return;}
    if(Id==TEXT("ToolPrepare")){if(RolloutMaterialBinding)RolloutMaterialBinding->Submit();RefreshLiveData();return;}
    if(Id==TEXT("ToolReworkMode")){bRework=!bRework;MaterialSelection.Reset();if(RolloutMaterialBinding)RolloutMaterialBinding->Configure(FrontendOwner,PageId,bRework);RebuildPanel();return;}
    if(Id==TEXT("ToolCancelBreak"))
    {
        const auto Snapshot=W->Snapshot(),Attempt=Obj(Snapshot,TEXT("tribulation"));const FString Status=Str(Attempt,TEXT("status"));
        if(Status==TEXT("Reconcile"))
        {
            const FString AttemptId=Str(Attempt,TEXT("id"));if(AttemptId.IsEmpty()){FeedbackMessage(TEXT("服务未提供可对账的 attempt id"));return;}
            auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("attempt_id"),AttemptId);W->SubmitPrototypeCommand(TEXT("ReconcileBreakthrough"),Json(A));return;
        }
        double TrialStart=0,ServerUtc=0;
        if(Status==TEXT("Prepare")&&Attempt&&Attempt->TryGetNumberField(TEXT("trial_start"),TrialStart)&&Snapshot&&Snapshot->TryGetNumberField(TEXT("server_utc"),ServerUtc)&&ServerUtc<TrialStart){W->CancelBreakthrough();return;}
        FeedbackMessage(Status==TEXT("Prepare")?TEXT("试炼已开始或服务时间字段缺失，不能取消准备"):TEXT("仅 Prepare 且试炼开始前可取消；仅 Reconcile 可查询对账"));return;
    }
    if(Id==TEXT("ToolClearSelection")){MaterialSelection.Reset();if(RolloutMaterialBinding)RolloutMaterialBinding->ClearSelection();RebuildPanel();return;}
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
    {auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("race_id"),Str(Obj(W->Snapshot(),TEXT("race")),TEXT("id")));A->SetStringField(TEXT("kind"),Id==TEXT("ToolRaceAnchor")?TEXT("anchor"):Id==TEXT("ToolRaceRepresentative")?TEXT("representative"):TEXT("common"));A->SetStringField(TEXT("action"),Id==TEXT("ToolRaceGuard")?TEXT("护阵"):Id==TEXT("ToolRaceBreak")?TEXT("破障"):TEXT("响应"));W->SubmitPrototypeCommand(TEXT("SubmitRaceAction"),Json(A));return;}
    if(Id.StartsWith(TEXT("ToolRow_")))
    {
        const int32 I=FCString::Atoi(*Id.Mid(8));if(!DisplayRows.IsValidIndex(I))return;auto O=DisplayRows[I];auto A=MakeShared<FJsonObject>();const FString Kind=CatalogKind();
        if(PageId==TEXT("WulingBreakthrough")){const FString Item=Str(O,TEXT("item_id"));const int32 OwnedCount=FMath::Max(0,int32(Num(O,TEXT("owned_count"))));const int32 SelectableCount=FMath::Min(BreakthroughMaterialCountLimit,OwnedCount);if(Item.StartsWith(TEXT("MAT_BR"))&&MaterialSelection.FindRef(Item)<SelectableCount)++MaterialSelection.FindOrAdd(Item);W->SubmitPrototypeCommand(bRework?TEXT("QuoteReworkQuality"):TEXT("QuoteBreakthrough"),TEXT("{\"selection\":")+SelectionJson()+TEXT("}"));RebuildPanel();return;}
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
