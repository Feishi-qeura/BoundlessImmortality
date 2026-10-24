#pragma once

#include "CoreMinimal.h"

class UCanvasPanel;
class UInkFrontendWidget;
class UWidget;
class UWidgetTree;

/** Visual-only composition for the existing Main/HUD routes. The host retains
 *  the camera, route state, save data, and every action implementation.
 */
namespace InkPlayerVisibleEntryWidgets
{
	UWidget* BuildMainMenuSurface(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		bool bCanContinue, const FText& ContinueStatus);

	UWidget* BuildHudNavigation(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		UCanvasPanel* HudPage);
}
