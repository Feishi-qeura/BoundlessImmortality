#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Interfaces/IHttpRequest.h"
#include "Frontend/InkFrontendTypes.h"
#include "CultivationTypes.h"
#include "CultivationPrototypeClient.generated.h"
class UCultivationSubsystem;
struct FLiveSession;
class FJsonObject;
class FJsonValue;

/** Loopback-only, explicitly enabled development service. Contains no platform credentials. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UCultivationPrototypeClient : public UGameInstanceSubsystem,public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection)override;
    virtual void Deinitialize()override;
    virtual void Tick(float DeltaTime)override;
    virtual TStatId GetStatId()const override;
    virtual bool IsTickable()const override;
    virtual UWorld* GetTickableGameObjectWorld()const override;
    void BeginRoom(const FLiveSession& Session);
    void SuspendRoom();
    void TrackParticipant(const FString& Id,bool bAllowReclaim=true);
    void LeaveParticipant(const FString& Id);
    bool CanProcessGift(const FString& Id,const FString& MessageId)const;
    bool RegisterGiftReceipt(const FCultivationGiftEvent& Gift);
    bool QueueGiftForRecovery(const FCultivationGiftEvent& Gift);
    bool IsGiftDurable(const FString& Id,const FString& MessageId)const;
    bool IsParticipantRevoked(const FString& Id)const;
    UFUNCTION(BlueprintPure) bool IsEnabled()const{return bEnabled;}
    UFUNCTION(BlueprintPure) bool IsConnected()const{return bEnabled&&bConnected;}
    UFUNCTION(BlueprintPure) FString GetStatus()const{return Status;}
    UFUNCTION(BlueprintPure) TArray<FInkLeaderboardEntry> GetLeaderboardEntries()const{return bConnected?Leaderboards:TArray<FInkLeaderboardEntry>();}
private:
    struct FOwnership { FString Lease;int64 Version=0;double JoinedUtc=0;bool bReady=false,bRevoked=false,bPresent=false; };
    UPROPERTY(Transient) TObjectPtr<UCultivationSubsystem> Cultivation;
    UPROPERTY(Transient) TArray<FInkLeaderboardEntry> Leaderboards;
    TMap<FString,FOwnership> Ownership;
    TMap<FString,TArray<FCultivationGiftEvent>> PendingGifts;
    TSet<FString> DurableReceipts,ReadyGifts,Joins;
    TMap<FString,int64> Leaves;
    int64 PresenceIntentSequence=0;
    // Keep across connection resets: a repeated authority import must not repeat
    // the entry notice for this identity in the same room.
    TSet<FString> PotionRestorationNotices;
    TArray<TSharedPtr<FJsonValue>> Damage;
    TSharedPtr<FJsonObject> RetryPayload;
    FHttpRequestPtr Request;
    FDelegateHandle DamageHandle;
    FString Endpoint=TEXT("http://127.0.0.1:18768"),ClientId,Token,RoomId,AppId,Namespace,Status=TEXT("共享原型未启用");
    int64 Generation=0;
    int32 SyncCursor=0;
    double Elapsed=0;
    bool bEnabled=false,bConnected=false,bSuspended=false;
    void Send(const FString& Route,const TSharedRef<FJsonObject>& Payload,bool bSession);
    void Receive(const TSharedPtr<FJsonObject>& Data,bool bSession,const TSharedPtr<FJsonObject>& Submitted);
    TSharedRef<FJsonObject> BuildSync();
    void RecordDamage(const FString& Id,double Amount,double Utc);
    FString OutboxPath()const;
    bool SaveOutbox()const;
    void LoadOutbox();
    void LoadParticipantOutboxes(const FString& Id);
    void LoadOutboxData(const TSharedPtr<FJsonObject>& Data,const FString& OnlyId=FString());
    void ClearConfirmedOutboxes()const;
    FString OutboxDirectory()const;
    FString OutboxDirectoryOverride;
    friend class FCultivationOutboxTransferTest;
};
