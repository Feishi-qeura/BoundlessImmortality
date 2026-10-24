#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "RebuildActivityWidgets.h"
#include "RebuildEconomyWidgets.generated.h"

class UBorder;
class UEditableTextBox;

USTRUCT(BlueprintType)
struct FRebuildShopCatalogEntryViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName Sku;
    UPROPERTY(BlueprintReadWrite) FText Name;
    UPROPERTY(BlueprintReadWrite) FText Description;
    UPROPERTY(BlueprintReadWrite) FText PriceText;
    UPROPERTY(BlueprintReadWrite) FText CostText;
    UPROPERTY(BlueprintReadWrite) FText OutputText;
    UPROPERTY(BlueprintReadWrite) FText StateText;
    UPROPERTY(BlueprintReadWrite) FString QuoteVersion;
    UPROPERTY(BlueprintReadWrite) bool bSkuKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bPriceKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bCanPurchase = false;
};

USTRUCT(BlueprintType)
struct FRebuildShopViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bEntriesProvided = false;
    UPROPERTY(BlueprintReadWrite) bool bQueryAllowed = false;
    UPROPERTY(BlueprintReadWrite) bool bConnected = false;
    UPROPERTY(BlueprintReadWrite) bool bBusy = false;
    UPROPERTY(BlueprintReadWrite) bool bOwnedOnly = false;
    UPROPERTY(BlueprintReadWrite) int32 Offset = 0;
    UPROPERTY(BlueprintReadWrite) int32 PageSize = 0;
    UPROPERTY(BlueprintReadWrite) bool bHasNextPage = false;
    UPROPERTY(BlueprintReadWrite) FName SelectedSku;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildShopCatalogEntryViewModel> Entries;
};

USTRUCT(BlueprintType)
struct FRebuildConsignmentInventoryEntryViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName ItemId;
    UPROPERTY(BlueprintReadWrite) FText Name;
    UPROPERTY(BlueprintReadWrite) FText AvailableCountText;
    UPROPERTY(BlueprintReadWrite) bool bNameKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bCountKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bCanList = false;
};

USTRUCT(BlueprintType)
struct FRebuildConsignmentListingViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName ListingId;
    UPROPERTY(BlueprintReadWrite) FName ListingVersion;
    UPROPERTY(BlueprintReadWrite) FName ItemId;
    UPROPERTY(BlueprintReadWrite) FText Name;
    UPROPERTY(BlueprintReadWrite) FText PriceText;
    UPROPERTY(BlueprintReadWrite) FText StateText;
    UPROPERTY(BlueprintReadWrite) bool bNameKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bPriceKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bCanBuy = false;
};

USTRUCT(BlueprintType)
struct FRebuildConsignmentViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bListingsProvided = false;
    UPROPERTY(BlueprintReadWrite) bool bInventoryProvided = false;
    UPROPERTY(BlueprintReadWrite) bool bPriceValidationAvailable = false;
    UPROPERTY(BlueprintReadWrite) bool bCreateAllowed = false;
    UPROPERTY(BlueprintReadWrite) bool bQueryAllowed = false;
    UPROPERTY(BlueprintReadWrite) bool bConnected = false;
    UPROPERTY(BlueprintReadWrite) bool bBusy = false;
    UPROPERTY(BlueprintReadWrite) bool bOwnedOnly = false;
    UPROPERTY(BlueprintReadWrite) int32 Offset = 0;
    UPROPERTY(BlueprintReadWrite) int32 PageSize = 0;
    UPROPERTY(BlueprintReadWrite) bool bHasNextPage = false;
    UPROPERTY(BlueprintReadWrite) FName SelectedListingId;
    UPROPERTY(BlueprintReadWrite) FName SelectedInventoryItemId;
    UPROPERTY(BlueprintReadWrite) FString DraftQuantityText;
    UPROPERTY(BlueprintReadWrite) FString DraftUnitPriceText;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildConsignmentListingViewModel> Listings;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildConsignmentInventoryEntryViewModel> InventoryEntries;
};

USTRUCT(BlueprintType)
struct FRebuildOrderRowViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FName ListingId;
    UPROPERTY(BlueprintReadWrite) FName ListingVersion;
    UPROPERTY(BlueprintReadWrite) FText ItemText;
    UPROPERTY(BlueprintReadWrite) FText PriceText;
    UPROPERTY(BlueprintReadWrite) FText StateText;
    UPROPERTY(BlueprintReadWrite) FText TimeText;
    UPROPERTY(BlueprintReadWrite) bool bItemKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bPriceKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bTimeKnown = false;
    UPROPERTY(BlueprintReadWrite) bool bCanCancel = false;
};

USTRUCT(BlueprintType)
struct FRebuildOrdersViewModel
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString ContentKey;
    UPROPERTY(BlueprintReadWrite) FText StatusText;
    UPROPERTY(BlueprintReadWrite) ERebuildActivityPageState State = ERebuildActivityPageState::Unknown;
    UPROPERTY(BlueprintReadWrite) bool bOrdersProvided = false;
    UPROPERTY(BlueprintReadWrite) bool bConnected = false;
    UPROPERTY(BlueprintReadWrite) bool bBusy = false;
    UPROPERTY(BlueprintReadWrite) FName SelectedListingId;
    UPROPERTY(BlueprintReadWrite) TArray<FRebuildOrderRowViewModel> Orders;
};

UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildShopWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildShopViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    UFUNCTION() void HandleShopIntent(FRebuildActivityActionRequest Request);
    UPROPERTY(Transient) FRebuildShopViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StableRoot = nullptr;
    UPROPERTY(Transient) UBorder* StableFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    FName ConfirmedSku;
    FString ConfirmedQuoteVersion;
    FText ConfirmedPriceText;
    bool bConfirmingPurchase = false;
    bool bWideLayout = false;
    bool bMeasured = false;
};

UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildConsignmentWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildConsignmentViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    UFUNCTION() void HandleQuantityChanged(const FText& Text);
    UFUNCTION() void HandlePriceChanged(const FText& Text);
    UFUNCTION() void HandleConsignmentIntent(FRebuildActivityActionRequest Request);
    UPROPERTY(Transient) FRebuildConsignmentViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StableRoot = nullptr;
    UPROPERTY(Transient) UBorder* StableFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    UPROPERTY(Transient) UEditableTextBox* QuantityInput = nullptr;
    UPROPERTY(Transient) UEditableTextBox* PriceInput = nullptr;
    FName ConfirmedItemId;
    FName ConfirmedListingId;
    FName ConfirmedListingVersion;
    FString ConfirmedBuyPriceText;
    int32 ConfirmedQuantity = 0;
    FString ConfirmedPriceText;
    bool bConfirmingCreate = false;
    bool bConfirmingBuy = false;
    bool bWideLayout = false;
    bool bMeasured = false;
};

UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API URebuildOrdersWidget : public URebuildActivityPageWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void ApplyViewModel(const FRebuildOrdersViewModel& Model);
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    void BuildPage();
    UFUNCTION() void HandleCancelIntent(FRebuildActivityActionRequest Request);
    UPROPERTY(Transient) FRebuildOrdersViewModel ViewModel;
    UPROPERTY(Transient) UVerticalBox* StableRoot = nullptr;
    UPROPERTY(Transient) UBorder* StableFrame = nullptr;
    UPROPERTY(Transient) UScrollBox* ActiveScroll = nullptr;
    FName ConfirmedListingId;
    FName ConfirmedListingVersion;
    bool bConfirmingCancel = false;
    bool bWideLayout = false;
    bool bMeasured = false;
};
