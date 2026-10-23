#pragma once
#include "LiveWeapons/LiveWeaponSkill.h"
#include "LiveImpactRegressionSkill.generated.h"
/** Native fixture for callback reentrancy. No gameplay asset references this class. */
UCLASS(Transient,NotBlueprintable)
class ULiveImpactRegressionSkill:public ULiveWeaponSkill
{
    GENERATED_BODY()
public:
    static int32 Behavior,Executions;
    virtual bool CanExecute_Implementation(const FLiveSkillContext& Context)const override;
    virtual bool Execute_Implementation(const FLiveSkillContext& Context)override;
};
