#include "LiveWeaponSwarm.h"
#include "Cultivation/CultivationSubsystem.h"

bool ALiveWeaponSwarm::AcquireAlertLease(const FString& Id,const FString& Source,double End,int64 Generation)
{
    if(!Roster.Find(Id)||Source.IsEmpty()||!FMath::IsFinite(End)||End<=UCultivationSubsystem::UtcNow()||Generation<HighestAlertGeneration)return false;
    HighestAlertGeneration=FMath::Max(HighestAlertGeneration,Generation);AlertLeases.FindOrAdd(Id).Add(Source,{End,Generation});return true;
}
void ALiveWeaponSwarm::ReleaseAlertLease(const FString& Id,const FString& Source)
{if(auto* Sources=AlertLeases.Find(Id)){Sources->Remove(Source);if(Sources->IsEmpty())AlertLeases.Remove(Id);}}
void ALiveWeaponSwarm::ReleaseAlertLeasesForGeneration(int64 Generation)
{
    for(auto It=AlertLeases.CreateIterator();It;++It){for(auto S=It.Value().CreateIterator();S;++S)if(S.Value().Generation==Generation)S.RemoveCurrent();if(It.Value().IsEmpty())It.RemoveCurrent();}
}
int32 ALiveWeaponSwarm::GetAlertLeaseCount(const FString& Id)const
{
    const auto* Sources=AlertLeases.Find(Id);if(!Sources)return 0;const double Now=UCultivationSubsystem::UtcNow();int32 Count=0;
    for(const auto& Pair:*Sources)if(Pair.Value.ExpiresUtc>Now)++Count;return Count;
}
bool ALiveWeaponSwarm::WantsAlert(int32 Slot)const
{return bNearbyThreat||(Roster.GetStates().IsValidIndex(Slot)&&GetAlertLeaseCount(Roster.GetStates()[Slot].Profile.ParticipantId)>0);}
