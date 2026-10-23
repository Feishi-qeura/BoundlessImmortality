#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LiveMonsterRegistrySubsystem.generated.h"
class ALiveMonster;
UCLASS()
class BOUNDLESSIMMORTALITY_API ULiveMonsterRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void Register(ALiveMonster* Monster);
    void Unregister(ALiveMonster* Monster);
    void CollectInRange(const FVector& Origin,float Radius,TArray<TWeakObjectPtr<ALiveMonster>>& Out)const;
private:
    TArray<TWeakObjectPtr<ALiveMonster>> Monsters;
};
