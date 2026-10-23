#include "LiveAnchorAbilityComponent.h"
#include "LiveMonster.h"
#include "LiveMonsterRegistrySubsystem.h"
#include "Engine/World.h"
bool ULiveAnchorAbilityComponent::TryPush()
{
    if(!GetOwner()||!GetWorld()||!GetOwner()->HasAuthority()||GetRemainingCooldown()>0)return false;
    const FVector Origin=GetOwner()->GetActorLocation();TArray<TWeakObjectPtr<ALiveMonster>> Targets;
    GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(Origin,Radius,Targets);
    for(auto Weak:Targets)if(auto* M=Weak.Get())M->ApplyPush(Origin,PushDistance,PushDuration);
    ReadyAt=GetWorld()->GetTimeSeconds()+FMath::Max(.05f,Cooldown);return true;
}
float ULiveAnchorAbilityComponent::GetRemainingCooldown()const{return GetWorld()?FMath::Max(0.0,ReadyAt-GetWorld()->GetTimeSeconds()):0;}
