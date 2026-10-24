#pragma once

#include "CoreMinimal.h"
#include "InkRankWulingPageBase.h"
#include "InkRankWulingPages.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UImage;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UImage;

UENUM(BlueprintType)
enum class EInkRankPageState : uint8 { Unknown, Unavailable, Loading, Empty, Data, Pending, Offline, Error, Disabled };

USTRUCT(BlueprintType)
struct FInkNewRankRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Rank = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText EarnedTitle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Realm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Metric;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UTexture2D> Avatar = nullptr;
};

USTRUCT(BlueprintType)
struct FInkNewRankBoardData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Category = TEXT("Damage");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Scope = TEXT("Local");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText MetricLabel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ProviderStatus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInkRankPageState State = EInkRankPageState::Unavailable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkNewRankRow> Rows;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SelfPosition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SelfMetric;
};

USTRUCT(BlueprintType)
struct FInkWulingActionView
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ActionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bEnabled = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisabledReason;
};

USTRUCT(BlueprintType)
struct FInkWulingOverviewData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SectName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ServiceStatus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText MemberCount;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ParticipatingCount;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText HelperCount;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText PublicSupply;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Reputation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UTexture2D> Insignia = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkWulingActionView> ExistingActions;
};

USTRUCT(BlueprintType)
struct FInkWulingMemberRow
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString UserId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
};

USTRUCT(BlueprintType)
struct FInkWulingMembersData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInkRankPageState State = EInkRankPageState::Unavailable;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ServiceStatus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkWulingMemberRow> Rows;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Cursor = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PreviousCursor = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 NextCursor = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PageSize = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString SelectedUserId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanRemoveSelected = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRemovePending = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText RemoveDisabledReason;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkRankCategoryRequested, FName, Category);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkRankScopeRequested, FName, Scope);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkRankRetryRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkWulingActionRequested, FName, ActionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkWulingMemberSelected, FString, UserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkWulingMemberPageRequested, int32, Cursor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkWulingMemberRemoveRequested, FString, UserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkRankAvatarRequested, FString, ParticipantId);

/** Independent UMG leaderboard page. The host maps authoritative providers into data and owns route changes. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkRankBoardPage final : public UInkRankWulingPageBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FInkRankCategoryRequested OnCategoryRequested;
    UPROPERTY(BlueprintAssignable) FInkRankScopeRequested OnScopeRequested;
    UPROPERTY(BlueprintAssignable) FInkRankRetryRequested OnRetryRequested;
    UPROPERTY(BlueprintAssignable) FInkRankAvatarRequested OnAvatarRequested;
    UFUNCTION(BlueprintCallable) void SetBoardData(const FInkNewRankBoardData& InData);
    /** Non-null completes and caches a participant avatar; null marks async failure and releases the in-flight key. */
    UFUNCTION(BlueprintCallable) void SetParticipantAvatar(const FString& ParticipantId, UTexture2D* Avatar);
    /** Call only after the host's avatar retry/backoff policy allows another request. */
    UFUNCTION(BlueprintCallable) void RetryParticipantAvatarAfterHostBackoff(const FString& ParticipantId);
protected:
    virtual void NativeOnInitialized() override;
private:
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> RowsBox = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MetricHeaderText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MetricColumnText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelfText = nullptr;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelfMetricText = nullptr;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> CategoryTabs;
    UPROPERTY(Transient) TArray<TObjectPtr<UButton>> ScopeTabs;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UTexture2D>> ParticipantAvatarCache;
    UPROPERTY(Transient) TMap<FString, TObjectPtr<UImage>> AvatarImagesByParticipant;
    TSet<FString> AvatarRequestsInFlight;
    FInkNewRankBoardData Data;
    void BuildPage();
    void RefreshPage();
    virtual void RebuildForThemeChange() override;
};

/** Independent Wuling summary page; actions are descriptions and ids supplied by the host. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkWulingOverviewPage final : public UInkRankWulingPageBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FInkWulingActionRequested OnActionRequested;
    UFUNCTION(BlueprintCallable) void SetOverviewData(const FInkWulingOverviewData& InData);
protected:
    virtual void NativeOnInitialized() override;
private:
    FInkWulingOverviewData Data;
    void BuildPage();
    void RefreshPage();
    virtual void RebuildForThemeChange() override;
};

/** Independent paged member page; host supplies selection and authoritative permission state. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkWulingMembersPage final : public UInkRankWulingPageBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FInkWulingMemberSelected OnMemberSelected;
    UPROPERTY(BlueprintAssignable) FInkWulingMemberPageRequested OnPageRequested;
    UPROPERTY(BlueprintAssignable) FInkWulingMemberRemoveRequested OnRemoveRequested;
    UPROPERTY(BlueprintAssignable) FInkRankRetryRequested OnRetryRequested;
    UFUNCTION(BlueprintCallable) void SetMembersData(const FInkWulingMembersData& InData);
protected:
    virtual void NativeOnInitialized() override;
private:
    FInkWulingMembersData Data;
    FString PendingRemoveUserId;
    bool bConfirmRemove = false;
    bool bRemoveSubmitIssued = false;
    void BuildPage();
    void RefreshPage();
    void ConfirmRemove();
    void CancelRemove();
    virtual void RebuildForThemeChange() override;
};
