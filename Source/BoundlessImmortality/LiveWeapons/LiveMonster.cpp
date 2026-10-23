#include "LiveMonster.h"
#include "LiveMonsterRegistrySubsystem.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Engine/GameInstance.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
ALiveMonster::ALiveMonster()
{
    PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.05f;
    Collision=CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));SetRootComponent(Collision);
    Collision->InitSphereRadius(40);Collision->SetCollisionProfileName(TEXT("Pawn"));
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));Body->SetupAttachment(Collision);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Body->SetStaticMesh(Cube.Object);Body->SetRelativeScale3D(FVector(.7,.7,.85));
}
void ALiveMonster::BeginPlay(){Super::BeginPlay();Health=FMath::Max(1.f,MaxHealth);RefreshEnemyModifier();GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->Register(this);}
void ALiveMonster::RefreshEnemyModifier()
{
    const float Desired=GetWorld()->GetGameInstance()?float(GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->GetEnemyAttributeMultiplier()):1.f;
    if(!FMath::IsNearlyEqual(Desired,EnemyAttributeMultiplier)){const float Fraction=GetHealthFraction();EnemyAttributeMultiplier=FMath::Max(.01f,Desired);Health=MaxHealth*EnemyAttributeMultiplier*Fraction;}
}
void ALiveMonster::EndPlay(const EEndPlayReason::Type Reason){if(auto* Registry=GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>())Registry->Unregister(this);Super::EndPlay(Reason);}
bool ALiveMonster::ApplyWeaponDamage(float Amount,const FString& Id)
{
    if(!IsAlive()||!FMath::IsFinite(Amount)||Amount<=0)return false;
    RefreshEnemyModifier();Amount*=100/(100+FMath::Max(0.f,Defense)*EnemyAttributeMultiplier);
    const float Applied=FMath::Min(Health,Amount);Health=FMath::Max(0.f,Health-Amount);
    const bool Lethal=Health<=0;
    if(GetWorld()->GetGameInstance())GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>()->RecordActualDamage(Id,Applied,Lethal);
    OnDamaged(Applied,Id);
    if(Lethal){Health=0;GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->Unregister(this);OnDeath(Id);SetActorEnableCollision(false);SetActorHiddenInGame(true);SetActorTickEnabled(false);SetLifeSpan(.1f);}
    return true;
}
void ALiveMonster::ApplyPush(const FVector& Origin,float Distance,float Duration)
{
    if(!IsAlive()||!FMath::IsFinite(Distance)||!FMath::IsFinite(Duration)||Distance<=0||Duration<=0)return;
    FVector Direction=(GetActorLocation()-Origin).GetSafeNormal2D();if(Direction.IsNearlyZero())Direction=FVector::ForwardVector;
    PushVelocity=Direction*(Distance/FMath::Max(.05f,Duration));PushRemaining=FMath::Max(.05f,Duration);
}
void ALiveMonster::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);if(!IsAlive()||!HasAuthority()||!FMath::IsFinite(DeltaSeconds)||DeltaSeconds<=0)return;RefreshEnemyModifier();AttackCooldown=FMath::Max(0.f,AttackCooldown-DeltaSeconds);
    if(PushRemaining>0){const float Step=FMath::Min(DeltaSeconds,PushRemaining);AddActorWorldOffset(PushVelocity*Step,true);PushRemaining-=Step;return;}
    if(IsValid(FollowTarget))
    {
        const FVector Delta=FollowTarget->GetActorLocation()-GetActorLocation();
        if(Delta.SizeSquared2D()>FMath::Square(FMath::Max(1.f,AttackRange)))AddActorWorldOffset(Delta.GetSafeNormal2D()*FMath::Max(0.f,MoveSpeed)*EnemyAttributeMultiplier*DeltaSeconds,true);
        else if(AttackCooldown<=0)
        {
            AttackCooldown=FMath::Max(.1f,AttackInterval);
            if(auto* HealthComponent=FollowTarget->FindComponentByClass<UCultivationHealthComponent>())HealthComponent->ApplyIncomingDamage(AttackDamage*EnemyAttributeMultiplier);
        }
    }
}
