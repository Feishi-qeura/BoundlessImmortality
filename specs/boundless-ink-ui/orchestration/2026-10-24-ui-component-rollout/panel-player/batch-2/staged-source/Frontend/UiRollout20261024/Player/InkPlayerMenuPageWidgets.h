#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/InkSaveSubsystem.h"

class UInkFrontendWidget;
class UWidget;
class UWidgetTree;

/** Runtime-only composable bodies for the existing Saves and Settings routes.
 *  The host remains responsible for route selection, feedback, and business actions.
 */
namespace InkPlayerMenuPageWidgets
{
	/** Creates one row for each real local manual slot. Pass GetSlots() unchanged. */
	UWidget* BuildSavesBody(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		const TArray<FInkSaveSlotInfo>& Slots, bool bCanSave, const FText& SaveDisabledReason);

	/** Builds only capabilities already exposed by the existing frontend. */
	UWidget* BuildSettingsBody(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		int32 OverallQuality, float EffectiveFrameRateLimit, bool bReducedMotion);
}
