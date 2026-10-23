#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkSignInPresentation.h"
#include "Frontend/InkLeaderboardPresentation.h"
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkChestProgressTest,"BoundlessImmortality.Frontend.SignIn.ChestProgress",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkChestProgressTest::RunTest(const FString&)
{
    const TArray<int32> Monthly={3,7,14,21},Weekly={1,2,3,4,5,6,7};
    for(const auto& Days:{Monthly,Weekly})
    {
        TestEqual(TEXT("No progress before first chest"),InkSignIn::MilestoneFraction(0,Days),0.f);
        for(int32 I=0;I<Days.Num();++I)TestTrue(TEXT("Every milestone fill ends exactly at its chest center"),FMath::IsNearlyEqual(InkSignIn::MilestoneFraction(Days[I],Days),float(I)/(Days.Num()-1)));
        TestEqual(TEXT("Over-completion stays on final chest"),InkSignIn::MilestoneFraction(99,Days),1.f);
    }
    TestTrue(TEXT("Between 7 and 14 days interpolates inside that segment"),FMath::IsNearlyEqual(InkSignIn::MilestoneFraction(10.5f,Monthly),.5f));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkManualRankFadeTest,"BoundlessImmortality.Frontend.Leaderboard.ManualFade",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkManualRankFadeTest::RunTest(const FString&)
{
    InkLeaderboard::FFadeState S;S.TickRequested(1,true,NAME_None,.4f,.5f);TestEqual(TEXT("No board before view request"),S.Opacity(),0.f);
    S.TickRequested(.2f,true,TEXT("Damage"),.4f,.5f);TestTrue(TEXT("Click fades in"),S.Opacity()>0&&S.Opacity()<1);
    S.TickRequested(.2f,true,TEXT("Damage"),.4f,.5f);TestEqual(TEXT("Requested board remains visible without combat"),S.Opacity(),1.f);
    S.TickRequested(.2f,true,TEXT("CultivationSpeed"),.4f,.5f);TestEqual(TEXT("Switch fades previous category first"),S.Category,FName(TEXT("Damage")));
    S.TickRequested(.5f,true,TEXT("CultivationSpeed"),.4f,.5f);S.TickRequested(.4f,true,TEXT("CultivationSpeed"),.4f,.5f);TestEqual(TEXT("New category visible"),S.Opacity(),1.f);
    S.TickRequested(.2f,true,NAME_None,.4f,.5f);TestTrue(TEXT("Close fades out"),S.Opacity()>0&&S.Opacity()<1);
    S.TickRequested(1,true,NAME_None,.4f,.5f);TestTrue(TEXT("Closed state empty"),S.Category.IsNone());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRankDataEqualityTest,"BoundlessImmortality.Frontend.Leaderboard.DataChanges",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRankDataEqualityTest::RunTest(const FString&)
{
    FInkLeaderboardEntry A;A.ParticipantId=TEXT("viewer");A.DisplayName=FText::FromString(TEXT("剑灵"));A.RealmIndex=3;
    auto B=A;B.DisplayName=FText::FromString(TEXT("剑灵"));TestTrue(TEXT("Equivalent snapshots do not refresh rows"),InkLeaderboard::SameEntry(A,B));
    B.NumericMetric=12;TestFalse(TEXT("Changed metric is applied"),InkLeaderboard::SameEntry(A,B));B=A;B.RealmIndex=4;TestFalse(TEXT("Realm advancement changes artwork"),InkLeaderboard::SameEntry(A,B));B=A;B.AvatarUrl=TEXT("https://example.invalid/new.png");TestFalse(TEXT("New avatar URL invalidates identity"),InkLeaderboard::SameEntry(A,B));return true;
}
#endif
