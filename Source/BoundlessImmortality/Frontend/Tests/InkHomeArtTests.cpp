#include "Frontend/InkHomeArt.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkHomeFitTest,"Boundless.UI.HomeV2.Geometry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkHomeFitTest::RunTest(const FString&)
{
    auto Narrow=InkHomeArt::ArtBounds(TEXT("home-rank-paper"),FSlateRect(0,0,1260,399));
    TestFalse(TEXT("Narrow uses full Fit"),InkHomeArt::HorizontalSlice(TEXT("home-rank-paper"),FVector2D(1260,399)));
    TestTrue(TEXT("Narrow exact art origin"),FMath::IsNearlyEqual(Narrow.Top,39.9f,.01f));
    TestTrue(TEXT("Narrow exact fitted height"),FMath::IsNearlyEqual(Narrow.GetSize().Y,319.2,.01));
    for(float W:{1575.f,2205.f})
    {
        TestTrue(TEXT("Native and wide slice horizontally"),InkHomeArt::HorizontalSlice(TEXT("home-rank-paper"),FVector2D(W,399)));
        TestTrue(TEXT("Wide retains destination bounds"),InkHomeArt::ArtBounds(TEXT("home-rank-paper"),FSlateRect(0,0,W,399)).GetSize().Equals(FVector2D(W,399)));
    }
    TestFalse(TEXT("Zero size is never sliced"),InkHomeArt::HorizontalSlice(TEXT("home-rank-paper"),FVector2D(0)));
    TestFalse(TEXT("Top tile always Fits"),InkHomeArt::HorizontalSlice(TEXT("home-top-tile"),FVector2D(2000,500)));
    const auto Margin=InkHomeArt::NormalizedMargin(TEXT("home-rank-paper"));
    TestTrue(TEXT("Normalized left uses cropped width"),FMath::IsNearlyEqual(Margin.Left,820.f/1575));
    TestTrue(TEXT("Normalized top uses cropped height"),FMath::IsNearlyEqual(Margin.Top,115.f/399));
    TestTrue(TEXT("Normalized right uses cropped width"),FMath::IsNearlyEqual(Margin.Right,400.f/1575));
    auto Track=InkHomeArt::NormalizedMargin(TEXT("home-bar-track"));
    TestTrue(TEXT("Track normalized margins"),FMath::IsNearlyEqual(Track.Left,260.f/2046)&&FMath::IsNearlyEqual(Track.Top,60.f/226));
    for(FVector2D Box:{FVector2D(712,1097),FVector2D(1245,1097),FVector2D(889,1481)})
    {
        auto Rect=InkHomeArt::ArtBounds(TEXT("home-top-tile"),FSlateRect(FVector2D(0),Box));
        TestTrue(TEXT("Top preserves original aspect"),FMath::IsNearlyEqual(Rect.GetSize().X/Rect.GetSize().Y,889./1097.,.0001));
        TestTrue(TEXT("Fit contained in container"),Rect.Left>=0&&Rect.Top>=0&&Rect.Right<=Box.X+.01&&Rect.Bottom<=Box.Y+.01);
    }
    return true;
}
#endif
