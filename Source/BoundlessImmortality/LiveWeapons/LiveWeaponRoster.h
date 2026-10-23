#pragma once
#include "LiveWeaponTypes.h"

class BOUNDLESSIMMORTALITY_API FLiveWeaponRoster
{
public:
    FLiveWeaponHandle Join(const FLiveParticipantProfile& Profile);
    bool SwitchWeapon(const FString& ParticipantId,ELiveWeaponType Type);
    FLiveWeaponState* Find(const FString& ParticipantId);
    const FLiveWeaponState* Find(const FString& ParticipantId)const;
    int32 Num()const{return States.Num();}
    void Reset();
    TArray<FLiveWeaponState>& GetStates(){return States;}
    const TArray<FLiveWeaponState>& GetStates()const{return States;}
private:
    TMap<FString,int32> Indices;
    TArray<FLiveWeaponState> States;
    uint32 Generation=1;
};
