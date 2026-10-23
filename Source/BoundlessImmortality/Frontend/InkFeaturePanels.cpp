#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "WulingFrontendData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "InkBufferedProgressWidget.h"
#include "Components/Border.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Wuling/WulingPrototypeSubsystem.h"

using ESurface=InkPresentationStyle::ESurface;
using EIcon=InkPresentationStyle::EIcon;

#define PANEL_TITLE(Class,Value) FText Class::PanelTitle()const{return FText::FromString(TEXT(Value));}
PANEL_TITLE(UInkFeatureHubPanel,"仙途百事")
PANEL_TITLE(UInkSignInPanel,"签到")
PANEL_TITLE(UInkMonthlySignInPanel,"每月签到")
PANEL_TITLE(UInkDailyFortunePanel,"每日剑诀")
PANEL_TITLE(UInkActivitiesPanel,"活动")
PANEL_TITLE(UInkDailyActivitiesPanel,"日常活动")
PANEL_TITLE(UInkTimedActivitiesPanel,"限时活动")
PANEL_TITLE(UInkDrawPanel,"仙缘寻宝")
PANEL_TITLE(UInkExpeditionPanel,"行医出诊")
PANEL_TITLE(UInkWorldCultivationPanel,"世界历练")
PANEL_TITLE(UInkResourceContestPanel,"资源争夺")
PANEL_TITLE(UInkCharacterProfilePanel,"角色属性")
PANEL_TITLE(UInkCharacterEquipmentPanel,"角色装备")
PANEL_TITLE(UInkCharacterSkillsPanel,"角色技能")
PANEL_TITLE(UInkCharacterArtifactsPanel,"法宝")
PANEL_TITLE(UInkCharacterFatePanel,"仙缘")
PANEL_TITLE(UInkCharacterAppearancePanel,"名片外观")
PANEL_TITLE(UInkResourceRankingPanel,"占领排行")
PANEL_TITLE(UInkResourceRewardsPanel,"争夺奖励")
#undef PANEL_TITLE

void UInkFeatureHubPanel::BuildPanelContent()
{
    const auto* Service=WulingFrontendData::Service(this);const bool ShowWuling=Service&&Service->IsEnabled();
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float HeroW=Wide?BodyWidth*.42f:BodyWidth,HeroH=Wide?340.f:200.f;
    Scene(BodyCanvas,4,0,0,HeroW,HeroH);
    Label(BodyCanvas,TEXT("JourneyHeading"),TEXT("山海寻道"),0,HeroH+16,HeroW,64,40);
    Label(BodyCanvas,TEXT("JourneyDescription"),TEXT("游历山川，积累修行。\n地区条件与产出以当前服务为准。"),0,HeroH+92,HeroW,100,26);
    Action(BodyCanvas,TEXT("Open_WorldCultivation"),TEXT("世界历练"),0,HeroH+212,HeroW,88,true);
    const float X=Wide?HeroW+28:0,Y=Wide?0:HeroH+332,W=BodyWidth-X;
    Label(BodyCanvas,TEXT("JourneySystems"),TEXT("修行百事"),X,Y,W,48,32);
    struct FEntry{const TCHAR* Route;const TCHAR* Title;const TCHAR* Caption;int32 Tile;};
    TArray<FEntry> Entries;
    if(ShowWuling){Entries.Add({TEXT("Wuling"),TEXT("武灵历练"),TEXT("成员归属 · 阶段收益"),4});Entries.Add({TEXT("WulingRights"),TEXT("我的剑灵"),TEXT("个人权益 · 来源与期限"),6});}
    else Entries.Add({TEXT("Expedition"),TEXT("行医出诊"),TEXT("旧委托预览 · 尚未开放"),2});
    Entries.Add({TEXT("ResourceContest"),TEXT("资源争夺"),TEXT("灵脉资格 · 开采权与收益"),5});
    Entries.Add({TEXT("SignIn"),TEXT("签到有礼"),TEXT("每日相伴 · 月历与剑诀"),0});
    Entries.Add({TEXT("Activities"),TEXT("仙途活动"),TEXT("日常历练 · 限时仙缘"),7});
    Entries.Add({TEXT("CharacterProfile"),TEXT("角色名片"),TEXT("属性修为 · 装备外观"),6});
    for(int32 I=0;I<Entries.Num();++I)
    {
        const auto& E=Entries[I];
        if(!Wide){Link(BodyCanvas,E.Route,E.Title,E.Caption,E.Tile,X,Y+68+I*164,W,148);continue;}
        const float CardW=(W-16)/2,CardX=X+(I%2)*(CardW+16),CardY=Y+68+(I/2)*264;
        auto* Entry=Action(BodyCanvas,FString(TEXT("Open_"))+E.Route,TEXT(""),CardX,CardY,CardW,244);
        auto Style=Entry->GetStyle();Style.SetNormal(InkPresentationStyle::Surface(ESurface::Card,FVector2D(CardW,244)));Entry->SetStyle(Style);
        AtlasIcon(BodyCanvas,NAME_None,E.Tile,CardX+16,CardY+16,48);
        Label(BodyCanvas,NAME_None,E.Title,CardX+16,CardY+84,CardW-32,48,28);
        Label(BodyCanvas,NAME_None,E.Caption,CardX+16,CardY+144,CardW-32,84,24);
    }
}


void UInkExpeditionPanel::BuildPanelContent()
{
    if(ActiveTab.IsEmpty())ActiveTab=TEXT("全部");
    const TCHAR* Tabs[]={TEXT("全部"),TEXT("紧急"),TEXT("常规"),TEXT("个人委托")};
    const float TabW=(BodyWidth-24)/4;
    for(int32 I=0;I<4;++I)Action(BodyCanvas,TEXT("Filter_")+FString(Tabs[I]),Tabs[I],I*(TabW+8),0,TabW,88,false,false,ActiveTab==Tabs[I]);
    Label(BodyCanvas,TEXT("ExpeditionPreview"),TEXT("旧委托预览 · 队伍、行程与奖励尚未开放"),0,108,BodyWidth,76,26);
    const TCHAR* Names[]={TEXT("山村义诊"),TEXT("瘟疫调查"),TEXT("名医请托")};const TCHAR* Types[]={TEXT("常规"),TEXT("紧急"),TEXT("个人委托")};
    int32 Row=0;const int32 Columns=!bPortraitLayout&&BodyWidth>=800?2:1;const float W=(BodyWidth-(Columns-1)*24)/Columns;
    for(int32 I=0;I<3;++I)if(ActiveTab==TEXT("全部")||ActiveTab==Types[I])
    {
        const float X=(Row%Columns)*(W+24),Y=204+(Row/Columns)*192;++Row;
        Link(BodyCanvas,TEXT("ExpeditionDetail|")+FString(Names[I]),Names[I],FString(Types[I])+TEXT(" · 委托与队伍说明"),2,X,Y,W,168);
    }
}

void UInkExpeditionPanel::HandlePanelAction(const FString& Id)
{if(Id.StartsWith(TEXT("Filter_"))){ActiveTab=Id.Mid(7);RebuildPanel();}else Super::HandlePanelAction(Id);}

void UInkWorldCultivationPanel::BuildPanelContent()
{
    if(ActiveTab.IsEmpty())ActiveTab=TEXT("东域");
    const TCHAR* Areas[]={TEXT("东域"),TEXT("南域"),TEXT("西域"),TEXT("北域"),TEXT("秘境")};const TCHAR* Names[]={TEXT("青云山"),TEXT("落霞谷"),TEXT("玄铁岭"),TEXT("玄冰原"),TEXT("妖兽林")};
    int32 Selected=0;const int32 TabColumns=FMath::Clamp(FMath::FloorToInt((BodyWidth+8)/96),1,5);const float TabW=(BodyWidth-(TabColumns-1)*8)/TabColumns,ArtY=FMath::DivideAndRoundUp(5,TabColumns)*104.f+8;
    for(int32 I=0;I<5;++I){Action(BodyCanvas,TEXT("Region_")+FString(Areas[I]),Areas[I],(I%TabColumns)*(TabW+8),(I/TabColumns)*104,TabW,88,false,false,ActiveTab==Areas[I]);if(ActiveTab==Areas[I])Selected=I;}
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.48f:BodyWidth,ArtH=Wide?300.f:200.f;
    Scene(BodyCanvas,4,0,ArtY,ArtW,ArtH);
    const float X=Wide?ArtW+28:0,Y=Wide?ArtY:ArtY+ArtH+28,W=BodyWidth-X;
    Label(BodyCanvas,TEXT("RegionSelected"),Names[Selected],X,Y,W,56,40);
    Label(BodyCanvas,TEXT("WorldLiveState"),TEXT("修炼状态读取中"),X,Y+76,W,64,26);
    auto* Progress=WidgetTree->ConstructWidget<UInkBufferedProgressWidget>(UInkBufferedProgressWidget::StaticClass(),TEXT("WorldCultivationProgress"));auto* Layout=BodyCanvas->AddChildToCanvas(Progress);Layout->SetOffsets(FMargin(X,Y+156,W,16));
    Label(BodyCanvas,TEXT("WorldLiveValues"),TEXT(""),X,Y+192,W,88,26);
    const auto* Authority=WulingFrontendData::Service(this);const bool LocalGameplay=Authority&&Authority->IsEnabled();
    Label(BodyCanvas,TEXT("WorldRewardHeading"),LocalGameplay?(Selected<3?TEXT("三阶段基础供给 · 进入后查看条件与产出"):TEXT("后续章节尚未开放")):TEXT("区域产出 · 占位预览"),X,Y+300,W,80,26);
    for(int32 I=0;I<3;++I)AtlasIcon(BodyCanvas,NAME_None,8+I,X+I*W/3+(W/3-64)/2,Y+396,64);
    const FString EntryAction=(LocalGameplay?FString(TEXT("EnterRegion_")):FString(TEXT("Open_RegionDetail|")))+Names[Selected];
    auto* Enter=Action(BodyCanvas,EntryAction,LocalGameplay?(Selected<3?TEXT("进入历练准备"):TEXT("尚未开放")):TEXT("区域详情"),X,Y+484,W,88,true);
    if(LocalGameplay)Enter->SetIsEnabled(Selected<3);
    RefreshLiveData();
}

void UInkWorldCultivationPanel::RefreshLiveData()
{
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;if(!State||!WidgetTree)return;const auto S=State->GetParticipantSnapshot(State->GetAnchorId());
    if(auto* Text=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("WorldLiveState"))))Text->SetText(FText::FromString(State->IsMeditating()?TEXT("主播正在静心修炼"):TEXT("主播当前未入定")));
    if(auto* Text=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("WorldLiveValues"))))Text->SetText(FText::FromString(FString::Printf(TEXT("修为 %.1f / %.1f · %.1f /秒"),S.Cultivation,S.RequiredCultivation,S.EffectiveCultivationPerSecond)));
    if(auto* Progress=Cast<UInkBufferedProgressWidget>(WidgetTree->FindWidget(TEXT("WorldCultivationProgress"))))Progress->SetFraction(S.RequiredCultivation>0?float(S.Cultivation/S.RequiredCultivation):1,ProgressContext.Update(S.ParticipantId,S.RealmIndex,S.Attributes.MaxHealth,S.RequiredCultivation));
}
void UInkWorldCultivationPanel::HandlePanelAction(const FString& Id)
{
    if(auto* Wuling=GetGameInstance()?GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;Wuling&&Wuling->IsEnabled()&&Id.StartsWith(TEXT("EnterRegion_")))
    {
        const FString Region=Id.EndsWith(TEXT("青云山"))?TEXT("qingyun"):Id.EndsWith(TEXT("落霞谷"))?TEXT("luoxia"):Id.EndsWith(TEXT("玄铁岭"))?TEXT("xuantie"):TEXT("");
        if(!Region.IsEmpty())Open(TEXT("Wuling"),Region);else FeedbackMessage(TEXT("该章节尚未开放"));return;
    }
    if(Id.StartsWith(TEXT("Region_"))){ActiveTab=Id.Mid(7);RebuildPanel();}else Super::HandlePanelAction(Id);
}

void UInkResourceContestPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    if(auto* W=WulingFrontendData::Service(this);W&&W->IsEnabled())
    {
        const TSharedPtr<FJsonObject>* Lease=nullptr;FString Status=W->GetStatus()+TEXT("\n当前没有开采权");
        if(W->Snapshot()&&W->Snapshot()->TryGetObjectField(TEXT("lease"),Lease))Status=W->GetStatus()+TEXT("\n节点：")+WulingFrontendData::String(*Lease,TEXT("node_id"))+TEXT("\n状态：")+WulingFrontendData::String(*Lease,TEXT("status"))+FString::Printf(TEXT("\n有效在线：%d秒"),WulingFrontendData::Number(*Lease,TEXT("online_seconds")));
        const float ArtW=Wide?BodyWidth*.42f:BodyWidth,ArtH=Wide?300.f:180.f;
        Scene(BodyCanvas,5,0,0,ArtW,ArtH);
        const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+24,CW=BodyWidth-X;
        Label(BodyCanvas,TEXT("ResourceHeading"),TEXT("当前开采权"),X,Y,CW,56,40);
        Label(BodyCanvas,TEXT("ResourceLeaseData"),Status,X,Y+76,CW,228,26);
        Link(BodyCanvas,TEXT("WulingRace"),TEXT("灵脉竞采"),TEXT("五类资源 · 实际武灵匹配"),5,X,Y+328,CW,176);
        Link(BodyCanvas,TEXT("WulingRights"),TEXT("查看权益"),TEXT("收益来源 · 有效期限"),6,X,Y+524,CW,148);return;
    }
    if(ActiveTab.IsEmpty())ActiveTab=TEXT("全部");
    const TCHAR* Tabs[]={TEXT("全部"),TEXT("灵矿"),TEXT("灵草"),TEXT("阵法")};const float TabW=(BodyWidth-24)/4;
    for(int32 I=0;I<4;++I)Action(BodyCanvas,TEXT("Filter_")+FString(Tabs[I]),Tabs[I],I*(TabW+8),0,TabW,88,false,false,ActiveTab==Tabs[I]);
    Label(BodyCanvas,TEXT("ResourcePreview"),TEXT("资源预览 · 开放、占用与参与条件待同步"),0,108,BodyWidth,72,26);
    const TCHAR* Names[]={TEXT("玄铁灵矿"),TEXT("千年灵药园"),TEXT("上古阵眼")};int32 Row=0;const float ListW=Wide?BodyWidth*.60f:BodyWidth;
    for(int32 I=0;I<3;++I)if(ActiveTab==TEXT("全部")||ActiveTab==Tabs[I+1])
    {
        const float Y=200+Row++*192;Link(BodyCanvas,TEXT("ResourceDetail|")+FString(Names[I]),Names[I],TEXT("条件与产出 · 查看资源说明"),5,0,Y,ListW,168);
    }
    const float X=Wide?ListW+24:0,Y=Wide?200:200+Row*192,W=BodyWidth-X;
    Label(BodyCanvas,TEXT("ResourceRecordsHeading"),TEXT("本期记录"),X,Y,W,48,32);
    Action(BodyCanvas,TEXT("Open_ResourceRanking"),TEXT("占领排行"),X,Y+72,W,88);
    Action(BodyCanvas,TEXT("Open_ResourceRewards"),TEXT("争夺奖励"),X,Y+180,W,88,true);
}

void UInkResourceContestPanel::HandlePanelAction(const FString& Id)
{if(Id.StartsWith(TEXT("Filter_"))){ActiveTab=Id.Mid(7);RebuildPanel();}else Super::HandlePanelAction(Id);}

FText UInkDetailPanel::PanelTitle()const{return FText::FromString(SelectedEntry.IsEmpty()?TEXT("活动详情"):SelectedEntry);}
FString UInkDetailPanel::Description()const{return TEXT("静候仙缘，精彩即将开启。");}
int32 UInkDetailPanel::SceneTile()const{return 7;}
FString UInkDailyActivityDetailPanel::Description()const{return TEXT("日常历练，精进修行。\n活动条件与奖励将在开放时公布。");}
FString UInkTimedActivityDetailPanel::Description()const{return TEXT("限时仙缘，静候开启。\n活动开放后可查看参与条件与奖励。");}
FString UInkExpeditionDetailPanel::Description()const{return TEXT("行医问道，济世修心。\n委托开放后可编队出诊并查看行程收益。");}
int32 UInkExpeditionDetailPanel::SceneTile()const{return 2;}
FString UInkRegionDetailPanel::Description()const{return TEXT("山川蕴灵，入定修行。\n区域开放后可查看产出、修炼阶段与领取收益。");}
int32 UInkRegionDetailPanel::SceneTile()const{return 4;}
FString UInkResourceDetailPanel::Description()const{return TEXT("天地珍藏，静候有缘人。\n争夺开放后可查看占领条件与资源产出。");}
int32 UInkResourceDetailPanel::SceneTile()const{return 5;}
void UInkDetailPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.42f:BodyWidth,ArtH=Wide?300.f:200.f;
    Scene(BodyCanvas,SceneTile(),0,0,ArtW,ArtH);
    const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+24,W=BodyWidth-X;
    const FString Heading=IsA<UInkRegionDetailPanel>()?TEXT("区域条件"):IsA<UInkResourceDetailPanel>()?TEXT("参与资格"):IsA<UInkExpeditionDetailPanel>()?TEXT("委托与队伍"):IsA<UInkTimedActivityDetailPanel>()?TEXT("周期与阶段"):TEXT("目标与进度");
    Label(BodyCanvas,TEXT("DetailSectionHeading"),Heading,X,Y,W,56,40);
    Label(BodyCanvas,TEXT("DetailDescription"),Description(),X,Y+80,W,152,26);
    Label(BodyCanvas,TEXT("DetailRewardHeading"),TEXT("奖励来源 · 占位预览"),X,Y+260,W,48,28);
    const float Cell=FMath::Min(112.f,(W-32)/3);for(int32 I=0;I<3;++I)Reward(BodyCanvas,NAME_None,I,TEXT("占位"),X+I*(Cell+16),Y+328,Cell);
    Label(BodyCanvas,TEXT("DetailAvailability"),TEXT("条件和奖励尚未接入，当前页面不产生收益。"),X,Y+Cell+352,W,88,24);
    Action(BodyCanvas,TEXT("Preview_Participate"),TEXT("参与条件待接入"),X,Y+Cell+464,W,88,true);
    if(IsA<UInkTimedActivityDetailPanel>()&&SelectedEntry==TEXT("仙缘寻宝"))Action(BodyCanvas,TEXT("Open_Draw"),TEXT("查看演示奖池"),X,Y+Cell+572,W,88);
}


void UInkResourceRankingPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.42f:BodyWidth,ArtH=Wide?280.f:180.f;
    Scene(BodyCanvas,3,0,0,ArtW,ArtH);
    const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+24,W=BodyWidth-X;
    Label(BodyCanvas,TEXT("ResourceRankingHeading"),TEXT("本期贡献记录"),X,Y,W,56,40);
    Label(BodyCanvas,TEXT("ResourceRankingEmpty"),TEXT("本期争夺尚未开启\n暂无占领排行"),X,Y+88,W,128,30);
    Label(BodyCanvas,TEXT("ResourceRankingRules"),TEXT("周期、三类得分与本人位置等待实际竞采结果。"),X,Y+252,W,120,26);
}
void UInkResourceRewardsPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.38f:BodyWidth,ArtH=Wide?260.f:160.f;
    Scene(BodyCanvas,5,0,0,ArtW,ArtH);
    Label(BodyCanvas,TEXT("ResourceRewardsEmpty"),TEXT("争夺奖励 · 占位预览\n尚无已确认的收益收据。"),0,ArtH+24,ArtW,128,28);
    const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+184,W=BodyWidth-X;
    Label(BodyCanvas,TEXT("ResourceRewardsHeading"),TEXT("奖励展示"),X,Y,W,56,40);
    const int32 Columns=3;const float Step=W/Columns,Cell=FMath::Min(128.f,Step-16);
    for(int32 I=0;I<6;++I)Reward(BodyCanvas,NAME_None,I%4,TEXT("占位"),X+(I%Columns)*Step+(Step-Cell)/2,Y+88+(I/Columns)*(Cell+24),Cell);
    Label(BodyCanvas,TEXT("ResourceRewardsReceiptState"),TEXT("图标仅为占位，不表示已获得物品。实际收益以服务确认的记录为准。"),X,Y+88+2*(Cell+24)+16,W,128,26);
}
