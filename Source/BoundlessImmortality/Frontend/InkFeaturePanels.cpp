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

namespace
{
TSharedPtr<FJsonObject> FindActivityTemplate(const UUserWidget* Widget,const FString& Entry,const TCHAR* ExpectedCycle)
{
    if(Entry.IsEmpty())return nullptr;
    const auto Activities=WulingFrontendData::Activity(Widget);
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    if(!Activities||!Activities->TryGetArrayField(TEXT("templates"),Templates)||!Templates)return nullptr;
    for(const auto& Value:*Templates)
    {
        const auto Row=Value.IsValid()?Value->AsObject():nullptr;
        if(!Row)continue;
        const FString Id=WulingFrontendData::String(Row,TEXT("activity_id"));
        const FString Cycle=WulingFrontendData::String(Row,TEXT("cycle"));
        if(Entry.Equals(Id,ESearchCase::CaseSensitive)&&Cycle==ExpectedCycle)return Row;
    }
    return nullptr;
}

FString ActivityField(const TSharedPtr<FJsonObject>& Row,const TCHAR* Key)
{
    FString Value;
    return Row&&Row->TryGetStringField(Key,Value)&&!Value.IsEmpty()?Value:TEXT("服务未提供");
}

FString ActivityEnabled(const TSharedPtr<FJsonObject>& Row)
{
    bool Enabled=false;
    if(!Row||!Row->TryGetBoolField(TEXT("enabled"),Enabled))return TEXT("服务端未提供状态");
    return Enabled?TEXT("已启用"):TEXT("未启用");
}

FString ActivityAuthorityDetailState(const UWulingPrototypeSubsystem* Service,const TSharedPtr<FJsonObject>& Activities,const TSharedPtr<FJsonObject>& Row)
{
    if(!Service||!Service->IsEnabled())return TEXT("活动信息暂未同步");
    if(!Service->IsConnected())return TEXT("暂时无法连接，请稍后再查看。");
    if(!Activities)return TEXT("正在等待活动信息同步。");
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    if(!Activities->TryGetArrayField(TEXT("templates"),Templates)||!Templates)return TEXT("暂无活动信息");
    if(!Row)return TEXT("当前活动暂不可用。");
    return TEXT("活动资料已同步，参与条件与进度仍待确认。");
}

FString ActivityRewardStatus(const TSharedPtr<FJsonObject>& Activities,const TSharedPtr<FJsonObject>& Row)
{
    const FString Id=WulingFrontendData::String(Row,TEXT("activity_id"));
    if(Id==TEXT("ACT_SIGN"))
    {
        const FString Mode=WulingFrontendData::String(Activities,TEXT("sign_in_reward_mode"));
        double QiReward=-1;
        const bool HasQiReward=Activities&&Activities->TryGetNumberField(TEXT("sign_in_qi_reward"),QiReward);
        if(Mode==TEXT("placeholder")&&HasQiReward&&QiReward==0)
            return TEXT("本次签到仅记录出勤，不发放修为奖励。");
        return TEXT("签到奖励信息待同步。");
    }
    if(Id==TEXT("ACT_VERSE"))return TEXT("每日剑诀仅供品读，不发放数值奖励。");
    return TEXT("奖励信息待同步。");
}

}

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
PANEL_TITLE(UInkResourceRankingPanel,"资源贡献榜")
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
        Link(BodyCanvas,TEXT("WulingRights"),TEXT("查看权益"),TEXT("收益来源 · 有效期限"),6,X,Y+524,CW,148);
        Action(BodyCanvas,TEXT("Open_ResourceMap"),TEXT("查看资源地图"),X,Y+692,CW,88);
        Action(BodyCanvas,TEXT("Open_ResourceRanking"),TEXT("查看资源贡献榜"),X,Y+796,CW,88,true);return;
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

FText UInkDetailPanel::PanelTitle()const
{
    if(IsA<UInkDailyActivityDetailPanel>()||IsA<UInkTimedActivityDetailPanel>())
    {
        const auto Row=FindActivityTemplate(this,SelectedEntry,IsA<UInkTimedActivityDetailPanel>()?TEXT("limited"):TEXT("daily"));
        if(Row)return FText::FromString(ActivityField(Row,TEXT("name")));
        return FText::FromString(TEXT("活动详情 · 待同步"));
    }
    return FText::FromString(SelectedEntry.IsEmpty()?TEXT("活动详情"):SelectedEntry);
}
FString UInkDetailPanel::Description()const{return TEXT("静候仙缘，精彩即将开启。");}
int32 UInkDetailPanel::SceneTile()const{return 7;}
FString UInkDailyActivityDetailPanel::Description()const{return TEXT("日常活动资料由服务端快照提供。");}
FString UInkTimedActivityDetailPanel::Description()const{return TEXT("限时活动资料由服务端快照提供。");}
FString UInkExpeditionDetailPanel::Description()const{return TEXT("行医问道，济世修心。\n委托开放后可编队出诊并查看行程收益。");}
int32 UInkExpeditionDetailPanel::SceneTile()const{return 2;}
FString UInkRegionDetailPanel::Description()const{return TEXT("山川蕴灵，入定修行。\n区域开放后可查看产出、修炼阶段与领取收益。");}
int32 UInkRegionDetailPanel::SceneTile()const{return 4;}
FString UInkResourceDetailPanel::Description()const{return TEXT("天地珍藏，静候有缘人。\n争夺开放后可查看占领条件与资源产出。");}
int32 UInkResourceDetailPanel::SceneTile()const{return 5;}
void UInkDetailPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const bool IsActivityDetail=IsA<UInkDailyActivityDetailPanel>()||IsA<UInkTimedActivityDetailPanel>();
    const float ArtW=IsActivityDetail?0.f:Wide?BodyWidth*.42f:BodyWidth;
    const float ArtH=IsActivityDetail?0.f:Wide?300.f:200.f;
    if(IsActivityDetail)
    {
        Surface(BodyCanvas,TEXT("ActivityDetailPaper"),ESurface::FeatureCard,0,0,BodyWidth,BodyHeight);
        Surface(BodyCanvas,TEXT("ActivityDetailHeadingPaper"),ESurface::FeatureHeading,8,8,BodyWidth-16,56);
    }
    else Scene(BodyCanvas,SceneTile(),0,0,ArtW,ArtH);
    const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+24,W=BodyWidth-X;
    if(IsActivityDetail)
    {
        Label(BodyCanvas,TEXT("DetailSectionHeading"),IsA<UInkTimedActivityDetailPanel>()?TEXT("限时活动资料"):TEXT("日常活动资料"),X+16,Y+12,W-32,44,32);
        const auto* Service=WulingFrontendData::Service(this);
        const auto Activities=WulingFrontendData::Activity(this);
        const TCHAR* ExpectedCycle=IsA<UInkTimedActivityDetailPanel>()?TEXT("limited"):TEXT("daily");
        const auto Row=FindActivityTemplate(this,SelectedEntry,ExpectedCycle);
        const FString AuthorityStatus=ActivityAuthorityDetailState(Service,Activities,Row);
        const float DetailY=Y+68;
        Label(BodyCanvas,TEXT("DetailAuthorityStatus"),AuthorityStatus,X,DetailY,W,64,22);
        if(!Row)
        {
            Label(BodyCanvas,TEXT("DetailActivityEmpty"),TEXT("暂无可查看的活动\n活动开放并同步后，可在此查看条件、进度与奖励。"),X+16,DetailY+80,W-32,120,25);
            return;
        }
        if(Wide)
        {
            const float Half=(W-12)*.5f;
            Label(BodyCanvas,TEXT("DetailActivityName"),TEXT("活动名称：")+ActivityField(Row,TEXT("name")),X,DetailY+56,Half,38,22);
            Label(BodyCanvas,TEXT("DetailActivityCycle"),TEXT("周期：")+ActivityField(Row,TEXT("cycle"))+TEXT(" · ")+ActivityEnabled(Row),X+Half+12,DetailY+56,Half,38,19);
            Label(BodyCanvas,TEXT("DetailActivityDescription"),TEXT("活动说明与目标待同步"),X,DetailY+94,W,36,20);
            Label(BodyCanvas,TEXT("DetailActivityEligibility"),TEXT("参与条件待同步\n要求：")+ActivityField(Row,TEXT("profile_requirement")),X,DetailY+130,W,46,18);
            Label(BodyCanvas,TEXT("DetailActivityProgress"),TEXT("活动进度待同步"),X,DetailY+178,W,36,20);
            Label(BodyCanvas,TEXT("DetailActivityStart"),TEXT("开始时间：")+ActivityField(Row,TEXT("start_at")),X,DetailY+214,Half,40,18);
            Label(BodyCanvas,TEXT("DetailActivityDeadline"),TEXT("截止时间：")+ActivityField(Row,TEXT("end_at")),X+Half+12,DetailY+214,Half,40,18);
            Label(BodyCanvas,TEXT("DetailActivityRewardConfig"),TEXT("奖励信息：")+ActivityField(Row,TEXT("reward_budget")),X,DetailY+254,Half,46,17);
            Label(BodyCanvas,TEXT("DetailActivityRewardStatus"),ActivityRewardStatus(Activities,Row),X+Half+12,DetailY+254,Half,46,17);
        }
        else
        {
            Label(BodyCanvas,TEXT("DetailActivityName"),TEXT("活动名称：")+ActivityField(Row,TEXT("name")),X,DetailY+76,W,48,27);
            Label(BodyCanvas,TEXT("DetailActivityDescription"),TEXT("活动说明与目标待同步"),X,DetailY+128,W,48,23);
            Label(BodyCanvas,TEXT("DetailActivityCycle"),TEXT("周期：")+ActivityField(Row,TEXT("cycle"))+TEXT(" · 状态：")+ActivityEnabled(Row),X,DetailY+180,W,48,23);
            Label(BodyCanvas,TEXT("DetailActivityEligibility"),TEXT("参与条件待同步\n要求：")+ActivityField(Row,TEXT("profile_requirement")),X,DetailY+232,W,68,22);
            Label(BodyCanvas,TEXT("DetailActivityProgress"),TEXT("活动进度待同步"),X,DetailY+304,W,48,23);
            Label(BodyCanvas,TEXT("DetailActivityStart"),TEXT("开始时间：")+ActivityField(Row,TEXT("start_at")),X,DetailY+356,W,48,23);
            Label(BodyCanvas,TEXT("DetailActivityDeadline"),TEXT("截止时间：")+ActivityField(Row,TEXT("end_at")),X,DetailY+408,W,48,23);
            Label(BodyCanvas,TEXT("DetailActivityRewardConfig"),TEXT("奖励信息：")+ActivityField(Row,TEXT("reward_budget")),X,DetailY+460,W,48,23);
            Label(BodyCanvas,TEXT("DetailActivityRewardStatus"),ActivityRewardStatus(Activities,Row),X,DetailY+512,W,48,23);
        }

        const bool IsSignIn=WulingFrontendData::String(Row,TEXT("activity_id"))==TEXT("ACT_SIGN");
        const float ButtonY=Wide?DetailY+342:DetailY+616;
        if(IsSignIn)
        {
            bool Signed=false;
            const bool HasSigned=Activities&&Activities->TryGetBoolField(TEXT("signed"),Signed);
            const FString ServiceDay=WulingFrontendData::String(Activities,TEXT("day"));
            RolloutSignInDay=ServiceDay;
            const bool Connected=Service&&Service->IsEnabled()&&Service->IsConnected();
            const bool Enabled=ActivityEnabled(Row)==TEXT("已启用");
            const bool CanSubmit=Connected&&!Service->IsBusy()&&Enabled&&HasSigned&&!Signed&&!ServiceDay.IsEmpty();
            const FString SignState=!HasSigned?TEXT("签到状态未提供，暂不能提交。"):
                Signed?TEXT("服务已记录今日签到，不能重复提交。"):
                ServiceDay.IsEmpty()?TEXT("服务日期未提供，暂不能提交签到。"):
                !Connected?TEXT("签到服务未连接，暂不能提交。"):
                Service->IsBusy()?TEXT("服务请求处理中，请稍后再试。"):
                !Enabled?TEXT("authority 模板未启用，不能提交签到。"):
                TEXT("可提交签到记录；是否成功以服务确认结果为准。");
            Label(BodyCanvas,TEXT("DetailSignInStatus"),SignState,X,ButtonY-36,W,32,18);
            Action(BodyCanvas,TEXT("SignInClaim"),Signed?TEXT("今日已签到"):TEXT("提交签到记录"),X,ButtonY,W,48,true)->SetIsEnabled(CanSubmit);
        }
        else
        {
            const FString UnsupportedReason=WulingFrontendData::String(Row,TEXT("activity_id"))==TEXT("ACT_VERSE")
                ?TEXT("每日剑诀仅展示文本；当前没有参与或领奖端点。")
                :TEXT("参与条件、进度和奖励尚未同步，暂不可操作。");
            Label(BodyCanvas,TEXT("DetailActivityActionReason"),UnsupportedReason,X,ButtonY-36,W,32,18);
            Action(BodyCanvas,TEXT("DetailUnsupportedAction"),TEXT("当前无可用业务操作"),X,ButtonY,W,48,true)->SetIsEnabled(false);
        }
        return;
    }
    const FString Heading=IsA<UInkRegionDetailPanel>()?TEXT("区域条件"):IsA<UInkResourceDetailPanel>()?TEXT("参与资格"):IsA<UInkExpeditionDetailPanel>()?TEXT("委托与队伍"):IsA<UInkTimedActivityDetailPanel>()?TEXT("周期与阶段"):TEXT("目标与进度");
    Label(BodyCanvas,TEXT("DetailSectionHeading"),Heading,X,Y,W,56,40);
    Label(BodyCanvas,TEXT("DetailDescription"),Description(),X,Y+80,W,152,26);
    Label(BodyCanvas,TEXT("DetailRewardHeading"),TEXT("奖励来源 · 占位预览"),X,Y+260,W,48,28);
    const float Cell=FMath::Min(112.f,(W-32)/3);for(int32 I=0;I<3;++I)Reward(BodyCanvas,NAME_None,I,TEXT("占位"),X+I*(Cell+16),Y+328,Cell);
    Label(BodyCanvas,TEXT("DetailAvailability"),TEXT("条件和奖励尚未接入，当前页面不产生收益。"),X,Y+Cell+352,W,88,24);
    auto* Participate=Action(BodyCanvas,TEXT("Preview_Participate"),IsA<UInkExpeditionDetailPanel>()?TEXT("出诊暂未开放"):TEXT("参与条件待接入"),X,Y+Cell+464,W,88,true);
    if(IsA<UInkExpeditionDetailPanel>())Participate->SetIsEnabled(false);
}


void UInkResourceRankingPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.42f:BodyWidth,ArtH=Wide?280.f:180.f;
    Scene(BodyCanvas,3,0,0,ArtW,ArtH);
    const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+24,W=BodyWidth-X;
    auto* Service=WulingFrontendData::Service(this);
    const auto Snapshot=Service&&Service->IsEnabled()?Service->Snapshot():nullptr;
    const auto Race=WulingFrontendData::Object(Snapshot,TEXT("race"));
    const FString RaceStatus=WulingFrontendData::String(Race,TEXT("status"));
    const FString ServiceStatus=Service?Service->GetStatus():TEXT("服务不可用");
    FString State;
    if(!Service||!Service->IsEnabled())
        State=TEXT("资源竞采功能未启用\n贡献榜、周期和个人名次均不可用。服务开放后再查看。");
    else if(!Service->IsConnected()||!Snapshot.IsValid())
        State=TEXT("资源服务未连接\n贡献榜和结算记录尚未同步。恢复连接后可重新查看。");
    else if(Race.IsValid())
        State=FString::Printf(TEXT("已连接 · 当前竞采状态：%s\n当前服务仅提供单轮竞采快照，未提供资源周期榜的周期、分项、名次或个人位置。单轮分数不作为周期贡献榜。"),*RaceStatus);
    else
        State=TEXT("已连接 · 当前没有竞采快照\n当前服务未提供资源周期榜的周期、分项、名次或个人位置。不会用开服榜或礼物数据代替。");
    Label(BodyCanvas,TEXT("ResourceRankingHeading"),TEXT("资源贡献榜"),X,Y,W,56,40);
    Label(BodyCanvas,TEXT("ResourceRankingEmpty"),State,X,Y+88,W,192,26);
    Label(BodyCanvas,TEXT("ResourceRankingRules"),TEXT("榜单周期、贡献分数与个人名次将在同步后显示。"),X,Y+300,W,112,24);
    Action(BodyCanvas,TEXT("Open_WulingRace"),TEXT("查看当前竞采"),X,Y+436,W,88,true);
    AuthorityStateKey=FString::Printf(TEXT("%d|%d|%d|%s|%d|%s"),Service&&Service->IsEnabled(),Service&&Service->IsConnected(),Service&&Service->IsBusy(),*ServiceStatus,WulingFrontendData::Number(Snapshot,TEXT("version")),*RaceStatus);
}
void UInkResourceRankingPanel::RefreshLiveData()
{
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;
    auto* Service=WulingFrontendData::Service(this);
    const auto Snapshot=Service&&Service->IsEnabled()?Service->Snapshot():nullptr;
    const auto Race=WulingFrontendData::Object(Snapshot,TEXT("race"));
    const FString ServiceStatus=Service?Service->GetStatus():TEXT("服务不可用");
    const FString Key=FString::Printf(TEXT("%d|%d|%d|%s|%d|%s"),Service&&Service->IsEnabled(),Service&&Service->IsConnected(),Service&&Service->IsBusy(),*ServiceStatus,WulingFrontendData::Number(Snapshot,TEXT("version")),*WulingFrontendData::String(Race,TEXT("status")));
    if(Key!=AuthorityStateKey)RebuildPanel();
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
