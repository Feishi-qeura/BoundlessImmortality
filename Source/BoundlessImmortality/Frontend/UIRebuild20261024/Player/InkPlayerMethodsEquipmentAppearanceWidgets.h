#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Frontend/InkFrontendTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "InkPlayerMethodsEquipmentAppearanceWidgets.generated.h"

class UTexture2D;
class UInkInventoryModel;
class UWidget;
class UWidgetTree;

namespace InkPlayerMethodsEquipmentAppearanceWidgets
{
	enum class EProviderState : uint8
	{
		Unknown, Loading, Empty, Data, Pending, Offline, Error, Disabled
	};

	enum class ELayoutMode : uint8
	{
		Portrait, Compact, Landscape
	};

	struct FProviderSlice
	{
		EProviderState State = EProviderState::Unknown;
		FText Message;
	};

	/** Values are injected from the canonical style contract by the newUI host. */
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

	/** Typed adapter payload. Display selection is local; only contract actions reach business handlers. */
	struct FActionPayload
	{
		FName ItemId;
		FName SlotId;
		FGuid EntryId;
		FName MainMethodId;
		TArray<FName> SupportMethodIds;
		FName PreviewStyleId;
		int32 Quantity = 1;
	};

	DECLARE_DELEGATE_TwoParams(FActionRequested, FName /* Canonical action id or ui.* local intent */, const FActionPayload&);

	struct FCatalogEntry
	{
		FName ItemId;
		FGuid EntryId;
		FText Name;
		FText Detail;
		UTexture2D* Icon = nullptr;
		bool bOwnedKnown = false;
		bool bOwned = false;
	};

	struct FMethodsVm
	{
		FProviderSlice CatalogState;
		FProviderSlice OwnedMethodsState;
		FProviderSlice MainState;
		FProviderSlice SupportsState;
		FProviderSlice InventoryState;
		UInkInventoryModel* Inventory = nullptr;
		FGuid SelectedInventoryEntryId;
		TArray<FCatalogEntry> Catalog;
		TArray<FName> OwnedMethodIds;
		bool bMainKnown = false;
		FName MainMethodId;
		bool bSupportsKnown = false;
		TArray<FName> SupportMethodIds;
		FName DraftMainMethodId;
		TArray<FName> DraftSupportMethodIds;
		FName SelectedMethodId;
		bool bLearnActionAvailable = false;
		FText LearnDisabledReason;
		bool bConfigureActionAvailable = false;
		FText ConfigureDisabledReason;
	};

	struct FEquipmentSlot
	{
		FName SlotId;
		FText Label;
		bool bEntryKnown = false;
		FGuid EntryId;
		FName ItemId;
		FText ItemName;
		FText ItemDetail;
		UTexture2D* Icon = nullptr;
	};

	struct FEquipmentVm
	{
		FProviderSlice EquipmentState;
		FProviderSlice InventoryState;
		FProviderSlice CatalogState;
		TArray<FEquipmentSlot> Slots;
		TArray<FCatalogEntry> OwnedItems;
		bool bFabaoKnown = false;
		FGuid FabaoEntryId;
		FText FabaoName;
		UTexture2D* FabaoIcon = nullptr;
		FGuid SelectedEntryId;
		FName SelectedSlotId;
		int32 Quantity = 1;
		bool bEquipActionAvailable = false;
		FText EquipDisabledReason;
		bool bUnequipActionAvailable = false;
		FText UnequipDisabledReason;
		bool bActivateFabaoAvailable = false;
		FText ActivateFabaoDisabledReason;
	};

	struct FPreviewOption
	{
		FName StyleId;
		FText Label;
	};

	struct FCharacterAttributePreview
	{
		FProviderSlice State;
		TArray<FInkCharacterAttribute> Attributes;
	};

	struct FAppearanceVm
	{
		FProviderSlice AppearanceState;
		bool bIdentityKnown = false;
		FText Identity;
		bool bPortraitKnown = false;
		UTexture2D* Portrait = nullptr;
		bool bPreviewFrameKnown = false;
		FName PreviewFrame;
		FName DraftPreviewFrame;
		bool bPreviewBarKnown = false;
		FName PreviewBar;
		FName DraftPreviewBar;
		TArray<FPreviewOption> FrameOptions;
		TArray<FPreviewOption> BarOptions;
		FName SelectedPreviewStyleId;
		bool bFrameOptionsKnown = false;
		bool bBarOptionsKnown = false;
		FName ActiveTab = TEXT("Frame");
		bool bPreviewActionAvailable = false;
		FText PreviewDisabledReason;
	};

	struct FPageAtlases
	{
		UTexture2D* MethodsGlyphs = nullptr;
		UTexture2D* EquipmentGlyphs = nullptr;
		UTexture2D* AppearanceOrnaments = nullptr;
		UTexture2D* CharacterAttributeGlyphs = nullptr;
	};

	UWidget* BuildMethodsPage(UWidgetTree* Tree, const FMethodsVm& ViewModel,
		const FStyleTokens& Style, ELayoutMode Layout, const FPageAtlases& Atlases,
		FActionRequested OnAction);

	UWidget* BuildEquipmentPage(UWidgetTree* Tree, const FEquipmentVm& ViewModel,
		const FStyleTokens& Style, ELayoutMode Layout, const FPageAtlases& Atlases,
		FActionRequested OnAction);

	UWidget* BuildAppearancePage(UWidgetTree* Tree, const FAppearanceVm& ViewModel,
		const FCharacterAttributePreview& CharacterPreview, const FStyleTokens& Style,
		ELayoutMode Layout, const FPageAtlases& Atlases,
		FActionRequested OnAction);
}

/** UMG button that forwards a canonical action id with its typed page payload. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkPlayerCActionButton : public UButton
{
	GENERATED_BODY()

public:
	void SetActionRequest(InkPlayerMethodsEquipmentAppearanceWidgets::FActionRequested InAction,
		FName InActionId, const InkPlayerMethodsEquipmentAppearanceWidgets::FActionPayload& InPayload);

	UFUNCTION()
	void HandleClicked();

private:
	InkPlayerMethodsEquipmentAppearanceWidgets::FActionRequested Action;
	FName ActionId;
	InkPlayerMethodsEquipmentAppearanceWidgets::FActionPayload Payload;
};
