#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Fonts/SlateFontInfo.h"
#include "InkPlayerCraftSourcesWidgets.generated.h"

class UTexture2D;
class UWidget;
class UWidgetTree;

namespace InkPlayerCraftSourcesWidgets
{
	enum class EProviderState : uint8 { Unknown, Loading, Empty, Data, Pending, Offline, Error, Disabled };
	enum class ELayoutMode : uint8 { Portrait, Compact, Landscape };

	struct FProviderSlice
	{
		EProviderState State = EProviderState::Unknown;
		FText Message;
	};

	/** Values are supplied by the newUI host from the canonical style contract. */
	struct FStyleTokens
	{
		FLinearColor Primary;
		FLinearColor Secondary;
		FLinearColor Accent;
		FLinearColor Page;
		FLinearColor Paper;
		FLinearColor Ink;
		FLinearColor Muted;
		FLinearColor Jade;
		FLinearColor Warning;
		FLinearColor Disabled;
		FSlateFontInfo TitleFont;
		FSlateFontInfo SectionFont;
		FSlateFontInfo BodyFont;
		FSlateFontInfo ButtonFont;
		FSlateFontInfo SecondaryFont;
		float ContentInset = 24.f;
		float MinimumHitTarget = 88.f;
	};

	struct FActionPayload
	{
		FName RecipeId;
		FName SourceId;
		FName ItemId;
		int32 Offset = 0;
		bool bOwnedOnly = false;
	};

	DECLARE_DELEGATE_TwoParams(FActionRequested, FName /* canonical action id or ui.* intent */, const FActionPayload&);

	struct FRecipeRow
	{
		FName RecipeId;
		FText Name;
		FText Summary;
		UTexture2D* Icon = nullptr;
	};

	struct FRecipeIngredient
	{
		FName ItemId;
		FText Name;
		bool bCountKnown = false;
		int64 Count = 0;
		UTexture2D* Icon = nullptr;
	};

	struct FRecipeOutput
	{
		FName ItemId;
		FText Name;
		bool bCountKnown = false;
		int64 Count = 0;
		UTexture2D* Icon = nullptr;
	};

	struct FInventoryEntry
	{
		FGuid EntryId;
		FName ItemId;
		FText Name;
		int64 Count = 0;
	};

	struct FRecipeVm
	{
		FProviderSlice CatalogState;
		FProviderSlice SelectionState;
		FProviderSlice IngredientsState;
		FProviderSlice OutputState;
		TArray<FRecipeRow> Recipes;
		FName SelectedRecipeId;
		TArray<FRecipeIngredient> Ingredients;
		FRecipeOutput Output;
		int32 QueryOffset = 0;
		bool bOwnedOnly = false;
		bool bQueryAvailable = false;
		FText QueryDisabledReason;
		bool bCraftAvailable = false;
		FText CraftDisabledReason;
		bool bCraftConfirmationOpen = false;
		FName ConfirmationRecipeId;
		FText ConfirmationSummary;
	};

	struct FInventoryVm
	{
		FProviderSlice State;
		TArray<FInventoryEntry> Entries;
	};

	struct FItemSourceRow
	{
		FName SourceId;
		FName ItemId;
		FName Method;
		FText Source;
		FText Activity;
		UTexture2D* Icon = nullptr;
	};

	struct FItemSourceVm
	{
		FProviderSlice State;
		FName ItemId;
		FName SelectedSourceId;
		TArray<FItemSourceRow> Sources;
		int32 QueryOffset = 0;
		bool bOwnedOnly = false;
		bool bQueryAvailable = false;
		FText QueryDisabledReason;
		bool bChooseFocusAvailable = false;
		FText ChooseFocusDisabledReason;
	};

	UWidget* BuildCraftPage(UWidgetTree* Tree, const FRecipeVm& Recipe,
		const FInventoryVm& Inventory, const FStyleTokens& Style, ELayoutMode Layout,
		FActionRequested OnAction);

	UWidget* BuildItemSourcesPage(UWidgetTree* Tree, const FItemSourceVm& ViewModel,
		const FStyleTokens& Style, ELayoutMode Layout, FActionRequested OnAction);
}

/** UMG's dynamic OnClicked delegate forwards the typed page payload. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkPlayerDActionButton : public UButton
{
	GENERATED_BODY()

public:
	void SetActionRequest(InkPlayerCraftSourcesWidgets::FActionRequested InAction,
		FName InActionId, const InkPlayerCraftSourcesWidgets::FActionPayload& InPayload);

	UFUNCTION()
	void HandleClicked();

private:
	InkPlayerCraftSourcesWidgets::FActionRequested Action;
	FName ActionId;
	InkPlayerCraftSourcesWidgets::FActionPayload Payload;
};
