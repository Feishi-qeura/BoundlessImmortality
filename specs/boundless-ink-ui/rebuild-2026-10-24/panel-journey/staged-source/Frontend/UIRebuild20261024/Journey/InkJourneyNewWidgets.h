#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "InkJourneyNewWidgets.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UHorizontalBox;
class UUniformGridPanel;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class UWrapBox;

UENUM(BlueprintType)
enum class EInkJourneyNewEntryState : uint8
{
    Unknown,
    Available,
    Locked
};

UENUM(BlueprintType)
enum class EInkJourneyNewClaimState : uint8
{
    Unknown,
    Available,
    Pending,
    Claimed,
    Offline,
    Disabled,
    Error
};

USTRUCT(BlueprintType)
struct FInkJourneyNewDisplayRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FText Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    bool bEmphasized = false;
};

USTRUCT(BlueprintType)
struct FInkJourneyNewMapEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FString EntryId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FString GroupId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FText StateText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    EInkJourneyNewEntryState State = EInkJourneyNewEntryState::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey", meta=(ClampMin="0", ClampMax="1"))
    FVector2D NormalizedPosition = FVector2D(.5f, .5f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    TArray<FInkJourneyNewDisplayRow> Details;
};

USTRUCT(BlueprintType)
struct FInkJourneyNewMilestone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    int32 Milestone = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    bool bKnown = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    bool bEligible = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    bool bClaimed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    FText Status;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")
    TArray<FInkJourneyNewDisplayRow> Rewards;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkJourneyNewBackRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkJourneyNewEntrySelected, const FString&, EntryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkJourneyNewTravelRequested, const FString&, EntryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkJourneyNewSignInRequested, const FString&, ExpectedDay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInkJourneyNewMilestoneClaimRequested, const FString&, ExpectedDay, int32, Milestone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkJourneyNewMilestoneSelected, int32, Milestone);

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkJourneyMilestoneButton : public UButton
{
    GENERATED_BODY()
public:
    void Configure(int32 InMilestone, const FText& Caption, const FText& Status, bool bSelected);
    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewMilestoneSelected OnMilestoneSelected;
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void HandleClicked();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CaptionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    int32 Milestone = 0;
};

/** Map pins own their exact runtime EntryId and communicate it through a typed delegate. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkJourneyMapPinButton : public UButton
{
    GENERATED_BODY()
public:
    void Configure(const FString& InEntryId, const FText& Caption, EInkJourneyNewEntryState State, bool bSelected, int32 EntryCount=1);
    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewEntrySelected OnEntryPressed;
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void HandleClicked();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CaptionText;
    FString EntryId;
};

/** Responsive map page base. Host supplies only current authority rows and binds the exact-ID events. */
UCLASS(Abstract, Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyMapPageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetAuthorityData(const FText& State, const TArray<FInkJourneyNewMapEntry>& Entries, const FString& SelectedEntryId);

    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetTravelFeedback(const FText& Feedback, bool bRequestPending);

    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void ResetInteractionForReopen();

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewBackRequested OnBackRequested;

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewEntrySelected OnEntrySelected;

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewTravelRequested OnTravelRequested;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FText PageTitle() const PURE_VIRTUAL(UInkJourneyMapPageWidget::PageTitle, return FText::GetEmpty(););
    virtual bool IsResourceMap() const PURE_VIRTUAL(UInkJourneyMapPageWidget::IsResourceMap, return false;);
    UFUNCTION() void HandlePinSelected(const FString& EntryId);
    UFUNCTION() void HandlePrimaryAction();
    UFUNCTION() void HandleBackAction();
private:
    void BuildTree();
    void BuildMapPins(UCanvasPanel* MapCanvas, TArray<TObjectPtr<UInkJourneyMapPinButton>>& OutPins, float Width, float Height, bool bLandscape);
    void BuildDetails(UVerticalBox* Target);
    void RefreshDetails();
    void LayoutResponsive(const FVector2D& Size);
    const FInkJourneyNewMapEntry* FindSelectedEntry() const;
    FString EntryGroup(const FInkJourneyNewMapEntry& Entry) const;

    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Root;
    UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> LayoutSwitcher;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> PortraitLayout;
    UPROPERTY(Transient) TObjectPtr<UHorizontalBox> LandscapeLayout;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> PortraitMapCanvas;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> LandscapeMapCanvas;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> PortraitDetails;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> LandscapeDetails;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> PortraitDetailRows;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> LandscapeDetailRows;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> AuthorityStatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PortraitTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PortraitSelectedTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LandscapeTitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PortraitActionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> LandscapeActionText;
    UPROPERTY(Transient) TObjectPtr<UButton> PortraitActionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> LandscapeActionButton;
    UPROPERTY(Transient) TObjectPtr<UImage> PortraitMapImage;
    UPROPERTY(Transient) TObjectPtr<UImage> LandscapeMapImage;
    UPROPERTY(Transient) TArray<TObjectPtr<UInkJourneyMapPinButton>> PortraitPins;
    UPROPERTY(Transient) TArray<TObjectPtr<UInkJourneyMapPinButton>> LandscapePins;
    UPROPERTY(Transient) TArray<FInkJourneyNewMapEntry> Entries;
    UPROPERTY(Transient) FString SelectedEntryId;
    UPROPERTY(Transient) FText AuthorityState;
    UPROPERTY(Transient) FText TravelFeedback;
    FString LastSelectedEntryId;
    bool bConfirmationOpen = false;
    bool bRequestPending = false;
    bool bHasLayout = false;
    bool bPortrait = true;
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyWorldMapPageWidget : public UInkJourneyMapPageWidget
{
    GENERATED_BODY()
protected:
    virtual FText PageTitle() const override { return FText::FromString(TEXT("山海寻道 · 世界地图")); }
    virtual bool IsResourceMap() const override { return false; }
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyResourceMapPageWidget : public UInkJourneyMapPageWidget
{
    GENERATED_BODY()
protected:
    virtual FText PageTitle() const override { return FText::FromString(TEXT("灵脉寻踪 · 资源地图")); }
    virtual bool IsResourceMap() const override { return true; }
};

/** Daily sign-in and daily verse are separate display/actions; this page never invents fortune. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyDailySignInPageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetPresentation(const FString& ServerDay, EInkJourneyNewClaimState State, const FText& Status,
        bool bHasMonthCount, int32 MonthCount, bool bVerseProvided, const FText& Verse);

    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetClaimFeedback(const FText& Feedback, bool bPending);

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewBackRequested OnBackRequested;

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewSignInRequested OnClaimRequested;

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewBackRequested OnMonthlySignInRequested;

protected:
    virtual void NativeOnInitialized() override;
private:
    void BuildTree();
    void RefreshPresentation();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleClaim();
    UFUNCTION() void HandleOpenMonthly();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DayText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> VerseText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MonthCountText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ClaimButtonText;
    UPROPERTY(Transient) TObjectPtr<UButton> ClaimButton;
    UPROPERTY(Transient) TObjectPtr<UButton> MonthlyButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
    UPROPERTY(Transient) TObjectPtr<UWrapBox> ContentWrap;
    FString ServerDay;
    EInkJourneyNewClaimState ClaimState = EInkJourneyNewClaimState::Unknown;
    FText ClaimStatus;
    FText ClaimFeedback;
    FText Verse;
    int32 MonthCount = 0;
    bool bHasMonthCount = false;
    bool bVerseProvided = false;
    bool bLocalPending = false;
};

/** Month grid is read-only unless the service provides a milestone that is claimable. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyMonthlySignInPageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetPresentation(const FString& ExpectedServiceDay, const FString& ServerMonth, int32 CalendarDayCount, int32 ServerDay,
        bool bHasMonthCount, int32 MonthCount, const TArray<FInkJourneyNewMilestone>& Milestones, const FText& Status);

    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetMilestoneFeedback(const FText& Feedback, bool bPending);

    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void ResetConfirmationForReopen();

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewBackRequested OnBackRequested;

    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewMilestoneClaimRequested OnMilestoneClaimRequested;

protected:
    virtual void NativeOnInitialized() override;
private:
    void BuildTree();
    void RefreshPresentation();
    void RefreshMilestones();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleConfirmClaim();
    UFUNCTION() void HandleMilestoneSelected(int32 Milestone);
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MonthText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CalendarStateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MilestoneText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmText;
    UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> CalendarGrid;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> MilestoneList;
    UPROPERTY(Transient) TObjectPtr<UWrapBox> ContentWrap;
    UPROPERTY(Transient) TObjectPtr<UButton> ConfirmButton;
    UPROPERTY(Transient) TArray<TObjectPtr<UInkJourneyMilestoneButton>> MilestoneButtons;
    UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
    FString ServerMonth;
    FString ExpectedServiceDay;
    int32 CalendarDayCount = 0;
    int32 ServerDay = 0;
    int32 MonthCount = 0;
    bool bHasMonthCount = false;
    TArray<FInkJourneyNewMilestone> Milestones;
    int32 SelectedMilestone = 0;
    bool bConfirmationOpen = false;
    bool bRequestPending = false;
    FText Status;
    FText Feedback;
};

/** Detail/result shell for resource detail, world result, and race result routes. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyDetailResultPageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetPageData(const FText& Title, const FString& ExactSourceId, const FText& Status, const TArray<FInkJourneyNewDisplayRow>& Rows, bool bCanTravel);
    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void SetTravelFeedback(const FText& Feedback, bool bPending);
    UFUNCTION(BlueprintCallable, Category="newUI|Journey")
    void ResetForReopen();
    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewBackRequested OnBackRequested;
    UPROPERTY(BlueprintAssignable, Category="newUI|Journey")
    FInkJourneyNewTravelRequested OnTravelRequested;
protected:
    virtual void NativeOnInitialized() override;
private:
    void BuildTree();
    void Refresh();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandlePrimary();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ActionText;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> RowsScroll;
    UPROPERTY(Transient) TObjectPtr<UButton> ActionButton;
    UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
    UPROPERTY(Transient) TArray<FInkJourneyNewDisplayRow> Rows;
    FString ExactSourceId;
    FText Title;
    FText Status;
    FText Feedback;
    bool bCanTravel = false;
    bool bConfirmationOpen = false;
    bool bPending = false;
};

/** Distinct route classes keep integration from collapsing unrelated page IDs. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyResourceDetailPageWidget : public UInkJourneyDetailResultPageWidget
{
    GENERATED_BODY()
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyWorldResultPageWidget : public UInkJourneyDetailResultPageWidget
{
    GENERATED_BODY()
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyRaceResultPageWidget : public UInkJourneyDetailResultPageWidget
{
    GENERATED_BODY()
};
