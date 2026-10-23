#if WITH_DEV_AUTOMATION_TESTS
#include "Frontend/InkAnchorStatusWidget.h"
#include "CultivationGameplayComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkAnchorActionProgressTest,"BoundlessImmortality.Frontend.AnchorStatus.RealActionProgress",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkAnchorActionProgressTest::RunTest(const FString&)
{
    for(auto State:{ECultivationActionState::MeditationLoop,ECultivationActionState::ChargeLoop})
    {
        const auto P=UInkAnchorStatusWidget::ResolveAction(State,999,3);
        TestTrue(TEXT("A looping action remains active and indeterminate even with a looping clip duration"),P.bActive&&P.bIndeterminate);
        TestEqual(TEXT("A looping clip never pretends to be completion progress"),P.Fraction,0.f);
    }
    auto P=UInkAnchorStatusWidget::ResolveAction(ECultivationActionState::MeditationStart,.45f,.9f);
    TestEqual(TEXT("Entry uses the actual transition duration"),P.Fraction,.5f);
    P=UInkAnchorStatusWidget::ResolveAction(ECultivationActionState::ChargeRelease,9,2);
    TestEqual(TEXT("Completed finite action clamps to one"),P.Fraction,1.f);
    P=UInkAnchorStatusWidget::ResolveAction(ECultivationActionState::ChargeStart,.5f,0);
    TestTrue(TEXT("An unknown clip duration never invents progress"),P.bIndeterminate&&P.Fraction==0);
    P=UInkAnchorStatusWidget::ResolveAction(ECultivationActionState::Idle,999,1);
    TestFalse(TEXT("Idle is not an active progress bar"),P.bActive);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkAnchorSafePresentationTest,"BoundlessImmortality.Frontend.AnchorStatus.SafePresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkAnchorSafePresentationTest::RunTest(const FString&)
{
    TestEqual(TEXT("No shield has no displayed fill"),UInkAnchorStatusWidget::SafeFraction(0,0),0.f);
    for(float H:{22.f,28.f,44.f})
    {
        const FSlateRect Track(104,38,356,38+H);const auto Inner=UInkAnchorStatusWidget::MeterInterior(Track);
        TestTrue(TEXT("Full meter never reaches or exceeds any track edge"),Inner.Left>Track.Left&&Inner.Top>Track.Top&&Inner.Right<Track.Right&&Inner.Bottom<Track.Bottom);
        TestTrue(TEXT("Positive inset retains usable channel"),Inner.GetSize().X>0&&Inner.GetSize().Y>0);
    }
    TestEqual(TEXT("Unbounded cultivation stays in the bar"),UInkAnchorStatusWidget::SafeFraction(1000,100),1.f);
    TestEqual(TEXT("Invalid external presentation input does not enter Slate"),UInkAnchorStatusWidget::SafeFraction(std::numeric_limits<double>::quiet_NaN(),100),0.f);
    const auto Portrait=UInkAnchorStatusWidget::ResponsiveSize(FVector2D(320,720));
    TestTrue(TEXT("Narrow identity stays within safe width"),Portrait.X<=320-48);
    TestTrue(TEXT("Narrow portrait remains a compact HUD group rather than a tall paper panel"),Portrait.Y<=128.);
    TestEqual(TEXT("Normal portrait follows identity design token"),UInkAnchorStatusWidget::ResponsiveSize(FVector2D(720,1280)),FVector2D(440,128));
    TestEqual(TEXT("Landscape keeps readable identity without shrinking text"),UInkAnchorStatusWidget::ResponsiveSize(FVector2D(1280,720)),FVector2D(440,128));
    const auto Frame=UInkAnchorStatusWidget::AvatarFrameBounds(),Photo=UInkAnchorStatusWidget::AvatarPortraitBounds();
    TestTrue(TEXT("Photo and Niagara frame share their exact center"),Frame.Left+Frame.Right==Photo.Left+Photo.Right&&Frame.Top+Frame.Bottom==Photo.Top+Photo.Bottom);
    for(int32 Kind=0;Kind<3;++Kind){const auto B=UInkAnchorStatusWidget::MeterBounds(Kind);TestTrue(TEXT("Every bar has margin to thin gold frame"),B.Left>=12&&B.Top>=12&&B.Right<=440-12&&B.Bottom<=128-8);}
    const auto Realm=UInkAnchorStatusWidget::RealmTitleBounds();TestTrue(TEXT("Realm plaque fits compact status band"),Realm.GetSize().X>=144&&Realm.GetSize().Y<=36);TestTrue(TEXT("Realm title retains gold frame clearance"),Realm.Left>=4&&Realm.Bottom<=128);
    return true;
}
#endif
