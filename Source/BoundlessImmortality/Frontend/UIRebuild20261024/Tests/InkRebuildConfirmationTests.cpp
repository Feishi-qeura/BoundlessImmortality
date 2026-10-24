#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/UIRebuild20261024/InkRebuildFeatureData.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRebuildConfirmedStageTest,
    "BoundlessImmortality.Frontend.NewUI.Features.ConfirmedStageSubjectRejectsChangedQuote",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRebuildConfirmedStageTest::RunTest(const FString&)
{
    auto State=MakeShared<FJsonObject>(),Quote=MakeShared<FJsonObject>(),Stage=MakeShared<FJsonObject>();
    State->SetObjectField(TEXT("breakthrough_quote"),Quote);Quote->SetObjectField(TEXT("stage"),Stage);
    Stage->SetStringField(TEXT("stage_id"),TEXT("STAGE_A"));Stage->SetNumberField(TEXT("from_realm_index"),0);Stage->SetNumberField(TEXT("to_realm_index"),1);
    const FString Captured=InkNewFeatureData::StageSubjectIdentity(State);
    TestEqual(TEXT("Zero source realm remains a valid captured identity"),Captured,FString(TEXT("STAGE_A|0|1")));
    TestTrue(TEXT("Same authoritative stage matches"),InkNewFeatureData::ConfirmedStageMatches(State,Captured));
    Quote->SetNumberField(TEXT("q"),42);Quote->SetBoolField(TEXT("readiness"),false);Stage->SetStringField(TEXT("target_stage"),TEXT("optional display changed"));
    TestTrue(TEXT("Display/readiness changes are not stage identity gates"),InkNewFeatureData::ConfirmedStageMatches(State,Captured));
    Stage->SetStringField(TEXT("stage_id"),TEXT("STAGE_B"));
    TestFalse(TEXT("A fresh quote for another stage cannot reuse confirmation"),InkNewFeatureData::ConfirmedStageMatches(State,Captured));
    Stage->SetStringField(TEXT("stage_id"),TEXT("STAGE_A"));Stage->SetNumberField(TEXT("from_realm_index"),1);
    TestFalse(TEXT("Changed source realm rejects stale confirmation"),InkNewFeatureData::ConfirmedStageMatches(State,Captured));
    Stage->SetNumberField(TEXT("from_realm_index"),0);Stage->SetNumberField(TEXT("to_realm_index"),2);
    TestFalse(TEXT("Changed destination realm rejects stale confirmation"),InkNewFeatureData::ConfirmedStageMatches(State,Captured));
    return true;
}
#endif
