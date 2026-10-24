#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Fonts/SlateFontInfo.h"
#include "InkPlayerBreakthroughTribulationWidgets.generated.h"

class UTexture2D;
class UWidget;
class UWidgetTree;

namespace InkPlayerBreakthroughTribulationWidgets
{
	enum class EProviderState : uint8 { Unknown, Loading, Empty, Data, Pending, Offline, Error, Disabled };
	enum class ELayoutMode : uint8 { Portrait, Compact, Landscape };
	enum class ETribulationPhase : uint8 { Unknown, Prepare, Trial, Reconcile, Succeeded, Failed, Cancelled };

	struct FProviderSlice
	{
		EProviderState State = EProviderState::Unknown;
		FText Message;
	};

	/** Palette and fonts are injected from the canonical style contract by the host. */
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
		FSlateFontInfo NumberFont;
		FSlateFontInfo ButtonFont;
		FSlateFontInfo SecondaryFont;
		float ContentInset = 24.f;
		float MinimumHitTarget = 88.f;
	};

	struct FMaterialSelection
	{
		FName ItemId;
		int64 Count = 0;
		bool bCountKnown = false;
	};

	struct FActionPayload
	{
		FName ItemId;
		TArray<FMaterialSelection> Selection;
		FName AttemptId;
		bool bReworkMode = false;
		int32 CountDelta = 0;
		uint64 SelectionRevision = 0;
	};

	DECLARE_DELEGATE_TwoParams(FActionRequested, FName /* canonical action id or ui.* intent */, const FActionPayload&);

	struct FInventoryRow
	{
		FGuid EntryId;
		FName ItemId;
		FText Name;
		int64 Count = 0;
		UTexture2D* Icon = nullptr;
	};

	struct FInventoryVm
	{
		FProviderSlice State;
		TArray<FInventoryRow> Entries;
	};

	struct FBreakthroughStage
	{
		bool bKnown = false;
		FName StageId;
		bool bFromRealmKnown = false;
		int32 FromRealmIndex = 0;
		bool bToRealmKnown = false;
		int32 ToRealmIndex = 0;
	};

	struct FBreakthroughMaterial
	{
		FName ItemId;
		FText Name;
		bool bCountKnown = false;
		int64 Count = 0;
		UTexture2D* Icon = nullptr;
	};

	struct FBreakthroughQuote
	{
		FProviderSlice State;
		bool bQKnown = false;
		FText Q;
		bool bBandKnown = false;
		FText Band;
		bool bKindKnown = false;
		FText Kind;
		bool bQualityKnown = false;
		FText Quality;
		bool bForecastKnown = false;
		TArray<FText> OptionalForecast;
		bool bNormalizedQKnown = false;
		FText NormalizedQ;
		bool bReadinessKnown = false;
		FText Readiness;
		bool bSlotsKnown = false;
		FText Slots;
		bool bSourcesKnown = false;
		FText Sources;
		bool bGrowthRangeKnown = false;
		FText GrowthRange;
	};

	struct FBreakthroughVm
	{
		FProviderSlice State;
	FProviderSlice SelectionState;
		FBreakthroughStage Stage;
		TArray<FBreakthroughMaterial> Selection;
		FName SelectedItemId;
		uint64 SelectionRevision = 0;
		FBreakthroughQuote Quote;
		bool bReworkMode = false;
		bool bQuoteAvailable = false;
		FText QuoteDisabledReason;
		bool bPrepareAvailable = false;
		FText PrepareDisabledReason;
		bool bReworkAvailable = false;
		FText ReworkDisabledReason;
		bool bConfirmationOpen = false;
		bool bConfirmationReworkMode = false;
		uint64 ConfirmationSelectionRevision = 0;
	};

	struct FTribulationVm
	{
		FProviderSlice State;
		bool bAttemptKnown = false;
		FName AttemptId;
		ETribulationPhase Phase = ETribulationPhase::Unknown;
		FText PhaseLabel;
		bool bTrialStartKnown = false;
		FText TrialStart;
		bool bDeadlineKnown = false;
		FText Deadline;
		bool bCleanupAtKnown = false;
		FText CleanupAt;
		bool bHelperCountKnown = false;
		int64 HelperCount = 0;
		bool bProbabilityKnown = false;
		int64 ProbabilityBasisPoints = 0;
		bool bOutcomeKnown = false;
		FText ExclusiveOutcome;
		bool bReceiptKnown = false;
		FName ReceiptId;
		bool bServerUtcKnown = false;
		FText ServerUtc;
		bool bCancelAvailable = false;
		FText CancelDisabledReason;
		bool bReconcileAvailable = false;
		FText ReconcileDisabledReason;
		bool bCancelConfirmationOpen = false;
		FName ConfirmationAttemptId;
	};

	UWidget* BuildBreakthroughPage(UWidgetTree* Tree, const FBreakthroughVm& ViewModel,
		const FInventoryVm& Inventory, const FStyleTokens& Style, ELayoutMode Layout,
		FActionRequested OnAction);

	UWidget* BuildTribulationPage(UWidgetTree* Tree, const FTribulationVm& ViewModel,
		const FStyleTokens& Style, ELayoutMode Layout, FActionRequested OnAction);
}

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkPlayerEActionButton : public UButton
{
	GENERATED_BODY()

public:
	void SetActionRequest(InkPlayerBreakthroughTribulationWidgets::FActionRequested InAction,
		FName InActionId, const InkPlayerBreakthroughTribulationWidgets::FActionPayload& InPayload);

	UFUNCTION()
	void HandleClicked();

private:
	InkPlayerBreakthroughTribulationWidgets::FActionRequested Action;
	FName ActionId;
	InkPlayerBreakthroughTribulationWidgets::FActionPayload Payload;
};
