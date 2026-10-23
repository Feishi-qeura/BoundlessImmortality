#pragma once
#include "CoreMinimal.h"
/** Presentation-only. Call Begin with an already selected result; never grants items. */
struct FInkWheelPresentation
{
    double Angle=0,StartAngle=0,TargetAngle=0,Elapsed=0,Duration=3.6;
    int32 ResultSector=0,SectorCount=8;
    bool bAnimating=false;
    TSet<FString> AcceptedReceipts;
    static double Normalize(double Value){return FMath::Fmod(FMath::Fmod(Value,360.)+360.,360.);}
    bool Begin(const FString& Receipt,int32 Sector,int32 Count)
    {
        if(bAnimating||Receipt.IsEmpty()||AcceptedReceipts.Contains(Receipt)||Count<1||Sector<0||Sector>=Count)return false;
        AcceptedReceipts.Add(Receipt);ResultSector=Sector;SectorCount=Count;Elapsed=0;
        StartAngle=FMath::IsFinite(Angle)?Angle:0.;
        const double Final=Normalize(-(Sector+.5)*360./Count);
        TargetAngle=StartAngle+4*360.+Normalize(Final-Normalize(StartAngle));bAnimating=true;return true;
    }
    void Advance(double Delta)
    {
        if(!bAnimating||!FMath::IsFinite(Delta)||Delta<0)return;
        Elapsed=FMath::Min(FMath::Max(.01,Duration),Elapsed+Delta);const double T=Elapsed/FMath::Max(.01,Duration);
        const double Ease=T*T*T*(10+T*(-15+T*6));Angle=FMath::Lerp(StartAngle,TargetAngle,Ease);
        if(T>=1){Angle=TargetAngle;bAnimating=false;}
    }
    void Skip(){if(bAnimating){Angle=TargetAngle;Elapsed=Duration;bAnimating=false;}}
    int32 PointedSector()const{return FMath::Clamp(FMath::FloorToInt(Normalize(-Angle)*SectorCount/360.),0,SectorCount-1);}
};
