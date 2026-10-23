#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LiveWeaponGameMode.generated.h"
class ALiveWeaponSwarm;
class ALiveMonster;
class ULiveAudienceAdapterComponent;
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API ALiveWeaponGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ALiveWeaponGameMode();
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALiveWeaponSwarm> SwarmClass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TSubclassOf<ALiveMonster> MonsterClass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 InitialMonsters=12;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<ALiveWeaponSwarm> Swarm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<ULiveAudienceAdapterComponent> Audience;
    UFUNCTION(BlueprintCallable) void SpawnMonsters(int32 Count,bool bHighHealth=false);
    virtual void Tick(float DeltaSeconds)override;
private:
    bool bStarted=false;
};
