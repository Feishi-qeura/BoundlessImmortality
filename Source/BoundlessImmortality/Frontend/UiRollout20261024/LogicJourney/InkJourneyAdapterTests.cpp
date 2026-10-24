#if WITH_DEV_AUTOMATION_TESTS
#include "InkJourneyAdapter.h"
#include "InkIntegratedActivityPanels.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkJourneyRolloutGuardTest, "Boundless.InkUI.Rollout20261024.Journey.Guards", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkJourneyRolloutGuardTest::RunTest(const FString&)
{
    using namespace InkJourneyAdapter;
    FContext C;
    TestTrue(TEXT("Disabled is explicit"), Read(C).State == EState::Disabled);
    C.bEnabled = true;
    TestTrue(TEXT("Offline is explicit"), Read(C).State == EState::Offline);
    C.bConnected = true;
    TestTrue(TEXT("Missing snapshot is loading"), Read(C).State == EState::Loading);
    C.Snapshot = MakeShared<FJsonObject>(); C.bActivePage = true;
    auto A = MakeShared<FJsonObject>(); C.Snapshot->SetObjectField(TEXT("activities"), A);
    TestFalse(TEXT("Missing list is unknown"), Read(C).WeeklyChests.bProvided);
    A->SetArrayField(TEXT("weekly_chests"), {});
    TestTrue(TEXT("Explicit empty is provided"), Read(C).WeeklyChests.bProvided);
    TestFalse(TEXT("Missing signed is unknown"), Read(C).Signed.IsSet());
    FRequest R; R.ExpectedDay = TEXT("2026-10-24");
    TestFalse(TEXT("Incomplete sign data is denied"), Evaluate(C,R).bAllowed);
    A->SetStringField(TEXT("day"), R.ExpectedDay); A->SetBoolField(TEXT("signed"), false);
    auto T = MakeShared<FJsonObject>(); T->SetStringField(TEXT("activity_id"), TEXT("ACT_SIGN"));
    T->SetStringField(TEXT("cycle"), TEXT("daily")); T->SetBoolField(TEXT("enabled"), true);
    A->SetArrayField(TEXT("templates"), {MakeShared<FJsonValueObject>(T)});
    TestTrue(TEXT("Complete sign contract is allowed"), Evaluate(C,R).bAllowed);
    C.bBusy = true; TestFalse(TEXT("Repeated pending action is denied"), Evaluate(C,R).bAllowed); C.bBusy = false;
    C.bActivePage = false; TestFalse(TEXT("Hidden page cannot submit"), Evaluate(C,R).bAllowed); C.bActivePage = true;
    C.bConnected = false; TestFalse(TEXT("Cached snapshot never authorizes offline"), Evaluate(C,R).bAllowed); C.bConnected = true;
    A->SetStringField(TEXT("day"), TEXT("2026-10-25"));
    TestFalse(TEXT("Cross-day old confirmation is denied"), Evaluate(C,R).bAllowed);
    R.ExpectedDay = TEXT("2026-10-25"); A->SetBoolField(TEXT("signed"), true);
    TestFalse(TEXT("Signed state denies duplicate"), Evaluate(C,R).bAllowed);
    R.Action = EAction::MonthlyChest; R.Milestone = 3; R.bConfirmed = true;
    auto Chest = MakeShared<FJsonObject>(); Chest->SetNumberField(TEXT("milestone"), 3);
    Chest->SetBoolField(TEXT("eligible"), true); Chest->SetBoolField(TEXT("claimed"), false);
    A->SetArrayField(TEXT("monthly_chests"), {MakeShared<FJsonValueObject>(Chest)});
    TestTrue(TEXT("Real eligible 3-day chest accepted"), Evaluate(C,R).bAllowed);
    Chest->SetBoolField(TEXT("claimed"), true); TestFalse(TEXT("Claimed chest denied"), Evaluate(C,R).bAllowed);
    R.Milestone = 28; TestFalse(TEXT("Unsupported old 28-day milestone denied"), Evaluate(C,R).bAllowed);
    R.Action = EAction::WorldTravel; R.EntryId = TEXT("qingyun");
    auto TravelIdentity=MakeShared<FJsonObject>();TravelIdentity->SetStringField(TEXT("role"),TEXT("anchor"));TravelIdentity->SetStringField(TEXT("user"),TEXT("host"));
    C.Snapshot->SetObjectField(TEXT("identity"),TravelIdentity);
    TestFalse(TEXT("Missing geography never authorizes fallback travel"), Evaluate(C,R).bAllowed);
    auto J = MakeShared<FJsonObject>(), W = MakeShared<FJsonObject>(); C.Snapshot->SetObjectField(TEXT("journey"), J);
    W->SetStringField(TEXT("id"), R.EntryId); W->SetBoolField(TEXT("open"), true);
    J->SetArrayField(TEXT("worlds"), {MakeShared<FJsonValueObject>(W)});
    TestTrue(TEXT("Confirmed authority world accepted"), Evaluate(C,R).bAllowed);
    TravelIdentity->SetStringField(TEXT("role"),TEXT("viewer"));TestFalse(TEXT("Viewer cannot initiate world travel"),Evaluate(C,R).bAllowed);TravelIdentity->SetStringField(TEXT("role"),TEXT("anchor"));
    R.bConfirmed = false; TestFalse(TEXT("Travel requires confirmation"), Evaluate(C,R).bAllowed); R.bConfirmed = true;
    W->SetBoolField(TEXT("open"), false); TestFalse(TEXT("Closed world denied"), Evaluate(C,R).bAllowed);
    R.Action = EAction::ResourceTravel;
    auto TravelGuild=MakeShared<FJsonObject>();TravelGuild->SetStringField(TEXT("id"),TEXT("guild"));C.Snapshot->SetObjectField(TEXT("guild"),TravelGuild);
    J->SetArrayField(TEXT("resources"), {MakeShared<FJsonValueObject>(W)}); W->SetBoolField(TEXT("open"), true);
    TestFalse(TEXT("Missing resource node id denied"), Evaluate(C,R).bAllowed);
    W->SetStringField(TEXT("node_id"), TEXT("NODE_QI")); TestTrue(TEXT("Real resource accepted"), Evaluate(C,R).bAllowed);
    A->SetArrayField(TEXT("weekly_chests"), {MakeShared<FJsonValueString>(TEXT("malformed"))});
    TestFalse(TEXT("Malformed rows cannot look like successful empty data"), Read(C).WeeklyChests.bProvided);
    TestTrue(TEXT("Permanent list is known empty when templates are provided"), ActivitiesForCycle(Read(C),TEXT("permanent")).bProvided);
    TestEqual(TEXT("Daily templates are not permanent activities"), ActivitiesForCycle(Read(C),TEXT("permanent")).Items.Num(),0);
    TestEqual(TEXT("Exact daily classification"), ActivitiesForCycle(Read(C),TEXT("daily")).Items.Num(),1);
    R.Action = EAction::StartLaunch;
    TestFalse(TEXT("Missing launch field is unknown, cannot start"), Evaluate(C,R).bAllowed);
    C.Snapshot->SetField(TEXT("launch"), MakeShared<FJsonValueNull>());
    auto I = MakeShared<FJsonObject>(), G = MakeShared<FJsonObject>(), M = MakeShared<FJsonObject>();
    C.Snapshot->SetObjectField(TEXT("identity"), I); C.Snapshot->SetObjectField(TEXT("guild"), G); C.Snapshot->SetObjectField(TEXT("manifest"), M);
    I->SetStringField(TEXT("user"), TEXT("host")); I->SetStringField(TEXT("role"), TEXT("anchor"));
    G->SetStringField(TEXT("id"), TEXT("guild")); G->SetStringField(TEXT("anchor"), TEXT("host"));
    M->SetStringField(TEXT("profile"), TEXT("prototype_full")); M->SetNumberField(TEXT("window_end"), 2000);
    C.Snapshot->SetNumberField(TEXT("server_utc"), 1000);
    TestTrue(TEXT("Matching authorized host can start known absent event"), Evaluate(C,R).bAllowed);
    I->SetStringField(TEXT("role"), TEXT("viewer")); TestFalse(TEXT("Viewer cannot start event"), Evaluate(C,R).bAllowed);
    I->SetStringField(TEXT("role"), TEXT("anchor"));
    M->SetStringField(TEXT("profile"), TEXT("douyin_release_candidate")); TestFalse(TEXT("Release candidate cannot start event"), Evaluate(C,R).bAllowed);
    M->SetStringField(TEXT("profile"), TEXT("prototype_full")); C.Snapshot->SetNumberField(TEXT("server_utc"), 2000);
    TestFalse(TEXT("Expired entitlement denies event start"), Evaluate(C,R).bAllowed);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkJourneyRolloutRoutesTest, "Boundless.InkUI.Rollout20261024.Journey.Routes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkJourneyRolloutRoutesTest::RunTest(const FString&)
{
    TestTrue(TEXT("Activity player entry resolves integrated delivered shell"),UInkFeaturePanel::ClassForRoute(TEXT("Activities")).Get()==UInkIntegratedActivitiesPanel::StaticClass());
    TestTrue(TEXT("Daily route has its own reachable retained page"),UInkFeaturePanel::ClassForRoute(TEXT("DailyActivities")).Get()==UInkIntegratedDailyPanel::StaticClass());
    TestTrue(TEXT("Limited route uses authority cycle renderer"),UInkFeaturePanel::ClassForRoute(TEXT("TimedActivities")).Get()==UInkIntegratedLimitedPanel::StaticClass());
    TestTrue(TEXT("Permanent route preserves rich authority fields"),UInkFeaturePanel::ClassForRoute(TEXT("PermanentActivities")).Get()==UInkIntegratedPermanentPanel::StaticClass());
    TestTrue(TEXT("Sign group remains the actual retained parent"),UInkFeaturePanel::ClassForRoute(TEXT("SignIn")).Get()==UInkSignInGroupPanel::StaticClass());
    return true;
}
#endif
