#pragma once
#include "CoreMinimal.h"
#include "Layout/SlateRect.h"

struct FInkHudLayoutRects
{
    FSlateRect Identity;
    FSlateRect Navigation;
    FSlateRect RuntimeMenu;
    /** Logical safe-area coordinates; Canvas consumers multiply by their viewport UI scale. */
    FSlateRect Announcement;
    TArray<FSlateRect> TopActions;
    TArray<FSlateRect> BottomActions;
    bool bTopActionsOnSecondRow = false;
};

namespace InkUiLayout
{
    inline bool IsLandscape(FVector2D Size) { return Size.Y>0 && Size.X/Size.Y>=1.25; }
    BOUNDLESSIMMORTALITY_API FInkHudLayoutRects ResolveHud(FVector2D SafeSize, float ExtraStreamBottomInset = 0);
    /** Coordinates are already inside the frontend SafeZone. */
    BOUNDLESSIMMORTALITY_API FSlateRect ResolveFeatureFrame(FVector2D SafeSize, bool bDense = false);
}
