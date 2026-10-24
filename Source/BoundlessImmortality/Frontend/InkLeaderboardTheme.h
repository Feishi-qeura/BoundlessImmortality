#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

namespace InkLeaderboardTheme
{
    FString CategoryTitle(FName Category);
    FString CategoryLabel(FName Category);
    FString MetricLabel(FName Category);
    FLinearColor Color(FName Category);
}
