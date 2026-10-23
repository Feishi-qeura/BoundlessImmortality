#include "LiveMonsterRegistrySubsystem.h"
#include "LiveMonster.h"
void ULiveMonsterRegistrySubsystem::Register(ALiveMonster* M){Monsters.AddUnique(M);}
void ULiveMonsterRegistrySubsystem::Unregister(ALiveMonster* M){Monsters.Remove(M);}
void ULiveMonsterRegistrySubsystem::CollectInRange(const FVector& Origin,float Radius,TArray<TWeakObjectPtr<ALiveMonster>>& Out)const
{
    Out.Reset();const float RangeSquared=FMath::Square(FMath::Max(0.f,Radius));
    for(const auto& Weak:Monsters)if(auto* M=Weak.Get();M&&M->IsAlive()&&FVector::DistSquared2D(Origin,M->GetActorLocation())<=RangeSquared)Out.Add(Weak);
}
