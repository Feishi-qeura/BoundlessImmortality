#include "WulingPackageAcceptance.h"
#include "WulingPrototypeSubsystem.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkFeaturePanel.h"
#include "Frontend/UI/InkLaunchEventPanel.h"
#include "Frontend/InkSignInChestWidget.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/UI/InkWulingToolsPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

namespace
{
FString Field(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key)
{FString Value;if(Object)Object->TryGetStringField(Key,Value);return Value;}
TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& Parent,const TCHAR* Key)
{const TSharedPtr<FJsonObject>* Value=nullptr;return Parent&&Parent->TryGetObjectField(Key,Value)?*Value:nullptr;}
FString ActivityIdForCycle(const UWulingPrototypeSubsystem* Service,const TCHAR* Cycle)
{
    if(!Service)return FString();
    const auto Activities=Object(Service->Snapshot(),TEXT("activities"));
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    if(!Activities||!Activities->TryGetArrayField(TEXT("templates"),Templates)||!Templates)return FString();
    for(const auto& Value:*Templates)
    {
        const auto Row=Value.IsValid()?Value->AsObject():nullptr;
        if(!Row)continue;
        FString RowCycle,Id;
        if(Row->TryGetStringField(TEXT("cycle"),RowCycle)&&RowCycle==Cycle&&Row->TryGetStringField(TEXT("activity_id"),Id)&&!Id.IsEmpty())return Id;
    }
    return FString();
}
bool HasActivityCycle(const UWulingPrototypeSubsystem* Service,const TCHAR* Cycle)
{return !ActivityIdForCycle(Service,Cycle).IsEmpty();}
FString FirstPermanentActivityTitle(const UWulingPrototypeSubsystem* Service)
{
    if(!Service)return FString();
    const auto Activities=Object(Service->Snapshot(),TEXT("activities"));
    auto Title=[](const TSharedPtr<FJsonObject>& Row)
    {
        const FString Value=Field(Row,TEXT("title"));
        if(!Value.IsEmpty())return Value;
        const FString Name=Field(Row,TEXT("name"));
        return Name.IsEmpty()?FString(TEXT("活动名称未提供")):Name;
    };
    const TArray<TSharedPtr<FJsonValue>>* Categories=nullptr;
    if(Activities&&Activities->TryGetArrayField(TEXT("categories"),Categories))
    {
        for(const auto& Value:*Categories)
        {
            const auto Category=Value.IsValid()?Value->AsObject():nullptr;
            FString Id=Field(Category,TEXT("id"));
            if(Id.IsEmpty())Id=Field(Category,TEXT("type"));
            if(Id.IsEmpty())Id=Field(Category,TEXT("key"));
            if(!Id.Equals(TEXT("permanent"),ESearchCase::IgnoreCase)&&!Id.Equals(TEXT("常驻"),ESearchCase::CaseSensitive))continue;
            const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
            if(Category&&(Category->TryGetArrayField(TEXT("items"),Rows)||Category->TryGetArrayField(TEXT("activities"),Rows)))
                for(const auto& RowValue:*Rows)if(RowValue.IsValid()&&RowValue->AsObject())return Title(RowValue->AsObject());
            return FString();
        }
    }
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    if(Activities&&Activities->TryGetArrayField(TEXT("templates"),Templates))
        for(const auto& RowValue:*Templates)
        {
            const auto Row=RowValue.IsValid()?RowValue->AsObject():nullptr;
            if(Row&&Field(Row,TEXT("cycle")).Equals(TEXT("permanent"),ESearchCase::IgnoreCase))return Title(Row);
        }
    return FString();
}
bool HasVisibleWidget(const UInkFeaturePanel* Panel,const TCHAR* Name)
{
    const UWidget* Widget=Panel&&Panel->WidgetTree?Panel->WidgetTree->FindWidget(Name):nullptr;
    return Widget&&Widget->GetVisibility()!=ESlateVisibility::Collapsed&&Widget->GetVisibility()!=ESlateVisibility::Hidden;
}
FString ActivityDetailCycle(const UInkFrontendWidget* UI,FName Route)
{
    const auto Panel=UI?Cast<UInkFeaturePanel>(UI->ExtensionPages.FindRef(Route).Get()):nullptr;
    const auto Text=Panel&&Panel->WidgetTree?Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("DetailActivityCycle"))):nullptr;
    return Text?Text->GetText().ToString():FString();
}
UInkFeaturePanel* FeaturePanel(UInkFrontendWidget* UI,FName Route)
{return UI?Cast<UInkFeaturePanel>(UI->ExtensionPages.FindRef(Route).Get()):nullptr;}
}

void UWulingPackageAcceptance::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
    bRunning=FParse::Param(FCommandLine::Get(),TEXT("WulingPackageAcceptance"))&&FParse::Param(FCommandLine::Get(),TEXT("WulingPrototype"))&&FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest"));
    if(bRunning)
    {
        Collection.InitializeDependency<UWulingPrototypeSubsystem>();Authority=GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>();
        Output=FPaths::ProjectSavedDir()/TEXT("PackageAcceptance");IFileManager::Get().MakeDirectory(*Output,true);Started=FPlatformTime::Seconds();Next=Started+5;
    }
#endif
}
TStatId UWulingPackageAcceptance::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UWulingPackageAcceptance,STATGROUP_Tickables);}
UWorld* UWulingPackageAcceptance::GetTickableGameObjectWorld()const{return GetWorld();}
bool UWulingPackageAcceptance::IsTickable()const{return bRunning&&!HasAnyFlags(RF_ClassDefaultObject)&&GetWorld()&&GetWorld()->IsGameWorld();}
void UWulingPackageAcceptance::Check(const FString& Name,bool Passed)
{
    auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("name"),Name);Item->SetBoolField(TEXT("passed"),Passed);Checks.Add(MakeShared<FJsonValueObject>(Item));bPassed&=Passed;
    UE_LOG(LogTemp,Display,TEXT("PackageAcceptance %s: %s"),*Name,Passed?TEXT("passed"):TEXT("FAILED"));
}
void UWulingPackageAcceptance::Capture(const FString& Name)
{
    const FString File=Output/(Name+TEXT(".png"));Captures.Add(File);FScreenshotRequest::RequestScreenshot(File,true,false);
}
bool UWulingPackageAcceptance::Click(FName Action)
{
    auto* UI=Frontend.Get();if(!UI||!FSlateApplication::IsInitialized())return false;
    for(const auto& Button:UI->ActionButtons)if(Button&&Button->Action==Action&&Button->GetIsEnabled())
    {
        bool Visible=true;for(UWidget* Parent=Button;Parent;Parent=Parent->GetParent())Visible&=Parent->GetVisibility()!=ESlateVisibility::Collapsed&&Parent->GetVisibility()!=ESlateVisibility::Hidden;
        if(!Visible)continue;
        auto ScrollToButton=[&](){for(UWidget* Parent=Button->GetParent();Parent;Parent=Parent->GetParent())if(auto* Scroll=Cast<UScrollBox>(Parent)){Scroll->ScrollWidgetIntoView(Button,false,EDescendantScrollDestination::Center);break;}};
        const auto& Geometry=Button->GetCachedGeometry();if(Geometry.GetLocalSize().IsNearlyZero()){ScrollToButton();return false;}
        const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
        auto& Slate=FSlateApplication::Get();const FWidgetPath Hit=Slate.LocateWindowUnderMouse(Position,Slate.GetInteractiveTopLevelWindows());
        if(!Button->GetCachedWidget()||!Hit.ContainsWidget(Button->GetCachedWidget().Get()))
        {
            ScrollToButton();
            return false;
        }
        Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::Invalid,0,FModifierKeysState()));
        const bool Handled=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));return Handled;
    }
    return false;
}
void UWulingPackageAcceptance::Tick(float)
{
    const double Now=FPlatformTime::Seconds();if(Now-Started>300){Check(FString::Printf(TEXT("acceptance timeout at step %d page %d"),Step,Page),false);Finish();return;}if(Now<Next)return;Next=Now+.5;
    auto* W=Authority.Get();auto* UI=Frontend.Get();
    if(Step==0)
    {
        for(TActorIterator<AInkMenuDirector> It(GetWorld());It;++It)if(It->FrontendWidget){Director=*It;Frontend=It->FrontendWidget;break;}
        UI=Frontend.Get();if(!UI)return;Check(TEXT("packaged frontend main menu"),UI->CurrentPage==TEXT("Main"));Check(TEXT("cooked text font"),UI->TextFont.LoadSynchronous()!=nullptr);Capture(TEXT("01-main"));++Step;Next=Now+3;return;
    }
    if(!UI||!W){Check(TEXT("runtime subsystems available"),false);Finish();return;}
    if(Step==1){if(!Click(TEXT("StartGame")))return;Check(TEXT("real Slate start click"),true);++Step;Next=Now+3;return;}
    if(Step==2)
    {
        if(!W->IsConnected()||W->IsBusy())return;Check(TEXT("local authority connected"),W->IsEnabled());Check(TEXT("gameplay pawn exists"),UGameplayStatics::GetPlayerPawn(GetWorld(),0)!=nullptr);Capture(TEXT("02-hud"));
        if(!Object(W->Snapshot(),TEXT("guild")))Check(TEXT("create guild submitted"),W->CreateGuild(TEXT("验收武灵")));++Step;Next=Now+3;return;
    }
    if(Step==3){if(W->IsBusy())return;Check(TEXT("guild confirmed by authority"),Object(W->Snapshot(),TEXT("guild")).IsValid());Check(TEXT("business hub opens"),UI->OpenFeaturePanel(TEXT("Extensions"),TEXT("")));Step=30;Next=Now+2;return;}
    if(Step==30){Check(TEXT("business hub excludes legacy expedition preview"),!UI->ActionButtons.ContainsByPredicate([](const TObjectPtr<UInkFrontendActionButton>& B){return B&&B->Action==TEXT("Ext_Open_Expedition");}));Capture(TEXT("business-hub"));Step=31;Next=Now+3;return;}
    if(Step==31){Check(TEXT("world page opens"),UI->OpenFeaturePanel(TEXT("WorldCultivation"),TEXT("")));Step=4;Next=Now+2;return;}
    if(Step==4){if(!Click(TEXT("Ext_Region_南域")))return;Check(TEXT("Slate region tab"),true);++Step;Next=Now+2;return;}
    if(Step==5){if(!Click(TEXT("Ext_EnterRegion_落霞谷")))return;Check(TEXT("Slate region entry"),true);++Step;Next=Now+2;return;}
    if(Step==6){Check(TEXT("region entry reaches functional Wuling panel"),UI->CurrentPage==TEXT("Wuling"));Capture(TEXT("03-luoxia-prepare"));++Step;Next=Now+3;return;}
    if(Step==7){if(W->IsBusy()||!Click(TEXT("Ext_WulingStart")))return;Check(TEXT("Slate start selected region"),true);++Step;Next=Now+3;return;}
    if(Step==8)
    {
        if(W->IsBusy())return;const auto Round=Object(W->Snapshot(),TEXT("round"));double Stage=0;if(Round)Round->TryGetNumberField(TEXT("stage"),Stage);if(Stage!=1)return;
        bool GatherOpen=false;if(Round)Round->TryGetBoolField(TEXT("gather_route_open"),GatherOpen);
        if(!GatherOpen)
        {
            if(Click(TEXT("Ext_WulingRegionMechanic"))){Check(TEXT("real Slate opens free gathering mechanic"),true);Next=Now+3;}
            else if(!Captures.Contains(Output/TEXT("mechanic-wait.png")))
            {
                auto Debug=MakeShared<FJsonObject>();Debug->SetStringField(TEXT("page"),UI->CurrentPage.ToString());Debug->SetBoolField(TEXT("connected"),W->IsConnected());Debug->SetBoolField(TEXT("busy"),W->IsBusy());TArray<TSharedPtr<FJsonValue>> Buttons;
                for(const auto& B:UI->ActionButtons)if(B&&B->Action==TEXT("Ext_WulingRegionMechanic"))
                {auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),B->GetName());Row->SetBoolField(TEXT("enabled"),B->GetIsEnabled());const auto G=B->GetCachedGeometry();const auto P=G.LocalToAbsolute(G.GetLocalSize()*.5f);Row->SetNumberField(TEXT("x"),P.X);Row->SetNumberField(TEXT("y"),P.Y);FString Parents;for(UWidget* Parent=B->GetParent();Parent;Parent=Parent->GetParent())Parents+=Parent->GetName()+TEXT("/");Row->SetStringField(TEXT("parents"),Parents);Buttons.Add(MakeShared<FJsonValueObject>(Row));}
                Debug->SetArrayField(TEXT("buttons"),Buttons);FString Json;FJsonSerializer::Serialize(Debug,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Output/TEXT("mechanic-debug.json")));Capture(TEXT("mechanic-wait"));
            }
            return;
        }
        Check(TEXT("selected region persisted by authority"),Field(Round,TEXT("region_id"))==TEXT("luoxia"));Check(TEXT("gather route confirmed by authority"),GatherOpen);Capture(TEXT("04-luoxia-gathering"));Check(TEXT("interrupt confirmed round submitted"),W->InterruptWorldRound());++Step;Next=Now+3;return;
    }
    if(Step==9){if(W->IsBusy())return;Check(TEXT("interruption retains authority state"),Field(Object(W->Snapshot(),TEXT("round")),TEXT("status"))==TEXT("Interrupted"));Check(TEXT("third region starts"),W->StartWorldRound(TEXT("ENC_SOLO"),TEXT("gather"),TEXT("xuantie")));++Step;Next=Now+3;return;}
    if(Step==10){if(W->IsBusy())return;Check(TEXT("third region confirmed"),Field(Object(W->Snapshot(),TEXT("round")),TEXT("region_id"))==TEXT("xuantie"));Check(TEXT("third region interruption submitted"),W->InterruptWorldRound());Capture(TEXT("05-xuantie"));++Step;Next=Now+3;return;}
    if(Step==11)
    {
        const TCHAR* Routes[]={TEXT("WulingCraft"),TEXT("WulingSources"),TEXT("WulingBreakthrough"),TEXT("BreakthroughPrepare"),TEXT("Tribulation"),TEXT("WulingEquipment"),TEXT("CultivationMethods"),TEXT("EquipmentFabao"),TEXT("WulingConstruction"),TEXT("GuildConstruction"),TEXT("Wuling"),TEXT("WulingActivity"),TEXT("WulingRace"),TEXT("RaceLobby"),TEXT("RaceLive"),TEXT("RaceSettlement"),TEXT("ResourceLease"),TEXT("WulingShop"),TEXT("Shop"),TEXT("WulingMarket"),TEXT("Market"),TEXT("WulingGuild"),TEXT("WulingOrders"),TEXT("TradeOrders"),TEXT("WulingOffline"),TEXT("OfflineRewards"),TEXT("WulingRights"),TEXT("SwordSpirit"),TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune"),TEXT("Activities"),TEXT("DailyActivities"),TEXT("TimedActivities"),TEXT("PermanentActivities"),TEXT("Extensions"),TEXT("ResourceMap"),TEXT("ResourceContest"),TEXT("WorldCultivation"),TEXT("EventSettlement"),TEXT("CharacterAppearance"),TEXT("CharacterEquipment"),TEXT("CharacterSkills"),TEXT("CharacterArtifacts"),TEXT("CharacterFate"),TEXT("CharacterProfile"),TEXT("LaunchRank")};
        if(W->IsBusy())return;
        if(Page<UE_ARRAY_COUNT(Routes))
        {
            const FName Route(Routes[Page]);
            Check(FString(TEXT("open packaged page "))+Routes[Page],UI->OpenFeaturePanel(Route,TEXT("")));
            if(Route==TEXT("WulingActivity"))
            {
                const auto Panel=FeaturePanel(UI,Route);
                Check(TEXT("WulingActivity opens the organization activity tools panel"),Panel&&Panel->IsA<UInkWulingToolsPanel>());
            }
            if(Route==TEXT("BreakthroughPrepare")||Route==TEXT("Tribulation"))
                Check(FString(TEXT("contract alias resolves to breakthrough authority panel: "))+Route.ToString(),UI->CurrentPage==TEXT("WulingBreakthrough")&&FeaturePanel(UI,TEXT("WulingBreakthrough"))&&FeaturePanel(UI,TEXT("WulingBreakthrough"))->IsA<UInkWulingToolsPanel>());
            if(Route==TEXT("CultivationMethods"))
                Check(TEXT("contract methods alias resolves to WulingEquipment method catalog"),UI->CurrentPage==TEXT("WulingEquipment")&&FeaturePanel(UI,TEXT("WulingEquipment"))&&FeaturePanel(UI,TEXT("WulingEquipment"))->SelectedEntry==TEXT("methods"));
            if(Route==TEXT("RaceLobby")||Route==TEXT("RaceLive")||Route==TEXT("RaceSettlement")||Route==TEXT("ResourceLease"))
                Check(FString(TEXT("contract race-state alias resolves to authority race panel: "))+Route.ToString(),UI->CurrentPage==TEXT("WulingRace")&&FeaturePanel(UI,TEXT("WulingRace"))&&FeaturePanel(UI,TEXT("WulingRace"))->IsA<UInkWulingToolsPanel>());
            if(Route==TEXT("ItemSources")||Route==TEXT("GuildConstruction")||Route==TEXT("EquipmentFabao")||Route==TEXT("OfflineRewards")||Route==TEXT("Shop")||Route==TEXT("Market")||Route==TEXT("TradeOrders")||Route==TEXT("SwordSpirit")||Route==TEXT("EventSettlement"))
            {
                FName Expected=TEXT("WulingWorld");
                if(Route==TEXT("ItemSources"))Expected=TEXT("WulingSources");
                else if(Route==TEXT("GuildConstruction"))Expected=TEXT("WulingConstruction");
                else if(Route==TEXT("EquipmentFabao"))Expected=TEXT("CharacterEquipment");
                else if(Route==TEXT("OfflineRewards"))Expected=TEXT("WulingOffline");
                else if(Route==TEXT("Shop"))Expected=TEXT("WulingShop");
                else if(Route==TEXT("Market"))Expected=TEXT("WulingMarket");
                else if(Route==TEXT("TradeOrders"))Expected=TEXT("WulingOrders");
                else if(Route==TEXT("SwordSpirit"))Expected=TEXT("WulingRights");
                Check(FString(TEXT("contract route alias resolves to "))+Expected.ToString(),UI->CurrentPage==Expected&&FeaturePanel(UI,Expected)!=nullptr);
                if(Route==TEXT("EquipmentFabao"))
                    Check(TEXT("EquipmentFabao opens the authority-backed character equipment panel"),FeaturePanel(UI,Expected)&&FeaturePanel(UI,Expected)->IsA<UInkCharacterEquipmentPanel>()&&HasVisibleWidget(FeaturePanel(UI,Expected),TEXT("CharacterAuthorityLoadout")));
            }
            if(Route==TEXT("LaunchRank"))
                Check(TEXT("LaunchRank resolves to its authority-backed panel"),UI->CurrentPage==TEXT("LaunchRank")&&FeaturePanel(UI,Route)&&FeaturePanel(UI,Route)->IsA<UInkLaunchEventPanel>()&&HasVisibleWidget(FeaturePanel(UI,Route),TEXT("LaunchHonorExpiry")));
            if(Route==TEXT("Wuling")){Step=111;Next=Now+1;return;}
            if(Route==TEXT("ResourceMap")){Step=112;Next=Now+1;return;}
            ++Step;Next=Now+2;return;
        }
        UI->ShowPage(TEXT("Inventory"));Check(TEXT("packaged inventory opens"),UI->CurrentPage==TEXT("Inventory"));Step=39;Next=Now+3;return;
    }
    if(Step==111)
    {
        if(!Click(TEXT("Ext_Open_WulingActivity")))return;
        const auto Panel=FeaturePanel(UI,TEXT("WulingActivity"));
        Check(TEXT("Wuling organization activity entry opens its tools route"),UI->CurrentPage==TEXT("WulingActivity")&&Panel&&Panel->IsA<UInkWulingToolsPanel>());
        Step=12;Next=Now+2;return;
    }
    if(Step==112)
    {
        if(!Click(TEXT("Ext_Open_ResourceContest")))return;
        const auto Panel=FeaturePanel(UI,TEXT("ResourceContest"));
        Check(TEXT("resource map CTA opens ResourceContest route and mapped panel"),UI->CurrentPage==TEXT("ResourceContest")&&Panel&&Panel->IsA<UInkResourceContestPanel>());
        Step=113;Next=Now+1;return;
    }
    if(Step==113)
    {
        if(!Click(TEXT("Ext_Open_ResourceRanking")))return;
        const auto Panel=FeaturePanel(UI,TEXT("ResourceRanking"));
        Check(TEXT("resource contest ranking CTA opens ResourceRanking route and mapped panel"),UI->CurrentPage==TEXT("ResourceRanking")&&Panel&&Panel->IsA<UInkResourceRankingPanel>());
        Step=114;Next=Now+1;return;
    }
    if(Step==114)
    {
        UI->CloseFeaturePanel(false);
        Check(TEXT("resource ranking returns to ResourceContest"),UI->CurrentPage==TEXT("ResourceContest"));
        UI->CloseFeaturePanel(false);
        Check(TEXT("resource contest returns to ResourceMap"),UI->CurrentPage==TEXT("ResourceMap"));
        Step=12;Next=Now+2;return;
    }
    if(Step==12)
    {
        auto* Panel=UI->ExtensionPages.FindRef(UI->CurrentPage).Get();
        Check(TEXT("functional page has actual widget tree"),Panel&&Panel->WidgetTree);
        int32 Papers=0;if(Panel&&Panel->WidgetTree)Panel->WidgetTree->ForEachWidget([&](UWidget* Child){if(Child->GetFName()==TEXT("FeaturePaper"))++Papers;});
        Check(TEXT("single panel paper ")+UI->CurrentPage.ToString(),Papers==1);
        Capture(FString::Printf(TEXT("page-%02d-%s"),Page,*UI->CurrentPage.ToString()));++Page;Step=11;Next=Now+2;return;
    }
    // Screenshot requests render at frame end. Never change pages in their capture tick.
    if(Step==39){Capture(TEXT("inventory"));Step=40;Next=Now+2;return;}
    if(Step==40){UI->ShowPage(TEXT("Leaderboard"));Check(TEXT("preserved leaderboard opens"),UI->CurrentPage==TEXT("Leaderboard"));Step=400;Next=Now+3;return;}
    if(Step==400){Capture(TEXT("leaderboard"));Step=41;Next=Now+2;return;}
    if(Step==41){Check(TEXT("signin group opens for tabs"),UI->OpenFeaturePanel(TEXT("SignIn"),TEXT("")));++Step;Next=Now+2;return;}
    if(Step==42){if(!Click(TEXT("Ext_GroupTab_MonthlySignIn")))return;Check(TEXT("Slate monthly tab"),true);++Step;Next=Now+2;return;}
    if(Step==43){Capture(TEXT("signin-month-tab"));++Step;Next=Now+2;return;}
    if(Step==44){if(!Click(TEXT("Ext_GroupTab_DailyFortune")))return;Check(TEXT("Slate daily verse tab"),true);++Step;Next=Now+2;return;}
    if(Step==45){Capture(TEXT("signin-verse-tab"));Step=450;Next=Now+2;return;}
    if(Step==450){Check(TEXT("activity group opens for tabs"),UI->OpenFeaturePanel(TEXT("Activities"),TEXT("")));Step=451;Next=Now+2;return;}
    if(Step==451)
    {
        const FString Id=ActivityIdForCycle(W,TEXT("daily"));
        if(Id.IsEmpty()){Check(TEXT("daily authority template available for CTA validation"),false);Step=46;Next=Now+1;return;}
        if(!Click(FName(*(TEXT("Ext_Open_DailyActivityDetail|")+Id))))return;
        const auto DailyPanel=FeaturePanel(UI,TEXT("DailyActivityDetail"));
        Check(TEXT("daily CTA opens matching detail route and context"),UI->CurrentPage==TEXT("DailyActivityDetail")&&DailyPanel&&DailyPanel->SelectedEntry==Id);
        Step=452;Next=Now+1;return;
    }
    if(Step==452)
    {
        const auto Panel=FeaturePanel(UI,TEXT("DailyActivityDetail"));
        Check(TEXT("daily detail displays daily cycle"),ActivityDetailCycle(UI,TEXT("DailyActivityDetail")).Contains(TEXT("daily"))&&!HasVisibleWidget(Panel,TEXT("DetailActivityEmpty")));
        const FString Id=Panel?Panel->SelectedEntry:FString();
        const bool OpenedMismatch=UI->OpenFeaturePanel(TEXT("TimedActivityDetail"),Id);
        Check(TEXT("daily id rejected by timed detail route"),OpenedMismatch&&UI->CurrentPage==TEXT("TimedActivityDetail")&&HasVisibleWidget(FeaturePanel(UI,TEXT("TimedActivityDetail")),TEXT("DetailActivityEmpty")));
        Check(TEXT("return to activity group after cycle mismatch"),UI->OpenFeaturePanel(TEXT("Activities"),TEXT(""))&&UI->CurrentPage==TEXT("Activities"));
        Step=46;Next=Now+1;return;
    }
    if(Step==46){if(!Click(TEXT("Ext_GroupTab_TimedActivities")))return;Check(TEXT("Slate timed activity tab"),true);++Step;Next=Now+2;return;}
    if(Step==47){Capture(TEXT("activities-timed-tab"));Step=473;Next=Now+2;return;}
    if(Step==473)
    {
        const FString Id=ActivityIdForCycle(W,TEXT("limited"));
        if(Id.IsEmpty()){Check(TEXT("limited authority template available for CTA validation"),false);Step=471;Next=Now+1;return;}
        if(!Click(FName(*(TEXT("Ext_Open_TimedActivityDetail|")+Id))))return;
        const auto TimedPanel=FeaturePanel(UI,TEXT("TimedActivityDetail"));
        Check(TEXT("limited CTA opens matching detail route and context"),UI->CurrentPage==TEXT("TimedActivityDetail")&&TimedPanel&&TimedPanel->SelectedEntry==Id);
        Step=474;Next=Now+1;return;
    }
    if(Step==474)
    {
        const auto Panel=FeaturePanel(UI,TEXT("TimedActivityDetail"));
        Check(TEXT("timed detail displays limited cycle"),ActivityDetailCycle(UI,TEXT("TimedActivityDetail")).Contains(TEXT("limited"))&&!HasVisibleWidget(Panel,TEXT("DetailActivityEmpty")));
        const FString Id=Panel?Panel->SelectedEntry:FString();
        const bool OpenedMismatch=UI->OpenFeaturePanel(TEXT("DailyActivityDetail"),Id);
        Check(TEXT("limited id rejected by daily detail route"),OpenedMismatch&&UI->CurrentPage==TEXT("DailyActivityDetail")&&HasVisibleWidget(FeaturePanel(UI,TEXT("DailyActivityDetail")),TEXT("DetailActivityEmpty")));
        Check(TEXT("return to activity group after cycle mismatch"),UI->OpenFeaturePanel(TEXT("Activities"),TEXT(""))&&UI->CurrentPage==TEXT("Activities"));
        Step=471;Next=Now+1;return;
    }
    if(Step==471)
    {
        if(!Click(TEXT("Ext_GroupTab_PermanentActivities")))return;
        Check(TEXT("Slate permanent activity tab"),true);
        const auto Group=Cast<UInkFeatureGroupPanel>(UI->ExtensionPages.FindRef(TEXT("Activities")).Get());
        const auto Permanent=Group?Group->ContentPanels.FindRef(TEXT("PermanentActivities")).Get():nullptr;
        const FString ExpectedTitle=FirstPermanentActivityTitle(W);
        if(ExpectedTitle.IsEmpty())
            Check(TEXT("permanent page shows empty state when authority has no permanent rows"),HasVisibleWidget(Permanent,TEXT("PermanentActivitiesEmpty")));
        else
        {
            const auto Title=Permanent&&Permanent->WidgetTree?Cast<UTextBlock>(Permanent->WidgetTree->FindWidget(TEXT("PermanentActivityTitle_0"))):nullptr;
            Check(TEXT("permanent page shows the first authority row title"),Title&&HasVisibleWidget(Permanent,TEXT("PermanentActivityTitle_0"))&&Title->GetText().ToString()==ExpectedTitle&&!HasVisibleWidget(Permanent,TEXT("PermanentActivitiesEmpty")));
        }
        Step=472;Next=Now+2;return;
    }
    if(Step==472){Capture(TEXT("activities-permanent-tab"));Step=470;Next=Now+2;return;}
    if(Step==470)
    {
        if(!Click(TEXT("Ext_Open_LaunchEvent")))return;
        const auto Panel=FeaturePanel(UI,TEXT("LaunchEvent"));
        Check(TEXT("Slate launch activity entry opens LaunchEvent"),UI->CurrentPage==TEXT("LaunchEvent")&&Panel&&Panel->IsA<UInkLaunchEventPanel>());
        Step=475;Next=Now+1;return;
    }
    if(Step==475){if(!Click(TEXT("Ext_LaunchTab_honors")))return;Check(TEXT("Slate launch honors tab opens the service leaderboard section"),HasVisibleWidget(FeaturePanel(UI,TEXT("LaunchEvent")),TEXT("LaunchHonorExpiry")));Step=476;Next=Now+1;return;}
    if(Step==476)
    {
        if(!Click(TEXT("Ext_Open_LaunchRank")))return;
        const auto Panel=FeaturePanel(UI,TEXT("LaunchRank"));
        Check(TEXT("Slate launch honors CTA opens LaunchRank route and mapped panel"),UI->CurrentPage==TEXT("LaunchRank")&&Panel&&Panel->IsA<UInkLaunchEventPanel>()&&HasVisibleWidget(Panel,TEXT("LaunchHonorExpiry")));
        Step=477;Next=Now+1;return;
    }
    if(Step==477){UI->CloseFeaturePanel(false);Check(TEXT("LaunchRank returns to LaunchEvent"),UI->CurrentPage==TEXT("LaunchEvent"));Step=48;Next=Now+2;return;}
    if(Step==48){Capture(TEXT("activities-launch-entry"));Step=200;Next=Now+2;return;}
    if(Step==200){Check(TEXT("GC regression reopen monthly sign-in"),UI->OpenFeaturePanel(TEXT("MonthlySignIn"),TEXT("")));Step=201;Next=Now+1;return;}
    if(Step==201)
    {
        LifecycleChest.Reset();
        for(TObjectIterator<UInkSignInChestWidget> It;It;++It)
            if(It->GetWorld()==GetWorld()&&It->GetTypedOuter<UInkMonthlySignInPanel>()&&It->GetCachedWidget().IsValid()&&It->IsTickable()){LifecycleChest=*It;break;}
        Check(TEXT("GC regression observed a constructed monthly chest"),LifecycleChest.IsValid());
        UI->ShowPage(TEXT("HUD"));Step=202;Next=Now+1;return;
    }
    if(Step==202){GEngine->ForceGarbageCollection(true);Step=203;Next=Now+1;return;}
    if(Step==203)
    {
        bool ReleasedChestsStopped=true;
        for(TObjectIterator<UInkSignInChestWidget> It;It;++It)
            if(It->GetWorld()==GetWorld()&&!It->GetCachedWidget().IsValid())ReleasedChestsStopped&=!It->IsTickable();
        Check(TEXT("released sign-in widgets cannot tick after forced GC"),ReleasedChestsStopped);
        Check(TEXT("observed chest collected or stopped after forced GC"),!LifecycleChest.IsValid()||!LifecycleChest->IsTickable());
        Check(TEXT("HUD survives sign-in release and forced GC"),UI->CurrentPage==TEXT("HUD"));
        Step=++LifecyclePass<3?200:13;Next=Now+1;return;
    }
    if(Step==13){UI->ShowPage(TEXT("HUD"));Step=130;Next=Now+3;return;}
    if(Step==130){Capture(TEXT("final-hud"));Step=131;Next=Now+2;return;}
    if(Step==131)
    {
        auto* Cultivation=GetGameInstance()->GetSubsystem<UCultivationSubsystem>();
        Check(TEXT("synthetic announcement fixture for HUD layout only"),Cultivation!=nullptr);
        if(Cultivation)Cultivation->OnAnnouncement.Broadcast(TEXT("仙途传讯 · 同心护阵，共赴山海，修行有序，问道不息"));
        Step=132;Next=Now+1;return;
    }
    if(Step==132){Capture(TEXT("hud-announcement-fixture"));Step=14;Next=Now+2;return;}
    if(Step==14)Finish();
}
void UWulingPackageAcceptance::Finish()
{
    bRunning=false;for(const auto& File:Captures)Check(TEXT("screenshot written ")+FPaths::GetCleanFilename(File),IFileManager::Get().FileSize(*File)>1024);
    auto Report=MakeShared<FJsonObject>();Report->SetBoolField(TEXT("finished"),true);Report->SetBoolField(TEXT("passed"),bPassed);Report->SetArrayField(TEXT("checks"),Checks);
    Report->SetStringField(TEXT("boundary"),TEXT("Actual rendered Windows Development package, isolated local save; no production platform or device claim"));
    Report->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);Report->SetNumberField(TEXT("screenshot_count"),Captures.Num());
    FString Json;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));FFileHelper::SaveStringToFile(Json,*(Output/TEXT("report.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FPlatformMisc::RequestExitWithStatus(false,bPassed?0:1);
}
