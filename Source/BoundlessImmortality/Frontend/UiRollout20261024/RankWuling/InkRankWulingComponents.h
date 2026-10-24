#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkPresentationStyle.h"

class UCanvasPanel;
class UImage;
class UInkFrontendWidget;
class UTextBlock;
class UTexture2D;
class UWidgetTree;
class UInkFrontendActionButton;

/** Reusable native UMG pieces for the rank and Wuling candidate rollout.
 *  The host panel owns routing, live business state, and the containing layout.
 */
namespace InkRankWulingComponents
{
    struct BOUNDLESSIMMORTALITY_API FPlacement
    {
        float X = 0.f;
        float Y = 0.f;
        float Width = 0.f;
        float Height = 0.f;
        int32 ZOrder = 0;
    };

    /** Add the candidate parchment texture when imported; native fallback remains usable. */
    BOUNDLESSIMMORTALITY_API UImage* AddPaperPanel(
        UWidgetTree* Tree, UCanvasPanel* Parent, FName Name, UTexture2D* Texture,
        const FPlacement& Placement, FVector4f SlicePixels = FVector4f(0, 0, 0, 0));

    /** Add a dynamic label. Business text is always supplied by the host at runtime. */
    BOUNDLESSIMMORTALITY_API UTextBlock* AddDynamicText(
        UWidgetTree* Tree, UCanvasPanel* Parent, FName Name, const FText& Text,
        int32 FontSize, const FPlacement& Placement, FLinearColor Color = FLinearColor::White,
        ETextJustify::Type Justification = ETextJustify::Left);

    /** Create a frontend action button with the existing shared ink style and route API. */
    BOUNDLESSIMMORTALITY_API UInkFrontendActionButton* AddActionButton(
        UWidgetTree* Tree, UCanvasPanel* Parent, UInkFrontendWidget* Frontend,
        FName WidgetName, FName Action, const FText& Caption, bool bSelected,
        bool bPrimary, const FPlacement& Placement);

    /** Add a shared native surface for headings, cards, or selected category tabs. */
    BOUNDLESSIMMORTALITY_API UImage* AddNativeSurface(
        UWidgetTree* Tree, UCanvasPanel* Parent, FName Name,
        InkPresentationStyle::ESurface Surface, const FPlacement& Placement);
}
