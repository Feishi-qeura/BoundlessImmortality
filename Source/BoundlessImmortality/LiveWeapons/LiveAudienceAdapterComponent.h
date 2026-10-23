#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiveInteractionSubsystem.h"
#include "Cultivation/CultivationTypes.h"
#include "LiveAudienceAdapterComponent.generated.h"
class ALiveWeaponSwarm;
class UCultivationSubsystem;
class UCultivationPrototypeClient;
struct FCultivationPendingGift {FLiveGift Event;double ReceivedUtc=0;bool bApplied=false;};
USTRUCT(BlueprintType)
struct FLiveGiftUnlockRule
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString GiftName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FName> SkillIds;
};
struct FLivePendingAck
{
    FLiveSession Session;
    FString MessageId,Type,RequestId;
    double RetryAt=0;
};
UCLASS(ClassGroup=(LiveWeapons),meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API ULiveAudienceAdapterComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULiveAudienceAdapterComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FLiveGiftUnlockRule> GiftUnlocks;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bEnableSessionGiftUnlocks=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修仙互动") bool bEnableCultivationGifts=true;
    void Bind(ALiveWeaponSwarm* InSwarm,ULiveInteractionSubsystem* InHost=nullptr);
    void Unbind();
    static int32 ParseCommand(const FString& Text);
    UFUNCTION(BlueprintCallable) bool SimulateComment(const FString& Id,const FString& Name,const FString& Text);
    UFUNCTION(BlueprintCallable) void AddSimulatedAudience(int32 Count);
    UFUNCTION(BlueprintCallable) bool EnableDevelopmentSimulation();
    UFUNCTION(BlueprintCallable) int32 AppendSimulatedAudience(int32 Count);
    UFUNCTION(BlueprintCallable) bool ClearSimulatedAudience();
    UTexture2D* GetAvatar(const FString& Id);
    void RequestVisibleAvatar(const FString& Id);
    bool GetAvatarUV(const FString& Id,FVector2D& UV)const;
    UTexture2D* GetAvatarAtlas()const{return AvatarAtlas;}
    int32 GetReadyAvatarCount()const{return AvatarTiles.Num();}
    int32 GetPendingAcknowledgementCount()const{return PendingAcks.Num();}
    void NotifyPresented();
    FString GetStatus()const;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* ThisTick)override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    friend struct FLiveAudienceAdapterTestAccess;
    UPROPERTY(Transient) TObjectPtr<ULiveInteractionSubsystem> Host;
    UPROPERTY(Transient) TObjectPtr<ALiveWeaponSwarm> Swarm;
    UPROPERTY(Transient) TObjectPtr<UCultivationSubsystem> Cultivation;
    UPROPERTY(Transient) TObjectPtr<UCultivationPrototypeClient> SharedClient;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UTexture2D>> Avatars;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> AvatarAtlas;
    TMap<FString,int32> AvatarTiles;
    TArray<int32> FreeAvatarTiles;
    int32 NextAvatarTile=0;
    double AvatarBudgetTime=-1;
    int32 AvatarBudget=0;
    TMap<FString,FString> RawIds;
    TMap<FString,double> AvatarLastUsed,AvatarRetryAt,AvatarPending;
    TArray<FLivePendingAck> PendingAcks;
    TArray<FCultivationPendingGift> PendingCultivationGifts;
    FString RoomKey;
    FString InteractionFeedback;
    bool bPresented=false;
    FDelegateHandle ReplyHandle;
    UFUNCTION() void HandleComment(const FLiveComment& Event);
    UFUNCTION() void HandleGift(const FLiveGift& Event);
    UFUNCTION() void HandlePresence(const FLivePresence& Event);
    UFUNCTION() void HandleLike(const FLiveLike& Event);
    UFUNCTION() void HandleFollow(const FLiveFollow& Event);
    UFUNCTION() void HandleTeam(const FLiveTeamSelection& Event);
    UFUNCTION() void HandleSessionChanged();
    UFUNCTION() void HandleAnchorProfileChanged();
    UFUNCTION() void HandleAvatar(const FString& RawId,UTexture2D* Texture);
    void QueueAck(const FLiveSession& Session,const FString& Id,const FString& Type);
    void OnCommandReply(const FLiveCommandReply& Reply);
    void InvalidateAvatar(const FString& Id);
    bool JoinCultivator(const FString& RawId,const FString& Nickname,const FString& AvatarUrl);
    void ProcessCultivationGifts();
};
