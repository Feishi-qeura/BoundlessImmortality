#pragma once
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
// A fully initialized isolated world: spawned actors must run BeginPlay and register themselves.
struct FLiveWeaponTestWorld
{
    UWorld* World;
    FLiveWeaponTestWorld()
    {
        World=UWorld::CreateWorld(EWorldType::Game,false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());World->BeginPlay();World->GetWorldSettings()->NotifyBeginPlay();
    }
    ~FLiveWeaponTestWorld(){World->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(World);World->DestroyWorld(false);}
};
