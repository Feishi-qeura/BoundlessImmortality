#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "RebuildActivityWidgets.h"
#include "RebuildActivitySecondaryWidgets.generated.h"

USTRUCT(BlueprintType)
struct FRebuildLaunchRankRowViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName ParticipantId;
    UPROPERTY(BlueprintReadWrite) FText RankText;
    UPROPERTY(BlueprintReadWrite) FText DisplayName;
    UPROPERTY(BlueprintReadWrite) FText ContributionText;
    UPROPERTY(BlueprintReadWrite) bool bRankKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bNameKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bContributionKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bIsCurrentParticipant = false;
};

USTRUCT(BlueprintType)
struct FRebuildLaunchRankViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FName EventId;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) FText ScopeText;
    UPROPERTY(BlueprintReadWrite) FText PeriodText;
    UPROPERTY(BlueprintReadWrite) FText OwnRankText;
    UPROPERTY(BlueprintReadWrite) FText OwnContributionText;
    UPROPERTY(BlueprintReadWrite) bool bOwnRankKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bOwnContributionKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bRowsProvided = false;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildLaunchRankRowViewModel> Rows;
};

USTRUCT(BlueprintType)
struct FRebuildLaunchOfferViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FName OfferId;
    UPROPERTY(BlueprintReadWrite) FName Sku;
    UPROPERTY(BlueprintReadWrite) FString QuoteVersion;
    UPROPERTY(BlueprintReadWrite) FText Title;
    UPROPERTY(BlueprintReadWrite) FText Description;
    UPROPERTY(BlueprintReadWrite) FText PriceText;
    UPROPERTY(BlueprintReadWrite) FText EntitlementText;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bSkuKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bPriceKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bEntitlementKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bOwnsEntitlement = false;
    UPROPERTY(BlueprintReadWrite) bool bPlatformPaymentAvailable = false;
    UPROPERTY(BlueprintReadWrite) bool bServicePurchaseAvailable = false;
    UPROPERTY(BlueprintReadWrite) bool bConnected = false;
    UPROPERTY(BlueprintReadWrite) bool bBusy = false;
};

USTRUCT(BlueprintType)
struct FRebuildOfflineReceiptRowViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName ReceiptId;
    UPROPERTY(BlueprintReadWrite) FName SourceId;
    UPROPERTY(BlueprintReadWrite) FText SourceText;
    UPROPERTY(BlueprintReadWrite) FText TimeText;
    UPROPERTY(BlueprintReadWrite) FText QiText;
    UPROPERTY(BlueprintReadWrite) FText StateText;
    UPROPERTY(BlueprintReadWrite) bool bSourceKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bTimeKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bQiKnown = false;
};

USTRUCT(BlueprintType)
struct FRebuildOfflineRewardsViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FName SourceId;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) FText OfflineTotalQiText;
    UPROPERTY(BlueprintReadWrite) FText AcknowledgedQiText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bTotalQiKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bAcknowledgedQiKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bReceiptsProvided = false;
    UPROPERTY(BlueprintReadWrite) bool bCanAcknowledge = false;
    UPROPERTY(BlueprintReadWrite) bool bConnected = false;
    UPROPERTY(BlueprintReadWrite) bool bBusy = false;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildOfflineReceiptRowViewModel> Receipts;
};

UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildLaunchRankWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildLaunchRankViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    UPROPERTY(Transient) FRebuildLaunchRankViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StableRoot = nullptr;
    UPROPERTY(Transient) UBorder* StableFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    bool bWideLayout = false;
    bool bMeasured = false;
};

/** Embedded under LaunchEvent's offers-unavailable variant; it is not a new route. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildLaunchSpecialOfferWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildLaunchOfferViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    void HandlePurchaseIntent(FRebuildActivityActionRequest Request);
    UFUNCTION() void HandleOfferButton(FRebuildActivityActionRequest Request);
    UPROPERTY(Transient) FRebuildLaunchOfferViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StableRoot = nullptr;
    UPROPERTY(Transient) UBorder* StableFrame = nullptr;
    FName ConfirmedOfferId;
    FName ConfirmedSku;
    FString ConfirmedQuoteVersion;
    FText ConfirmedPriceText;
    bool bConfirmingPurchase = false;
    bool bWideLayout = false;
    bool bMeasured = false;
};

UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildOfflineRewardsWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildOfflineRewardsViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    UFUNCTION() void HandleAcknowledge(FRebuildActivityActionRequest Request);
    UPROPERTY(Transient) FRebuildOfflineRewardsViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StableRoot = nullptr;
    UPROPERTY(Transient) UBorder* StableFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    bool bWideLayout = false;
    bool bMeasured = false;
};
