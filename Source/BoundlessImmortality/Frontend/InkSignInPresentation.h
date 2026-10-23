#pragma once
#include "CoreMinimal.h"
namespace InkSignIn
{
    inline float MilestoneFraction(float Count,const TArray<int32>& Days)
    {
        if(Days.Num()<2||Count<=Days[0])return 0;
        for(int32 I=1;I<Days.Num();++I)if(Count<=Days[I])return (I-1+FMath::Clamp((Count-Days[I-1])/float(Days[I]-Days[I-1]),0.f,1.f))/(Days.Num()-1);
        return 1;
    }
}
