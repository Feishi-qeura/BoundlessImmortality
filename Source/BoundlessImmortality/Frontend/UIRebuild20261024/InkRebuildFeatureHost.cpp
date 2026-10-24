#include "InkRebuildFeatureHost.h"
#include "InkRebuildFeatureData.h"
#include "InkRebuildTheme.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Journey/InkJourneyNewWidgets.h"
#include "Activities/RebuildActivitySecondaryWidgets.h"
#include "Activities/RebuildEconomyWidgets.h"
#include "Rank/InkRankWulingExtendedPages.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Engine/Font.h"
#include "Wuling/WulingPrototypeSubsystem.h"

namespace
{
TArray<TPair<FName,FText>> RelatedRoutes(FName Page)
{
    TArray<FName> Routes;
    const FString Id=Page.ToString();
    if(Id.Contains(TEXT("Race")))Routes={TEXT("RaceLobby"),TEXT("RaceLive"),TEXT("RaceSettlement"),TEXT("ResourceLease")};
    else if(Id.Contains(TEXT("Resource")))Routes={TEXT("ResourceContest"),TEXT("ResourceDetail"),TEXT("RaceLobby"),TEXT("ResourceLease"),TEXT("ResourceRanking"),TEXT("ResourceRewards")};
    else if(Page==TEXT("WorldCultivation")||Page==TEXT("RegionDetail")||Page==TEXT("EventSettlement"))Routes={TEXT("WorldCultivation"),TEXT("RegionDetail"),TEXT("EventSettlement")};
    else if(Page==TEXT("SignIn")||Page==TEXT("MonthlySignIn")||Page==TEXT("DailyFortune"))Routes={TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune"),TEXT("MonthPrevious"),TEXT("MonthCurrent"),TEXT("MonthNext")};
    else if(Id.Contains(TEXT("Activit"))||Id.Contains(TEXT("Launch")))Routes={TEXT("Activities"),TEXT("DailyActivities"),TEXT("TimedActivities"),TEXT("LaunchEvent"),TEXT("LaunchRank")};
    else if(Page==TEXT("BreakthroughPrepare")||Page==TEXT("Tribulation"))Routes={TEXT("BreakthroughPrepare"),TEXT("Tribulation")};
    else if(Page==TEXT("EquipmentFabao")||Page==TEXT("CultivationMethods")||Page==TEXT("CharacterAppearance"))Routes={TEXT("EquipmentFabao"),TEXT("CultivationMethods"),TEXT("CharacterAppearance")};
    else if(Page==TEXT("WulingCraft")||Page==TEXT("ItemSources"))Routes={TEXT("WulingCraft"),TEXT("ItemSources")};
    else if(Page==TEXT("Shop")||Page==TEXT("Market")||Page==TEXT("TradeOrders"))Routes={TEXT("Shop"),TEXT("Market"),TEXT("TradeOrders"),TEXT("QueryItems")};
    else if(Page==TEXT("GiftRules")||Page==TEXT("SwordSpirit"))Routes={TEXT("SwordSpirit"),TEXT("GiftRules")};
    static const TMap<FName,FString> Names={{TEXT("MonthPrevious"),TEXT("浏览上月")},{TEXT("MonthCurrent"),TEXT("返回本月")},{TEXT("MonthNext"),TEXT("浏览下月")},{TEXT("RaceLobby"),TEXT("竞采大厅")},{TEXT("RaceLive"),TEXT("竞采现场")},{TEXT("RaceSettlement"),TEXT("竞采结算")},{TEXT("ResourceLease"),TEXT("开采权")},{TEXT("ResourceContest"),TEXT("资源地图")},{TEXT("ResourceDetail"),TEXT("资源详情")},{TEXT("ResourceRanking"),TEXT("资源排名")},{TEXT("ResourceRewards"),TEXT("资源奖励")},{TEXT("WorldCultivation"),TEXT("世界地图")},{TEXT("RegionDetail"),TEXT("地域详情")},{TEXT("EventSettlement"),TEXT("事件结算")},{TEXT("SignIn"),TEXT("每日签到")},{TEXT("MonthlySignIn"),TEXT("月签奖励")},{TEXT("DailyFortune"),TEXT("每日签文")},{TEXT("Activities"),TEXT("活动中心")},{TEXT("DailyActivities"),TEXT("每日活动")},{TEXT("TimedActivities"),TEXT("限时活动")},{TEXT("LaunchEvent"),TEXT("青云初启")},{TEXT("LaunchRank"),TEXT("活动贡献榜")},{TEXT("BreakthroughPrepare"),TEXT("突破准备")},{TEXT("Tribulation"),TEXT("天劫进程")},{TEXT("EquipmentFabao"),TEXT("装备法宝")},{TEXT("CultivationMethods"),TEXT("功法")},{TEXT("CharacterAppearance"),TEXT("外观")},{TEXT("WulingCraft"),TEXT("物品炼制")},{TEXT("ItemSources"),TEXT("物品来源")},{TEXT("Shop"),TEXT("坊市商店")},{TEXT("Market"),TEXT("寄售")},{TEXT("TradeOrders"),TEXT("交易订单")},{TEXT("QueryItems"),TEXT("读取可寄售物品")},{TEXT("SwordSpirit"),TEXT("剑灵")},{TEXT("GiftRules"),TEXT("礼物规则")}};
    TArray<TPair<FName,FText>> Out;for(FName Route:Routes)if(Route!=Page&&(Route!=TEXT("QueryItems")||Page==TEXT("Market"))&&(!Route.ToString().StartsWith(TEXT("MonthP"))&&!Route.ToString().StartsWith(TEXT("MonthC"))&&!Route.ToString().StartsWith(TEXT("MonthN"))||Page==TEXT("MonthlySignIn")))Out.Emplace(Route,FText::FromString(Names.FindRef(Route)));return Out;
}
const FName RebuiltFeatureRoutes[]={TEXT("Wuling"),TEXT("SwordSpirit"),TEXT("GiftRules"),TEXT("CultivationMethods"),
TEXT("RaceLobby"),TEXT("RaceLive"),TEXT("RaceSettlement"),TEXT("ResourceLease"),TEXT("EventSettlement"),TEXT("OfflineRewards"),
TEXT("Shop"),TEXT("Market"),TEXT("TradeOrders"),TEXT("WorldCultivation"),TEXT("RegionDetail"),TEXT("ResourceContest"),TEXT("ResourceDetail"),
TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune"),TEXT("Activities"),TEXT("DailyActivities"),TEXT("TimedActivities"),
TEXT("DailyActivityDetail"),TEXT("TimedActivityDetail"),TEXT("LaunchEvent"),TEXT("LaunchRank"),TEXT("BreakthroughPrepare"),TEXT("Tribulation"),
TEXT("ItemSources"),TEXT("GuildConstruction"),TEXT("EquipmentFabao"),TEXT("Extensions"),TEXT("WulingCraft"),TEXT("CharacterAppearance"),
TEXT("ResourceRanking"),TEXT("ResourceRewards"),TEXT("WulingMembers")};
}
bool UInkRebuildFeatureHost::SupportsRoute(FName Route)
{for(FName Known:RebuiltFeatureRoutes)if(Route==Known)return true;return false;}
FName UInkRebuildFeatureHost::CanonicalRoute(FName Route,const FString& Context)
{
    if(Route==TEXT("WulingEquipment"))return Context.StartsWith(TEXT("methods"))?FName(TEXT("CultivationMethods")):FName(TEXT("EquipmentFabao"));
    static const TMap<FName,FName> Aliases={{TEXT("WulingWorld"),TEXT("WorldCultivation")},{TEXT("WulingRights"),TEXT("SwordSpirit")},
    {TEXT("WulingOffline"),TEXT("OfflineRewards")},{TEXT("WulingShop"),TEXT("Shop")},{TEXT("WulingMarket"),TEXT("Market")},
    {TEXT("WulingOrders"),TEXT("TradeOrders")},{TEXT("WulingSources"),TEXT("ItemSources")},{TEXT("WulingConstruction"),TEXT("GuildConstruction")},
    {TEXT("WulingBreakthrough"),TEXT("BreakthroughPrepare")},{TEXT("WulingActivity"),TEXT("SignIn")},{TEXT("WulingRace"),TEXT("RaceLobby")},
    {TEXT("WulingGuild"),TEXT("WulingMembers")},{TEXT("CharacterEquipment"),TEXT("EquipmentFabao")},{TEXT("CharacterArtifacts"),TEXT("EquipmentFabao")},
    {TEXT("CharacterSkills"),TEXT("CultivationMethods")},{TEXT("CharacterProfile"),TEXT("Character")},{TEXT("Expedition"),TEXT("WorldCultivation")},
    {TEXT("ExpeditionDetail"),TEXT("RegionDetail")},{TEXT("JourneyWorldDetail"),TEXT("RegionDetail")},{TEXT("JourneyResourceDetail"),TEXT("ResourceDetail")},
    {TEXT("ResourceMap"),TEXT("ResourceContest")},{TEXT("PermanentActivities"),TEXT("Activities")}};
    if(const auto* Target=Aliases.Find(Route))return *Target;return Route;
}
FName UInkRebuildFeatureHost::GetLayoutVersion()const
{return FName(*(TEXT("newui-r1-feature-")+PageId.ToString().ToLower()));}
void UInkRebuildFeatureHost::NativeOnInitialized()
{Super::NativeOnInitialized();BuildShell();}
void UInkRebuildFeatureHost::Configure(UInkFrontendWidget* InFrontend,FName InPage)
{FrontendOwner=InFrontend;PageId=InPage;bConfirmOpen=false;PendingGuildOperation.Empty();if(auto* Dialog=WidgetTree?WidgetTree->FindWidget(TEXT("NewGuildDialog")):nullptr)Dialog->SetVisibility(ESlateVisibility::Collapsed);SetIsFocusable(true);BuildShell();RefreshAuthority(true);}
void UInkRebuildFeatureHost::SetContext(const FString& Entry)
{
    if(SelectedEntry==Entry)return;SelectedEntry=Entry;SecondaryEntry.Empty();bConfirmOpen=false;
    ConfirmedMaterialSelection.Empty();SemanticKey.Empty();RefreshAuthority(true);
}
void UInkRebuildFeatureHost::BuildShell()
{
    if(RootCanvas)return;if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    RootCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewFeatureRoot_r1"));WidgetTree->RootWidget=RootCanvas;
    auto* Background=InkRebuildUI::Card(WidgetTree,RootCanvas,TEXT("NewFeatureBackground"),FVector4(0,0,720,1280));
    Background->SetBrush(InkRebuildUI::Surface(InkRebuildUI::Background(),0,false));
    BodyCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewFeatureBody"));InkRebuildUI::Place(RootCanvas,BodyCanvas,FVector4(0,0,720,1184));
    FeedbackText=InkRebuildUI::Text(WidgetTree,RootCanvas,TEXT("NewFeatureFeedback"),FText(),InkRebuildUI::ETextRole::Secondary,FVector4(24,1212,672,56));
    FeedbackText->SetVisibility(ESlateVisibility::HitTestInvisible);LayoutShell();
}
void UInkRebuildFeatureHost::LayoutShell()
{
    if(!RootCanvas)return;
    auto Move=[&](const TCHAR* Name,FVector4 Bounds){if(auto* W=WidgetTree->FindWidget(Name))if(auto* WidgetSlot=Cast<UCanvasPanelSlot>(W->Slot))WidgetSlot->SetOffsets(FMargin(Bounds.X,Bounds.Y,Bounds.Z,Bounds.W));};
    Move(TEXT("NewFeatureBackground"),FVector4(0,0,LastSize.X,LastSize.Y));
    Move(TEXT("NewFeatureBody"),FVector4(0,0,LastSize.X,FMath::Max(200.f,LastSize.Y-88)));
    Move(TEXT("NewFeatureFeedback"),FVector4(24,LastSize.Y-64,FMath::Max(88.f,LastSize.X-(RelatedRoutes(PageId).IsEmpty()?48:272)),56));
    Move(TEXT("NewFeatureRelatedToggle"),FVector4(FMath::Max(24.f,LastSize.X-224),LastSize.Y-88,200,88));
    Move(TEXT("NewFeatureRelatedMenu"),FVector4(FMath::Max(24.f,LastSize.X-344),FMath::Max(24.f,LastSize.Y-508),FMath::Min(320.f,LastSize.X-48),400));
    Move(TEXT("NewFeatureBack"),FVector4(LastSize.X-112,24,88,88));
    if(PageId==TEXT("Wuling")&&OwnerPage)if(auto* Extent=Cast<USizeBox>(WidgetTree->FindWidget(TEXT("NewFeatureWulingExtent")))){Extent->SetWidthOverride(FMath::Max(88.f,LastSize.X-64));const float Desired=OwnerPage->GetDesiredSize().Y;if(Desired>0)Extent->SetHeightOverride(FMath::Max(LastSize.Y-136,Desired));}
    const float DialogWidth=FMath::Min(600.f,LastSize.X-48);Move(TEXT("NewGuildDialog"),FVector4((LastSize.X-DialogWidth)*.5f,(LastSize.Y-420)*.5f,DialogWidth,420));Move(TEXT("NewGuildDialogPaper"),FVector4(0,0,DialogWidth,420));Move(TEXT("NewGuildDialogTitle"),FVector4(24,24,DialogWidth-48,64));Move(TEXT("NewGuildDialogSubject"),FVector4(24,88,DialogWidth-48,96));Move(TEXT("NewGuildNameInput"),FVector4(24,188,DialogWidth-48,88));Move(TEXT("NewGuildCancel"),FVector4(24,308,(DialogWidth-72)*.5f,88));Move(TEXT("NewGuildConfirm"),FVector4(48+(DialogWidth-72)*.5f,308,(DialogWidth-72)*.5f,88));
}
void UInkRebuildFeatureHost::EnsureOwnerPage()
{
    const auto Routes=RelatedRoutes(PageId);
    if(!Routes.IsEmpty()&&FrontendOwner&&!WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))
    {
        InkRebuildUI::Action(WidgetTree,RootCanvas,FrontendOwner,TEXT("NewFeatureRelatedToggle"),TEXT("Ext_ToggleRelated"),FText::FromString(TEXT("相关页面")),FVector4(LastSize.X-224,LastSize.Y-88,200,88));
        auto* Menu=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewFeatureRelatedMenu"));InkRebuildUI::Place(RootCanvas,Menu,FVector4(LastSize.X-344,LastSize.Y-508,320,400),50);
        InkRebuildUI::Card(WidgetTree,Menu,TEXT("NewFeatureRelatedPaper"),FVector4(0,0,320,400));
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewFeatureRelatedScroll"));InkRebuildUI::Place(Menu,Scroll,FVector4(8,8,304,384));
        auto* Extent=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewFeatureRelatedExtent"));Extent->SetWidthOverride(296);Extent->SetHeightOverride(Routes.Num()*100);Scroll->AddChild(Extent);
        auto* Choices=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewFeatureRelatedChoices"));Extent->SetContent(Choices);
        for(int32 Index=0;Index<Routes.Num();++Index)InkRebuildUI::Action(WidgetTree,Choices,FrontendOwner,FName(*FString::Printf(TEXT("NewRelated_%d"),Index)),Routes[Index].Key==TEXT("QueryItems")?FName(TEXT("Ext_QueryItems")):FName(*(TEXT("Ext_Open_")+Routes[Index].Key.ToString())),Routes[Index].Value,FVector4(0,Index*100,296,88));
        Menu->SetVisibility(ESlateVisibility::Collapsed);
    }
    if(OwnerPage||PurePage||!FrontendOwner||!GetOwningPlayer())return;
    UClass* Class=nullptr;
    if(PageId==TEXT("Extensions"))Class=UInkJourneyHubPageWidget::StaticClass();
    else if(PageId==TEXT("WorldCultivation"))Class=UInkJourneyWorldMapPageWidget::StaticClass();
    else if(PageId==TEXT("ResourceContest"))Class=UInkJourneyResourceMapPageWidget::StaticClass();
    else if(PageId==TEXT("RegionDetail"))Class=UInkJourneyRegionPageWidget::StaticClass();
    else if(PageId==TEXT("ResourceDetail"))Class=UInkJourneyResourceDetailPageWidget::StaticClass();
    else if(PageId==TEXT("EventSettlement"))Class=UInkJourneyWorldResultPageWidget::StaticClass();
    else if(PageId==TEXT("RaceSettlement"))Class=UInkJourneyRaceResultPageWidget::StaticClass();
    else if(PageId==TEXT("RaceLobby"))Class=UInkJourneyRaceLobbyPageWidget::StaticClass();
    else if(PageId==TEXT("RaceLive"))Class=UInkJourneyRaceLivePageWidget::StaticClass();
    else if(PageId==TEXT("ResourceLease"))Class=UInkJourneyLeasePageWidget::StaticClass();
    else if(PageId==TEXT("ResourceRanking"))Class=UInkJourneyResourceRankPageWidget::StaticClass();
    else if(PageId==TEXT("ResourceRewards"))Class=UInkJourneyResourceRewardsPageWidget::StaticClass();
    else if(PageId==TEXT("DailyFortune"))Class=UInkJourneyDailyVersePageWidget::StaticClass();
    else if(PageId==TEXT("SignIn"))Class=UInkJourneyDailySignInPageWidget::StaticClass();
    else if(PageId==TEXT("MonthlySignIn"))Class=UInkJourneyMonthlySignInPageWidget::StaticClass();
    else if(PageId==TEXT("Activities")||PageId==TEXT("DailyActivities")||PageId==TEXT("TimedActivities"))Class=URebuildActivitiesHubWidget::StaticClass();
    else if(PageId==TEXT("DailyActivityDetail")||PageId==TEXT("TimedActivityDetail"))Class=URebuildActivityCategoryDetailWidget::StaticClass();
    else if(PageId==TEXT("LaunchEvent"))Class=URebuildLaunchActivityWidget::StaticClass();
    else if(PageId==TEXT("LaunchRank"))Class=URebuildLaunchRankWidget::StaticClass();
    else if(PageId==TEXT("OfflineRewards"))Class=URebuildOfflineRewardsWidget::StaticClass();
    else if(PageId==TEXT("Shop"))Class=URebuildShopWidget::StaticClass();
    else if(PageId==TEXT("Market"))Class=URebuildConsignmentWidget::StaticClass();
    else if(PageId==TEXT("TradeOrders"))Class=URebuildOrdersWidget::StaticClass();
    else if(PageId==TEXT("Wuling"))Class=UInkWulingOverviewPage::StaticClass();
    else if(PageId==TEXT("WulingMembers"))Class=UInkWulingMembersPage::StaticClass();
    else if(PageId==TEXT("GuildConstruction"))Class=UInkGuildBuildPage::StaticClass();
    else if(PageId==TEXT("SwordSpirit"))Class=UInkSwordSpiritPage::StaticClass();
    else if(PageId==TEXT("GiftRules"))Class=UInkGiftRulesPage::StaticClass();
    if(!Class)return;
    OwnerPage=CreateWidget<UUserWidget>(GetOwningPlayer(),Class);if(!OwnerPage)return;
    CompositionName=Class->GetPathName();
    if(PageId==TEXT("Wuling"))
    {
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewFeatureWulingScroll"));PurePage=Scroll;auto* Layout=BodyCanvas->AddChildToCanvas(Scroll);Layout->SetAnchors(FAnchors(0,0,1,1));Layout->SetOffsets(FMargin(24));
        auto* Extent=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewFeatureWulingExtent"));Scroll->AddChild(Extent);Extent->SetContent(OwnerPage);
    }
    else{auto* WidgetSlot=BodyCanvas->AddChildToCanvas(OwnerPage);WidgetSlot->SetAnchors(FAnchors(0,0,1,1));WidgetSlot->SetOffsets(FMargin(24));}
    if(auto* Rank=Cast<UInkRankWulingPageBase>(OwnerPage))
    {
        Rank->SetSharedThemeFonts(LoadObject<UFont>(nullptr,TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkDisplay.F_InkDisplay")),LoadObject<UFont>(nullptr,TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkBody.F_InkBody")));
        if(!WidgetTree->FindWidget(TEXT("NewFeatureBack")))InkRebuildUI::Action(WidgetTree,RootCanvas,FrontendOwner,TEXT("NewFeatureBack"),TEXT("Ext_Back"),FText::FromString(TEXT("返回")),FVector4(LastSize.X-112,24,88,88));
    }
    if((PageId==TEXT("OfflineRewards")||PageId==TEXT("Activities")||PageId==TEXT("DailyActivities")||PageId==TEXT("TimedActivities")||PageId==TEXT("DailyActivityDetail")||PageId==TEXT("TimedActivityDetail")||PageId==TEXT("LaunchEvent"))&&!WidgetTree->FindWidget(TEXT("NewFeatureBack")))InkRebuildUI::Action(WidgetTree,RootCanvas,FrontendOwner,TEXT("NewFeatureBack"),TEXT("Ext_Back"),FText::FromString(TEXT("返回")),FVector4(LastSize.X-112,24,88,88));
    if(auto* Activity=Cast<URebuildActivityPageWidget>(OwnerPage))Activity->OnActionRequested.AddUniqueDynamic(this,&ThisClass::HandleActivity);
    if(auto* Hub=Cast<UInkJourneyHubPageWidget>(OwnerPage)){Hub->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Hub->OnNavigationRequested.AddUniqueDynamic(this,&ThisClass::HandleHubNavigation);}
    if(auto* Map=Cast<UInkJourneyMapPageWidget>(OwnerPage)){Map->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Map->OnEntrySelected.AddUniqueDynamic(this,&ThisClass::HandleJourneySelected);Map->OnTravelRequested.AddUniqueDynamic(this,&ThisClass::HandleJourneyTravel);}
    if(auto* Region=Cast<UInkJourneyRegionPageWidget>(OwnerPage)){Region->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Region->OnTravelRequested.AddUniqueDynamic(this,&ThisClass::HandleJourneyTravel);}
    if(auto* Result=Cast<UInkJourneyDetailResultPageWidget>(OwnerPage)){Result->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Result->OnTravelRequested.AddUniqueDynamic(this,&ThisClass::HandleJourneyTravel);}
    if(auto* ReadOnly=Cast<UInkJourneyReadOnlyDataPageWidget>(OwnerPage))ReadOnly->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);
    if(auto* Daily=Cast<UInkJourneyDailySignInPageWidget>(OwnerPage)){Daily->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Daily->OnClaimRequested.AddUniqueDynamic(this,&ThisClass::HandleSignIn);Daily->OnMonthlySignInRequested.AddUniqueDynamic(this,&ThisClass::HandleMonthlyNavigation);}
    if(auto* Monthly=Cast<UInkJourneyMonthlySignInPageWidget>(OwnerPage)){Monthly->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Monthly->OnMilestoneClaimRequested.AddUniqueDynamic(this,&ThisClass::HandleMilestone);}
    if(auto* Lobby=Cast<UInkJourneyRaceLobbyPageWidget>(OwnerPage)){Lobby->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Lobby->OnCancelRequested.AddUniqueDynamic(this,&ThisClass::HandleCancelQueue);}
    if(auto* Race=Cast<UInkJourneyRaceLivePageWidget>(OwnerPage)){Race->OnBackRequested.AddUniqueDynamic(this,&ThisClass::HandleBack);Race->OnRaceActionRequested.AddUniqueDynamic(this,&ThisClass::HandleRaceAction);}
    if(auto* Wuling=Cast<UInkWulingOverviewPage>(OwnerPage))Wuling->OnActionRequested.AddUniqueDynamic(this,&ThisClass::HandleWulingAction);
    if(auto* Members=Cast<UInkWulingMembersPage>(OwnerPage)){Members->OnMemberSelected.AddUniqueDynamic(this,&ThisClass::HandleMemberSelected);Members->OnPageRequested.AddUniqueDynamic(this,&ThisClass::HandleMemberPage);Members->OnRemoveRequested.AddUniqueDynamic(this,&ThisClass::HandleMemberRemove);Members->OnRetryRequested.AddUniqueDynamic(this,&ThisClass::HandleRetry);}
    if(auto* Building=Cast<UInkGuildBuildPage>(OwnerPage)){Building->OnQueryRequested.AddUniqueDynamic(this,&ThisClass::HandleBuildQuery);Building->OnBuildRequested.AddUniqueDynamic(this,&ThisClass::HandleBuild);Building->OnRetryRequested.AddUniqueDynamic(this,&ThisClass::HandleRetry);}
    if(auto* Sword=Cast<UInkSwordSpiritPage>(OwnerPage))Sword->OnOpenGiftRulesRequested.AddUniqueDynamic(this,&ThisClass::HandleGiftNavigation);
}
void UInkRebuildFeatureHost::MountPure(UWidget* Page,const FString& Name)
{
    if(!Page)return;if(PurePage)PurePage->RemoveFromParent();PurePage=Page;CompositionName=Name;
    auto* WidgetSlot=BodyCanvas->AddChildToCanvas(Page);WidgetSlot->SetAnchors(FAnchors(0,0,1,1));WidgetSlot->SetOffsets(FMargin(24));
}
UWulingPrototypeSubsystem* UInkRebuildFeatureHost::Service()const{return InkCore::Service(this);}
bool UInkRebuildFeatureHost::IsActivePage()const
{return FrontendOwner&&FrontendOwner->CurrentPage==PageId&&FrontendOwner->IsInViewport()&&FrontendOwner->IsVisible()&&ActorSubject==InkNewFeatureData::Identity(Service()?Service()->Snapshot():nullptr);}
bool UInkRebuildFeatureHost::CanDispatch()const
{
    const auto* W=Service();return IsActivePage()&&W&&W->IsEnabled()&&W->IsConnected()&&!W->IsBusy()
        &&InkNewFeatureData::String(InkNewFeatureData::Object(W->Snapshot(),TEXT("identity")),TEXT("role"))==TEXT("anchor");
}
bool UInkRebuildFeatureHost::Submit(const FString& Operation,const TSharedRef<FJsonObject>& Args)
{
    if(!CanDispatch()){Feedback(TEXT("当前页面未激活、缺少主播权限、服务未连接或请求处理中"));return false;}
    const bool Accepted=Service()->SubmitPrototypeCommand(Operation,InkNewFeatureData::Json(Args));
    Feedback(Accepted?TEXT("请求已提交，等待服务确认"):Service()->GetStatus());if(Accepted)bConfirmOpen=false;
    SemanticKey.Empty();return Accepted;
}
void UInkRebuildFeatureHost::QueryCatalog(const FString& Kind,int32 Offset,bool OwnedOnly)
{
    if(!CanDispatch()||Offset<0)return;
    RequestedCatalog=Kind;QueryOffset=Offset;bOwnedOnly=OwnedOnly;
    const bool Accepted=Service()->RequestCatalog(Kind,Offset,OwnedOnly);
    Feedback(Accepted?TEXT("正在读取目录"):Service()->GetStatus());SemanticKey.Empty();
}
void UInkRebuildFeatureHost::RefreshAuthority(bool Force)
{
    if(bRefreshing||!FrontendOwner||!RootCanvas)return;TGuardValue<bool> Guard(bRefreshing,true);
    auto* W=Service();DisplaySnapshot=W?W->Snapshot():nullptr;
    const FString Actor=InkNewFeatureData::Identity(DisplaySnapshot);
    if(!ActorSubject.IsEmpty()&&ActorSubject!=Actor)
    {bConfirmOpen=false;ConfirmedMaterialSelection.Empty();MaterialSelection.Empty();SelectedEntry.Empty();SecondaryEntry.Empty();SelectedInventoryEntry.Invalidate();if(OwnerPage)OwnerPage->RemoveFromParent();OwnerPage=nullptr;RetirePurePage();RequestedCatalog.Empty();PendingGuildOperation.Empty();if(auto* Dialog=WidgetTree->FindWidget(TEXT("NewGuildDialog")))Dialog->SetVisibility(ESlateVisibility::Collapsed);}
    ActorSubject=Actor;
    FString Projection;
    if(DisplaySnapshot){auto Stable=MakeShared<FJsonObject>(*DisplaySnapshot);if(PageId!=TEXT("Tribulation")&&PageId!=TEXT("RaceLive"))Stable->RemoveField(TEXT("server_utc"));Projection=InkNewFeatureData::Json(Stable);}
    const FString Key=Projection+(W?W->GetStatus():TEXT("服务未启用"))+FString::Printf(TEXT("|%d|%d|%d|%d|%.0f|%.0f|"),W&&W->IsBusy(),IsActivePage(),bRework,QueryOffset,LastSize.X,LastSize.Y)+SelectedEntry+SecondaryEntry;
    if(!Force&&Key==SemanticKey)return;SemanticKey=Key;
    EnsureOwnerPage();
    const FString Kind=InkNewFeatureData::CatalogKindForRoute(PageId);
    if(!Kind.IsEmpty()&&RequestedCatalog.IsEmpty()&&CanDispatch()&&!InkNewFeatureData::Catalog(DisplaySnapshot,Kind,QueryOffset))QueryCatalog(Kind,QueryOffset,bOwnedOnly);
    RefreshJourney();RefreshActivities();RefreshEconomy();RefreshRankFamily();RefreshPlayer();LayoutShell();
}
void UInkRebuildFeatureHost::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);const auto Size=Geometry.GetLocalSize();
    if(Size.X>0&&Size.Y>0&&!Size.Equals(LastSize,1)){LastSize=Size;LayoutShell();SemanticKey.Empty();}
    RefreshElapsed+=DeltaSeconds;if(RefreshElapsed>=.35f){RefreshElapsed=0;RefreshAuthority();}
}
void UInkRebuildFeatureHost::Feedback(const FString& Message)
{PreviewFeedback=Message;if(FeedbackText){FeedbackText->SetText(FText::FromString(Message));FeedbackText->SetToolTipText(FText::FromString(Message));}}
void UInkRebuildFeatureHost::Navigate(FName Route,const FString& Context)
{if(auto* Menu=WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))Menu->SetVisibility(ESlateVisibility::Collapsed);if(FrontendOwner)FrontendOwner->OpenFeaturePanel(Route,Context);}
void UInkRebuildFeatureHost::HandleBack(){if(FrontendOwner)FrontendOwner->CloseFeaturePanel(false);}
void UInkRebuildFeatureHost::HandleMonthlyNavigation(){Navigate(TEXT("MonthlySignIn"));}
void UInkRebuildFeatureHost::HandleGiftNavigation(){Navigate(TEXT("GiftRules"));}
void UInkRebuildFeatureHost::HandleRetry(){RequestedCatalog.Empty();RefreshAuthority(true);}
void UInkRebuildFeatureHost::HandleAction(FName Action)
{
    if(Action==TEXT("Back")||Action==TEXT("List")){HandleBack();return;}
    if(Action==TEXT("Close")){if(FrontendOwner)FrontendOwner->CloseFeaturePanel(true);return;}
    const FString Id=Action.ToString();if(Id==TEXT("GuildConfirm")||Id==TEXT("GuildCancel")){HandleWulingAction(Action);return;}if(Id==TEXT("ToggleRelated")){if(auto* Menu=WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))Menu->SetVisibility(Menu->IsVisible()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);return;}if(Id==TEXT("QueryItems")){QueryCatalog(TEXT("items"),0,true);return;}if(Id==TEXT("Open_MonthPrevious")||Id==TEXT("Open_MonthNext")||Id==TEXT("Open_MonthCurrent")){CalendarOffset=Id==TEXT("Open_MonthCurrent")?0:FMath::Clamp(CalendarOffset+(Id==TEXT("Open_MonthNext")?1:-1),-12,12);if(auto* Menu=WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))Menu->SetVisibility(ESlateVisibility::Collapsed);RefreshAuthority(true);return;}if(Id.StartsWith(TEXT("Open_")))Navigate(FName(*Id.Mid(5)),SelectedEntry);else Feedback(TEXT("该操作暂不可用"));
}
FReply UInkRebuildFeatureHost::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{if(Event.GetKey()==EKeys::Escape){HandleBack();return FReply::Handled();}return Super::NativeOnKeyDown(Geometry,Event);}
