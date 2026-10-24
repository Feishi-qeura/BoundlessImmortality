#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkJourneyBatch2Widgets.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UScrollBox;
class UInkJourneySettlementCardWidget;

USTRUCT(BlueprintType)
struct FInkJourneyDisplayRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Journey")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Journey")
    FText Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Journey")
    bool bEmphasized = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkJourneyBatch2EntryRequested, const FString&, EntryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkJourneyBatch2BackRequested);

/** Shared live-text paper card. Callers pass only authority-derived result rows. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneySettlementCardWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetCardContent(const FText& Heading, const FText& Status, const TArray<FInkJourneyDisplayRow>& Rows);

protected:
    virtual void NativeOnInitialized() override;

private:
    void EnsureTree();
    void RebuildRows();
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HeadingText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> RowsScroll;
    FText HeadingValue;
    FText StatusValue;
    TArray<FInkJourneyDisplayRow> DisplayRows;
};

/** Resource details and confirmation only; the feature host revalidates before submit. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkResourceDetailBatch2Widget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetResourceDetail(const FString& EntryId, const FText& ResourceName, const FText& Status, const TArray<FInkJourneyDisplayRow>& Rows);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetTravelFeedback(const FText& Feedback, bool bRequestPending);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void ResetInteractionForReopen();

    UFUNCTION(BlueprintPure, Category="Ink UI|Journey")
    bool IsRequestPending() const { return bPending; }

    UFUNCTION(BlueprintPure, Category="Ink UI|Journey")
    FString GetSelectedEntryId() const { return SelectedEntryId; }

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Journey")
    FInkJourneyBatch2EntryRequested OnTravelRequested;

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Journey")
    FInkJourneyBatch2BackRequested OnBackRequested;

protected:
    virtual void NativeOnInitialized() override;

private:
    void EnsureTree();
    UFUNCTION() void HandlePrimaryAction();
    UFUNCTION() void HandleBackAction();
    void RefreshText();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> RowsScroll;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PrimaryText;
    UPROPERTY(Transient) TObjectPtr<UButton> PrimaryButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
    FString SelectedEntryId;
    FText ResourceName;
    FText ResourceStatus;
    FText TravelFeedback;
    TArray<FInkJourneyDisplayRow> DisplayRows;
    bool bConfirmationOpen = false;
    bool bPending = false;
};

/** Read-only result shell. It never claims, grants, consumes, or acknowledges a receipt. */
UCLASS(Abstract, Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyOutcomeBatch2Widget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetOutcome(const FText& Heading, const FText& Status, const TArray<FInkJourneyDisplayRow>& Rows);

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Journey")
    FInkJourneyBatch2BackRequested OnBackRequested;

protected:
    virtual void NativeOnInitialized() override;
    virtual FText DefaultHeading() const PURE_VIRTUAL(UInkJourneyOutcomeBatch2Widget::DefaultHeading, return FText::GetEmpty(););
    UFUNCTION() void HandleBackAction();

private:
    void EnsureTree();
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> RootContent;
    UPROPERTY(Transient) TObjectPtr<UInkJourneySettlementCardWidget> SettlementCard;
    UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
    FText HeadingValue;
    FText StatusValue;
    TArray<FInkJourneyDisplayRow> DisplayRows;
    bool bHasAuthorityPayload = false;
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkWorldResultBatch2Widget : public UInkJourneyOutcomeBatch2Widget
{
    GENERATED_BODY()
protected:
    virtual FText DefaultHeading() const override { return FText::FromString(TEXT("世界历练结果")); }
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkRaceResultBatch2Widget : public UInkJourneyOutcomeBatch2Widget
{
    GENERATED_BODY()
protected:
    virtual FText DefaultHeading() const override { return FText::FromString(TEXT("灵脉竞采结果")); }
};
