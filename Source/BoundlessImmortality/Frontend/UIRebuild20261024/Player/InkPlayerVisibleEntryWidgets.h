#pragma once

#include "CoreMinimal.h"

class UCanvasPanel;
class UInkAnchorStatusWidget;
class UInkFrontendWidget;
class UWidget;
class UWidgetTree;

/** New visual composition for the existing Main/HUD routes.
 *  All actions dispatch through the existing frontend; the live scene remains behind these widgets.
 */
namespace InkPlayerVisibleEntryWidgets
{
	UWidget* BuildMainMenuSurface(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		bool bCanContinue, const FText& ContinueStatus);

	void BuildHudSurface(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		UCanvasPanel* HudPage, UInkAnchorStatusWidget* LiveStatus, const FText& ActivityStatus);
	void RefreshHudStatus(UWidgetTree* Tree, UInkAnchorStatusWidget* LiveStatus,
		const FText& ActivityStatus);
}
