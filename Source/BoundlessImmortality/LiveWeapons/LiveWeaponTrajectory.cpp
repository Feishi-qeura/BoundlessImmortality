#include "LiveWeaponTrajectory.h"
FLiveBezierFlight FLiveBezierFlight::Build(const FVector& From,const FVector& To,const FVector& InitialVelocity,const FVector& FinalVelocity,float DesiredSeconds,float MaxAcceleration,float MaxSpeed)
{
    FLiveBezierFlight P;P.Start=From;P.End=To;P.Duration=FMath::Max(.08f,DesiredSeconds);
    MaxSpeed=FMath::Max(MaxSpeed,float(InitialVelocity.Size()));
    const FVector V0=InitialVelocity;
    const FVector V1=FinalVelocity.GetClampedToMaxSize(FMath::Max(1.f,MaxSpeed));
    for(int I=0;I<32;++I)
    {
        P.ControlA=From+V0*(P.Duration/3);P.ControlB=To-V1*(P.Duration/3);
        const double Peak=FMath::Max(P.Acceleration(0).Size(),P.Acceleration(P.Duration).Size());
        double PeakSpeed=0;for(int J=0;J<=8;++J)PeakSpeed=FMath::Max(PeakSpeed,P.Velocity(P.Duration*J/8).Size());
        if(Peak<=FMath::Max(1.f,MaxAcceleration)&&PeakSpeed<=FMath::Max(1.f,MaxSpeed)*1.01)break;
        P.Duration*=1.12f;
    }
    return P;
}
FLiveBezierFlight FLiveBezierFlight::BuildSpatial(const FVector& From,const FVector& To,const FVector& InitialVelocity,const FVector& FinalVelocity,float DesiredSeconds,const FVector& Arc,const FVector& Weave,float MaxAcceleration,float MaxSpeed)
{
    auto P=Build(From,To,InitialVelocity,FinalVelocity,DesiredSeconds,MaxAcceleration,MaxSpeed);P.ArcOffset=Arc;P.WeaveOffset=Weave;
    MaxSpeed=FMath::Max(MaxSpeed,float(InitialVelocity.Size()));
    for(int I=0;I<48;++I)
    {
        P.ControlA=From+InitialVelocity*(P.Duration/3);P.ControlB=To-FinalVelocity.GetClampedToMaxSize(MaxSpeed)*(P.Duration/3);
        double PeakAcceleration=0,PeakSpeed=0;
        for(int J=0;J<=32;++J){const float T=P.Duration*J/32;PeakAcceleration=FMath::Max(PeakAcceleration,P.Acceleration(T).Size());PeakSpeed=FMath::Max(PeakSpeed,P.Velocity(T).Size());}
        if(PeakAcceleration<=FMath::Max(1.f,MaxAcceleration)*.98f&&PeakSpeed<=FMath::Max(1.f,MaxSpeed)*1.001f)break;
        P.Duration*=1.1f;
    }
    return P;
}
void FLiveBezierFlight::KeepAboveLinearFloor(float StartFloorZ,float EndFloorZ,float HeightMargin)
{
    // The Bezier convex hull stays above a linear baseline when each control
    // point does. Keep vertical weave inside the upward arc as well.
    const double Margin=FMath::Max(0.f,HeightMargin);
    Start.Z=FMath::Max(Start.Z,StartFloorZ+Margin);
    ControlA.Z=FMath::Max(ControlA.Z,FMath::Lerp(StartFloorZ,EndFloorZ,1.f/3)+Margin);
    ControlB.Z=FMath::Max(ControlB.Z,FMath::Lerp(StartFloorZ,EndFloorZ,2.f/3)+Margin);
    End.Z=FMath::Max(End.Z,EndFloorZ+Margin);
    ArcOffset.Z=FMath::Max(ArcOffset.Z,FMath::Abs(WeaveOffset.Z));
}
FVector FLiveBezierFlight::Position(float Seconds)const
{
    const float T=FMath::Clamp(Seconds/Duration,0.f,1.f),U=1-T;
    const float Bump=16*T*T*U*U;
    return Start*(U*U*U)+ControlA*(3*U*U*T)+ControlB*(3*U*T*T)+End*(T*T*T)+ArcOffset*Bump+WeaveOffset*(Bump*(2*T-1));
}
FVector FLiveBezierFlight::Velocity(float Seconds)const
{
    const float T=FMath::Clamp(Seconds/Duration,0.f,1.f),U=1-T;
    const float Bump=16*T*T*U*U,Derivative=32*T-96*T*T+64*T*T*T;
    return ((ControlA-Start)*(3*U*U)+(ControlB-ControlA)*(6*U*T)+(End-ControlB)*(3*T*T)+ArcOffset*Derivative+WeaveOffset*(Derivative*(2*T-1)+2*Bump))/Duration;
}
FVector FLiveBezierFlight::Acceleration(float Seconds)const
{
    const float T=FMath::Clamp(Seconds/Duration,0.f,1.f);
    const float Derivative=32*T-96*T*T+64*T*T*T,Second=32-192*T+192*T*T;
    return ((ControlB-2*ControlA+Start)*(6*(1-T))+(End-2*ControlB+ControlA)*(6*T)+ArcOffset*Second+WeaveOffset*(Second*(2*T-1)+4*Derivative))/(Duration*Duration);
}
bool FLiveBezierFlight::SweptBoxHits(const FVector& From,const FVector& To,const FVector& Target,float Radius,const FVector& HalfExtent)
{
    const FVector Delta=To-From;
    const FQuat Basis=Delta.IsNearlyZero()?FQuat::Identity:FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).ToQuat();
    const FVector LocalTarget=Basis.UnrotateVector(Target-From),Extent=HalfExtent.GetAbs();
    // Motion is along local Z. The swept union is an extruded box; sphere/AABB distance
    // retains rounded corners, unlike expanding every axis by the sphere's radius.
    const FBox Sweep(-Extent,Extent+FVector(0,0,Delta.Size()));
    return FVector::DistSquared(LocalTarget,Sweep.GetClosestPointTo(LocalTarget))<=FMath::Square(FMath::Max(0.f,Radius));
}
