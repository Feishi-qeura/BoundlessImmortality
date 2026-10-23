#pragma once
#include "CoreMinimal.h"
#include "InkProgressState.generated.h"

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkProgressSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Progress",meta=(ClampMin="0")) float DelaySeconds=.12f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Progress",meta=(ClampMin="0.01")) float BaseSpeed=.16f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Progress",meta=(ClampMin="0")) float GapAcceleration=4.f;
};

/** Foreground is authoritative; the darker trail is presentation only. */
struct BOUNDLESSIMMORTALITY_API FInkProgressState
{
    float Value=0,Trail=0,Hold=0,PeakGap=0;
    bool bInitialized=false;
    static float Fraction(float V){return FMath::IsFinite(V)?FMath::Clamp(V,0.f,1.f):0.f;}
    void Reset(float V){Value=Trail=Fraction(V);Hold=PeakGap=0;bInitialized=true;}
    void SetTarget(float V,const FInkProgressSettings& Settings=FInkProgressSettings())
    {
        V=Fraction(V);if(!bInitialized){Reset(V);return;}
        // Increasing progress must not continuously restart a damage hold.
        if(V<Value-KINDA_SMALL_NUMBER)Hold=FMath::Max(0.f,Settings.DelaySeconds);
        PeakGap=FMath::Max(PeakGap,FMath::Abs(Trail-V));
        Value=V;
    }
    void Tick(float Delta,const FInkProgressSettings& Settings=FInkProgressSettings())
    {
        if(!FMath::IsFinite(Delta)||Delta<=0)return;
        if(Hold>0){const float Used=FMath::Min(Delta,Hold);Hold-=Used;Delta-=Used;}
        if(Delta<=0)return;
        const float Speed=FMath::Max(.01f,Settings.BaseSpeed)+FMath::Max(0.f,Settings.GapAcceleration)*PeakGap*PeakGap;
        Trail=FMath::FInterpConstantTo(Trail,Value,Delta,Speed);
        if(FMath::Abs(Trail-Value)<.001f){Trail=Value;PeakGap=0;}
    }
};

/** A new identity/realm/denominator is a new scale, not a loss of progress. */
struct BOUNDLESSIMMORTALITY_API FInkProgressContext
{
    FString Identity;
    int32 Realm=INDEX_NONE;
    double HealthMaximum=-1,Required=-1;
    bool Update(const FString& Id,int32 R,double Max,double Threshold)
    {
        const bool Changed=Identity!=Id||Realm!=R||HealthMaximum!=Max||Required!=Threshold;
        Identity=Id;Realm=R;HealthMaximum=Max;Required=Threshold;return Changed;
    }
};

namespace InkFeedback
{
    inline int32 DamageTier(double Ratio)
    {
        if(!FMath::IsFinite(Ratio)||Ratio<.2)return 0;
        return Ratio<.26?1:Ratio<.42?2:Ratio<=.6?3:4;
    }
}
