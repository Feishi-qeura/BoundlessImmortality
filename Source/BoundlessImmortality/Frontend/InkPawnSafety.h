#pragma once
#include "CoreMinimal.h"
class APawn;
namespace InkPawnSafety
{
    /** A walkable floor plus room for the actual capsule, never an arbitrary XY bound. */
    bool FindSupportedTransform(APawn* Pawn,const FTransform& Candidate,FTransform& Out,const AActor* Ignore=nullptr);
    bool ResolveTransform(APawn* Pawn,const FTransform& Candidate,const FTransform& Fallback,FTransform& Out,const AActor* Ignore=nullptr);
    void ClearFallingMotion(APawn* Pawn);
}
