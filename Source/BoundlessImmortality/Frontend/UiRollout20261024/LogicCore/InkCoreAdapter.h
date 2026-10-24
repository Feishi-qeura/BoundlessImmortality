#pragma once
#include "CoreMinimal.h"
#include "InkCorePolicy.h"
#include "Frontend/InkFrontendTypes.h"
#include "Frontend/InkInventoryModel.h"

class UInkFrontendWidget;
class UUserWidget;
class UWulingPrototypeSubsystem;
class FJsonObject;

/** Thin candidate binding. Existing subsystems remain the only owners of gameplay writes. */
namespace InkCore
{
struct FQuoteDecision
{
    InkCorePolicy::FQuoteFacts Facts;
    bool CanSubmit=false, CanCancel=false, CanReconcile=false;
    FString Reason;
};
struct FView
{
    InkCorePolicy::FTransport Transport;
    InkCorePolicy::EState CharacterState=InkCorePolicy::EState::Unavailable;
    InkCorePolicy::EState InventoryState=InkCorePolicy::EState::Unavailable;
    InkCorePolicy::EState GuildState=InkCorePolicy::EState::Unavailable;
    bool WulingAuthority=false;
    bool CanAct=false;
    FString Status;
    FText CharacterName;
    TArray<FInkCharacterAttribute> Attributes;
    TArray<FInkDisciplineEntry> Disciplines;
    TArray<FInkInventoryEntry> Inventory;
    // Read only; actual optional fields remain absent until the service provides them.
    TSharedPtr<FJsonObject> Guild, Growth, Quote, Tribulation;
};
struct FBoardView
{
    InkCorePolicy::EState State=InkCorePolicy::EState::Unavailable;
    FName Scope, Category;
    FString Status;
    TArray<FInkLeaderboardEntry> Entries;
};
struct FActionResult
{
    bool Accepted=false;
    bool PendingAuthority=false;
    FString Message;
};

BOUNDLESSIMMORTALITY_API UWulingPrototypeSubsystem* Service(const UUserWidget* Context);
BOUNDLESSIMMORTALITY_API InkCorePolicy::FTransport Transport(const UUserWidget* Context,bool Active);
BOUNDLESSIMMORTALITY_API FView Read(UInkFrontendWidget* Frontend,bool Active);
// RoomRows must come from the existing PublishGameplayData provider; never re-rank here.
BOUNDLESSIMMORTALITY_API FBoardView ReadBoard(const UUserWidget* Context,const TArray<FInkLeaderboardEntry>& RoomRows,FName Scope,FName Category,bool RoomDataProvided=false);
BOUNDLESSIMMORTALITY_API FQuoteDecision EvaluateQuote(const TSharedPtr<FJsonObject>& State,const TMap<FString,int32>& Selection,bool Rework=false);
BOUNDLESSIMMORTALITY_API FActionResult InventoryAction(UInkFrontendWidget* Frontend,FGuid EntryId,InkCorePolicy::EInventoryAction Action,int32 Quantity,bool Active);
BOUNDLESSIMMORTALITY_API FActionResult QuoteBreakthrough(const UUserWidget* Context,const TMap<FString,int32>& Selection,bool Rework,bool Active);
BOUNDLESSIMMORTALITY_API FActionResult PrepareBreakthrough(const UUserWidget* Context,const TMap<FString,int32>& Selection,bool Rework,bool Active);
BOUNDLESSIMMORTALITY_API FActionResult CancelOrReconcile(const UUserWidget* Context,bool Active);
BOUNDLESSIMMORTALITY_API FActionResult CreateGuild(const UUserWidget* Context,const FString& Name,bool Active);
BOUNDLESSIMMORTALITY_API FActionResult RequestMembers(const UUserWidget* Context,int32 Cursor,bool Active);
}
