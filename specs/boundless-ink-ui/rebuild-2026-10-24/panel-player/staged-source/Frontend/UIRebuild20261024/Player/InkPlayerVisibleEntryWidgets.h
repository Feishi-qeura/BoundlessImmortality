#pragma once

#include "CoreMinimal.h"

class UCanvasPanel;
class UInkAnchorStatusWidget;
class UInkFrontendWidget;
class UTexture2D;
class UWidget;
class UWidgetTree;

/** New visual composition for the existing Main/HUD routes.
 *  All actions dispatch through the existing frontend; the live scene remains behind these widgets.
 */
namespace InkPlayerVisibleEntryWidgets
{
	UWidget* BuildMainMenuSurface(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		bool bCanContinue, const FText& ContinueStatus, UTexture2D* MainActionOrnament);

	void BuildHudSurface(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		UCanvasPanel* HudPage, UInkAnchorStatusWidget* LiveStatus, const FText& ActivityStatus,
		UTexture2D* RouteGlyphAtlas);
	void RefreshHudStatus(UWidgetTree* Tree, UInkAnchorStatusWidget* LiveStatus,
		const FText& ActivityStatus);
}
