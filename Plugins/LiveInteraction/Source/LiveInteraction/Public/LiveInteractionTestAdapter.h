#pragma once
#include "LiveInteractionSubsystem.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Texture2D.h"
/** Explicit automation opt-in, unavailable in production builds. */
class FLiveInteractionTestAdapter
{
public:
    static void EnableLocalTest(ULiveInteractionSubsystem& Host) { Host.EnableLocalTest(); }
    static void CacheAvatar(ULiveInteractionSubsystem& Host,const FString& Url,UTexture2D* Texture) { Host.AvatarCache.Add(Url,Texture);Host.AvatarCacheAccess.Add(Url,0); }
    static uint64 BeginAvatarRequest(ULiveInteractionSubsystem& Host,const FString& UserId,const FString& Url) { return Host.AvatarRequests.Begin(Host.GetScopedUserId(UserId),Url); }
    static bool CompleteAvatarRequest(ULiveInteractionSubsystem& Host,const FString& UserId,uint64 Ticket) { return Host.AvatarRequests.Complete(Host.GetScopedUserId(UserId),Ticket); }
    template<typename T> static T Stamp(ULiveInteractionSubsystem& Host,const T& Event) { T Copy=Event; Copy.Session=Host.GetCurrentSession(); return Copy; }
    static void Receive(ULiveInteractionSubsystem& Host,const FString& Json) { Host.Receive(Json); }
    static void BeginDevRelay(ULiveInteractionSubsystem& Host,const FString& Room) {
        Host.EnableLocalTest(); Host.bDevRelayMode=true;
        Host.ChangeSession(TEXT("local"),TEXT("dev-relay"),Room,true);
        Host.ConnectionStatus=TEXT("DEV_TEST_PROTOCOL");
    }
};
#endif
