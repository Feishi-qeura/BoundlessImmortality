#include "Frontend/InkLeaderboardTheme.h"
#include "Frontend/InkPresentationStyle.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkNativeRankWulingStyleTest,
    "BoundlessImmortality.Frontend.NativeRankWulingStyle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInkNativeRankWulingStyleTest::RunTest(const FString&)
{
    using InkPresentationStyle::ESurface;
    for (const auto Part : {ESurface::FeaturePaper, ESurface::FeatureHeading, ESurface::FeatureCard})
    {
        const auto Brush = InkPresentationStyle::Surface(Part, FVector2D(320, 160));
        TestTrue(TEXT("Rank and sect surfaces use native rounded-box brushes"), Brush.DrawAs == ESlateBrushDrawType::RoundedBox);
        TestNull(TEXT("Native surfaces do not load a material resource"), Brush.GetResourceObject());
    }

    const auto Button = InkPresentationStyle::FeatureButton();
    TestTrue(TEXT("Feature buttons use native normal styling"), Button.Normal.DrawAs == ESlateBrushDrawType::RoundedBox);
    TestTrue(TEXT("Feature buttons use native hover styling"), Button.Hovered.DrawAs == ESlateBrushDrawType::RoundedBox);

    TestEqual(TEXT("Damage keeps its own live title"), InkLeaderboardTheme::CategoryTitle(TEXT("Damage")), FString(TEXT("伤害榜")));
    TestEqual(TEXT("Cultivation keeps its own live title"), InkLeaderboardTheme::CategoryTitle(TEXT("CultivationSpeed")), FString(TEXT("修炼速度榜")));
    TestEqual(TEXT("Anchor stays the streamer sword-spirit category"), InkLeaderboardTheme::CategoryTitle(TEXT("Anchor")), FString(TEXT("主播剑灵榜")));
    TestEqual(TEXT("Damage unit is explicit without inventing a time window"), InkLeaderboardTheme::MetricLabel(TEXT("Damage")), FString(TEXT("伤害 / 秒")));
    TestEqual(TEXT("Cultivation unit is explicit"), InkLeaderboardTheme::MetricLabel(TEXT("CultivationSpeed")), FString(TEXT("修为 / 秒")));
    return true;
}
#endif
