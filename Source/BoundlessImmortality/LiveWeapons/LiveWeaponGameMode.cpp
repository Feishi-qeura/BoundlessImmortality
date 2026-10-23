#include "LiveWeaponGameMode.h"
#include "LiveWeaponSwarm.h"
#include "LiveMonster.h"
#include "LiveAudienceAdapterComponent.h"
#include "LiveAnchorController.h"
#include "LiveWeaponHUD.h"
#include "LiveWeaponPerformanceComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
ALiveWeaponGameMode::ALiveWeaponGameMode()
{
    PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=ACharacter::StaticClass();PlayerControllerClass=ALiveAnchorController::StaticClass();HUDClass=ALiveWeaponHUD::StaticClass();
    SwarmClass=ALiveWeaponSwarm::StaticClass();MonsterClass=ALiveMonster::StaticClass();Audience=CreateDefaultSubobject<ULiveAudienceAdapterComponent>(TEXT("Audience"));
    CreateDefaultSubobject<ULiveWeaponPerformanceComponent>(TEXT("DevelopmentPerformanceCapture"));
}
void ALiveWeaponGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);if(bStarted)return;
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC||!PC->GetPawn())return;
    Swarm=GetWorld()->SpawnActor<ALiveWeaponSwarm>(SwarmClass);if(!Swarm)return;Swarm->SetAnchor(PC->GetPawn());Audience->Bind(Swarm);bStarted=true;
    for(TActorIterator<ALiveMonster> It(GetWorld());It;++It)It->FollowTarget=PC->GetPawn();
    int32 MonsterCount=InitialMonsters,ViewerCount=0;
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(),TEXT("LiveMonsters="),MonsterCount);FParse::Value(FCommandLine::Get(),TEXT("LiveViewers="),ViewerCount);
#endif
    SpawnMonsters(MonsterCount,FParse::Param(FCommandLine::Get(),TEXT("LiveStress")));Audience->AddSimulatedAudience(ViewerCount);SetActorTickEnabled(false);
}
void ALiveWeaponGameMode::SpawnMonsters(int32 Count,bool bHighHealth)
{
    APawn* Pawn=GetWorld()->GetFirstPlayerController()?GetWorld()->GetFirstPlayerController()->GetPawn():nullptr;if(!Pawn)return;
    Count=FMath::Clamp(Count,0,100);
    for(int I=0;I<Count;++I)
    {
        const float Angle=I*2.399963f;const float Radius=900+I%5*110;
        FVector Position=Pawn->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
        FHitResult Floor;FCollisionQueryParams Params;Params.AddIgnoredActor(Pawn);
        if(GetWorld()->LineTraceSingleByObjectType(Floor,Position+FVector(0,0,200),Position-FVector(0,0,2000),FCollisionObjectQueryParams(ECC_WorldStatic),Params))Position.Z=Floor.ImpactPoint.Z+45;
        const FTransform Transform(FRotator::ZeroRotator,Position);
        auto* M=GetWorld()->SpawnActorDeferred<ALiveMonster>(MonsterClass,Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if(M){M->MaxHealth=bHighHealth?1000000.f:2000;M->FollowTarget=Pawn;M->FinishSpawning(Transform);}
    }
}
