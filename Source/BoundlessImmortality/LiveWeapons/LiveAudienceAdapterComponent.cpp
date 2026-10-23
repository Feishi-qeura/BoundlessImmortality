#include "LiveAudienceAdapterComponent.h"
#include "LiveGMTestSession.h"
#include "Misc/CommandLine.h"
#include "LiveWeaponSwarm.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Dom/JsonObject.h"
#include "Misc/App.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationPrototypeClient.h"
namespace {
bool CultivationGift(const FString& Name,ECultivationGift& Gift)
{
    static const TCHAR* Names[]={TEXT("仙女棒"),TEXT("爱的爆炸"),TEXT("稀有宝箱"),TEXT("超能喷射"),TEXT("神秘空投"),TEXT("草莓甜点"),TEXT("超级空投"),TEXT("生命药水")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Names);++I)if(Name==Names[I]){Gift=ECultivationGift(I);return true;}return false;
}
}
ULiveAudienceAdapterComponent::ULiveAudienceAdapterComponent(){PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickInterval=.1f;}
int32 ULiveAudienceAdapterComponent::ParseCommand(const FString& Text)
{
    const FString Command=Text.TrimStartAndEnd();if(Command==TEXT("加入"))return 0;
    if(Command.Len()==1&&Command[0]>='1'&&Command[0]<='4')return Command[0]-'0';return -1;
}
void ULiveAudienceAdapterComponent::Bind(ALiveWeaponSwarm* InSwarm,ULiveInteractionSubsystem* InHost)
{
    Unbind();Swarm=InSwarm;Host=InHost?InHost:(GetWorld()&&GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>():nullptr);
    if(!Host)return;Host->OnComment.AddDynamic(this,&ThisClass::HandleComment);Host->OnGift.AddDynamic(this,&ThisClass::HandleGift);
    Host->OnPresence.AddDynamic(this,&ThisClass::HandlePresence);Host->OnLike.AddDynamic(this,&ThisClass::HandleLike);
    Host->OnFollow.AddDynamic(this,&ThisClass::HandleFollow);Host->OnTeamSelection.AddDynamic(this,&ThisClass::HandleTeam);
    if(GetWorld()&&GetWorld()->GetGameInstance()){Cultivation=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();SharedClient=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationPrototypeClient>();}
    Host->OnSessionChanged.AddDynamic(this,&ThisClass::HandleSessionChanged);Host->OnAvatarReady.AddDynamic(this,&ThisClass::HandleAvatar);
    Host->OnAnchorProfileChanged.AddDynamic(this,&ThisClass::HandleAnchorProfileChanged);
    ReplyHandle=Host->OnCommandReply.AddUObject(this,&ThisClass::OnCommandReply);HandleSessionChanged();
}
void ULiveAudienceAdapterComponent::Unbind()
{
    if(Host){Host->OnComment.RemoveAll(this);Host->OnGift.RemoveAll(this);Host->OnPresence.RemoveAll(this);Host->OnLike.RemoveAll(this);Host->OnFollow.RemoveAll(this);Host->OnTeamSelection.RemoveAll(this);Host->OnSessionChanged.RemoveAll(this);Host->OnAnchorProfileChanged.RemoveAll(this);Host->OnAvatarReady.RemoveAll(this);Host->OnCommandReply.Remove(ReplyHandle);}
    Host=nullptr;Swarm=nullptr;Cultivation=nullptr;SharedClient=nullptr;Avatars.Reset();AvatarAtlas=nullptr;AvatarTiles.Reset();FreeAvatarTiles.Reset();NextAvatarTile=0;RawIds.Reset();AvatarPending.Reset();AvatarRetryAt.Reset();AvatarLastUsed.Reset();PendingAcks.Reset();PendingCultivationGifts.Reset();RoomKey.Reset();bPresented=false;
}
void ULiveAudienceAdapterComponent::HandleSessionChanged()
{
    Avatars.Reset();AvatarAtlas=nullptr;AvatarTiles.Reset();FreeAvatarTiles.Reset();NextAvatarTile=0;AvatarPending.Reset();AvatarRetryAt.Reset();AvatarLastUsed.Reset();bPresented=false;
    for(auto& Ack:PendingAcks){Ack.RequestId.Reset();Ack.RetryAt=0;}
    if(!Host)return;const auto& S=Host->GetCurrentSession();if(!S.Nonce.IsValid()){if(SharedClient)SharedClient->SuspendRoom();return;}
    const FString NewKey=S.PlatformId+TEXT("|")+S.AppId+TEXT("|")+S.RoomId+FString::FromInt(S.bLocalTest?1:0);
    if(!RoomKey.IsEmpty()&&RoomKey!=NewKey){if(Swarm)Swarm->ResetParticipants();if(Cultivation)Cultivation->LeaveRoomParticipants();RawIds.Reset();PendingAcks.Reset();PendingCultivationGifts.Reset();}
    if(Cultivation)
    {
        Cultivation->SetRoomContext(S.RoomId,S.bLocalTest);
        const FString AnchorId=S.AnchorUserId.IsEmpty()?TEXT("anchor"):Host->GetScopedUserId(S.AnchorUserId);
        Cultivation->SetAnchor(Swarm?Swarm->GetAnchor():nullptr,AnchorId,Host->GetAnchorNickname().IsEmpty()?TEXT("主播"):Host->GetAnchorNickname());
        HandleAnchorProfileChanged();
        if(SharedClient){SharedClient->BeginRoom(S);SharedClient->TrackParticipant(AnchorId,false);}
    }
    for(auto& Ack:PendingAcks)Ack.Session=S;
    RoomKey=NewKey;
}
void ULiveAudienceAdapterComponent::HandleAnchorProfileChanged()
{
    if(!Host || !Cultivation)return;
    const auto& Session=Host->GetCurrentSession();
    if(!Session.Nonce.IsValid())return;
    const FString AnchorId=Session.AnchorUserId.IsEmpty()?TEXT("anchor"):Host->GetScopedUserId(Session.AnchorUserId);
    const FString Name=Host->GetAnchorNickname().IsEmpty()?TEXT("主播"):Host->GetAnchorNickname();
    Cultivation->RegisterParticipant(AnchorId,Name,Host->GetAnchorAvatarUrl(),true);
}
bool ULiveAudienceAdapterComponent::JoinCultivator(const FString& RawId,const FString& Name,const FString& Avatar)
{
    if(!Host||!Swarm)return false;const FString Id=Host->GetScopedUserId(RawId);if(Id.IsEmpty())return false;
    if(Cultivation&&Id==Cultivation->GetAnchorId())
    {
        const FString AnchorAvatar=Host->GetAnchorAvatarUrl().IsEmpty()?(Avatar.IsEmpty()?Cultivation->GetParticipantSnapshot(Id).AvatarUrl:Avatar):Host->GetAnchorAvatarUrl();
        const FString AnchorName=Host->GetAnchorNickname().IsEmpty()?Name:Host->GetAnchorNickname();
        Cultivation->RegisterParticipant(Id,AnchorName,AnchorAvatar,true);if(SharedClient)SharedClient->TrackParticipant(Id);return true;
    }
    const auto* Existing=Swarm->GetRoster().Find(Id);const FString Url=Avatar.IsEmpty()&&Existing?Existing->Profile.AvatarUrl:Avatar;
    if(Existing&&Existing->Profile.AvatarUrl!=Url)InvalidateAvatar(Id);
    if(!Swarm->AddParticipant({Id,Name,Url}))return false;RawIds.Add(Id,RawId);
    if(Cultivation)Cultivation->RegisterParticipant(Id,Name,Url);
    if(SharedClient)SharedClient->TrackParticipant(Id);return true;
}
void ULiveAudienceAdapterComponent::HandlePresence(const FLivePresence& E)
{
    if(!Host||!Swarm||!Host->IsEventFromCurrentSession(E.Session))return;
    const FString Id=Host->GetScopedUserId(E.UserId);bool Applied=false;
    if(E.EnterType==1)Applied=JoinCultivator(E.UserId,E.Nickname,E.AvatarUrl);
    else if(E.EnterType==2){Swarm->RemoveParticipant(Id);if(Cultivation)Cultivation->LeaveParticipant(Id);if(SharedClient)SharedClient->LeaveParticipant(Id);InvalidateAvatar(Id);Applied=true;}
    if(Applied)QueueAck(E.Session,E.MessageId,TEXT("live_enter"));
}
void ULiveAudienceAdapterComponent::HandleLike(const FLiveLike& E)
{
    if(!Host||!Host->IsEventFromCurrentSession(E.Session))return;
    if(Cultivation)Cultivation->OnAnnouncement.Broadcast(E.Nickname+TEXT(" 点赞了仙途"));QueueAck(E.Session,E.MessageId,TEXT("live_like"));
}
void ULiveAudienceAdapterComponent::HandleFollow(const FLiveFollow& E)
{
    if(!Host||!Host->IsEventFromCurrentSession(E.Session))return;
    if(Cultivation)Cultivation->OnAnnouncement.Broadcast(E.Nickname+(E.Action==2?TEXT(" 更新了关注状态"):TEXT(" 关注了主播")));QueueAck(E.Session,E.MessageId,TEXT("live_follow"));
}
void ULiveAudienceAdapterComponent::HandleTeam(const FLiveTeamSelection& E)
{
    if(Host&&Host->IsEventFromCurrentSession(E.Session)&&JoinCultivator(E.UserId,E.Nickname,E.AvatarUrl))QueueAck(E.Session,E.MessageId,TEXT("live_team"));
}
void ULiveAudienceAdapterComponent::HandleComment(const FLiveComment& E)
{
    if(!Host||!Swarm||!Host->IsEventFromCurrentSession(E.Session))return;
    const FString Id=Host->GetScopedUserId(E.UserId);if(Id.IsEmpty())return;
    const int32 Command=ParseCommand(E.Content);bool Applied=false;
    const auto* Existing=Swarm->GetRoster().Find(Id);
    if(Command>=0&&(Command==0||Existing))
    {
        const FString Url=E.AvatarUrl.IsEmpty()&&Existing?Existing->Profile.AvatarUrl:E.AvatarUrl;
        if(Existing&&Existing->Profile.AvatarUrl!=Url)InvalidateAvatar(Id);
        Applied=JoinCultivator(E.UserId,E.Nickname,Url);
        if(Applied&&Command>0)Applied=Swarm->SwitchParticipantWeapon(Id,ELiveWeaponType(Command-1));
    }
    if(Applied)QueueAck(E.Session,E.MessageId,TEXT("live_comment"));
}
void ULiveAudienceAdapterComponent::HandleGift(const FLiveGift& E)
{
    ECultivationGift Gift;
    if(bEnableCultivationGifts&&Cultivation&&Host&&Swarm&&Host->IsEventFromCurrentSession(E.Session)&&CultivationGift(E.GiftName,Gift))
    {
        if(E.bIsTestData&&!E.Session.bLocalTest){InteractionFeedback=TEXT("平台测试礼物已隔离：请使用独立开发测试场景");Cultivation->OnAnnouncement.Broadcast(InteractionFeedback);return;}
        if(E.Count<1||E.Count>MAX_int32||PendingCultivationGifts.Num()>=4096)return;
        if(!JoinCultivator(E.UserId,E.Nickname,TEXT("")))return;
        if(!PendingCultivationGifts.ContainsByPredicate([&](const auto& P){return P.Event.MessageId==E.MessageId&&P.Event.UserId==E.UserId;}))
        {
            FCultivationPendingGift P;P.Event=E;P.ReceivedUtc=UCultivationSubsystem::UtcNow();PendingCultivationGifts.Add(P);
            if(SharedClient){FCultivationGiftEvent G;G.ParticipantId=Host->GetScopedUserId(E.UserId);G.MessageId=E.MessageId;G.GiftId=Gift;G.Count=int32(E.Count);G.EventUtcSeconds=P.ReceivedUtc;G.bDevelopment=E.Session.bLocalTest;SharedClient->QueueGiftForRecovery(G);}
        }
        ProcessCultivationGifts();return;
    }
    if(!bEnableSessionGiftUnlocks||!Host||!Swarm||!Host->IsEventFromCurrentSession(E.Session))return;
    const FString Id=Host->GetScopedUserId(E.UserId);if(!Swarm->GetRoster().Find(Id))return;
    for(const auto& Rule:GiftUnlocks)if(Rule.GiftName==E.GiftName)
    {
        bool Applied=false;for(FName Skill:Rule.SkillIds)Applied|=Swarm->GrantSkill(Id,Skill);
        if(Applied)QueueAck(E.Session,E.MessageId,TEXT("live_gift"));break;
    }
}
void ULiveAudienceAdapterComponent::ProcessCultivationGifts()
{
    if(!Cultivation||!Host)return;
    for(int32 I=0;I<PendingCultivationGifts.Num();)
    {
        auto& Pending=PendingCultivationGifts[I];const auto& E=Pending.Event;
        if(!Host->IsEventFromCurrentSession(E.Session)){PendingCultivationGifts.RemoveAt(I);continue;}
        const FString Id=Host->GetScopedUserId(E.UserId);
        if(!Pending.bApplied&&(!SharedClient||SharedClient->CanProcessGift(Id,E.MessageId)))
        {
            FCultivationGiftEvent G;G.ParticipantId=Id;G.MessageId=E.MessageId;G.Count=int32(E.Count);G.EventUtcSeconds=Pending.ReceivedUtc;G.bDevelopment=E.Session.bLocalTest;
            CultivationGift(E.GiftName,G.GiftId);
            Pending.bApplied=SharedClient?SharedClient->RegisterGiftReceipt(G):Cultivation->ProcessGift(G);
        }
        if(Pending.bApplied&&(!SharedClient||SharedClient->IsGiftDurable(Id,E.MessageId))){QueueAck(E.Session,E.MessageId,TEXT("live_gift"));PendingCultivationGifts.RemoveAt(I);}
        else ++I;
    }
}
bool ULiveAudienceAdapterComponent::SimulateComment(const FString& Id,const FString& Name,const FString& Text)
{
#if !UE_BUILD_SHIPPING
    return Host&&Host->IsDevelopmentSimulation()&&Host->SimulateComment(Id,Name,Text);
#else
    return false;
#endif
}
void ULiveAudienceAdapterComponent::AddSimulatedAudience(int32 Count)
{
#if !UE_BUILD_SHIPPING
    if(!Host||!Host->IsDevelopmentSimulation())return;Count=FMath::Clamp(Count,0,1000);
    for(int I=0;I<Count;++I){const FString Id=FString::Printf(TEXT("viewer_%04d"),I);const FString Name=FString::Printf(TEXT("观众%04d"),I+1);SimulateComment(Id,Name,TEXT("加入"));SimulateComment(Id,Name,FString::FromInt(I%4+1));}
#endif
}
bool ULiveAudienceAdapterComponent::EnableDevelopmentSimulation()
{
    return Host&&Host->TryEnableDevelopmentSimulation();
}
int32 ULiveAudienceAdapterComponent::AppendSimulatedAudience(int32 Count)
{
    int32 Added=0;
#if !UE_BUILD_SHIPPING
    if(!Host||!Host->IsDevelopmentSimulation()||!Swarm)return 0;
    Count=FMath::Clamp(Count,0,FMath::Max(0,1000-Swarm->GetParticipantCount()));
    for(int32 I=0;I<1000&&Added<Count;++I)
    {
        const FString RawId=FString::Printf(TEXT("viewer_%04d"),I);
        if(const auto* Existing=Swarm->GetRoster().Find(TEXT("local:")+RawId);Existing&&Existing->bParticipating)continue;
        if(SimulateComment(RawId,FString::Printf(TEXT("观众%04d"),I+1),TEXT("加入")))++Added;
    }
#endif
    return Added;
}
bool ULiveAudienceAdapterComponent::ClearSimulatedAudience()
{
#if !UE_BUILD_SHIPPING
    if(!Host||!Host->IsDevelopmentSimulation()||!Swarm)return false;
    Swarm->ResetParticipants();RawIds.Reset();PendingAcks.Reset();HandleSessionChanged();bPresented=false;
    return true;
#else
    return false;
#endif
}
UTexture2D* ULiveAudienceAdapterComponent::GetAvatar(const FString& Id){if(auto* T=Avatars.Find(Id)){AvatarLastUsed.Add(Id,GetWorld()->GetTimeSeconds());return T->Get();}return nullptr;}
void ULiveAudienceAdapterComponent::RequestVisibleAvatar(const FString& Id)
{
    if(!Host||!Swarm||Avatars.Contains(Id)||!RawIds.Contains(Id))return;
    const double Now=GetWorld()->GetTimeSeconds();
    if(Now-AvatarBudgetTime>=.1){AvatarBudgetTime=Now;AvatarBudget=8;}
    if(AvatarBudget<=0)return;
    if(AvatarPending.Contains(Id)||AvatarPending.Num()>=4||AvatarRetryAt.FindRef(Id)>Now)return;
    const auto* S=Swarm->GetRoster().Find(Id);if(!S)return;
#if !UE_BUILD_SHIPPING
    if(Host->IsLocalTestMode()&&S->Profile.AvatarUrl.IsEmpty()&&Id.StartsWith(TEXT("local:viewer_")))
    {
        --AvatarBudget;
        TArray<uint8> Pixels;Pixels.SetNumUninitialized(64*64*4);const uint32 Seed=GetTypeHash(Id);
        for(int Y=0;Y<64;++Y)for(int X=0;X<64;++X)
        {
            const int P=(Y*64+X)*4;const bool Eye=(FMath::Square(X-22)+FMath::Square(Y-25)<9)||(FMath::Square(X-42)+FMath::Square(Y-25)<9);
            const bool Mouth=Y>39&&Y<43&&X>23&&X<41;const bool Ink=Eye||Mouth;
            Pixels[P]=Ink?30:80+(Seed&127);Pixels[P+1]=Ink?30:80+((Seed>>8)&127);Pixels[P+2]=Ink?30:80+((Seed>>16)&127);Pixels[P+3]=255;
        }
        AvatarPending.Add(Id,Now+12);
        HandleAvatar(RawIds[Id],UTexture2D::CreateTransient(64,64,PF_B8G8R8A8,NAME_None,TConstArrayView64<uint8>(Pixels.GetData(),Pixels.Num())));return;
    }
#endif
    if(S->Profile.AvatarUrl.IsEmpty())return;
    --AvatarBudget;AvatarPending.Add(Id,Now+12);AvatarRetryAt.Add(Id,Now+30);Host->RequestAvatar(RawIds[Id],S->Profile.AvatarUrl);
}
void ULiveAudienceAdapterComponent::HandleAvatar(const FString& RawId,UTexture2D* Texture)
{
    if(!Host||!Texture)return;const FString Id=Host->GetScopedUserId(RawId);if(!AvatarPending.Contains(Id))return;
    AvatarPending.Remove(Id);
    int32 Tile=INDEX_NONE;
    if(Avatars.Num()>=1024){FString Old;double Time=MAX_dbl;for(const auto& Pair:AvatarLastUsed)if(Pair.Value<Time){Time=Pair.Value;Old=Pair.Key;}Tile=AvatarTiles.FindRef(Old);Avatars.Remove(Old);AvatarTiles.Remove(Old);AvatarLastUsed.Remove(Old);}
    if(Tile==INDEX_NONE)Tile=FreeAvatarTiles.IsEmpty()?NextAvatarTile++:FreeAvatarTiles.Pop();
    if(!AvatarAtlas)
    {
        TArray64<uint8> Blank;Blank.SetNumZeroed(2048*2048*4);
        AvatarAtlas=UTexture2D::CreateTransient(2048,2048,PF_B8G8R8A8,NAME_None,TConstArrayView64<uint8>(Blank.GetData(),Blank.Num()));
    }
    if(AvatarAtlas&&Texture->GetSizeX()==64&&Texture->GetSizeY()==64&&Texture->GetPlatformData()&&!Texture->GetPlatformData()->Mips.IsEmpty())
    {
        auto& Bulk=Texture->GetPlatformData()->Mips[0].BulkData;
        if(Bulk.GetBulkDataSize()>=64*64*4)
        {
            auto* Pixels=new uint8[64*64*4];const void* Source=Bulk.LockReadOnly();FMemory::Memcpy(Pixels,Source,64*64*4);Bulk.Unlock();
            auto* Region=new FUpdateTextureRegion2D((Tile%32)*64,(Tile/32)*64,0,0,64,64);
            AvatarAtlas->UpdateTextureRegions(0,1,Region,64*4,4,Pixels,[](uint8* Data,const FUpdateTextureRegion2D* R){delete[] Data;delete R;});
            AvatarTiles.Add(Id,Tile);
        }
    }
    Avatars.Add(Id,Texture);AvatarLastUsed.Add(Id,GetWorld()->GetTimeSeconds());
}
void ULiveAudienceAdapterComponent::InvalidateAvatar(const FString& Id)
{
    if(const auto* Tile=AvatarTiles.Find(Id))FreeAvatarTiles.Add(*Tile);
    AvatarTiles.Remove(Id);Avatars.Remove(Id);AvatarPending.Remove(Id);AvatarRetryAt.Remove(Id);AvatarLastUsed.Remove(Id);
}
bool ULiveAudienceAdapterComponent::GetAvatarUV(const FString& Id,FVector2D& UV)const
{
    const auto* Tile=AvatarTiles.Find(Id);if(!Tile||!AvatarAtlas)return false;UV=FVector2D((*Tile%32)/32.f,(*Tile/32)/32.f);return true;
}
void ULiveAudienceAdapterComponent::QueueAck(const FLiveSession& Session,const FString& Id,const FString& Type)
{
    if(!Host||!Host->IsConnected()||PendingAcks.Num()>=4096)return;
    FLivePendingAck Ack;Ack.Session=Session;Ack.MessageId=Id;Ack.Type=Type;PendingAcks.Add(MoveTemp(Ack));bPresented=false;
}
void ULiveAudienceAdapterComponent::NotifyPresented(){if(FApp::CanEverRender())bPresented=true;}
void ULiveAudienceAdapterComponent::OnCommandReply(const FLiveCommandReply& Reply)
{
    for(int I=PendingAcks.Num()-1;I>=0;--I)if(PendingAcks[I].RequestId==Reply.RequestId)
    {
        if(Reply.bSuccess)PendingAcks.RemoveAtSwap(I);else{PendingAcks[I].RequestId.Reset();PendingAcks[I].RetryAt=GetWorld()->GetTimeSeconds()+2;}break;
    }
}
void ULiveAudienceAdapterComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Delta,Type,ThisTick);const double Now=GetWorld()->GetTimeSeconds();
    ProcessCultivationGifts();
    if(SharedClient&&Swarm)for(const auto& Entry:RawIds)if(SharedClient->IsParticipantRevoked(Entry.Key))Swarm->RemoveParticipant(Entry.Key);
    for(auto It=AvatarPending.CreateIterator();It;++It)if(It.Value()<Now)It.RemoveCurrent();
    if(!Host||!Host->IsConnected()||!bPresented)return;
    int32 Submitted=0;
    for(auto& Ack:PendingAcks)
    {
        if(!Ack.RequestId.IsEmpty()||Ack.RetryAt>Now)continue;
        auto Payload=MakeShared<FJsonObject>();Payload->SetStringField(TEXT("msg_id"),Ack.MessageId);Payload->SetStringField(TEXT("msg_type"),Ack.Type);
        Ack.RequestId=Host->SubmitCommand(TEXT("ack"),Payload);Ack.RetryAt=Now+2;if(++Submitted>=16)break;
    }
}
FString ULiveAudienceAdapterComponent::GetStatus()const{return !InteractionFeedback.IsEmpty()?InteractionFeedback:Host?(Host->IsDevelopmentSimulation()?TEXT("本地模拟"):Host->ConnectionStatus):TEXT("直播未初始化");}
void ULiveAudienceAdapterComponent::EndPlay(const EEndPlayReason::Type Reason){Unbind();Super::EndPlay(Reason);}
