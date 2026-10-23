#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

namespace InkLeaderboardTheme
{
    FSlateBrush Header(FName Category);
    FSlateBrush Row(FName Category,int32 Rank);
    FVector4 FitRowArt(FVector2D SourceSize,FVector2D RowSize);
    FLinearColor Color(FName Category);
    int32 Quality(int32 Rank);
    float Strength(int32 Rank);
}
