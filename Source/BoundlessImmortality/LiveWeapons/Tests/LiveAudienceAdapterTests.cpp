#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeapons/LiveAudienceAdapterComponent.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveInteractionTestAdapter.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Engine/Texture2D.h"
#include "Cultivation/CultivationSubsystem.h"
#include "LiveWeaponTestWorld.h"
struct FLiveAudienceAdapterTestAccess
{
    static void UseCultivation(ULiveAudienceAdapterComponent& Adapter,UCultivationSubsystem* Cultivation)
    {
        Adapter.Cultivation=Cultivation;Adapter.HandleSessionChanged();
    }
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveCommandTest,"BoundlessImmortality.LiveWeapons.AudienceCommands",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveCommandTest::RunTest(const FString&)
{
    TestEqual(TEXT("trim join command"),ULiveAudienceAdapterComponent::ParseCommand(TEXT(" 加入 ")),0);
    TestEqual(TEXT("sword"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("1")),1);
    TestEqual(TEXT("axe"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("2")),2);
    TestEqual(TEXT("spear"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("3")),3);
    TestEqual(TEXT("bow"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("4")),4);
    TestEqual(TEXT("no substring commands"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("加入123")),-1);
    TestEqual(TEXT("no multi digit commands"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("12")),-1);
    TestEqual(TEXT("comment never grants paid skills"),ULiveAudienceAdapterComponent::ParseCommand(TEXT("噬魂剑")),-1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAdapterRoutingTest,"BoundlessImmortality.LiveWeapons.AudienceRouting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveAdapterRoutingTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld TestWorld;auto* Swarm=TestWorld.World->SpawnActor<ALiveWeaponSwarm>();
    auto* GI=NewObject<UGameInstance>();auto* Host=NewObject<ULiveInteractionSubsystem>(GI);FLiveInteractionTestAdapter::EnableLocalTest(*Host);
    auto* Adapter=NewObject<ULiveAudienceAdapterComponent>(Swarm);Adapter->RegisterComponent();Adapter->Bind(Swarm,Host);
    Host->SimulateComment(TEXT("a"),TEXT("A"),TEXT("4"));TestEqual(TEXT("digit does not auto join"),Swarm->GetParticipantCount(),0);
    Host->SimulateComment(TEXT("a"),TEXT("A"),TEXT("加入"));Host->SimulateComment(TEXT("a"),TEXT("renamed"),TEXT("加入"));
    TestEqual(TEXT("repeated join idempotent"),Swarm->GetParticipantCount(),1);
    Host->SimulateComment(TEXT("a"),TEXT("A"),TEXT("4"));
    const auto* S=Swarm->GetRoster().Find(TEXT("local:a"));TestTrue(TEXT("live event switches same participant"),S&&S->WeaponType==ELiveWeaponType::Bow);
    FLiveComment Invalid;Invalid.UserId=TEXT("b");Invalid.Nickname=TEXT("B");Invalid.MessageId=TEXT("forged");Invalid.Content=TEXT("加入");
    TestFalse(TEXT("unstamped event rejected"),Host->DeliverComment(Invalid));TestEqual(TEXT("forged event not spawned"),Swarm->GetParticipantCount(),1);
    FLiveInteractionTestAdapter::BeginDevRelay(*Host,TEXT("different_room"));TestEqual(TEXT("new room clears participants"),Swarm->GetParticipantCount(),0);
    Adapter->Unbind();return true;
}
namespace {
class FLiveReconnectProvider final:public ILiveInteractionProvider
{
public:
    FLiveSession Session;
    FString GetPlatformId()const override{return TEXT("weapon-reconnect-test");}
    bool Start(ULiveInteractionSubsystem& Host)override{Session=Host.BeginProviderSession(*this,TEXT("app"),TEXT("room"));return Session.Nonce.IsValid();}
    void Stop()override{}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveReconnectAckTest,"BoundlessImmortality.LiveWeapons.AcknowledgementReconnect",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveReconnectAckTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Host=NewObject<ULiveInteractionSubsystem>(NewObject<UGameInstance>());
    auto P=MakeShared<FLiveReconnectProvider>();FLiveInteractionProviderRegistry::Register(P->GetPlatformId(),[P]()->TSharedPtr<ILiveInteractionProvider>{return P;});Host->StartPlatform(P->GetPlatformId());
    auto* Adapter=NewObject<ULiveAudienceAdapterComponent>(Swarm);Adapter->RegisterComponent();Adapter->Bind(Swarm,Host);
    FLiveComment E;E.Session=P->Session;E.UserId=TEXT("v");E.Nickname=TEXT("v");E.MessageId=TEXT("needs-ack");E.Content=TEXT("加入");Host->DeliverComment(E);
    TestEqual(TEXT("applied event waiting for presentation acknowledgement"),Adapter->GetPendingAcknowledgementCount(),1);
    Host->EndProviderSession(*P,TEXT("disconnected"));
    TestEqual(TEXT("disconnect retains pending acknowledgement"),Adapter->GetPendingAcknowledgementCount(),1);
    P->Session=Host->BeginProviderSession(*P,TEXT("app"),TEXT("room"));TestEqual(TEXT("same room preserves outbox"),Adapter->GetPendingAcknowledgementCount(),1);TestEqual(TEXT("same room preserves weapon"),Swarm->GetParticipantCount(),1);
    P->Session=Host->BeginProviderSession(*P,TEXT("app"),TEXT("other"));TestEqual(TEXT("other room clears outbox"),Adapter->GetPendingAcknowledgementCount(),0);TestEqual(TEXT("other room clears weapons"),Swarm->GetParticipantCount(),0);
    Adapter->Unbind();Host->StartPlatform(TEXT("missing-provider"));FLiveInteractionProviderRegistry::Unregister(P->GetPlatformId());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAvatarRefreshTest,"BoundlessImmortality.LiveWeapons.AvatarRefresh",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveAvatarRefreshTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Host=NewObject<ULiveInteractionSubsystem>(NewObject<UGameInstance>());FLiveInteractionTestAdapter::EnableLocalTest(*Host);
    auto* Adapter=NewObject<ULiveAudienceAdapterComponent>(Swarm);Adapter->RegisterComponent();Adapter->Bind(Swarm,Host);Host->SimulateComment(TEXT("viewer_0000"),TEXT("v"),TEXT("加入"));
    Adapter->RequestVisibleAvatar(TEXT("local:viewer_0000"));TestEqual(TEXT("local thumbnail ready"),Adapter->GetReadyAvatarCount(),1);
    FLiveComment E;E.Session=Host->GetCurrentSession();E.MessageId=TEXT("new-avatar");E.UserId=TEXT("viewer_0000");E.Nickname=TEXT("new name");E.AvatarUrl=TEXT("https://example.invalid/new-avatar.png");E.Content=TEXT("加入");Host->DeliverComment(E);
    TestEqual(TEXT("changed URL invalidates cached portrait"),Adapter->GetReadyAvatarCount(),0);Adapter->Unbind();return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveGMControlsTest,"BoundlessImmortality.LiveWeapons.GMControls",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveGMControlsTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Host=NewObject<ULiveInteractionSubsystem>(NewObject<UGameInstance>());
    auto* Adapter=NewObject<ULiveAudienceAdapterComponent>(Swarm);Adapter->RegisterComponent();Adapter->Bind(Swarm,Host);
    TestEqual(TEXT("GM cannot append before explicit simulation"),Adapter->AppendSimulatedAudience(1),0);
    TestTrue(TEXT("GM can explicitly start development simulation"),Host->TryEnableDevelopmentSimulation());
    TestEqual(TEXT("add first spirit"),Adapter->AppendSimulatedAudience(1),1);
    TestEqual(TEXT("add is incremental, not top-up"),Adapter->AppendSimulatedAudience(1),1);
    TestEqual(TEXT("two spirits after two clicks"),Swarm->GetParticipantCount(),2);
    const auto* Second=Swarm->GetRoster().Find(TEXT("local:viewer_0001"));TestTrue(TEXT("new spirits default to sword"),Second&&Second->WeaponType==ELiveWeaponType::Sword);
    Adapter->AppendSimulatedAudience(2000);TestEqual(TEXT("GM respects 1000 limit"),Swarm->GetParticipantCount(),1000);
    TestTrue(TEXT("clear local audience"),Adapter->ClearSimulatedAudience());TestEqual(TEXT("clear removes instances"),Swarm->GetRenderedCount(),0);
    TestEqual(TEXT("can add again after reset"),Adapter->AppendSimulatedAudience(1),1);
    FLiveInteractionTestAdapter::BeginDevRelay(*Host,TEXT("relay-room"));
    TestFalse(TEXT("GM cannot replace development relay"),Host->TryEnableDevelopmentSimulation());
    TestEqual(TEXT("GM cannot append to development relay"),Adapter->AppendSimulatedAudience(1),0);
    TestFalse(TEXT("GM cannot clear development relay"),Adapter->ClearSimulatedAudience());
    auto P=MakeShared<FLiveReconnectProvider>();FLiveInteractionProviderRegistry::Register(P->GetPlatformId(),[P]()->TSharedPtr<ILiveInteractionProvider>{return P;});Host->StartPlatform(P->GetPlatformId());
    const auto Session=Host->GetCurrentSession();TestFalse(TEXT("GM cannot replace authenticated provider"),Host->TryEnableDevelopmentSimulation());
    TestEqual(TEXT("GM cannot append to live session"),Adapter->AppendSimulatedAudience(1),0);TestFalse(TEXT("GM cannot clear live session"),Adapter->ClearSimulatedAudience());
    TestTrue(TEXT("failed GM action preserves provider session"),Host->IsEventFromCurrentSession(Session));
    Adapter->Unbind();Host->StartPlatform(TEXT("missing-provider"));FLiveInteractionProviderRegistry::Unregister(P->GetPlatformId());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAnchorProfileRoutingTest,"BoundlessImmortality.LiveWeapons.AnchorProfileRouting",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveAnchorProfileRoutingTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Pawn=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Pawn);
    auto* GI=NewObject<UGameInstance>();auto* Host=NewObject<ULiveInteractionSubsystem>(GI);
    auto* Cultivation=NewObject<UCultivationSubsystem>(GI);Cultivation->bPersistLocalRecords=false;
    auto P=MakeShared<FLiveReconnectProvider>();FLiveInteractionProviderRegistry::Register(P->GetPlatformId(),[P]()->TSharedPtr<ILiveInteractionProvider>{return P;});Host->StartPlatform(P->GetPlatformId());
    P->Session=Host->BeginProviderSession(*P,TEXT("app"),TEXT("room"),TEXT("streamer"));
    auto* Adapter=NewObject<ULiveAudienceAdapterComponent>(Swarm);Adapter->RegisterComponent();Adapter->Bind(Swarm,Host);FLiveAudienceAdapterTestAccess::UseCultivation(*Adapter,Cultivation);
    const FString AnchorId=Host->GetScopedUserId(TEXT("streamer"));
    TestEqual(TEXT("Session identity reaches cultivation before profile arrival"),Cultivation->GetAnchorId(),AnchorId);
    const FString AvatarUrl=TEXT("https://avatar.invalid/streamer.png");
    auto* Avatar=UTexture2D::CreateTransient(64,64);FLiveInteractionTestAdapter::CacheAvatar(*Host,AvatarUrl,Avatar);
    Host->SetAnchorProfile(P->Session,TEXT("Late streamer name"),AvatarUrl);
    auto Snapshot=Cultivation->GetParticipantSnapshot(AnchorId);
    TestEqual(TEXT("Late provider nickname reaches the existing cultivation anchor"),Snapshot.DisplayName,FString(TEXT("Late streamer name")));
    TestEqual(TEXT("Late provider avatar reaches the HUD snapshot"),Snapshot.AvatarUrl,AvatarUrl);
    Cultivation->SetAnchor(Pawn,AnchorId,TEXT("Late streamer name"));
    TestEqual(TEXT("Pawn reattachment preserves the provider avatar"),Cultivation->GetParticipantSnapshot(AnchorId).AvatarUrl,AvatarUrl);
    FLiveComment AnchorComment;AnchorComment.Session=P->Session;AnchorComment.MessageId=TEXT("anchor-without-avatar");AnchorComment.UserId=TEXT("streamer");AnchorComment.Nickname=TEXT("Late streamer name");AnchorComment.Content=TEXT("加入");Host->DeliverComment(AnchorComment);
    TestEqual(TEXT("Anchor events that omit the avatar preserve the current profile"),Cultivation->GetParticipantSnapshot(AnchorId).AvatarUrl,AvatarUrl);
    FLiveComment Viewer;Viewer.Session=P->Session;Viewer.MessageId=TEXT("unrelated-viewer");Viewer.UserId=TEXT("viewer");Viewer.Nickname=TEXT("Viewer");Viewer.Content=TEXT("加入");Host->DeliverComment(Viewer);
    Host->SetAnchorProfile(P->Session,TEXT("Renamed streamer"),TEXT(""));
    TestTrue(TEXT("Profile removal clears the old cultivation avatar"),Cultivation->GetParticipantSnapshot(AnchorId).AvatarUrl.IsEmpty());
    TestEqual(TEXT("Profile refresh does not reset audience gameplay"),Swarm->GetParticipantCount(),1);
    const auto OldSession=P->Session;
    P->Session=Host->BeginProviderSession(*P,TEXT("app"),TEXT("other-room"),TEXT("other-streamer"));
    TestEqual(TEXT("New room updates anchor identity"),Cultivation->GetAnchorId(),Host->GetScopedUserId(TEXT("other-streamer")));
    Host->SetAnchorProfile(OldSession,TEXT("Wrong room"),AvatarUrl);
    TestTrue(TEXT("Previous session cannot restore the old avatar in current HUD data"),Cultivation->GetParticipantSnapshot(Cultivation->GetAnchorId()).AvatarUrl.IsEmpty());
    Adapter->Unbind();Host->StartPlatform(TEXT("missing-provider"));FLiveInteractionProviderRegistry::Unregister(P->GetPlatformId());return true;
}
#endif
