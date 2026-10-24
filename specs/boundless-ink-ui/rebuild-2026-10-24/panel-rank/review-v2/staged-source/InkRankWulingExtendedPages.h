#pragma once

#include "CoreMinimal.h"
#include "InkRankWulingPages.h"
#include "InkRankWulingExtendedPages.generated.h"

class UVerticalBox;

USTRUCT(BlueprintType)
struct FInkGuildBuildingCardView
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString BuildingId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CurrentLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasCurrentLevel = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TargetLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasTargetLevel = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText AuthoritativeCosts;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText StateText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanBuild = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisabledReason;
};

USTRUCT(BlueprintType)
struct FInkGuildBuildPageData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInkRankPageState State = EInkRankPageState::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ServiceStatus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkGuildBuildingCardView> Buildings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Offset = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 PreviousOffset = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 NextOffset = INDEX_NONE;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString SnapshotVersion;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOwnedOnly = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCanQuery = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bBuildPending = false;
};

USTRUCT(BlueprintType)
struct FInkSwordSpiritAttributeView
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName FieldId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Value;
};

USTRUCT(BlueprintType)
struct FInkSwordSpiritPageData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInkRankPageState State = EInkRankPageState::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ServiceStatus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Realm;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Honor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkSwordSpiritAttributeView> Attributes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UTexture2D> Portrait = nullptr;
};

USTRUCT(BlueprintType)
struct FInkGiftRuleView
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString RuleId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ActualDescription;
};

USTRUCT(BlueprintType)
struct FInkGiftEffectPageData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInkRankPageState State = EInkRankPageState::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ServiceStatus;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRulesProvided = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkGiftRuleView> Rules;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FInkGiftRuleView> ActiveEffects;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInkGuildBuildQueryRequested, int32, Offset, bool, OwnedOnly);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInkGuildBuildRequested, FString, BuildingId, int32, TargetLevel, FString, SnapshotVersion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkRankPageRetryRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkGiftRulesNavigationRequested);

/** Catalog UI only; the host owns building queries, revalidation, and authority command submission. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkGuildBuildPage final : public UInkRankWulingPageBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FInkGuildBuildQueryRequested OnQueryRequested;
    UPROPERTY(BlueprintAssignable) FInkGuildBuildRequested OnBuildRequested;
    UPROPERTY(BlueprintAssignable) FInkRankPageRetryRequested OnRetryRequested;
    UFUNCTION(BlueprintCallable) void SetBuildPageData(const FInkGuildBuildPageData& InData);
protected:
    virtual void NativeOnInitialized() override;
private:
    FInkGuildBuildPageData Data;
    FString ConfirmBuildingId;
    int32 ConfirmTargetLevel = 0;
    bool bConfirmBuild = false;
    bool bBuildSubmitIssued = false;
    void BuildPage();
    void RefreshPage();
    void ConfirmBuild();
    void CancelBuild();
    virtual void RebuildForThemeChange() override;
};

/** Viewer-specific personal sword-spirit view. Missing profile data remains unknown/unavailable. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkSwordSpiritPage final : public UInkRankWulingPageBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FInkGiftRulesNavigationRequested OnOpenGiftRulesRequested;
    UFUNCTION(BlueprintCallable) void SetSwordSpiritData(const FInkSwordSpiritPageData& InData);
protected:
    virtual void NativeOnInitialized() override;
private:
    FInkSwordSpiritPageData Data;
    void BuildPage();
    void RefreshPage();
    virtual void RebuildForThemeChange() override;
};

/** Read-only page. Empty catalog is shown only when the host marks rules as provided. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkGiftRulesPage final : public UInkRankWulingPageBase
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) void SetGiftEffectData(const FInkGiftEffectPageData& InData);
protected:
    virtual void NativeOnInitialized() override;
private:
    FInkGiftEffectPageData Data;
    void BuildPage();
    void RefreshPage();
    virtual void RebuildForThemeChange() override;
};
