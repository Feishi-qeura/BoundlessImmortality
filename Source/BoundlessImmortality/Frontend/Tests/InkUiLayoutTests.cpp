#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/UI/InkUiResponsiveLayout.h"
#include "Frontend/UI/InkUiViewModels.h"
#include "Frontend/UI/InkUiTheme.h"
#include "Misc/ScopeExit.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkUiLayoutTest,"BoundlessImmortality.Frontend.Refactor.SafeLayout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkUiLayoutTest::RunTest(const FString&)
{
    for(const FVector2D Size:{FVector2D(720,1280),FVector2D(1280,720),FVector2D(680,1280),FVector2D(960,720),FVector2D(720,1560)})
    {
        const auto R=InkUiLayout::ResolveHud(Size);
        TestEqual(TEXT("Five bottom actions"),R.BottomActions.Num(),5);
        for(int I=0;I<5;++I)
        {
            TestTrue(TEXT("Each navigation target is touchable"),R.BottomActions[I].GetSize().X>=88&&R.BottomActions[I].GetSize().Y>=88);
            TestTrue(TEXT("Navigation stays inside safe content"),R.BottomActions[I].Left>=24&&R.BottomActions[I].Right<=Size.X-24&&R.BottomActions[I].Bottom<=Size.Y-24);
            if(I>0)TestTrue(TEXT("Navigation targets do not overlap"),R.BottomActions[I].Left>=R.BottomActions[I-1].Right-.01f);
        }
        TestEqual(TEXT("Center action stays centered"),R.BottomActions[2].GetCenter().X,float(Size.X*.5));
        TestTrue(TEXT("Top actions do not cover identity"),R.TopActions[0].Left>=R.Identity.Right||R.TopActions[0].Top>=R.Identity.Bottom);
        const auto Frame=InkUiLayout::ResolveFeatureFrame(Size);
        TestEqual(TEXT("Page has a 24-unit top inset"),Frame.Top,24.f);
        TestTrue(TEXT("Page stays inside safe content"),Frame.Left>=24&&Frame.Right<=Size.X-24&&Frame.Bottom<=Size.Y-24);
    }
    TestEqual(TEXT("Landscape navigation budget"),InkUiLayout::ResolveHud({1280,720}).Navigation.GetSize().X,560.f);
    TestTrue(TEXT("Narrow safe width makes a second top row"),InkUiLayout::ResolveHud({680,1280}).bTopActionsOnSecondRow);
    TestEqual(TEXT("Stream inset affects only the bottom group"),InkUiLayout::ResolveHud({720,1280},100).Navigation.Top,InkUiLayout::ResolveHud({720,1280}).Navigation.Top-100);
    TestTrue(TEXT("Wide feature content has room for list and detail"),InkUiLayout::ResolveFeatureFrame({1280,720}).GetSize().X>=1000);
    TestEqual(TEXT("Feature is centered in landscape"),InkUiLayout::ResolveFeatureFrame({1280,720}).GetCenter().X,640.f);
    auto* Theme=const_cast<UInkUiThemeData*>(InkUiTypography::Theme());
    const float OldPadding=Theme->OuterPadding,OldHit=Theme->MinHitSize;
    ON_SCOPE_EXIT { Theme->OuterPadding=OldPadding;Theme->MinHitSize=OldHit; };
    Theme->OuterPadding=32;Theme->MinHitSize=120;
    TestEqual(TEXT("Configured outer padding moves HUD identity"),InkUiLayout::ResolveHud({1280,720}).Identity.Left,32.f);
    TestEqual(TEXT("Configured outer padding moves page"),InkUiLayout::ResolveFeatureFrame({1280,720}).Top,32.f);
    TestTrue(TEXT("Configured touch size affects top targets"),InkUiLayout::ResolveHud({1280,720}).TopActions[0].GetSize().X>=120);
    TestTrue(TEXT("Configured touch size affects bottom targets"),InkUiLayout::ResolveHud({1280,720}).BottomActions[0].GetSize().X>=120);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkPortraitHudRailTest,"BoundlessImmortality.Frontend.Refactor.PortraitHudRail",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkPortraitHudRailTest::RunTest(const FString&)
{
    for(const FVector2D Size:{FVector2D(540,960),FVector2D(720,1280),FVector2D(1080,1920),FVector2D(1080,2400)})
    {
        const auto R=InkUiLayout::ResolveHud(Size);
        TestTrue(TEXT("Compact identity does not consume the upper scene"),R.Identity.GetSize().Y<=128&&R.Identity.GetSize().X<=440);
        TestTrue(TEXT("Dock is materially lower than the former 144 high row"),R.Navigation.GetSize().Y<=104);
        for(int32 I=0;I<3;++I)
        {
            const auto A=R.TopActions[I];
            TestTrue(TEXT("Right rail protects central 22-78 percent character area"),A.Left>=Size.X*.78);
            TestTrue(TEXT("Every shortcut retains 88 unit touch area"),A.GetSize().X>=88&&A.GetSize().Y>=88);
            TestEqual(TEXT("Shortcut rail is aligned"),A.Left,R.TopActions[0].Left);
            if(I>0)TestTrue(TEXT("Shortcut targets have an eight unit separation"),A.Top>=R.TopActions[I-1].Bottom+8);
        }
        TestTrue(TEXT("Compact rank entry below rail stays above bottom dock"),R.TopActions.Last().Bottom+12+88<R.Navigation.Top);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkHudAnnouncementLayoutTest,"BoundlessImmortality.Frontend.Refactor.AnnouncementClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkHudAnnouncementLayoutTest::RunTest(const FString&)
{
    const auto Overlaps=[](const FSlateRect& A,const FSlateRect& B)
    {return FMath::Min(A.Right,B.Right)>FMath::Max(A.Left,B.Left)&&FMath::Min(A.Bottom,B.Bottom)>FMath::Max(A.Top,B.Top);};
    for(const FVector2D Size:{FVector2D(540,960),FVector2D(720,1280),FVector2D(1080,1920),FVector2D(1080,2400),FVector2D(960,720),FVector2D(1280,720)})
    {
        const auto Hud=InkUiLayout::ResolveHud(Size);const auto A=Hud.Announcement;
        TestTrue(TEXT("Announcement has readable width and stays inside safe viewport"),A.GetSize().X>=280&&A.Left>=24&&A.Right<=Size.X-24&&A.Bottom<=Size.Y-24);
        TestTrue(TEXT("Announcement sits below identity with a gap"),A.Top>=Hud.Identity.Bottom+16);
        TestFalse(TEXT("Announcement never overlaps identity"),Overlaps(A,Hud.Identity));
        TestFalse(TEXT("Announcement never overlaps dock"),Overlaps(A,Hud.Navigation));
        for(const auto Shortcut:Hud.TopActions)TestFalse(TEXT("Announcement never overlaps a shortcut"),Overlaps(A,Shortcut));
        if(!InkUiLayout::IsLandscape(Size))TestTrue(TEXT("Portrait announcement ends before the right rail"),A.Right<=Hud.TopActions[0].Left-12);
        for(const float UIScale:{.75f,1.f,1.5f})
        {
            const FVector2D CanvasSize=Size*UIScale;
            const auto CanvasRect=InkUiLayout::ResolveHud(CanvasSize/UIScale).Announcement;
            TestTrue(TEXT("Canvas conversion uses same logical placement as Slate"),(CanvasRect.GetTopLeft()*UIScale).Equals(A.GetTopLeft()*UIScale,.01));
            TestTrue(TEXT("Canvas conversion scales extent once"),(CanvasRect.GetSize()*UIScale).Equals(A.GetSize()*UIScale,.01));
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkUiRequestTest,"BoundlessImmortality.Frontend.Refactor.RequestIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkUiRequestTest::RunTest(const FString&)
{
    FInkUiSnapshotContext S;FInkUiRequestContext R;
    TestFalse(TEXT("No authority cannot submit"),R.Begin(S));
    S.Profile=TEXT("prototype_full");S.App=TEXT("test");S.Room=TEXT("room");S.RuleVersion=TEXT("v1");S.ServerUtc=1000;S.bIsAuthoritative=true;S.SessionGeneration=2;S.SnapshotVersion=7;
    S.Profile=TEXT("douyin_release_candidate");
    TestFalse(TEXT("Release candidate cannot submit restricted prototype requests"),R.Begin(S));
    R=FInkUiRequestContext();S.Profile=TEXT("unknown_profile");
    TestFalse(TEXT("Unknown profile fails closed"),R.Begin(S));
    R=FInkUiRequestContext();S.Profile=TEXT("prototype_full");S.ServerUtc=0;
    TestFalse(TEXT("Missing service clock cannot submit"),R.Begin(S));
    S.ServerUtc=1000;
    TestTrue(TEXT("Authoritative context begins request"),R.Begin(S));
    const FGuid Id=R.RequestId;
    TestFalse(TEXT("Double click cannot start another request"),R.Begin(S));
    R.MarkTimeout();TestEqual(TEXT("Timeout preserves same request"),R.RequestId,Id);
    TestFalse(TEXT("Old generation ignored"),R.ApplyReply(Id,1,8,EInkUiRequestStatus::Confirmed,TEXT("r")));
    TestFalse(TEXT("Stale version ignored"),R.ApplyReply(Id,2,6,EInkUiRequestStatus::Confirmed,TEXT("r")));
    TestFalse(TEXT("No receipt cannot confirm"),R.ApplyReply(Id,2,8,EInkUiRequestStatus::Confirmed,TEXT("")));
    TestTrue(TEXT("Original timed-out request reconciles"),R.ApplyReply(Id,2,8,EInkUiRequestStatus::Confirmed,TEXT("receipt")));
    TestFalse(TEXT("Duplicate terminal reply ignored"),R.ApplyReply(Id,2,8,EInkUiRequestStatus::Confirmed,TEXT("receipt")));
    return true;
}
#endif
