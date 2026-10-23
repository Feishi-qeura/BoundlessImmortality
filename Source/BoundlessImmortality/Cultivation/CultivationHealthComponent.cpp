#include "CultivationHealthComponent.h"
#include "CultivationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UCultivationHealthComponent::UCultivationHealthComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.1f;}
UCultivationSubsystem* UCultivationHealthComponent::Cultivation()const{return GetWorld()&&GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;}
double UCultivationHealthComponent::ApplyIncomingDamage(double Amount)
{
    if(bDead||!FMath::IsFinite(Amount)||Amount<=0)return 0;
    auto* C=Cultivation();if(C&&C->IsWulingAuthorityEnabled())return 0;double Defense=0;
    if(C)
    {
        const auto Snapshot=C->GetParticipantSnapshot(C->GetAnchorId());if(!Snapshot.bPresent||C->GetAnchorHealth()!=this)return 0;
        const auto A=Snapshot.Attributes;
        const double DesiredMax=FMath::Max(1.0,A.MaxHealth);
        if(!FMath::IsNearlyEqual(DesiredMax,MaxHealth)){const double Fraction=GetHealthFraction();MaxHealth=DesiredMax;Health=MaxHealth*Fraction;}
        if(Random.FRand()<C->GetAnchorImmunityChance()||Random.FRand()<A.EvasionChance)return 0;
        Defense=FMath::Max(0.0,A.Defense);
    }
    const double Damage=Amount*(100/(100+Defense)),Applied=FMath::Min(Health,Damage);Health=FMath::Max(0.0,Health-Damage);
    if(Applied>0)OnDamageApplied.Broadcast(Applied,MaxHealth);
    if(Health<=0){bDead=true;OnDied.Broadcast();}
    else if(C&&Applied>0){const double Fraction=C->ConsumeStarHealing(GetHealthFraction());if(Fraction>0)Heal(MaxHealth*Fraction);}
    OnHealthChanged.Broadcast(Health,MaxHealth);return Applied;
}
double UCultivationHealthComponent::Heal(double Amount)
{
    if(auto* C=Cultivation();C&&C->IsWulingAuthorityEnabled())return 0;
    if(bDead||!FMath::IsFinite(Amount)||Amount<=0)return 0;const double Before=Health;Health=FMath::Min(MaxHealth,Health+Amount);if(Health!=Before)OnHealthChanged.Broadcast(Health,MaxHealth);return Health-Before;
}
void UCultivationHealthComponent::Revive(){bDead=false;Health=MaxHealth;OnHealthChanged.Broadcast(Health,MaxHealth);}
void UCultivationHealthComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);auto* C=Cultivation();if(!C||Delta<=0||C->IsWulingAuthorityEnabled())return;
    const auto S=C->GetParticipantSnapshot(C->GetAnchorId());const double NewMax=FMath::Max(1.0,S.Attributes.MaxHealth);
    if(!FMath::IsNearlyEqual(NewMax,MaxHealth)){const double Fraction=GetHealthFraction();MaxHealth=NewMax;Health=bDead?0:MaxHealth*Fraction;OnHealthChanged.Broadcast(Health,MaxHealth);}
    if(!bDead&&S.bPresent)Heal(FMath::Max(0.0,S.Attributes.HealthRegeneration)*Delta);
}
void UCultivationHealthComponent::ApplyWulingHealth(double Current,double Maximum)
{
    auto* C=Cultivation();if(!C||!C->IsWulingAuthorityEnabled()||!FMath::IsFinite(Current)||!FMath::IsFinite(Maximum)||Maximum<=0)return;
    const double Before=Health;MaxHealth=Maximum;Health=FMath::Clamp(Current,0.0,MaxHealth);bDead=Health<=0;
    if(Before>Health)OnDamageApplied.Broadcast(Before-Health,MaxHealth);OnHealthChanged.Broadcast(Health,MaxHealth);
}
