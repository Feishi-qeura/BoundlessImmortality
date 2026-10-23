#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Font.h"
#include "Dom/JsonObject.h"

namespace {
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(K,V)?*V:nullptr;}
FString Str(const TSharedPtr<FJsonObject>& O,const TCHAR* K){FString V;if(O)O->TryGetStringField(K,V);return V;}
double Num(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;if(O)O->TryGetNumberField(K,V);return V;}
FString PhaseName(const FString& S){if(S==TEXT("Preparing"))return TEXT("准备接引");if(S==TEXT("Running"))return TEXT("历练进行中");if(S==TEXT("Settling"))return TEXT("阶段结算");if(S==TEXT("Completed"))return TEXT("本轮已结算");if(S==TEXT("Interrupted"))return TEXT("历练已中断");return TEXT("尚未开始");}
}

void UInkWulingPanel::BuildPanelContent()
{
    if(GuildNameInput)DraftName=GuildNameInput->GetText().ToString();GuildNameInput=nullptr;
    const float Gap=20,Left=bPortraitLayout?BodyWidth:(BodyWidth-Gap)*.42f,Right=bPortraitLayout?BodyWidth:BodyWidth-Left-Gap;
    const float X=bPortraitLayout?0:Left+Gap,Top=bPortraitLayout?100:0,Footer=BodyHeight-88;
    if(bPortraitLayout)
    {
        Action(BodyCanvas,TEXT("WulingViewWorld"),TEXT("历练与成长"),0,0,(BodyWidth-12)/2,88,false,false,!bGuildView);
        Action(BodyCanvas,TEXT("WulingViewGuild"),TEXT("武灵与成员"),(BodyWidth+12)/2,0,(BodyWidth-12)/2,88,false,false,bGuildView);
    }
    if(!bPortraitLayout||bGuildView)
    {
        Label(BodyCanvas,TEXT("WulingGuildName"),TEXT("武灵名册"),0,Top,Left,44,32);
        Label(BodyCanvas,TEXT("WulingCounts"),TEXT("读取参与会话"),0,Top+48,Left,76,24);
        GuildNameInput=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("WulingNameInput"));
        GuildNameInput->SetText(FText::FromString(DraftName));GuildNameInput->SetHintText(FText::FromString(TEXT("2至12个汉字")));
        FSlateFontInfo Font;Font.FontObject=FrontendOwner?FrontendOwner->TextFont.LoadSynchronous():nullptr;Font.Size=24;
        auto InputStyle=GuildNameInput->GetWidgetStyle();InputStyle.SetFont(Font);GuildNameInput->SetWidgetStyle(InputStyle);
        auto* InputLayout=BodyCanvas->AddChildToCanvas(GuildNameInput);InputLayout->SetPosition({0,Top+136});InputLayout->SetSize({Left-152,88});
        Action(BodyCanvas,TEXT("WulingCreate"),TEXT("创建武灵"),Left-140,Top+136,140,88,true);
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("WulingRosterScroll"));Scroll->SetAllowOverscroll(false);auto* ScrollLayout=BodyCanvas->AddChildToCanvas(Scroll);
        ScrollLayout->SetPosition({0,Top+236});ScrollLayout->SetSize({Left,FMath::Max(64.f,Footer-Top-248)});
        auto* Members=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("WulingMembers"));Font.Size=24;Members->SetFont(Font);Members->SetAutoWrapText(true);Members->SetColorAndOpacity(InkPresentationStyle::Ink());Scroll->AddChild(Members);
        Action(BodyCanvas,TEXT("WulingPrevious"),TEXT("上一页"),0,Footer,(Left-12)/2,88);
        Action(BodyCanvas,TEXT("WulingNext"),TEXT("下一页"),(Left+12)/2,Footer,(Left-12)/2,88);
    }
    if(!bPortraitLayout||!bGuildView)
    {
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("WulingWorldScroll"));Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6));
        auto* WorldScrollLayout=BodyCanvas->AddChildToCanvas(Scroll);WorldScrollLayout->SetPosition({X,Top});WorldScrollLayout->SetSize({Right,Footer-Top-12});
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Right-10);Size->SetHeightOverride(1040);Scroll->AddChild(Size);
        auto* WorldCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(WorldCanvas);const float Width=Right-16;
        const FString RegionName=SelectedEntry==TEXT("luoxia")?TEXT("落霞谷"):SelectedEntry==TEXT("xuantie")?TEXT("玄铁岭"):TEXT("青云山");
        Scene(WorldCanvas,SelectedEntry==TEXT("luoxia")?1:SelectedEntry==TEXT("xuantie")?2:0,0,0,Width,140);
        Label(WorldCanvas,TEXT("WulingWorldTitle"),RegionName+TEXT(" · 十分钟历练"),0,156,Width,48,30);
        Label(WorldCanvas,TEXT("WulingWorldState"),TEXT("等待权威服务"),0,212,Width,76,24);
        Label(WorldCanvas,TEXT("WulingStageRewards"),TEXT("完成三阶段：80灵石 · 12破境石\n灵草6 · 玄铁4 · 残页2"),0,300,Width,76,24);
        const TCHAR* Presets[]={TEXT("ENC_SOLO"),TEXT("ENC_SMALL"),TEXT("ENC_MEDIUM"),TEXT("ENC_LARGE")};const TCHAR* Names[]={TEXT("独行"),TEXT("小型"),TEXT("中型"),TEXT("大型")};
        for(int32 I=0;I<4;++I)Action(WorldCanvas,TEXT("WulingPreset_")+FString(Presets[I]),Names[I],I*(Width+8)/4,388,(Width-24)/4,88,false,false,Preset==Presets[I]);
        Action(WorldCanvas,TEXT("WulingRoute"),Route==TEXT("guard")?TEXT("路线：先护阵"):TEXT("路线：先采集"),0,488,Width,88);
        Label(WorldCanvas,TEXT("WulingGrowth"),TEXT("修为与破境材料读取中"),0,588,Width,112,24);
        Action(WorldCanvas,TEXT("WulingBreakthrough"),TEXT("主播突破"),0,712,(Width-12)/2,88);
        Action(WorldCanvas,TEXT("WulingInterrupt"),TEXT("结束本轮"),(Width+12)/2,712,(Width-12)/2,88);
        Action(WorldCanvas,TEXT("Open_WulingCraft"),TEXT("玩法功能"),0,812,(Width-12)/2,88);
        Action(WorldCanvas,TEXT("Open_WulingGuild"),TEXT("组织与记录"),(Width+12)/2,812,(Width-12)/2,88);
        Action(WorldCanvas,TEXT("WulingRegionMechanic"),TEXT("区域机制"),0,912,Width,88);
        Action(BodyCanvas,TEXT("WulingStart"),TEXT("开始历练"),X,Footer,Right,88,true);
    }
    RefreshLiveData();
}
void UInkWulingPanel::RefreshLiveData()
{
    auto* W=GetGameInstance()?GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;if(!W||!WidgetTree)return;
    auto Set=[&](const TCHAR* Name,const FString& Value){if(auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(Name)))T->SetText(FText::FromString(Value));};
    const auto S=W->Snapshot(),Guild=Obj(S,TEXT("guild")),Counts=Obj(S,TEXT("counts")),Round=Obj(S,TEXT("round")),Growth=Obj(S,TEXT("growth"));
    Set(TEXT("WulingGuildName"),Guild?Str(Guild,TEXT("name")):TEXT("尚未创建武灵"));
    Set(TEXT("WulingCounts"),FString::Printf(TEXT("成员 %.0f · 参与成员 %.0f · 临时助战 %.0f\n公共资材 %.0f · 声望 %.0f"),Num(Counts,TEXT("members")),Num(Counts,TEXT("participating_members")),Num(Counts,TEXT("helpers")),Num(Guild,TEXT("supply")),Num(Guild,TEXT("reputation"))));
    FString Members;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(S&&S->TryGetArrayField(TEXT("members"),Rows))for(int32 I=0;I<Rows->Num();++I)
    {const auto Member=(*Rows)[I]->AsObject();FString Name=Str(Member,TEXT("name"));Members+=FString::Printf(TEXT("%02d  %s\n"),Cursor+I+1,Name.IsEmpty()?TEXT("剑灵"):*Name);}
    if(Members.IsEmpty())Members=TEXT("暂无正式成员\n观众发送“加入”成为成员\n护阵 / 破障 / 共修 响应阶段");
    Set(TEXT("WulingMembers"),Members);
    const FString Phase=Str(Round,TEXT("status"));const int32 Elapsed=int32(Num(Round,TEXT("elapsed")));
    const FString SelectedRegion=SelectedEntry==TEXT("luoxia")?TEXT("落霞谷"):SelectedEntry==TEXT("xuantie")?TEXT("玄铁岭"):TEXT("青云山");
    const FString ActiveRegion=Str(Round,TEXT("region_name"));
    Set(TEXT("WulingWorldTitle"),((Round&&W->IsWorldActive())?(ActiveRegion.IsEmpty()?FString(TEXT("青云山")):ActiveRegion):SelectedRegion)+TEXT(" · 十分钟历练"));
    FString Current=FString::Printf(TEXT("%s · %02d:%02d / 10:00"),*PhaseName(Phase),Elapsed/60,Elapsed%60);
    if(Phase==TEXT("Running"))Current+=FString::Printf(TEXT("\n阶段 %.0f · 护阵血量 %.0f"),Num(Round,TEXT("stage")),Num(Round,TEXT("anchor_health")));
    const FString Kind=Str(Obj(Round,TEXT("region_mechanic")),TEXT("kind"));const int32 Stage=int32(Num(Round,TEXT("stage")));
    bool GatherOpen=false;if(Round)Round->TryGetBoolField(TEXT("gather_route_open"),GatherOpen);
    FString MechanicCaption=TEXT("当前区域无需额外操作");bool MechanicReady=false;
    if(Kind==TEXT("gather_route")){MechanicCaption=GatherOpen?TEXT("采药路线已开启"):TEXT("选择采药路线 · 解除药田封印");MechanicReady=Phase==TEXT("Running")&&Stage==1&&!GatherOpen;}
    else if(Kind==TEXT("weakpoint")){MechanicCaption=FString::Printf(TEXT("破障 · 开启矿脉弱点 %.0f 秒"),Num(Obj(Round,TEXT("region_mechanic")),TEXT("weakpoint_seconds")));MechanicReady=Phase==TEXT("Running")&&Stage==2;}
    if(Num(Round,TEXT("recovery_until"))>Num(S,TEXT("server_utc")))Current=TEXT("主播恢复中 · 30秒后继续\n已完成阶段奖励保留");
    Set(TEXT("WulingWorldState"),W->IsConnected()?Current:TEXT("服务未连接 · 暂停权威进度"));
    const auto Wallet=Obj(S,TEXT("wallet")),Inventory=Obj(S,TEXT("inventory"));
    Set(TEXT("WulingGrowth"),FString::Printf(TEXT("%s · 修为 %.0f / %.0f\n灵石 %.0f · 破境石 %.0f\n本次突破需 %.0f 灵石 / %.0f 破境石"),*Str(Growth,TEXT("name")),Num(Growth,TEXT("qi")),Num(Growth,TEXT("qi_to_next")),Num(Wallet,TEXT("CUR_STONE")),Num(Inventory,TEXT("MAT_BREAK")),Num(Growth,TEXT("stone_to_next")),Num(Growth,TEXT("material_to_next"))));
    const bool Ready=W->IsEnabled()&&W->IsConnected()&&!W->IsBusy();
    auto FindAction=[&](const TCHAR* Id){UInkFrontendActionButton* Found=nullptr;WidgetTree->ForEachWidget([&](UWidget* Widget){if(auto* B=Cast<UInkFrontendActionButton>(Widget);B&&B->Action==FName(Id))Found=B;});return Found;};
    auto Enable=[&](const TCHAR* Id,bool Value){if(auto* B=FindAction(Id))B->SetIsEnabled(Value);};
    Enable(TEXT("Ext_WulingCreate"),Ready&&!Guild);if(GuildNameInput)GuildNameInput->SetIsEnabled(Ready&&!Guild);
    Enable(TEXT("Ext_WulingStart"),Ready&&!W->IsWorldActive());Enable(TEXT("Ext_WulingInterrupt"),W->IsConnected()&&W->IsWorldActive());
    Enable(TEXT("Ext_WulingBreakthrough"),Ready&&Num(Growth,TEXT("qi_to_next"))>0&&Num(Growth,TEXT("qi"))>=Num(Growth,TEXT("qi_to_next"))&&Num(Inventory,TEXT("MAT_BREAK"))>=Num(Growth,TEXT("material_to_next"))&&Num(Wallet,TEXT("CUR_STONE"))>=Num(Growth,TEXT("stone_to_next")));
    if(auto* B=FindAction(TEXT("Ext_WulingRegionMechanic")))
    {B->SetIsEnabled(Ready&&MechanicReady);if(auto* Text=Cast<UTextBlock>(B->GetContent()))Text->SetText(FText::FromString(MechanicCaption));}
    if(StatusText)StatusText->SetText(FText::FromString(W->GetStatus()));
}

void UInkWulingPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("WulingViewWorld")||Id==TEXT("WulingViewGuild")){bGuildView=Id==TEXT("WulingViewGuild");RebuildPanel();return;}
    auto* W=GetGameInstance()?GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;if(!W||!W->IsEnabled()){FeedbackMessage(TEXT("请使用本机武灵原型启动入口"));return;}
    if(Id==TEXT("WulingCreate")){DraftName=GuildNameInput?GuildNameInput->GetText().ToString():DraftName;W->CreateGuild(DraftName);}
    else if(Id==TEXT("WulingStart"))W->StartWorldRound(Preset,Route,SelectedEntry==TEXT("luoxia")||SelectedEntry==TEXT("xuantie")?SelectedEntry:FString(TEXT("qingyun")));
    else if(Id==TEXT("WulingBreakthrough"))W->Breakthrough();
    else if(Id==TEXT("WulingInterrupt"))W->InterruptWorldRound();
    else if(Id==TEXT("WulingRegionMechanic"))
    {
        if(!W->RespondToRegionMechanic())FeedbackMessage(TEXT("区域响应尚未开始或正在确认，请稍后重试"));
    }
    else if(Id.StartsWith(TEXT("WulingPreset_"))&&!W->IsWorldActive()){Preset=Id.Mid(13);RebuildPanel();}
    else if(Id==TEXT("WulingRoute")&&!W->IsWorldActive()){Route=Route==TEXT("guard")?TEXT("gather"):TEXT("guard");RebuildPanel();}
    else if(Id==TEXT("WulingNext")){const auto S=W->Snapshot();if(S&&S->HasTypedField<EJson::Number>(TEXT("next_cursor"))){Cursor=int32(Num(S,TEXT("next_cursor")));W->RequestMemberPage(Cursor);}}
    else if(Id==TEXT("WulingPrevious")){Cursor=FMath::Max(0,Cursor-20);W->RequestMemberPage(Cursor);}
    else Super::HandlePanelAction(Id);
}
