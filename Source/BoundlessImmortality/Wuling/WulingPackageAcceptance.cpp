#include "WulingPackageAcceptance.h"
#include "WulingPrototypeSubsystem.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkFeaturePanel.h"
#include "Frontend/InkSignInChestWidget.h"
#include "Frontend/InkMenuDirector.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
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
        const TCHAR* Routes[]={TEXT("WulingCraft"),TEXT("WulingSources"),TEXT("WulingBreakthrough"),TEXT("WulingEquipment"),TEXT("WulingConstruction"),TEXT("WulingActivity"),TEXT("WulingRace"),TEXT("WulingShop"),TEXT("WulingMarket"),TEXT("WulingGuild"),TEXT("WulingOrders"),TEXT("WulingOffline"),TEXT("WulingRights"),TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune"),TEXT("Activities"),TEXT("DailyActivities"),TEXT("TimedActivities"),TEXT("Extensions"),TEXT("ResourceContest"),TEXT("WorldCultivation"),TEXT("CharacterAppearance"),TEXT("CharacterEquipment"),TEXT("CharacterSkills"),TEXT("CharacterArtifacts"),TEXT("CharacterFate"),TEXT("CharacterProfile")};
        if(W->IsBusy())return;
        if(Page<UE_ARRAY_COUNT(Routes)){Check(FString(TEXT("open packaged page "))+Routes[Page],UI->OpenFeaturePanel(Routes[Page],TEXT("")));++Step;Next=Now+2;return;}
        UI->ShowPage(TEXT("Inventory"));Check(TEXT("packaged inventory opens"),UI->CurrentPage==TEXT("Inventory"));Step=39;Next=Now+3;return;
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
    if(Step==450){Check(TEXT("activity group opens for tabs"),UI->OpenFeaturePanel(TEXT("Activities"),TEXT("")));Step=46;Next=Now+2;return;}
    if(Step==46){if(!Click(TEXT("Ext_GroupTab_TimedActivities")))return;Check(TEXT("Slate timed activity tab"),true);++Step;Next=Now+2;return;}
    if(Step==47){Capture(TEXT("activities-timed-tab"));Step=470;Next=Now+2;return;}
    if(Step==470){if(!Click(TEXT("Ext_Open_WulingActivity")))return;Check(TEXT("Slate launch activity entry"),UI->CurrentPage==TEXT("WulingActivity"));Step=48;Next=Now+2;return;}
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
