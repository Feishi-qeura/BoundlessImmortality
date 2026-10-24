#pragma once
#include "CoreMinimal.h"
#include "IAssetTools.h"

struct FKnownRenameWarningResult
{
    bool bRenameSucceeded = false;
    int32 AcknowledgedKnownWarnings = 0;
    int32 RejectedUnknownDialogs = 0;
    FString Refusal;
    TArray<FString> DialogEvidence;
};

// Integration staging suggestion only; not compiled/executed by cleanup.
// Caller MUST preserve existing exact map/backup/pin/journal/referencer validations.
FKnownRenameWarningResult Rename582WithScopedKnownWarningAck(
    IAssetTools& AssetTools, const TArray<FAssetRenameData>& Exact582);
