#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFrontendTypes.h"
#include "Frontend/InkInventoryModel.h"

class UInkFrontendWidget;
class UTexture2D;
class UWidget;
class UWidgetTree;

namespace InkPlayerCharacterInventoryWidgets
{
	enum class EProviderState : uint8
	{
		Unknown, Loading, Empty, Data, Pending, Offline, Error, Disabled
	};

	/** The host supplies state and values; this builder never makes missing data look empty. */
	UWidget* BuildCharacterPage(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		const FText& CharacterName, const FText& RealmName,
		const TArray<FInkCharacterAttribute>& Attributes,
		const TArray<FInkDisciplineEntry>& Disciplines,
		UTexture2D* AttributeGlyphAtlas, EProviderState State, const FText& StateMessage);

	/** SelectedEntryId and known capacity come from the host's stable InventoryVm projection. */
	UWidget* BuildInventoryPage(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		UInkInventoryModel* Inventory, FGuid SelectedEntryId, FName ActiveFilter,
		const FString& SearchQuery, int32 PageIndex, int32 ActionQuantity,
		bool bCapacityKnown, int32 UnlockedSlotCount,
		UTexture2D* CategoryGlyphAtlas, EProviderState State, const FText& StateMessage,
		float AvailableWidth=672.f, bool bMutationsBusy=false);
}
