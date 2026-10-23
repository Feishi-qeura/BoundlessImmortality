#pragma once
#include "CoreMinimal.h"
class ALiveWeaponSwarm;
class FJsonObject;
class UWorld;

/** Owns only this attempt's alert sources; it never snapshots or overwrites flight state. */
struct FWulingTribulationPresentation
{
    void Reconcile(UWorld* World,const TSharedPtr<FJsonObject>& State,bool Active,int64 Generation);
    void Clear();
private:
    TWeakObjectPtr<ALiveWeaponSwarm> Swarm;
    TSet<FString> Helpers;
    FString Source;
    int64 LeaseGeneration=0;
};
