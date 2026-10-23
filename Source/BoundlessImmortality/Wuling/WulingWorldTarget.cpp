#include "WulingWorldTarget.h"
#include "WulingPrototypeSubsystem.h"
#include "GameFramework/Pawn.h"
bool AWulingWorldTarget::ApplyWeaponDamage(float Amount,const FString& ParticipantId)
{
    return !bMechanicLocked&&IsAlive()&&Authority.IsValid()&&Authority->SubmitStrike(ParticipantId,TargetId);
}
void AWulingWorldTarget::Mirror(float Current,float Maximum)
{MaxHealth=Maximum;Health=Current;EnemyAttributeMultiplier=1;}
void AWulingWorldTarget::Tick(float Delta)
{
    // Presentation movement only. The fixed prototype pressure/health budget is server owned.
    AActor::Tick(Delta);
    if(FollowTarget&&IsAlive())
    {
        const FVector Offset=FollowTarget->GetActorLocation()-GetActorLocation();
        if(Offset.SizeSquared2D()>FMath::Square(240.f))AddActorWorldOffset(Offset.GetSafeNormal2D()*MoveSpeed*FMath::Max(0.f,Delta),true);
    }
}
