#pragma once
#include "CoreMinimal.h"
/** World-space cubic Bezier segment with specified endpoint velocities. */
struct BOUNDLESSIMMORTALITY_API FLiveBezierFlight
{
    FVector Start=FVector::ZeroVector,End=FVector::ZeroVector;
    FVector ControlA=FVector::ZeroVector,ControlB=FVector::ZeroVector;
    // Degree-five Bezier displacement: both displacement and velocity vanish at the endpoints.
    FVector ArcOffset=FVector::ZeroVector,WeaveOffset=FVector::ZeroVector;
    float Duration=1;
    static FLiveBezierFlight Build(const FVector& From,const FVector& To,const FVector& InitialVelocity,const FVector& FinalVelocity,float DesiredSeconds,float MaxAcceleration=12000,float MaxSpeed=2600);
    static FLiveBezierFlight BuildSpatial(const FVector& From,const FVector& To,const FVector& InitialVelocity,const FVector& FinalVelocity,float DesiredSeconds,const FVector& Arc,const FVector& Weave,float MaxAcceleration=12000,float MaxSpeed=2600);
    // Pure curve construction: z(t) stays above the linearly blended endpoint floor.
    void KeepAboveLinearFloor(float StartFloorZ,float EndFloorZ,float HeightMargin);
    FVector Position(float Seconds)const;
    FVector Velocity(float Seconds)const;
    FVector Acceleration(float Seconds)const;
    static bool SweptBoxHits(const FVector& From,const FVector& To,const FVector& Target,float Radius,const FVector& HalfExtent);
};
