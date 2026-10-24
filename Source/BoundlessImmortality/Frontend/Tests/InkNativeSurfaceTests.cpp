#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/InkPresentationStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkNativeSurfaceTest,
    "BoundlessImmortality.Frontend.NativeStyle.AllControlEntryPoints",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkNativeSurfaceTest::RunTest(const FString&)
{
    using namespace InkPresentationStyle;
    for(const auto Part:{ESurface::PaperButton,ESurface::GoldButton,ESurface::BlueButton,ESurface::SelectedTab,ESurface::Heading,ESurface::Card,ESurface::FeaturePaper,ESurface::FeatureHeading,ESurface::FeatureCard,ESurface::FeatureTab})
    {
        for(const FVector2D Size:{FVector2D(180,72),FVector2D(680,1080)})
        {
            const auto Brush=Surface(Part,Size);
            TestNull(TEXT("Generic control never reintroduces a legacy texture/material"),Brush.GetResourceObject());
            TestTrue(TEXT("Native surface renders as a rounded box"),Brush.DrawAs==ESlateBrushDrawType::RoundedBox);
            TestTrue(TEXT("Requested control dimensions are retained"),FVector2D(Brush.ImageSize).Equals(Size));
        }
    }
    for(const auto Style:{Button(),Button(true),Button(false,true),Button(false,false,true)})
        for(const auto* Brush:{&Style.Normal,&Style.Hovered,&Style.Pressed,&Style.Disabled})
            TestNull(TEXT("Every generic button state is native"),Brush->GetResourceObject());
    const auto Normal=Button();
    TestTrue(TEXT("Secondary pressed state retains a light background for ink text"),Normal.Pressed.TintColor.GetSpecifiedColor().GetLuminance()>.5f);
    const auto Primary=Button(false,true);
    TestTrue(TEXT("Primary action retains a dark background for paper text"),Primary.Normal.TintColor.GetSpecifiedColor().GetLuminance()<.3f);
    TestNull(TEXT("Leaderboard frame follows native page shell"),LeaderboardBackground(FVector2D(680,1080)).GetResourceObject());
    return true;
}
#endif
