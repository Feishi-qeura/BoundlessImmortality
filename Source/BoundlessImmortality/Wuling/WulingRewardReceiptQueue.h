#pragma once
#include "CoreMinimal.h"

class FJsonObject;

/** Presentation-only, per game-instance inbox. Receipt sequence never moves back. */
struct BOUNDLESSIMMORTALITY_API FWulingRewardReceiptQueue
{
    void Observe(const TSharedPtr<FJsonObject>& Projection);
    FString TakeNext();
    int32 Num() const { return Pending.Num(); }
private:
    bool bBaselined=false;
    int64 HighWatermark=0;
    TArray<FString> Pending;
    TArray<FString> SeenOrder;
    TSet<FString> Seen;
};
