#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiveMonster.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class APawn;
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API ALiveMonster : public AActor
{
    GENERATED_BODY()
public:
    ALiveMonster();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1")) float MaxHealth=100;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Health=100;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0")) float MoveSpeed=85;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat",meta=(ClampMin="0")) float AttackDamage=8;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat",meta=(ClampMin="0")) float Defense=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat",meta=(ClampMin="0.1")) float AttackInterval=1.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat",meta=(ClampMin="1")) float AttackRange=115;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Combat") float EnemyAttributeMultiplier=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<APawn> FollowTarget;
    UFUNCTION(BlueprintPure) bool IsAlive()const{return Health>0 && !IsActorBeingDestroyed();}
    UFUNCTION(BlueprintPure) float GetHealthFraction()const{return MaxHealth>0?FMath::Clamp(Health/(MaxHealth*FMath::Max(.01f,EnemyAttributeMultiplier)),0.f,1.f):0.f;}
    UFUNCTION(BlueprintCallable) virtual bool ApplyWeaponDamage(float Amount,const FString& ParticipantId);
    UFUNCTION(BlueprintCallable) void ApplyPush(const FVector& Origin,float Distance,float Duration);
    UFUNCTION(BlueprintImplementableEvent) void OnDamaged(float Amount,const FString& ParticipantId);
    UFUNCTION(BlueprintImplementableEvent) void OnDeath(const FString& ParticipantId);
    virtual void Tick(float DeltaSeconds)override;
protected:
    virtual void BeginPlay()override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    FVector PushVelocity=FVector::ZeroVector;
    float PushRemaining=0;
    float AttackCooldown=0;
    void RefreshEnemyModifier();
};
