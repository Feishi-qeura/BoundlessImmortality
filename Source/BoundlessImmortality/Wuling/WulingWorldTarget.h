#pragma once
#include "CoreMinimal.h"
#include "LiveWeapons/LiveMonster.h"
#include "WulingWorldTarget.generated.h"
class UWulingPrototypeSubsystem;

/** Render-only mirror of an authority target. Legacy gifts cannot write its health or drops. */
UCLASS()
class BOUNDLESSIMMORTALITY_API AWulingWorldTarget : public ALiveMonster
{
    GENERATED_BODY()
public:
    FString TargetId;
    bool bMechanicLocked=false;
    TWeakObjectPtr<UWulingPrototypeSubsystem> Authority;
    virtual bool ApplyWeaponDamage(float Amount,const FString& ParticipantId) override;
    virtual void Tick(float DeltaSeconds) override;
    void Mirror(float Current,float Maximum);
};
