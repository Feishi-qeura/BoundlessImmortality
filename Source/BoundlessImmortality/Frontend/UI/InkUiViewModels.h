#pragma once
#include "CoreMinimal.h"
#include "InkUiViewModels.generated.h"

UENUM(BlueprintType)
enum class EInkUiPageState : uint8 { Loading, Ready, Empty, Pending, Disconnected, Error, Expired, FeatureDisabled };
UENUM(BlueprintType)
enum class EInkUiRequestStatus : uint8 { Ready, Submitting, PendingReconcile, Confirmed, Rejected };

USTRUCT(BlueprintType)
struct FInkUiSnapshotContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName Profile;
    UPROPERTY(BlueprintReadOnly) FString App;
    UPROPERTY(BlueprintReadOnly) FString Room;
    UPROPERTY(BlueprintReadOnly) FString RuleVersion;
    UPROPERTY(BlueprintReadOnly) int64 SessionGeneration=0;
    UPROPERTY(BlueprintReadOnly) int64 SnapshotVersion=0;
    UPROPERTY(BlueprintReadOnly) double ServerUtc=0;
    UPROPERTY(BlueprintReadOnly) bool bIsAuthoritative=false;
};

/** Presentation bookkeeping only. Never grants rewards or replaces server idempotency. */
USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkUiRequestContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid RequestId;
    UPROPERTY(BlueprintReadOnly) EInkUiRequestStatus Status=EInkUiRequestStatus::Ready;
    UPROPERTY(BlueprintReadOnly) FString ReceiptId;
    UPROPERTY(BlueprintReadOnly) int64 SessionGeneration=0;
    UPROPERTY(BlueprintReadOnly) int64 ExpectedVersion=0;
    bool Begin(const FInkUiSnapshotContext& Snapshot);
    void MarkTimeout();
    bool ApplyReply(FGuid Id, int64 Generation, int64 Version, EInkUiRequestStatus Result, const FString& Receipt);
    bool IsPending()const{return Status==EInkUiRequestStatus::Submitting||Status==EInkUiRequestStatus::PendingReconcile;}
};
