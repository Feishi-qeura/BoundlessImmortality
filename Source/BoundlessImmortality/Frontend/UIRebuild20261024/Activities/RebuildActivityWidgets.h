#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "RebuildActivityWidgets.generated.h"

class UBorder;
class UScrollBox;
class UVerticalBox;

UENUM(BlueprintType)
enum class ERebuildActivityPageState : uint8
{
    Unknown,
    Loading,
    Offline,
    Empty,
    Ready,
    Disabled,
    Error,
    Pending,
    Complete,
    Expired
};

USTRUCT(BlueprintType)
struct FRebuildLaunchStartConfirmationSubject
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString AnchorUserId;
    UPROPERTY(BlueprintReadOnly) FString GuildId;
    UPROPERTY(BlueprintReadOnly) FString RulesVersion;
    UPROPERTY(BlueprintReadOnly) FString ProfileId;
    UPROPERTY(BlueprintReadOnly) bool bLaunchKnownAbsent = false;
};

USTRUCT(BlueprintType)
struct FRebuildActivityActionRequest
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Action;
    UPROPERTY(BlueprintReadOnly) FName EntryId;
    UPROPERTY(BlueprintReadOnly) FName Cycle;
    UPROPERTY(BlueprintReadOnly) FString ExpectedServiceDay;
    UPROPERTY(BlueprintReadOnly) bool bConfirmed = false;
    UPROPERTY(BlueprintReadOnly) FString Version;
    UPROPERTY(BlueprintReadOnly) FString PriceText;
    UPROPERTY(BlueprintReadOnly) FName OfferId;
    UPROPERTY(BlueprintReadOnly) FString QuantityText;
    UPROPERTY(BlueprintReadOnly) int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly) int32 Offset = 0;
    UPROPERTY(BlueprintReadOnly) bool bOwnedOnly = false;
    UPROPERTY(BlueprintReadOnly) FRebuildLaunchStartConfirmationSubject StartSubject;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRebuildActivityActionRequested, FRebuildActivityActionRequest, Request);

UCLASS()
class BOUNDLESSIMMORTALITY_API URebuildActivityIntentButton : public UButton
{
    GENERATED_BODY()
public:
    URebuildActivityIntentButton(const FObjectInitializer& ObjectInitializer);
    UPROPERTY(BlueprintAssignable) FOnRebuildActivityActionRequested OnIntentRequested;
    UPROPERTY() FRebuildActivityActionRequest Request;
private:
    UFUNCTION() void HandleClicked();
};

UCLASS(Abstract)
class BOUNDLESSIMMORTALITY_API URebuildActivityPageWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnRebuildActivityActionRequested OnActionRequested;
    UFUNCTION() void RelayIntent(FRebuildActivityActionRequest Request);
};

USTRUCT(BlueprintType)
struct FRebuildActivityRowViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName Id;
    UPROPERTY(BlueprintReadWrite) FName Cycle;
    UPROPERTY(BlueprintReadWrite) FName AllowedAction;
    UPROPERTY(BlueprintReadWrite) FText Title;
    UPROPERTY(BlueprintReadWrite) FText Summary;
    UPROPERTY(BlueprintReadWrite) FText StateText;
    UPROPERTY(BlueprintReadWrite) FText EligibilityText;
    UPROPERTY(BlueprintReadWrite) FText DeadlineText;
    UPROPERTY(BlueprintReadWrite) FText RewardBudgetText;
    UPROPERTY(BlueprintReadWrite) FText ProgressText;
    UPROPERTY(BlueprintReadWrite) FText ActionDisabledReason;
    UPROPERTY(BlueprintReadWrite) FText ActionLabel;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bHasProgress = false;
    UPROPERTY(BlueprintReadWrite) float ProgressFraction = 0.f;
    UPROPERTY(BlueprintReadWrite) bool bActionAllowed = false;
};

USTRUCT(BlueprintType)
struct FRebuildActivityCategoryViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName Cycle;
    UPROPERTY(BlueprintReadWrite) FText Title;
    UPROPERTY(BlueprintReadWrite) FText Summary;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bHasCount = false;
    UPROPERTY(BlueprintReadWrite) int32 Count = 0;
    UPROPERTY(BlueprintReadWrite) bool bActivitiesProvided = false;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildActivityRowViewModel> Activities;
};

USTRUCT(BlueprintType)
struct FRebuildActivitiesHubViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FName RouteCycle;
    UPROPERTY(BlueprintReadWrite) FText ServiceStateText;
    UPROPERTY(BlueprintReadWrite) FText ServiceDayText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) FRebuildActivityRowViewModel FeaturedLaunch;
    UPROPERTY(BlueprintReadWrite) bool bHasFeaturedLaunch = false;
    UPROPERTY(BlueprintReadWrite) bool bCategoriesProvided = false;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildActivityCategoryViewModel> Categories;
};

USTRUCT(BlueprintType)
struct FRebuildActivityCategoryDetailViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FString ServiceDay;
    UPROPERTY(BlueprintReadWrite) FName SelectedCycle;
    UPROPERTY(BlueprintReadWrite) FName SelectedEntryId;
    UPROPERTY(BlueprintReadWrite) FText ServiceStateText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bCategoriesProvided = false;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildActivityCategoryViewModel> Categories;
    UPROPERTY(BlueprintReadWrite) FRebuildActivityRowViewModel SelectedActivity;
    UPROPERTY(BlueprintReadWrite) bool bHasSelectedActivity = false;
};

USTRUCT(BlueprintType)
struct FRebuildLaunchTaskViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName Id;
    UPROPERTY(BlueprintReadWrite) FText ConditionText;
    UPROPERTY(BlueprintReadWrite) FText StateText;
    UPROPERTY(BlueprintReadWrite) FText RewardText;
    UPROPERTY(BlueprintReadWrite) FText ProgressText;
    UPROPERTY(BlueprintReadWrite) bool bHasProgress = false;
    UPROPERTY(BlueprintReadWrite) bool bConfirmed = false;
    UPROPERTY(BlueprintReadWrite) float ProgressFraction = 0.f;
};

USTRUCT(BlueprintType)
struct FRebuildLaunchMilestoneViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName Id;
    UPROPERTY(BlueprintReadWrite) FText Title;
    UPROPERTY(BlueprintReadWrite) FText ProgressText;
    UPROPERTY(BlueprintReadWrite) FText StateText;
};

USTRUCT(BlueprintType)
struct FRebuildLaunchActivityViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FName EventId;
    UPROPERTY(BlueprintReadWrite) FString ServiceDay;
    UPROPERTY(BlueprintReadWrite) FRebuildLaunchStartConfirmationSubject StartSubject;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) FText StartTimeText;
    UPROPERTY(BlueprintReadWrite) FText EndTimeText;
    UPROPERTY(BlueprintReadWrite) FText PersonalScoreText;
    UPROPERTY(BlueprintReadWrite) bool bHasPersonalScore = false;
    UPROPERTY(BlueprintReadWrite) FText CompleteRoundsText;
    UPROPERTY(BlueprintReadWrite) bool bHasCompleteRounds = false;
    UPROPERTY(BlueprintReadWrite) int32 SelectedTab = 0;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildLaunchTaskViewModel> Tasks;
    UPROPERTY(BlueprintReadWrite) bool bTasksProvided = false;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildLaunchMilestoneViewModel> Milestones;
    UPROPERTY(BlueprintReadWrite) bool bMilestonesProvided = false;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bConnected = false;
    UPROPERTY(BlueprintReadWrite) bool bBusy = false;
    UPROPERTY(BlueprintReadWrite) bool bCanStart = false;
    UPROPERTY(BlueprintReadWrite) FText StartDisabledReason;
    UPROPERTY(BlueprintReadWrite) bool bCanEnterWorld = false;
    UPROPERTY(BlueprintReadWrite) bool bTaskNumericRewardsAllowed = false;
    UPROPERTY(BlueprintReadWrite) FText OfferDisabledReason;
};

/** Independent activity-center page. Host supplies authority view state and handles all intents. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildActivitiesHubWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure) FName GetPresentedCycle()const{return SelectedCycle;}
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildActivitiesHubViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
private:
    void BuildPage();
    void Emit(FName Action, FName EntryId = NAME_None, FName Cycle = NAME_None, const FString& Day = FString());
    UPROPERTY(Transient) FRebuildActivitiesHubViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StablePageRoot = nullptr;
    UPROPERTY(Transient) UBorder* StablePageFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    FName SelectedCycle = TEXT("daily");
};

/** Responsive portrait list/detail and wide split view for daily, limited and permanent cycles. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildActivityCategoryDetailWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildActivityCategoryDetailViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    UWidget* BuildActivityList();
    UWidget* BuildSelectedDetail();
    void Emit(FName Action, FName EntryId = NAME_None, FName Cycle = NAME_None, const FString& Day = FString());
    UPROPERTY(Transient) FRebuildActivityCategoryDetailViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StablePageRoot = nullptr;
    UPROPERTY(Transient) UBorder* StablePageFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    bool bWideLayout = false;
    bool bHasMeasuredLayout = false;
};

/** Launch overview/tasks/guild/honors summary with service-owned actions and disabled offer slot. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildLaunchActivityWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildLaunchActivityViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
private:
    void BuildPage();
    void Emit(FName Action, FName EntryId = NAME_None, FName Cycle = NAME_None, const FString& Day = FString());
    void SelectTab(int32 Index);
    void HandleStartClicked();
    UFUNCTION() void HandleLocalTab(FRebuildActivityActionRequest Request);
    UFUNCTION() void HandleStartIntent(FRebuildActivityActionRequest Request);
    UPROPERTY(Transient) FRebuildLaunchActivityViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StablePageRoot = nullptr;
    UPROPERTY(Transient) UBorder* StablePageFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    FString ConfirmedStartDay;
    FRebuildLaunchStartConfirmationSubject ConfirmedStartSubject;
    bool bConfirmingStart = false;
    bool bHasSelectedTab = false;
    int32 SelectedTab = 0;
};
