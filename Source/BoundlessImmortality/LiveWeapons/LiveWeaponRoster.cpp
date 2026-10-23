#include "LiveWeaponRoster.h"
FLiveWeaponHandle FLiveWeaponRoster::Join(const FLiveParticipantProfile& Profile)
{
    if(Profile.ParticipantId.TrimStartAndEnd().IsEmpty())return {};
    if(auto* Existing=Find(Profile.ParticipantId)){Existing->Profile=Profile;return Existing->Handle;}
    FLiveWeaponState State;
    State.Profile=Profile;
    State.Handle={States.Num(),Generation};
    Indices.Add(Profile.ParticipantId,State.Handle.Index);
    States.Add(MoveTemp(State));
    return States.Last().Handle;
}
bool FLiveWeaponRoster::SwitchWeapon(const FString& Id,ELiveWeaponType Type)
{
    if(uint8(Type)>uint8(ELiveWeaponType::Bow))return false;
    if(auto* State=Find(Id)){if(State->WeaponType!=Type)++State->WeaponRevision;State->WeaponType=Type;return true;}
    return false;
}
FLiveWeaponState* FLiveWeaponRoster::Find(const FString& Id)
{
    const int32* Index=Indices.Find(Id);
    return Index ? &States[*Index] : nullptr;
}
const FLiveWeaponState* FLiveWeaponRoster::Find(const FString& Id)const{return const_cast<FLiveWeaponRoster*>(this)->Find(Id);}
void FLiveWeaponRoster::Reset(){States.Reset();Indices.Reset();++Generation;}
