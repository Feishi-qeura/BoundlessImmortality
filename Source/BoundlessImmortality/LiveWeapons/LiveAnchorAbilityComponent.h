#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiveAnchorAbilityComponent.generated.h"
UCLASS(ClassGroup=(LiveWeapons),meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API ULiveAnchorAbilityComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Radius=650;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float PushDistance=350;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float PushDuration=.35f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Cooldown=5;
    UFUNCTION(BlueprintCallable) bool TryPush();
    UFUNCTION(BlueprintPure) float GetRemainingCooldown()const;
private:
    double ReadyAt=-1;
};
