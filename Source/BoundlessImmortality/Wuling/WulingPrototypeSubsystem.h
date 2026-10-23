#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Interfaces/IHttpRequest.h"
#include "LiveInteractionSubsystem.h"
#include "WulingTribulationPresentation.h"
#include "WulingEffectPresentation.h"
#include "WulingRewardReceiptQueue.h"
#include "WulingPrototypeSubsystem.generated.h"
class FJsonObject;
class AWulingWorldTarget;

/** Explicit local simulation. Separate authority, saves and rule version from legacy gifts. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UWulingPrototypeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;
    UFUNCTION(BlueprintPure) bool IsEnabled() const{return bEnabled;}
    UFUNCTION(BlueprintPure) bool IsConnected() const{return bConnected;}
    UFUNCTION(BlueprintPure) bool IsBusy() const;
    UFUNCTION(BlueprintPure) FString GetStatus() const{return Status;}
    UFUNCTION(BlueprintPure) FString GetSnapshotJson() const;
    /** Dequeues a confirmed post-baseline receipt. This never grants or spends. */
    UFUNCTION(BlueprintCallable,Category="武灵|奖励") FString TakeNextRewardReceiptJson();
    UFUNCTION(BlueprintCallable) bool CreateGuild(const FString& Name);
    UFUNCTION(BlueprintCallable) bool StartWorldRound(const FString& Preset=TEXT("ENC_SOLO"),const FString& Route=TEXT("guard"),const FString& RegionId=TEXT("qingyun"));
    UFUNCTION(BlueprintCallable) bool InterruptWorldRound();
    UFUNCTION(BlueprintCallable,Category="武灵|世界") bool RespondToRegionMechanic();
    UFUNCTION(BlueprintCallable) bool Breakthrough();
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool Craft(const FString& RecipeId);
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool PrepareBreakthrough(const FString& SelectionJson);
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool CancelBreakthrough();
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool SignIn();
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool QueueRace(const FString& NodeId);
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool StartLaunchEvent();
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool SubmitPrototypeCommand(const FString& Operation,const FString& ArgsJson);
    UFUNCTION(BlueprintCallable,Category="武灵|功能") bool RequestCatalog(const FString& Kind,int32 Offset=0,bool OwnedOnly=false);
    UFUNCTION(BlueprintCallable) void SetGameplayActive(bool Active);
    UFUNCTION(BlueprintCallable) void RequestMemberPage(int32 Cursor);
    UFUNCTION(BlueprintPure) bool IsWorldActive() const;
    bool SubmitStrike(const FString& Participant,const FString& Target);
    const TSharedPtr<FJsonObject>& Snapshot() const{return State;}
private:
    struct FCommand {FString Actor;TSharedPtr<FJsonObject> Data;};
    struct FParticipant {FString Token,DisplayName;bool bPresent=true;};
    TArray<FCommand> Commands;
    TMap<FString,FParticipant> Participants;
    TMap<FString,double> LastStrike;
    TMap<FString,TWeakObjectPtr<AWulingWorldTarget>> Targets;
    FWulingTribulationPresentation TribulationPresentation;
    FWulingEffectPresentation EffectPresentation;
    FWulingRewardReceiptQueue RewardReceipts;
    TSharedPtr<FJsonObject> State;
    FHttpRequestPtr Request;
    FString Endpoint=TEXT("http://127.0.0.1:18770"),App=TEXT("wuling-p01"),Room=TEXT("prototype-room"),User=TEXT("prototype-anchor"),Token;
    FString Status=TEXT("武灵原型未启用"),Outbox;
    int64 Version=0;
    int64 Generation=0;
    FString RulesHash;
    int32 MemberCursor=0;
    double Poll=0,RetryAt=0;
    bool bEnabled=false,bConnected=false,bGameplayActive=false,bEnding=false,bNeedMemberPage=false;
    bool Enqueue(const FString& Operation,const TSharedRef<FJsonObject>& Args,const FString& Actor=FString(),const FString& SourceEvent=FString());
    void Post(const FString& Route,const TSharedRef<FJsonObject>& Body,const FString& Actor,bool Session=false);
    void ReconcileTargets();
    bool SaveOutbox() const;
    void LoadOutbox();
    UFUNCTION() void HandleComment(const FLiveComment& Comment);
    UFUNCTION() void HandlePresence(const FLivePresence& Presence);
    UFUNCTION() void HandleSessionChanged();
};
