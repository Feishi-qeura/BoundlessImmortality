#include "LiveInteractionSubsystem.h"
#include "LiveInteractionTestConsumer.h"
#include "LiveInteractionTestAdapter.h"
#include "Engine/Texture2D.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace {
struct FRecordedLiveCommand { FString Id,Operation; TSharedPtr<FJsonObject> Payload; };
class FAdvancedAutomationProvider final : public ILiveInteractionProvider {
public:
    FLiveSession Session;
    TArray<FRecordedLiveCommand> Commands;
    bool bAcceptCommands=true;
    FString GetPlatformId() const override { return TEXT("advanced-neutral-test"); }
    bool Start(ULiveInteractionSubsystem& Host) override { Session=Host.BeginProviderSession(*this,TEXT("app"),TEXT("room"),TEXT("anchor"));return Session.Nonce.IsValid(); }
    void Stop() override {}
    bool SendCommand(const FString& Id,const FString& Operation,const TSharedRef<FJsonObject>& Payload) override {
        if(!bAcceptCommands)return false;
        Commands.Add({Id,Operation,Payload});return true;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAdvancedAdmissionTest,"GeometricWarfare.LiveInteraction.AdvancedAdmission",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLiveAdvancedAdmissionTest::RunTest(const FString&) {
    auto* Host=NewObject<ULiveInteractionSubsystem>(NewObject<UGameInstance>());
    auto Provider=MakeShared<FAdvancedAutomationProvider>();
    FLiveInteractionProviderRegistry::Register(Provider->GetPlatformId(),[Provider]() -> TSharedPtr<ILiveInteractionProvider> {return Provider;});
    Host->StartPlatform(Provider->GetPlatformId());
    FLiveFollow Follow;Follow.Session=Provider->Session;Follow.MessageId=TEXT("follow");Follow.UserId=TEXT("viewer");Follow.TargetUserId=TEXT("anchor");Follow.Action=1;
    FLivePresence Presence;Presence.Session=Provider->Session;Presence.MessageId=TEXT("presence");Presence.UserId=TEXT("viewer");Presence.TimestampMs=100;Presence.EnterType=1;Presence.FollowStatus=1;
    FLiveTeamSelection Team;Team.Session=Provider->Session;Team.MessageId=TEXT("team");Team.UserId=TEXT("viewer");Team.GroupId=TEXT("red");
    TestFalse(TEXT("Follow requires actual subscriber before consuming ID"),Host->DeliverFollow(Follow));
    TestFalse(TEXT("Presence requires actual subscriber before consuming ID"),Host->DeliverPresence(Presence));
    TestFalse(TEXT("Team selection requires actual subscriber before consuming ID"),Host->DeliverTeamSelection(Team));
    auto* Consumer=NewObject<ULiveInteractionTestConsumer>(Host);Consumer->Bind(*Host);
    TestTrue(TEXT("Current follow event accepted after subscribe"),Host->DeliverFollow(Follow));
    TestTrue(TEXT("Current presence event accepted after subscribe"),Host->DeliverPresence(Presence));
    TestTrue(TEXT("Current team selection accepted after subscribe"),Host->DeliverTeamSelection(Team));
    TestEqual(TEXT("Follow consumer receives once"),Consumer->Follows,1);
    TestEqual(TEXT("Presence consumer receives once"),Consumer->PresenceEvents,1);
    TestEqual(TEXT("Selection consumer receives once"),Consumer->TeamSelections,1);
    Follow.MessageId=Team.MessageId;TestFalse(TEXT("Advanced types share the same dedup window"),Host->DeliverFollow(Follow));
    Follow.MessageId=TEXT("invalid-action");Follow.Action=4;TestFalse(TEXT("Unknown follow action rejected"),Host->DeliverFollow(Follow));
    Follow.Action=3;TestTrue(TEXT("Corrected reciprocal follow can reuse rejected ID"),Host->DeliverFollow(Follow));
    Presence.MessageId=TEXT("invalid-presence");Presence.EnterType=3;TestFalse(TEXT("Unknown presence direction rejected"),Host->DeliverPresence(Presence));
    Presence.EnterType=2;Presence.FollowStatus=4;TestFalse(TEXT("Unknown follow status rejected"),Host->DeliverPresence(Presence));
    Presence.FollowStatus=0;Presence.TimestampMs=-1;TestFalse(TEXT("Negative timestamp rejected"),Host->DeliverPresence(Presence));
    Presence.TimestampMs=101;TestTrue(TEXT("Corrected leave can reuse rejected ID"),Host->DeliverPresence(Presence));
    Team.MessageId=TEXT("invalid-group");Team.GroupId=TEXT(" ");TestFalse(TEXT("Blank team ID rejected"),Host->DeliverTeamSelection(Team));
    Team.GroupId=TEXT("blue");TestTrue(TEXT("Corrected group does not lose event ID"),Host->DeliverTeamSelection(Team));
    const auto OldSession=Provider->Session;
    Provider->Session=Host->BeginProviderSession(*Provider,TEXT("app"),TEXT("new-room"),TEXT("anchor"));
    Follow.MessageId=TEXT("stale-follow");TestFalse(TEXT("Prior room follow rejected"),Host->DeliverFollow(Follow));
    Presence.MessageId=TEXT("stale-presence");TestFalse(TEXT("Prior room presence rejected"),Host->DeliverPresence(Presence));
    Team.MessageId=TEXT("stale-team");TestFalse(TEXT("Prior room selection rejected"),Host->DeliverTeamSelection(Team));
    Host->StartPlatform(TEXT("advanced-unregistered"));FLiveInteractionProviderRegistry::Unregister(Provider->GetPlatformId());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveCommandContractTest,"GeometricWarfare.LiveInteraction.CommandContract",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLiveCommandContractTest::RunTest(const FString&) {
    auto* Host=NewObject<ULiveInteractionSubsystem>(NewObject<UGameInstance>());
    auto Payload=MakeShared<FJsonObject>();Payload->SetStringField(TEXT("value"),TEXT("sample"));
    TestTrue(TEXT("Commands without authenticated provider fail explicitly"),Host->SubmitCommand(TEXT("test"),Payload).IsEmpty());
    auto Provider=MakeShared<FAdvancedAutomationProvider>();
    FLiveInteractionProviderRegistry::Register(Provider->GetPlatformId(),[Provider]() -> TSharedPtr<ILiveInteractionProvider> {return Provider;});
    Host->StartPlatform(Provider->GetPlatformId());
    int32 Results=0,Replies=0;TSharedPtr<FJsonObject> ResultData;FLiveSession ResultSession;FString ResultId;bool ResultSuccess=false;int32 ResultCode=0;
    Host->OnCommandReply.AddLambda([&](const FLiveCommandReply& Reply){++Replies;ResultData=Reply.Data;TestTrue(TEXT("JSON reply carries current provenance"),Host->IsEventFromCurrentSession(Reply.Session));});
    Host->OnCommandResult.AddLambda([&](const FLiveSession& Session,const FString& Id,bool Success,int32 Code){++Results;ResultSession=Session;ResultId=Id;ResultSuccess=Success;ResultCode=Code;});
    const FString Id=Host->SubmitCommand(TEXT("test"),Payload);
    TestFalse(TEXT("Accepted command has a concrete request ID"),Id.IsEmpty());
    TestEqual(TEXT("Acceptance is not platform confirmation"),Results,0);
    FAdvancedAutomationProvider WrongSource;
    Host->ReportCommandResult(WrongSource,Id,true,0);TestEqual(TEXT("Unselected provider cannot confirm command"),Results,0);
    auto Data=MakeShared<FJsonObject>();Data->SetStringField(TEXT("round_id"),TEXT("123"));Host->ReportCommandResult(*Provider,Id,false,42,Data);
    TestEqual(TEXT("JSON reply and legacy result each fire once"),Replies,1);TestTrue(TEXT("Optional JSON data reaches native reply without copying credentials"),ResultData==Data);
    TestEqual(TEXT("Provider result delivered exactly once"),Results,1);TestEqual(TEXT("Provider error preserved"),ResultCode,42);TestFalse(TEXT("Provider failure not represented as success"),ResultSuccess);
    TestEqual(TEXT("Result retains original request identity"),ResultId,Id);TestTrue(TEXT("Result snapshot has current session provenance"),Host->IsEventFromCurrentSession(ResultSession));
    Host->ReportCommandResult(*Provider,Id,true,0);TestEqual(TEXT("Duplicate command result ignored"),Results,1);
    const FString Late=Host->SubmitCommand(TEXT("test"),Payload);Host->EndProviderSession(*Provider,TEXT("SDK_DISCONNECTED"));
    Host->ReportCommandResult(*Provider,Late,true,0);TestEqual(TEXT("Prior session completion cannot update current game"),Results,1);
    TestEqual(TEXT("Duplicate and stale responses cannot deliver JSON reply"),Replies,1);
    Provider->Session=Host->BeginProviderSession(*Provider,TEXT("app"),TEXT("room"),TEXT("anchor"));
    Provider->bAcceptCommands=false;TestTrue(TEXT("Transport failure returns empty request ID"),Host->SubmitCommand(TEXT("test"),Payload).IsEmpty());
    Provider->bAcceptCommands=true;
    TestFalse(TEXT("Stale event cannot submit fulfilment"),Host->NotifyEventHandled(ResultSession,TEXT("message"),TEXT("live_comment")));
    TestTrue(TEXT("Handled current event submits fulfilment"),Host->NotifyEventHandled(Provider->Session,TEXT("message"),TEXT("live_comment")));
    const auto& Ack=Provider->Commands.Last();TestEqual(TEXT("ACK uses explicit operation"),Ack.Operation,FString(TEXT("ack")));
    TestEqual(TEXT("ACK preserves official message type"),Ack.Payload->GetStringField(TEXT("msg_type")),FString(TEXT("live_comment")));
    TestEqual(TEXT("ACK preserves actual message ID"),Ack.Payload->GetStringField(TEXT("msg_id")),FString(TEXT("message")));
    TestFalse(TEXT("Unsupported simulated share type cannot ACK"),Host->NotifyEventHandled(Provider->Session,TEXT("share"),TEXT("live_share")));
    Host->StartPlatform(TEXT("advanced-unregistered"));FLiveInteractionProviderRegistry::Unregister(Provider->GetPlatformId());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAnchorProfileTest,"GeometricWarfare.LiveInteraction.AnchorProfileLifecycle",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLiveAnchorProfileTest::RunTest(const FString&)
{
    auto* Host=NewObject<ULiveInteractionSubsystem>(NewObject<UGameInstance>());
    auto Provider=MakeShared<FAdvancedAutomationProvider>();
    FLiveInteractionProviderRegistry::Register(Provider->GetPlatformId(),[Provider]() -> TSharedPtr<ILiveInteractionProvider> {return Provider;});
    Host->StartPlatform(Provider->GetPlatformId());
    auto* Consumer=NewObject<ULiveInteractionTestConsumer>(Host);
    Host->OnAnchorProfileChanged.AddDynamic(Consumer,&ULiveInteractionTestConsumer::ConsumeAnchorProfile);
    Host->OnAvatarReady.AddDynamic(Consumer,&ULiveInteractionTestConsumer::ConsumeAvatar);
    const FString FirstUrl=TEXT("https://avatar.invalid/first.png"),SecondUrl=TEXT("https://avatar.invalid/second.png");
    auto* First=UTexture2D::CreateTransient(64,64);auto* Second=UTexture2D::CreateTransient(64,64);
    FLiveInteractionTestAdapter::CacheAvatar(*Host,FirstUrl,First);FLiveInteractionTestAdapter::CacheAvatar(*Host,SecondUrl,Second);
    Host->SetAnchorProfile(Provider->Session,TEXT("Actual streamer"),FirstUrl);
    TestEqual(TEXT("Late profile retains streamer nickname"),Host->GetAnchorNickname(),FString(TEXT("Actual streamer")));
    TestEqual(TEXT("Late profile retains the actual avatar URL"),Host->GetAnchorAvatarUrl(),FirstUrl);
    TestEqual(TEXT("Profile arrival notifies existing consumers"),Consumer->AnchorProfiles,1);
    TestTrue(TEXT("HUD can retrieve the thumbnail completed before it subscribed"),Host->GetCachedAvatar(FirstUrl)==First);
    TestEqual(TEXT("Provider avatar uses the actual anchor ID"),Consumer->LastAvatarUser,FString(TEXT("anchor")));
    Host->SetAnchorProfile(Provider->Session,TEXT("Actual streamer"),FirstUrl);
    TestEqual(TEXT("Unchanged profile does not repeat state notifications"),Consumer->AnchorProfiles,1);
    const uint64 FirstRequest=FLiveInteractionTestAdapter::BeginAvatarRequest(*Host,TEXT("anchor"),FirstUrl);
    Host->SetAnchorProfile(Provider->Session,TEXT("Renamed streamer"),SecondUrl);
    TestFalse(TEXT("Updated URL supersedes an earlier thumbnail download"),FLiveInteractionTestAdapter::CompleteAvatarRequest(*Host,TEXT("anchor"),FirstRequest));
    TestEqual(TEXT("Changed profile notifies existing consumers"),Consumer->AnchorProfiles,2);
    TestEqual(TEXT("Replacement profile uses new URL"),Host->GetAnchorAvatarUrl(),SecondUrl);
    TestTrue(TEXT("New URL resolves its own texture instead of the previous portrait"),Host->GetCachedAvatar(Host->GetAnchorAvatarUrl())==Second);
    const uint64 OldAvatar=FLiveInteractionTestAdapter::BeginAvatarRequest(*Host,TEXT("anchor"),SecondUrl);
    Host->SetAnchorProfile(Provider->Session,TEXT("Renamed streamer"),TEXT(""));
    TestTrue(TEXT("Profile can explicitly remove the old avatar URL"),Host->GetAnchorAvatarUrl().IsEmpty());
    TestFalse(TEXT("Removed avatar cannot complete an earlier download"),FLiveInteractionTestAdapter::CompleteAvatarRequest(*Host,TEXT("anchor"),OldAvatar));
    TestNull(TEXT("An empty URL cannot resolve a previous cached portrait"),Host->GetCachedAvatar(Host->GetAnchorAvatarUrl()));
    const auto OldSession=Provider->Session;
    const uint64 OldSessionAvatar=FLiveInteractionTestAdapter::BeginAvatarRequest(*Host,TEXT("anchor"),SecondUrl);
    Provider->Session=Host->BeginProviderSession(*Provider,TEXT("app"),TEXT("new-room"),TEXT("other-anchor"));
    TestFalse(TEXT("New session rejects the prior session thumbnail ticket"),FLiveInteractionTestAdapter::CompleteAvatarRequest(*Host,TEXT("anchor"),OldSessionAvatar));
    TestTrue(TEXT("Session switch clears the previous nickname"),Host->GetAnchorNickname().IsEmpty());
    TestTrue(TEXT("Session switch clears previous avatar identity"),Host->GetAnchorAvatarUrl().IsEmpty());
    TestNull(TEXT("Session switch clears prior avatar cache"),Host->GetCachedAvatar(SecondUrl));
    const int32 BeforeStale=Consumer->AnchorProfiles;
    Host->SetAnchorProfile(OldSession,TEXT("Stale streamer"),FirstUrl);
    TestTrue(TEXT("Stale profile cannot restore a previous room URL"),Host->GetAnchorAvatarUrl().IsEmpty());
    TestEqual(TEXT("Stale profile cannot notify consumers"),Consumer->AnchorProfiles,BeforeStale);
    Host->SetAnchorProfile(Provider->Session,TEXT("Current streamer"),TEXT("http://avatar.invalid/insecure.png"));
    TestTrue(TEXT("Profile retains only URLs accepted by avatar loading"),Host->GetAnchorAvatarUrl().IsEmpty());
    Host->EndProviderSession(*Provider,TEXT("SDK_DISCONNECTED"));
    TestTrue(TEXT("Disconnect clears profile nickname"),Host->GetAnchorNickname().IsEmpty());
    Host->StartPlatform(TEXT("advanced-unregistered"));FLiveInteractionProviderRegistry::Unregister(Provider->GetPlatformId());
    return true;
}
#endif
