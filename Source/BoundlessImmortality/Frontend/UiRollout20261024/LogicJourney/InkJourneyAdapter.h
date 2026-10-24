#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UWulingPrototypeSubsystem;

/** Candidate-shell adapter. Never creates rewards, dates, geography or entitlements. */
namespace InkJourneyAdapter
{
using FObject = TSharedPtr<FJsonObject>;
enum class EState : uint8 { Disabled, Offline, Loading, Pending, Ready };
enum class EAction : uint8 { SignIn, MonthlyChest, WorldTravel, ResourceTravel, StartLaunch };

struct FContext
{
    FObject Snapshot;
    bool bEnabled = false;
    bool bConnected = false;
    bool bBusy = false;
    bool bActivePage = false;
};

struct FRows
{
    // Empty + bProvided=false is loading/missing; true is an authoritative empty list.
    bool bProvided = false;
    TArray<FObject> Items;
};

struct FView
{
    EState State = EState::Disabled;
    FObject Activities, Journey, Launch;
    FRows Worlds, Resources, WeeklyChests, MonthlyChests, Templates;
    FString ServiceDay, Verse;
    TOptional<bool> Signed;
    TOptional<double> MonthSignCount;
    bool bLaunchKnownAbsent = false;
};

struct FRequest
{
    EAction Action = EAction::SignIn;
    // Capture day/month or selected entry when opening confirmation; revalidate on click.
    FString ExpectedDay;
    FString EntryId;
    int32 Milestone = 0;
    bool bConfirmed = false;
};

struct FGate
{
    bool bAllowed = false;
    FString Reason;
};

BOUNDLESSIMMORTALITY_API FContext Capture(UWulingPrototypeSubsystem* Service, bool bActivePage);
BOUNDLESSIMMORTALITY_API FView Read(const FContext& Context);
BOUNDLESSIMMORTALITY_API FRows Rows(const FObject& Object, const TCHAR* Field);
/** Explicit category rows take precedence; fallback classifies only exact template cycles. */
BOUNDLESSIMMORTALITY_API FRows ActivitiesForCycle(const FView& View, const FString& Cycle);
BOUNDLESSIMMORTALITY_API FGate Evaluate(const FContext& Context, const FRequest& Request);
/** true means enqueued, never granted/settled; receipts remain owned by the subsystem. */
BOUNDLESSIMMORTALITY_API bool Submit(UWulingPrototypeSubsystem* Service, bool bActivePage, const FRequest& Request, FString& Feedback);
}
