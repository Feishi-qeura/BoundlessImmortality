#include "WulingEffectPresentation.h"
#include "Cultivation/CultivationSubsystem.h"
#include "CultivationGameplayComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"

void FWulingEffectPresentation::Clear()
{
    for(auto& P:Particles)if(auto* Component=P.Value.Get()){Component->Deactivate();Component->DeactivateImmediate();Component->DestroyComponent();}
    Particles.Reset();ObservedEnds.Reset();LastCue.Reset();Replay.Reset();
}
void FWulingEffectPresentation::Reconcile(UWorld* World,const TSharedPtr<FJsonObject>& State,bool Active)
{
    auto* Pawn=World?UGameplayStatics::GetPlayerPawn(World,0):nullptr;
    const TSharedPtr<FJsonObject>* Data=nullptr;
    if(!Active||!Pawn||!State||!State->TryGetObjectField(TEXT("presentation"),Data)){Clear();return;}
    auto* C=World->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();
    auto Announce=[&](const FString& Text){if(C&&!Text.IsEmpty())C->OnAnnouncement.Broadcast(Text);};
    const TArray<TSharedPtr<FJsonValue>>* Effects=nullptr;TSet<FString> Visible;
    if((*Data)->TryGetArrayField(TEXT("effects"),Effects))for(const auto& V:*Effects)
    {
        auto O=V->AsObject();FString Key,Name;double End=0;if(!O||!O->TryGetStringField(TEXT("key"),Key)||!O->TryGetNumberField(TEXT("end"),End))continue;
        O->TryGetStringField(TEXT("name"),Name);Visible.Add(Key);
        if(ObservedEnds.FindRef(Key)!=End){ObservedEnds.Add(Key,End);Announce(Name);}
        if(Key!=TEXT("meditation_visual")&&Key!=TEXT("break_ceremony_visual"))continue;
        if(auto* Existing=Particles.FindRef(Key).Get())
        {if(Existing->IsActive())continue;Existing->Deactivate();Existing->DeactivateImmediate();Existing->DestroyComponent();Particles.Remove(Key);}
        auto* Gameplay=Pawn->FindComponentByClass<UCultivationGameplayComponent>();if(!Gameplay)continue;
        TInlineComponentArray<UNiagaraComponent*> Sources(Pawn);
        for(auto* Source:Sources)if(Source->GetFName()==Gameplay->InkEffectComponentName&&Source->GetAsset())
        {
            auto* Effect=UNiagaraFunctionLibrary::SpawnSystemAttached(Source->GetAsset(),Pawn->GetRootComponent(),NAME_None,Source->GetRelativeLocation(),Source->GetRelativeRotation(),EAttachLocation::KeepRelativeOffset,false,true,ENCPoolMethod::None,false);
            if(Effect){Effect->SetRelativeScale3D(Source->GetRelativeScale3D());Particles.Add(Key,Effect);}break;
        }
    }
    for(auto It=Particles.CreateIterator();It;++It)if(!Visible.Contains(It.Key())){if(auto* P=It.Value().Get()){P->Deactivate();P->DeactivateImmediate();P->DestroyComponent();}It.RemoveCurrent();}
    for(auto It=ObservedEnds.CreateIterator();It;++It)if(!Visible.Contains(It.Key()))It.RemoveCurrent();
    FString Cue;(*Data)->TryGetStringField(TEXT("next_cue"),Cue);FString Phase= Cue.Left(Cue.Find(TEXT(" · ")));
    if(Cue.IsEmpty())LastCue.Reset();else if(Phase!=LastCue){LastCue=Phase;Announce(Cue);}
    const TSharedPtr<FJsonObject>* Marker=nullptr;
    if((*Data)->TryGetObjectField(TEXT("local_replay"),Marker)){FString Status;(*Marker)->TryGetStringField(TEXT("status"),Status);double Q=0;(*Marker)->TryGetNumberField(TEXT("q"),Q);const FString Caption=FString::Printf(TEXT("本机突破记录：%s · Q%.0f"),*Status,Q);if(Caption!=Replay){Replay=Caption;Announce(Caption);}}
}
