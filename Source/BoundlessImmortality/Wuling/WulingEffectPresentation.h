#pragma once
#include "CoreMinimal.h"
class FJsonObject;
class UNiagaraComponent;
class UWorld;
struct FWulingEffectPresentation
{
    void Reconcile(UWorld* World,const TSharedPtr<FJsonObject>& State,bool Active);
    void Clear();
    TMap<FString,TWeakObjectPtr<UNiagaraComponent>> Particles;
    TMap<FString,double> ObservedEnds;
    FString LastCue,Replay;
};
