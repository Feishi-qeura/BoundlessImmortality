#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CultivationTypes.h"
#include "CultivationSubsystem.generated.h"
class APawn;
class UCultivationHealthComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCultivationAnnouncement,const FString&,Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCultivationStateChanged);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCultivationActualDamageEvent,const FString&,double,double);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCultivationRealmAdvancedEvent,const FString&,int32,int32);

/** Local prototype ledger. External shared authority imports versioned records and room potion leases. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API UCultivationSubsystem : public UGameInstanceSubsystem,public FTickableGameObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修仙规则") FCultivationRuleConfig Rules;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修仙规则|存储") bool bPersistLocalRecords=true;
    UPROPERTY(BlueprintAssignable) FCultivationAnnouncement OnAnnouncement;
    UPROPERTY(BlueprintAssignable) FCultivationStateChanged OnStateChanged;
    FCultivationActualDamageEvent OnActualDamage;
    FCultivationRealmAdvancedEvent OnRealmAdvanced;
    UFUNCTION(BlueprintCallable) void SetAnchor(APawn* Pawn,const FString& Id=TEXT("anchor"),const FString& Name=TEXT("主播"));
    UFUNCTION(BlueprintPure) FString GetAnchorId()const{return AnchorId;}
    UFUNCTION(BlueprintCallable) void SetRoomContext(const FString& RoomId,bool bDevelopment=true);
    UFUNCTION(BlueprintCallable) bool RegisterParticipant(const FString& Id,const FString& Name,const FString& AvatarUrl,bool bIsAnchor=false);
    UFUNCTION(BlueprintCallable) void LeaveParticipant(const FString& Id);
    UFUNCTION(BlueprintCallable) void LeaveRoomParticipants();
    UFUNCTION(BlueprintCallable) bool ProcessGift(const FCultivationGiftEvent& Event);
    UFUNCTION(BlueprintCallable) bool AddCultivation(const FString& Id,double Amount);
    UFUNCTION(BlueprintCallable) bool TryAnchorBreakthrough();
    /** New-rule growth mirrors only; legacy gift fields and receipts remain in this existing ledger. */
    void SetWulingAuthorityEnabled(bool Enabled){bWulingAuthority=Enabled;if(Enabled)bPersistLocalRecords=false;}
    bool IsWulingAuthorityEnabled()const{return bWulingAuthority;}
    bool ApplyWulingGrowth(const FString& Id,int32 Realm,double Qi,double Threshold,int32 MaterialCount,int32 MaterialCost,double Rate);
    void ApplyWulingAttributes(const FString& Id,const FCultivationAttributes& Attributes);
    void ApplyWulingHonorTitle(const FString& Id,const FString& Caption){if(bWulingAuthority)WulingHonorTitles.Add(Id,Caption.Left(32));}
    UFUNCTION(BlueprintCallable) void GrantMaterial(const FString& Id,FName MaterialId,int32 Count);
    UFUNCTION(BlueprintCallable) void RecordActualDamage(const FString& Id,double Amount,bool bKilled);
    UFUNCTION(BlueprintCallable) void GrantOfflineCultivation(const FString& Id,double Seconds);
    UFUNCTION(BlueprintPure) FCultivationParticipantSnapshot GetParticipantSnapshot(const FString& Id)const;
    UFUNCTION(BlueprintPure) TArray<FCultivationParticipantSnapshot> GetParticipants()const;
    UFUNCTION(BlueprintPure) double GetEnemyAttributeMultiplier()const;
    UFUNCTION(BlueprintPure) double GetAnchorImmunityChance()const;
    UFUNCTION(BlueprintCallable) double ConsumeStarHealing(double HealthFraction);
    UFUNCTION(BlueprintPure) FText GetRealmName(int32 Index)const;
    UFUNCTION(BlueprintPure) bool IsMeditating()const;
    UFUNCTION(BlueprintCallable) void SetRoomPotionSources(const TArray<FCultivationPotionSource>& Sources);
    UFUNCTION(BlueprintCallable) FString ExportRecords()const;
    UFUNCTION(BlueprintCallable) bool ImportRecords(const FString& Json,bool bReplace=false);
    UFUNCTION(BlueprintCallable) bool ImportAuthoritativeRecords(const FString& Json);
    UFUNCTION(BlueprintCallable) bool SaveRecords();
    UFUNCTION(BlueprintPure) bool IsDevelopmentContext()const{return bDevelopmentContext;}
    UFUNCTION(BlueprintPure) FString GetRoomId()const{return CurrentRoomId;}
    UFUNCTION(BlueprintPure) UCultivationHealthComponent* GetAnchorHealth()const;
    UFUNCTION(BlueprintCallable) bool EquipTitle(const FString& Id,FName Title);
    virtual void Initialize(FSubsystemCollectionBase& Collection)override;
    virtual void Deinitialize()override;
    virtual void Tick(float DeltaTime)override;
    virtual TStatId GetStatId()const override;
    virtual bool IsTickable()const override;
    virtual UWorld* GetTickableGameObjectWorld()const override;
    static double UtcNow();
#if WITH_DEV_AUTOMATION_TESTS
    void SetClockForTests(TFunction<double()> Clock){UtcProvider=MoveTemp(Clock);bAuraCacheDirty=true;}
#endif
private:
    TFunction<double()> UtcProvider;
    double CurrentUtc()const{return UtcProvider?UtcProvider():UtcNow();}
    bool ImportInternal(const FString& Json,bool bReplace,bool bAuthoritative);
    UPROPERTY(Transient) TMap<FString,FCultivationParticipantRecord> Records;
    TSet<FString> GiftReceipts;
    TMap<FString,TArray<TPair<double,double>>> DamageWindow;
    TMap<FString,double> RealmAdvanceTimes;
    TMap<FString,FVector4d> WulingGrowthMetrics;
    TMap<FString,FCultivationAttributes> WulingAttributes;
    TMap<FString,FString> WulingHonorTitles;
    bool bWulingAuthority=false;
    void NotifyRealmAdvanced(const FString& Id,int32 Previous,int32 Current);
    TArray<FCultivationPotionSource> RoomPotionSources;
    TWeakObjectPtr<APawn> AnchorPawn;
    FString AnchorId=TEXT("anchor"),CurrentRoomId=TEXT("prototype");
    bool bDevelopmentContext=true,bInitialized=false,bDirty=false,bAuthoritativePotionSources=false;
    double SaveElapsed=0,NotifyElapsed=0;
    mutable bool bAuraCacheDirty=true,bCachedJet=false,bCachedPotion=false,bCachedEnemyDebuff=false,bCachedImmunity=false;
    mutable double AuraCacheUtc=0;
    mutable FCultivationAttributes CachedSoulTotal;
    mutable TSet<FString> CachedSoulHolders;
    void RefreshAuraCache(double Now)const;
    FCultivationAttributes ResolveAttributes(const FCultivationParticipantRecord& Record,double Now)const;
    double CultivationRate(const FCultivationParticipantRecord& Record,double Now)const;
    bool HasRoomPotion(double Now)const;
    FString StoragePath()const;
    void LoadRecords();
};
