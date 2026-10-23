#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiveWeaponPerformanceComponent.generated.h"
UCLASS()
class BOUNDLESSIMMORTALITY_API ULiveWeaponPerformanceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULiveWeaponPerformanceComponent();
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* ThisTick)override;
protected:
    virtual void BeginPlay()override;
private:
    double Started=0,Previous=0,NextMemory=0,NextSwitch=45;
    int32 Seconds=180,SwitchBatch=0;
    uint64 MemoryBytes=0;
    FString Output,Rows;
    bool bCaptured=false;
};
