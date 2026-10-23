#pragma once
#include "Misc/ConfigCacheIni.h"

#if WITH_DEV_AUTOMATION_TESTS
/** The neutral plugin's legacy contract must not depend on the consuming game's gift list. */
struct FLiveLegacyGiftFixture
{
    TArray<FString> Before;
    bool bHad=false;
    FLiveLegacyGiftFixture()
    {
        bHad=GConfig->GetArray(TEXT("LiveInteraction"),TEXT("SupportedGiftNames"),Before,GGameIni)>0;
        const TArray<FString> Names={TEXT("仙女棒"),TEXT("能力药丸"),TEXT("魔法镜"),TEXT("甜甜圈"),TEXT("能量电池")};
        GConfig->SetArray(TEXT("LiveInteraction"),TEXT("SupportedGiftNames"),Names,GGameIni);
    }
    ~FLiveLegacyGiftFixture()
    {
        if(bHad)GConfig->SetArray(TEXT("LiveInteraction"),TEXT("SupportedGiftNames"),Before,GGameIni);
        else GConfig->RemoveKey(TEXT("LiveInteraction"),TEXT("SupportedGiftNames"),GGameIni);
    }
};
#endif
