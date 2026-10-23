#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkLeaderboardPresentation.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRankRowsTest,"BoundlessImmortality.Frontend.Leaderboard.Rows",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRankRowsTest::RunTest(const FString&)
{
    auto Rows=InkLeaderboard::MakeRows({},TEXT("Damage"));
    TestEqual(TEXT("Empty leaderboard retains all thirty positions"),Rows.Num(),30);
    for(int32 I=0;I<Rows.Num();++I)TestEqual(TEXT("Stable empty rank"),Rows[I].Rank,I+1);
    TArray<FInkLeaderboardEntry> Data;
    for(int32 Rank:{30,1,3,1,31,0})
    {
        FInkLeaderboardEntry E;E.Rank=Rank;E.Category=TEXT("Damage");E.DisplayName=FText::FromString(FString::FromInt(Data.Num()));
        E.TitleId=TEXT("Godslayer");Data.Add(E);
    }
    auto Other=Data[1];Other.Category=TEXT("Anchor");Data.Insert(Other,0);
    Rows=InkLeaderboard::MakeRows(Data,TEXT("Damage"));
    TestEqual(TEXT("Unordered updates preserve provider rank"),Rows[29].DisplayName.ToString(),FString(TEXT("0")));
    TestEqual(TEXT("Duplicate rank does not replace first participant"),Rows[0].DisplayName.ToString(),FString(TEXT("1")));
    TestTrue(TEXT("Missing second position remains empty"),Rows[1].DisplayName.IsEmpty());
    TestEqual(TEXT("Title remains associated with its participant"),Rows[0].TitleId,FName(TEXT("Godslayer")));
    TestEqual(TEXT("Invalid ranks do not grow list"),Rows.Num(),30);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRankFadeTest,"BoundlessImmortality.Frontend.Leaderboard.ActivityFade",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRankFadeTest::RunTest(const FString&)
{
    InkLeaderboard::FFadeState S;
    auto Step=[&](float Dt,bool HUD,bool Cult,bool Attack){S.Tick(Dt,HUD,Cult,Attack,.4f,.5f,2.f);};
    Step(.2f,true,true,false);
    TestEqual(TEXT("Cultivation selects speed board"),S.Category,FName(TEXT("CultivationSpeed")));
    TestTrue(TEXT("Fade enters gradually"),S.Opacity()>0 && S.Opacity()<1);
    Step(.2f,true,true,false);TestEqual(TEXT("Visible after fade"),S.Opacity(),1.f);
    Step(.1f,true,false,false);TestTrue(TEXT("Ending fades rather than popping"),S.Opacity()>0 && S.Opacity()<1);
    Step(.5f,true,false,false);TestTrue(TEXT("Ended activity clears board"),S.Category.IsNone());
    Step(.4f,true,false,true);TestEqual(TEXT("Attack selects damage"),S.Category,FName(TEXT("Damage")));
    Step(1.f,true,false,false);TestEqual(TEXT("Brief attack gaps retain board"),S.Opacity(),1.f);
    Step(1.1f,true,true,false);TestTrue(TEXT("Category transition fades old board out"),S.Progress==0);
    Step(.4f,true,true,false);TestEqual(TEXT("Cultivation resumes after damage hold"),S.Category,FName(TEXT("CultivationSpeed")));
    TestEqual(TEXT("Resumed board fades in"),S.Opacity(),1.f);
    Step(.01f,false,true,true);TestTrue(TEXT("Menus clear automatic boards"),S.Category.IsNone()&&S.Progress==0&&S.DamageRemaining==0);
    Step(.4f,true,false,false);TestTrue(TEXT("No stale attack after menu"),S.Category.IsNone());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRankPortraitTest,"BoundlessImmortality.Frontend.Leaderboard.PortraitRows",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRankPortraitTest::RunTest(const FString&)
{
    for(bool Portrait:{true,false})
    {
        const auto Row=InkLeaderboard::FullRowLayout(Portrait);
        TestTrue(TEXT("Nickname never overlaps metric"),Row.Name.X+Row.Name.Z<=Row.Metric.X);
        TestTrue(TEXT("Avatar stays before nickname"),Row.Avatar.X+Row.Avatar.Z<=Row.Name.X);
        TestTrue(TEXT("Eight-digit metrics have an explicit readable budget"),Row.Metric.Z>=140);
        for(const auto& Rect:{Row.Name,Row.Metric,Row.Title,Row.Avatar})
            TestTrue(TEXT("Every row element stays inside its responsive row"),Rect.X>=0&&Rect.Y>=0&&Rect.X+Rect.Z<=Row.Width&&Rect.Y+Rect.W<=Row.Height);
        if(Portrait)TestTrue(TEXT("Portrait title has a separate line under nickname"),Row.Name.Y+Row.Name.W<=Row.Title.Y);
    }
    return true;
}
#endif
