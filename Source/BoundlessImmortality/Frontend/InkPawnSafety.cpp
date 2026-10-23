#include "InkPawnSafety.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"

bool InkPawnSafety::FindSupportedTransform(APawn* Pawn,const FTransform& Candidate,FTransform& Out,const AActor* Ignore)
{
    if(!IsValid(Pawn)||!Pawn->GetWorld()||!Candidate.IsValid())return false;
    auto* Character=Cast<ACharacter>(Pawn);if(!Character)return false;
    const auto* Capsule=Character->GetCapsuleComponent();const auto* Move=Character->GetCharacterMovement();
    if(!Capsule||!Move)return false;
    const FVector Scale=Candidate.GetScale3D().GetAbs();
    const float Radius=Capsule->GetUnscaledCapsuleRadius()*FMath::Max(Scale.X,Scale.Y);
    const float Half=FMath::Max(double(Radius),Capsule->GetUnscaledCapsuleHalfHeight()*Scale.Z);
    if(!FMath::IsFinite(Half)||!FMath::IsFinite(Radius)||Radius<1||Half>100000)return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(InkSafeReentry),false,Pawn);if(Ignore)Query.AddIgnoredActor(Ignore);
    FHitResult Hit;const FVector Position=Candidate.GetLocation();
    if(!Pawn->GetWorld()->LineTraceSingleByChannel(Hit,Position+FVector(0,0,12),Position-FVector(0,0,Half+FMath::Max(64.f,Move->MaxStepHeight+8)),ECC_Pawn,Query))return false;
    if(Hit.bStartPenetrating||Hit.ImpactNormal.Z<Move->GetWalkableFloorZ()||Cast<APawn>(Hit.GetActor()))return false;
    // The bottom hemisphere touches a slope away from the capsule centre line.
    const float SlopeLift=Radius*(1.f/FMath::Max(.01f,Hit.ImpactNormal.Z)-1.f);
    Out=Candidate;Out.SetLocation(Hit.ImpactPoint+FVector(0,0,Half+SlopeLift+2));
    return !Pawn->GetWorld()->OverlapBlockingTestByChannel(Out.GetLocation(),Out.GetRotation(),ECC_Pawn,FCollisionShape::MakeCapsule(FMath::Max(1.f,Radius-1),FMath::Max(Radius,Half-2)),Query);
}
bool InkPawnSafety::ResolveTransform(APawn* Pawn,const FTransform& Candidate,const FTransform& Fallback,FTransform& Out,const AActor* Ignore)
{
    if(FindSupportedTransform(Pawn,Candidate,Out,Ignore)||FindSupportedTransform(Pawn,Fallback,Out,Ignore))return true;
    if(Pawn&&Pawn->GetWorld())if(auto* Mode=Pawn->GetWorld()->GetAuthGameMode())if(auto* Start=Mode->FindPlayerStart(Pawn->GetController()))return FindSupportedTransform(Pawn,Start->GetActorTransform(),Out,Ignore);
    return false;
}
void InkPawnSafety::ClearFallingMotion(APawn* Pawn)
{
    if(auto* Character=Cast<ACharacter>(Pawn))
    {
        auto* Move=Character->GetCharacterMovement();Move->StopMovementImmediately();Move->ClearAccumulatedForces();Move->SetMovementMode(MOVE_Walking);Character->ResetJumpState();
    }
    else if(Pawn&&Pawn->GetMovementComponent())Pawn->GetMovementComponent()->StopMovementImmediately();
}
