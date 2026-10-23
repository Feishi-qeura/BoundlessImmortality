#include "WulingTribulationPresentation.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void FWulingTribulationPresentation::Clear()
{
    if(auto* S=Swarm.Get())for(const auto& Id:Helpers)S->ReleaseAlertLease(Id,Source);
    Helpers.Reset();Source.Reset();Swarm.Reset();
}
void FWulingTribulationPresentation::Reconcile(UWorld* World,const TSharedPtr<FJsonObject>& State,bool Active,int64 Generation)
{
    const TSharedPtr<FJsonObject>* Attempt=nullptr;FString Status,Id;double Deadline=0,Now=0;
    if(!World||!Active||!State||!State->TryGetObjectField(TEXT("tribulation"),Attempt)||
       !(*Attempt)->TryGetStringField(TEXT("status"),Status)||Status!=TEXT("Trial")||!(*Attempt)->TryGetStringField(TEXT("id"),Id)||
       !(*Attempt)->TryGetNumberField(TEXT("deadline"),Deadline)||!State->TryGetNumberField(TEXT("server_utc"),Now)||Deadline<=Now)
    {Clear();return;}
    ALiveWeaponSwarm* Found=nullptr;const auto* Pawn=UGameplayStatics::GetPlayerPawn(World,0);
    for(TActorIterator<ALiveWeaponSwarm> It(World);It;++It)if(It->GetAnchor()==Pawn){Found=*It;break;}
    const FString NextSource=TEXT("tribulation:")+Id;
    if(Found!=Swarm.Get()||Source!=NextSource||LeaseGeneration!=Generation){Clear();Swarm=Found;Source=NextSource;LeaseGeneration=Generation;}
    if(!Found)return;
    TSet<FString> Current;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if((*Attempt)->TryGetArrayField(TEXT("helper_ids"),Rows))for(const auto& V:*Rows){FString User;if(V->TryGetString(User))Current.Add(User);}
    for(const auto& User:Helpers)if(!Current.Contains(User))Found->ReleaseAlertLease(User,Source);
    Helpers=Current;const double End=UCultivationSubsystem::UtcNow()+Deadline-Now;
    for(const auto& User:Helpers)Found->AcquireAlertLease(User,Source,End,Generation);
}
