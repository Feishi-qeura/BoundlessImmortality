#include "InkLeaderboardTheme.h"
#include "InkPresentationStyle.h"

namespace InkLeaderboardTheme
{
    FString CategoryTitle(FName Category)
    {
        if (Category == TEXT("Damage")) return TEXT("伤害榜");
        if (Category == TEXT("CultivationSpeed")) return TEXT("修炼速度榜");
        if (Category == TEXT("Anchor")) return TEXT("主播剑灵榜");
        return TEXT("排行榜");
    }

    FString CategoryLabel(FName Category)
    {
        if (Category == TEXT("Damage")) return TEXT("伤害");
        if (Category == TEXT("CultivationSpeed")) return TEXT("修炼速度");
        if (Category == TEXT("Anchor")) return TEXT("主播剑灵");
        return TEXT("榜单");
    }

    FString MetricLabel(FName Category)
    {
        if (Category == TEXT("Damage")) return TEXT("伤害 / 秒");
        if (Category == TEXT("CultivationSpeed")) return TEXT("修为 / 秒");
        if (Category == TEXT("Anchor")) return TEXT("剑灵数");
        return TEXT("服务计分");
    }

    FLinearColor Color(FName Category)
    {
        if (Category == TEXT("Damage")) return FLinearColor::FromSRGBColor(FColor(170, 73, 55));
        if (Category == TEXT("CultivationSpeed")) return InkPresentationStyle::Jade();
        if (Category == TEXT("Anchor")) return FLinearColor::FromSRGBColor(FColor(184, 139, 69));
        return InkPresentationStyle::Ink();
    }
}
