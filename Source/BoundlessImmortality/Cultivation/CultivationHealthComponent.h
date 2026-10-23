#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CultivationHealthComponent.generated.h"
class UCultivationSubsystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCultivationHealthChanged,double,Health,double,MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCultivationAnchorDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCultivationDamageApplied,double,Applied,double,Maximum);
UCLASS(ClassGroup=(Cultivation),meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API UCultivationHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCultivationHealthComponent();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) double Health=100;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) double MaxHealth=100;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bDead=false;
    UPROPERTY(BlueprintAssignable) FCultivationHealthChanged OnHealthChanged;
    UPROPERTY(BlueprintAssignable) FCultivationAnchorDied OnDied;
    UPROPERTY(BlueprintAssignable) FCultivationDamageApplied OnDamageApplied;
    UFUNCTION(BlueprintCallable) double ApplyIncomingDamage(double Amount);
    UFUNCTION(BlueprintCallable) double Heal(double Amount);
    UFUNCTION(BlueprintCallable) void Revive();
    void ApplyWulingHealth(double Current,double Maximum);
    UFUNCTION(BlueprintPure) double GetHealthFraction()const{return MaxHealth>0?FMath::Clamp(Health/MaxHealth,0.0,1.0):0;}
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)override;
private:
    UCultivationSubsystem* Cultivation()const;
    FRandomStream Random{68147};
};
