#include "InkMenuDirector.h"
#include "LiveWeapons/LiveMonster.h"
#include "LiveWeapons/LiveWeaponGameMode.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "NavigationSystem.h"
#include "CollisionShape.h"

int32 AInkMenuDirector::GetGameplayEncounterCount()const
{
    int32 Count=0;for(const auto& Weak:GameplayEncounters)if(const auto* Monster=Weak.Get();Monster&&Monster->IsAlive())++Count;return Count;
}
void AInkMenuDirector::ClearGameplayEncounters()
{
    for(const auto& Weak:GameplayEncounters)if(auto* Monster=Weak.Get())Monster->Destroy();
    GameplayEncounters.Reset();EncounterElapsed=0;bEncounterWaveInitialized=false;
}
bool AInkMenuDirector::SpawnGameplayEncounter()
{
    auto* Pawn=GameplayPawn.Get();if(!Pawn||!EncounterMonsterClass||!GetWorld()->IsGameWorld()||!HasAuthority())return false;
    const float MinRadius=FMath::Max(200.f,EncounterMinimumRadius),MaxRadius=FMath::Max(MinRadius,EncounterMaximumRadius);
    const FVector Origin=Pawn->GetActorLocation();auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    const bool HasNav=Nav&&Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CultivationEncounterFloor),false,Pawn);
    FCollisionQueryParams Occupancy(SCENE_QUERY_STAT(CultivationEncounterClearance),false,Pawn);
    for(const auto& Weak:GameplayEncounters)if(auto* Monster=Weak.Get())Query.AddIgnoredActor(Monster);
    for(int32 Attempt=0;Attempt<32;++Attempt)
    {
        FVector Proposal;
        if(HasNav)
        {
            FNavLocation Reachable;if(!Nav->GetRandomReachablePointInRadius(Origin,MaxRadius,Reachable))continue;
            Proposal=Reachable.Location;if(FVector::DistSquared2D(Origin,Proposal)<FMath::Square(MinRadius))continue;
        }
        else
        {
            const float Angle=FMath::FRandRange(0.f,2*PI),Radius=FMath::FRandRange(MinRadius,MaxRadius);
            Proposal=Origin+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
        }
        // Monster placement alone uses scene queries. Sword trajectories stay mathematical.
        FHitResult Floor;
        if(!GetWorld()->LineTraceSingleByObjectType(Floor,Proposal+FVector(0,0,500),Proposal-FVector(0,0,1000),FCollisionObjectQueryParams(ECC_WorldStatic),Query)||Floor.ImpactNormal.Z<.65f)continue;
        if(FMath::Abs(Floor.ImpactPoint.Z-Origin.Z)>600)continue;
        const FVector Position=Floor.ImpactPoint+FVector(0,0,45);
        if(GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(39),Occupancy))continue;
        const FTransform Transform(FRotator::ZeroRotator,Position);
        auto* Monster=GetWorld()->SpawnActorDeferred<ALiveMonster>(EncounterMonsterClass,Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Monster)return false;
        Monster->MaxHealth=FMath::Max(1.f,EncounterMonsterHealth);Monster->AttackDamage=FMath::Max(0.f,EncounterMonsterDamage);Monster->MoveSpeed=FMath::Max(0.f,EncounterMonsterSpeed);Monster->FollowTarget=Pawn;Monster->FinishSpawning(Transform);
        GameplayEncounters.Add(Monster);return true;
    }
    return false;
}
void AInkMenuDirector::TickGameplayEncounters(float Delta)
{
    if(bMenuActive||!bGameplaySystemsReady)return;
    if(auto* Wuling=GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>();Wuling&&Wuling->IsEnabled())
    {ClearGameplayEncounters();return;}
    if(!bEnableGameplayEncounters){ClearGameplayEncounters();return;}
    // The standalone live prototype already has its own configured spawn lifecycle.
    if(GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>())return;
    if(!FMath::IsFinite(Delta)||Delta<=0)return;
    if(auto* C=GameplayCultivation.Get())if(auto* Health=C->GetAnchorHealth();Health&&Health->bDead)return;
    GameplayEncounters.RemoveAll([](const auto& Weak){return !Weak.IsValid()||!Weak->IsAlive();});
    const int32 Limit=FMath::Clamp(MaximumEncounterCount,0,32);
    if(!bEncounterWaveInitialized)
    {
        const int32 Desired=FMath::Clamp(InitialEncounterCount,0,Limit);
        for(int32 I=GetGameplayEncounterCount();I<Desired;++I)if(!SpawnGameplayEncounter())break;
        bEncounterWaveInitialized=true;EncounterElapsed=0;return;
    }
    EncounterElapsed+=Delta;if(EncounterElapsed<FMath::Max(1.f,EncounterSpawnInterval))return;EncounterElapsed=0;
    if(GetGameplayEncounterCount()<Limit)SpawnGameplayEncounter();
}
