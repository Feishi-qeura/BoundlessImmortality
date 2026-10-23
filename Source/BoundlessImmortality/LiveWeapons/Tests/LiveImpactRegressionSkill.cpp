#include "LiveImpactRegressionSkill.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveMonster.h"
#include "Engine/World.h"
#include "EngineUtils.h"
int32 ULiveImpactRegressionSkill::Behavior=0;
int32 ULiveImpactRegressionSkill::Executions=0;
bool ULiveImpactRegressionSkill::CanExecute_Implementation(const FLiveSkillContext& Context)const
{
#if WITH_DEV_AUTOMATION_TESTS
    if(Behavior==5&&Context.ParticipantId==TEXT("b"))
        for(TActorIterator<ALiveWeaponSwarm> It(Context.Target->GetWorld());It;++It){It->SwitchParticipantWeapon(TEXT("a"),ELiveWeaponType::Bow);break;}
#endif
    return true;
}
bool ULiveImpactRegressionSkill::Execute_Implementation(const FLiveSkillContext& Context)
{
#if WITH_DEV_AUTOMATION_TESTS
    ++Executions;ALiveWeaponSwarm* Swarm=nullptr;
    for(TActorIterator<ALiveWeaponSwarm> It(Context.Target->GetWorld());It;++It){Swarm=*It;break;}
    if(!Swarm)return false;
    if(Behavior==1)return false;
    if(Behavior==2)for(int I=0;I<256;++I)Swarm->AddParticipant({FString::Printf(TEXT("callback-%d"),I),TEXT("x"),TEXT("")});
    if(Behavior==3){Swarm->ResetParticipants();return false;}
    if(Behavior==4){Swarm->SwitchParticipantWeapon(Context.ParticipantId,ELiveWeaponType::Axe);return false;}
    return Context.Target->ApplyWeaponDamage(7,Context.ParticipantId);
#else
    return false;
#endif
}
