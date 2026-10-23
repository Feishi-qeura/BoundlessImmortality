#include "CultivationPrototypeClient.h"
#include "CultivationSubsystem.h"
#include "LiveInteractionSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "HAL/CriticalSection.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace {
FString Text(const TSharedPtr<FJsonObject>& O,const TCHAR* K){FString V;if(O)O->TryGetStringField(K,V);return V;}
double Number(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;if(O)O->TryGetNumberField(K,V);return V;}
FString Encode(const TSharedRef<FJsonObject>& O){FString S;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&S));return S;}
const TArray<TSharedPtr<FJsonValue>>& Array(const TSharedPtr<FJsonObject>& O,const TCHAR* K){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* A=nullptr;return O&&O->TryGetArrayField(K,A)?*A:Empty;}
TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(K,V)?*V:nullptr;}
FString ReceiptKey(const FString& Id,const FString& Message){return Id+TEXT("|")+Message;}
TSharedPtr<FJsonObject> ReadOutbox(const FString& Path)
{
    FString Json;TSharedPtr<FJsonObject> Data;if(FFileHelper::LoadFileToString(Json,*Path))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Data);return Data;
}
bool WriteOutbox(const FString& Path,const TSharedRef<FJsonObject>& Data)
{
    const FString Pending=Path+TEXT(".pending");IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    return FFileHelper::SaveStringToFile(Encode(Data),*Pending,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)&&IFileManager::Get().Move(*Path,*Pending,true,true,false,true);
}
FString OutboxMutex(const FString& Path){return TEXT("CultivationOutbox_")+FMD5::HashAnsiString(*FPaths::ConvertRelativePathToFull(Path));}
}
void UCultivationPrototypeClient::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);Collection.InitializeDependency<UCultivationSubsystem>();Cultivation=GetGameInstance()->GetSubsystem<UCultivationSubsystem>();
    ClientId=FGuid::NewGuid().ToString(EGuidFormats::Digits);
#if !UE_BUILD_SHIPPING
    bEnabled=FParse::Param(FCommandLine::Get(),TEXT("CultivationPrototypeService"));
    if(FParse::Param(FCommandLine::Get(),TEXT("WulingPrototype"))&&FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest")))bEnabled=false;
    FString Configured;if(FParse::Value(FCommandLine::Get(),TEXT("CultivationPrototypeUrl="),Configured))Endpoint=Configured;
    // Exact numeric loopback authority, optional port. No URL credentials, path, query, redirects or DNS.
    FString Port=Endpoint;Port.RemoveFromStart(TEXT("http://127.0.0.1:"));int32 PortNumber=0;
    bool Valid=Endpoint.StartsWith(TEXT("http://127.0.0.1:"))&&!Port.IsEmpty();
    for(TCHAR C:Port)Valid&=C>='0'&&C<='9';
    Valid&=LexTryParseString(PortNumber,*Port)&&PortNumber>=1024&&PortNumber<=65535;
    if(!Valid){bEnabled=false;Status=TEXT("共享原型地址无效（仅允许本机端口）");}
#endif
    if(bEnabled)Status=TEXT("共享原型等待房间");
    if(Cultivation)DamageHandle=Cultivation->OnActualDamage.AddUObject(this,&ThisClass::RecordDamage);
}
void UCultivationPrototypeClient::BeginRoom(const FLiveSession& Session)
{
    if(!bEnabled)return;
    const FString NewNamespace=Session.bLocalTest?TEXT("development"):TEXT("live-prototype");
    FString NewRoom=Session.RoomId;
    if(Session.bLocalTest){FString LocalRoom;if(FParse::Value(FCommandLine::Get(),TEXT("CultivationPrototypeRoom="),LocalRoom)&&!LocalRoom.IsEmpty())NewRoom=LocalRoom;}
    if(RoomId==NewRoom&&AppId==Session.AppId&&Namespace==NewNamespace){bSuspended=false;Elapsed=10;if(Cultivation&&Session.bLocalTest)Cultivation->SetRoomContext(NewRoom,true);return;}bSuspended=false;
    ++Generation;if(Request){Request->CancelRequest();Request.Reset();}
    Token.Reset();Ownership.Reset();PendingGifts.Reset();DurableReceipts.Reset();ReadyGifts.Reset();Joins.Reset();Leaves.Reset();Damage.Reset();RetryPayload.Reset();Leaderboards.Reset();
    RoomId=NewRoom;AppId=Session.AppId;Namespace=NewNamespace;bConnected=false;Elapsed=10;Status=TEXT("共享原型连接中");
    if(Cultivation&&Session.bLocalTest)Cultivation->SetRoomContext(RoomId,true);
    LoadOutbox();if(Cultivation)Cultivation->SetRoomPotionSources({});
}
void UCultivationPrototypeClient::SuspendRoom()
{
    if(!bEnabled)return;++Generation;if(Request){Request->CancelRequest();Request.Reset();}bSuspended=true;bConnected=false;Status=TEXT("直播连接中断，共享原型已暂停");
}
void UCultivationPrototypeClient::TrackParticipant(const FString& Id,bool bAllowReclaim)
{
    if(!bEnabled||Id.IsEmpty())return;
    auto& Owner=Ownership.FindOrAdd(Id);
    if(Owner.bRevoked&&!bAllowReclaim)return;
    if(!Owner.bReady||!Owner.bPresent){Owner.bRevoked=false;Owner.JoinedUtc=UCultivationSubsystem::UtcNow();Joins.Add(Id);LoadParticipantOutboxes(Id);}Owner.bPresent=true;
    Leaves.Remove(Id);Elapsed=10;
}
void UCultivationPrototypeClient::LeaveParticipant(const FString& Id){if(bEnabled){Leaves.Add(Id,++PresenceIntentSequence);if(auto* Owner=Ownership.Find(Id))Owner->bPresent=false;Elapsed=10;}}
bool UCultivationPrototypeClient::CanProcessGift(const FString& Id,const FString& MessageId)const
{
    if(!bEnabled)return true;
    const auto* Owner=Ownership.Find(Id);
    const auto* Waiting=PendingGifts.Find(Id);
    return bConnected&&Owner&&Owner->bReady&&!Owner->bRevoked&&(!Waiting||Waiting->ContainsByPredicate([&](const auto& Gift){return Gift.MessageId==MessageId;}));
}
bool UCultivationPrototypeClient::RegisterGiftReceipt(const FCultivationGiftEvent& Gift)
{
    if(!bEnabled)return Cultivation&&Cultivation->ProcessGift(Gift);
    if(!QueueGiftForRecovery(Gift)||!Cultivation||!Cultivation->ProcessGift(Gift))return false;
    ReadyGifts.Add(ReceiptKey(Gift.ParticipantId,Gift.MessageId));return true;
}
bool UCultivationPrototypeClient::QueueGiftForRecovery(const FCultivationGiftEvent& Gift)
{
    if(!bEnabled)return true;
    auto& Gifts=PendingGifts.FindOrAdd(Gift.ParticipantId);
    if(!Gifts.ContainsByPredicate([&](const auto& G){return G.MessageId==Gift.MessageId;})){auto Pending=Gift;if(Pending.SourceRoomId.IsEmpty())Pending.SourceRoomId=RoomId;Gifts.Add(MoveTemp(Pending));}
    Elapsed=10;return SaveOutbox();
}
bool UCultivationPrototypeClient::IsGiftDurable(const FString& Id,const FString& MessageId)const{return !bEnabled||DurableReceipts.Contains(ReceiptKey(Id,MessageId));}
bool UCultivationPrototypeClient::IsParticipantRevoked(const FString& Id)const{const auto* Owner=Ownership.Find(Id);return bEnabled&&Owner&&Owner->bRevoked;}
void UCultivationPrototypeClient::RecordDamage(const FString& Id,double Amount,double Utc)
{
    if(!bEnabled||Damage.Num()>=4096||!FMath::IsFinite(Amount)||Amount<=0)return;
    auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("participant_id"),Id);E->SetStringField(TEXT("event_id"),FGuid::NewGuid().ToString(EGuidFormats::Digits));E->SetNumberField(TEXT("timestamp"),Utc);E->SetNumberField(TEXT("amount"),Amount);Damage.Add(MakeShared<FJsonValueObject>(E));
}
TSharedRef<FJsonObject> UCultivationPrototypeClient::BuildSync()
{
    auto Payload=MakeShared<FJsonObject>();TSharedPtr<FJsonObject> Export;
    if(!Cultivation||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Cultivation->ExportRecords()),Export))return Payload;
    TArray<TSharedPtr<FJsonValue>> JoinValues,WriteValues,LeaveValues;
    const auto Participants=Cultivation->GetParticipants();const auto& AllRecords=Array(Export,TEXT("records"));
    const int32 Count=AllRecords.Num();int32 Visited=0;
    for(;Visited<Count&&JoinValues.Num()+WriteValues.Num()<128;++Visited)
    {
        const auto& V=AllRecords[(SyncCursor+Visited)%Count];
        auto R=V->AsObject();const FString Id=Text(R,TEXT("participant_id"));
        if(Joins.Contains(Id))
        {
            auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("participant_id"),Id);J->SetNumberField(TEXT("observed_utc"),Ownership.FindChecked(Id).JoinedUtc);J->SetBoolField(TEXT("present"),Ownership.FindChecked(Id).bPresent);J->SetObjectField(TEXT("record"),R);JoinValues.Add(MakeShared<FJsonValueObject>(J));
        }
        else if(const auto* Owner=Ownership.Find(Id);Owner&&Owner->bReady&&!Owner->bRevoked)
        {
            auto W=MakeShared<FJsonObject>();W->SetStringField(TEXT("participant_id"),Id);W->SetStringField(TEXT("lease"),Owner->Lease);W->SetNumberField(TEXT("base_version"),double(Owner->Version));W->SetObjectField(TEXT("record"),R);
            TArray<TSharedPtr<FJsonValue>> Receipts;
            if(auto* Gifts=PendingGifts.Find(Id))for(const auto& Gift:*Gifts)if(ReadyGifts.Contains(ReceiptKey(Id,Gift.MessageId))){auto G=MakeShared<FJsonObject>();G->SetStringField(TEXT("message_id"),Gift.MessageId);G->SetStringField(TEXT("source_room"),Gift.SourceRoomId.IsEmpty()?RoomId:Gift.SourceRoomId);G->SetNumberField(TEXT("gift_id"),int32(Gift.GiftId));G->SetNumberField(TEXT("count"),Gift.Count);Receipts.Add(MakeShared<FJsonValueObject>(G));}
            W->SetArrayField(TEXT("receipts"),Receipts);
            const auto Snapshot=Cultivation->GetParticipantSnapshot(Id);auto Metric=MakeShared<FJsonObject>();Metric->SetNumberField(TEXT("speed"),Snapshot.EffectiveCultivationPerSecond);
            Metric->SetNumberField(TEXT("sword_count"),Snapshot.bIsAnchor?Participants.Num():1);W->SetObjectField(TEXT("metrics"),Metric);WriteValues.Add(MakeShared<FJsonValueObject>(W));
        }
    }
    if(Count>0)SyncCursor=(SyncCursor+FMath::Max(1,Visited))%Count;
    auto LeaveIntents=MakeShared<FJsonObject>();for(const auto& Leave:Leaves){LeaveValues.Add(MakeShared<FJsonValueString>(Leave.Key));LeaveIntents->SetNumberField(Leave.Key,double(Leave.Value));}
    Payload->SetArrayField(TEXT("joins"),JoinValues);Payload->SetArrayField(TEXT("writes"),WriteValues);Payload->SetArrayField(TEXT("leaves"),LeaveValues);Payload->SetObjectField(TEXT("leave_intents"),LeaveIntents);Payload->SetArrayField(TEXT("damage"),Damage);return Payload;
}
void UCultivationPrototypeClient::Send(const FString& Route,const TSharedRef<FJsonObject>& Payload,bool bSession)
{
    Request=FHttpModule::Get().CreateRequest();Request->SetURL(Endpoint+Route);Request->SetVerb(TEXT("POST"));Request->SetTimeout(5);Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
    if(!bSession)Request->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+Token);
    const FString Body=Encode(Payload);if(FTCHARToUTF8(*Body).Length()>4*1024*1024){Request.Reset();bConnected=false;Status=TEXT("共享原型请求超过容量");return;}
    Request->SetContentAsString(Body);const int64 SentGeneration=Generation;const TWeakObjectPtr<ThisClass> Weak(this);
    Request->OnRequestProgress64().BindLambda([](FHttpRequestPtr R,uint64,uint64 Received){if(Received>4*1024*1024)R->CancelRequest();});
    Request->OnProcessRequestComplete().BindLambda([Weak,SentGeneration,bSession,Payload](FHttpRequestPtr R,FHttpResponsePtr Response,bool Success)
    {
        if(!Weak.IsValid()||Weak->Generation!=SentGeneration)return;Weak->Request.Reset();
        TSharedPtr<FJsonObject> Data;
        const bool Valid=Success&&Response&&Response->GetResponseCode()==200&&Response->GetContent().Num()<=4*1024*1024&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response->GetContentAsString()),Data)&&Data;
        if(!Valid){Weak->bConnected=false;Weak->Status=TEXT("共享原型未连接，本地状态保留待重试");if(Response&&Response->GetResponseCode()==401)Weak->Token.Reset();return;}
        Weak->Receive(Data,bSession,Payload);
    });
    if(!Request->ProcessRequest()){Request.Reset();bConnected=false;Status=TEXT("共享原型未连接");}
}
void UCultivationPrototypeClient::Receive(const TSharedPtr<FJsonObject>& Data,bool bSession,const TSharedPtr<FJsonObject>& Submitted)
{
    if(bSession){Token=Text(Data,TEXT("token"));if(Token.IsEmpty()||Token.Len()>256){Status=TEXT("共享原型会话无效");return;}Elapsed=10;return;}
    bool Prototype=false;if(!Data->TryGetBoolField(TEXT("prototype"),Prototype)||!Prototype||Number(Data,TEXT("schema"))!=1)return;
    bConnected=true;Status=TEXT("全服开发原型已连接（本机共享）");
    TSharedPtr<FJsonObject> Current;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Cultivation->ExportRecords()),Current);
    for(const auto& Value:Array(Data,TEXT("participants")))
    {
        auto P=Value->AsObject();const FString Id=Text(P,TEXT("participant_id"));auto* Owner=Ownership.Find(Id);if(!Owner)continue;
        // A newer presence owns this identity's next state transition. Older
        // replies may confirm receipts, but cannot revoke or import over it.
        const bool CurrentJoin=Array(Submitted,TEXT("joins")).ContainsByPredicate([&](const auto& V){const auto J=V->AsObject();return Text(J,TEXT("participant_id"))==Id&&Number(J,TEXT("observed_utc"))==Owner->JoinedUtc;});
        if(Joins.Contains(Id)&&!CurrentJoin){for(const auto& Receipt:Array(P,TEXT("receipts")))DurableReceipts.Add(Receipt->AsString());continue;}
        if(Text(P,TEXT("status"))==TEXT("revoked")){for(const auto& Receipt:Array(P,TEXT("receipts")))DurableReceipts.Add(Receipt->AsString());Owner->bRevoked=true;Owner->bReady=false;Cultivation->LeaveParticipant(Id);continue;}
        Owner->Version=int64(Number(P,TEXT("server_version")));Owner->Lease=Text(P,TEXT("lease"));
        // A previous write/leave response must not consume a newly queued resume
        // join, nor may an older join response consume a later presence event.
        if(Joins.Contains(Id)&&CurrentJoin)
        {
            auto Record=Object(P,TEXT("record"));if(!Record)continue;
            // A join is an explicit authority handoff. Import only this identity and never a late room response.
            double LocalVersion=0;for(const auto& R:Array(Current,TEXT("records")))if(Text(R->AsObject(),TEXT("participant_id"))==Id)LocalVersion=Number(R->AsObject(),TEXT("version"));
            Record->SetNumberField(TEXT("version"),FMath::Max(LocalVersion,Number(Record,TEXT("version")))+1);const auto Identity=Cultivation->GetParticipantSnapshot(Id);Record->SetBoolField(TEXT("is_anchor"),Identity.bIsAnchor);
            if(!Identity.DisplayName.IsEmpty())Record->SetStringField(TEXT("display_name"),Identity.DisplayName);if(!Identity.AvatarUrl.IsEmpty())Record->SetStringField(TEXT("avatar_url"),Identity.AvatarUrl);
            auto Envelope=MakeShared<FJsonObject>();Envelope->SetNumberField(TEXT("schema"),1);Envelope->SetBoolField(TEXT("development"),Cultivation->IsDevelopmentContext());Envelope->SetArrayField(TEXT("records"),{MakeShared<FJsonValueObject>(Record)});Envelope->SetArrayField(TEXT("receipts"),Array(P,TEXT("receipts")));
            if(!Cultivation->ImportAuthoritativeRecords(Encode(Envelope)))continue;
            const auto Restored=Cultivation->GetParticipantSnapshot(Id);
            const FString NoticeKey=Namespace+TEXT("|")+AppId+TEXT("|")+RoomId+TEXT("|")+Id;
            if(Restored.bPresent&&Restored.PotionCount>0&&Restored.PotionExpiresUtc>UCultivationSubsystem::UtcNow()&&!PotionRestorationNotices.Contains(NoticeKey))
            {
                PotionRestorationNotices.Add(NoticeKey);
                // RegisterParticipant already reports locally known entitlements.
                // Inspect the imported profile before replaying new pending gifts.
                if(Identity.PotionCount<=0||Identity.PotionExpiresUtc<=UCultivationSubsystem::UtcNow())Cultivation->OnAnnouncement.Broadcast(Restored.DisplayName+TEXT(" 的生命药水效果已恢复"));
            }
            if(const auto* Gifts=PendingGifts.Find(Id))for(const auto& Gift:*Gifts){if(Cultivation->ProcessGift(Gift))ReadyGifts.Add(ReceiptKey(Id,Gift.MessageId));else Status=TEXT("共享原型恢复礼物失败，保留重试记录");}
            Joins.Remove(Id);Owner->bReady=true;
        }
        for(const auto& Receipt:Array(P,TEXT("receipts")))DurableReceipts.Add(Receipt->AsString());
    }
    for(const auto& Receipt:Array(Data,TEXT("accepted_receipts")))
    {
        const FString Key=Receipt->AsString();DurableReceipts.Add(Key);ReadyGifts.Remove(Key);
        for(auto It=PendingGifts.CreateIterator();It;++It){It.Value().RemoveAll([&](const auto& Gift){return ReceiptKey(Gift.ParticipantId,Gift.MessageId)==Key;});if(It.Value().IsEmpty())It.RemoveCurrent();}
    }
    const auto SubmittedLeaveIntents=Object(Submitted,TEXT("leave_intents"));
    for(const auto& Left:Array(Submitted,TEXT("leaves"))){const FString Id=Left->AsString();if(const auto* Intent=Leaves.Find(Id);Intent&&double(*Intent)==Number(SubmittedLeaveIntents,*Id))Leaves.Remove(Id);}
    const auto& SentDamage=Array(Submitted,TEXT("damage"));TSet<FString> Done;
    for(const auto& Hit:SentDamage)Done.Add(Text(Hit->AsObject(),TEXT("event_id")));
    Damage.RemoveAll([&](const auto& Hit){return Done.Contains(Text(Hit->AsObject(),TEXT("event_id")));});
    TArray<FCultivationPotionSource> Potions;
    for(const auto& Value:Array(Data,TEXT("room_potions"))){auto P=Value->AsObject();FCultivationPotionSource S;S.ParticipantId=Text(P,TEXT("participant_id"));S.RoomId=Text(P,TEXT("room_id"));S.ExpiresUtcSeconds=Number(P,TEXT("expires_utc"));Potions.Add(S);}Cultivation->SetRoomPotionSources(Potions);
    Leaderboards.Reset();auto Boards=Object(Data,TEXT("leaderboards"));
    for(const TCHAR* Category:{TEXT("Anchor"),TEXT("CultivationSpeed"),TEXT("Damage")})for(const auto& Value:Array(Boards,Category))
    {
        auto R=Value->AsObject();FInkLeaderboardEntry E;E.ParticipantId=Text(R,TEXT("participant_id"));E.DisplayName=FText::FromString(Text(R,TEXT("display_name")));E.Category=FName(Category);E.Rank=int32(Number(R,TEXT("rank")));E.NumericMetric=Number(R,TEXT("value"));E.Metric=FText::FromString(E.Category==TEXT("Anchor")?FString::Printf(TEXT("%.0f 把"),E.NumericMetric):FString::Printf(TEXT("%.2f /秒"),E.NumericMetric));E.AvatarUrl=Text(R,TEXT("avatar_url"));E.TitleId=FName(*Text(R,TEXT("title_id")));E.RealmIndex=int32(Number(R,TEXT("realm_index")));E.Realm=Cultivation->GetRealmName(E.RealmIndex);Leaderboards.Add(E);
    }
    SaveOutbox();ClearConfirmedOutboxes();RetryPayload.Reset();
}
void UCultivationPrototypeClient::Tick(float Delta)
{
    if(!bEnabled||bSuspended||RoomId.IsEmpty()||Request)return;Elapsed+=Delta;if(Elapsed<1)return;Elapsed=0;
    if(Token.IsEmpty())
    {
        auto P=MakeShared<FJsonObject>();P->SetBoolField(TEXT("development_only"),true);P->SetStringField(TEXT("namespace"),Namespace);P->SetStringField(TEXT("app_id"),AppId);P->SetStringField(TEXT("room_id"),RoomId);P->SetStringField(TEXT("client_id"),ClientId);Send(TEXT("/v1/prototype/session"),P,true);
    }
    else {if(!RetryPayload)RetryPayload=BuildSync();Send(TEXT("/v1/prototype/sync"),RetryPayload.ToSharedRef(),false);}
}
bool UCultivationPrototypeClient::IsTickable()const{return !IsTemplate()&&bEnabled;}
UWorld* UCultivationPrototypeClient::GetTickableGameObjectWorld()const{return GetWorld();}
TStatId UCultivationPrototypeClient::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UCultivationPrototypeClient,STATGROUP_Tickables);}
void UCultivationPrototypeClient::Deinitialize(){++Generation;if(Request)Request->CancelRequest();Request.Reset();if(Cultivation)Cultivation->OnActualDamage.Remove(DamageHandle);Token.Reset();Super::Deinitialize();}
FString UCultivationPrototypeClient::OutboxPath()const
{
    const FString Scope=Namespace+TEXT("|")+AppId+TEXT("|")+RoomId;
    return OutboxDirectory()/(TEXT("service-outbox-")+FMD5::HashAnsiString(*Scope)+TEXT(".json"));
}
FString UCultivationPrototypeClient::OutboxDirectory()const{return OutboxDirectoryOverride.IsEmpty()?FPaths::ProjectSavedDir()/TEXT("CultivationPrototype"):OutboxDirectoryOverride;}
bool UCultivationPrototypeClient::SaveOutbox()const
{
    if(!bEnabled||RoomId.IsEmpty())return true;const FString Path=OutboxPath();FSystemWideCriticalSection Lock(OutboxMutex(Path),FTimespan::FromMilliseconds(500));if(!Lock.IsValid())return false;
    auto Root=MakeShared<FJsonObject>();Root->SetStringField(TEXT("app_id"),AppId);Root->SetStringField(TEXT("namespace"),Namespace);Root->SetStringField(TEXT("room_id"),RoomId);
    TSet<FString> Completed;auto Existing=ReadOutbox(Path);for(const auto& V:Array(Existing,TEXT("completed")))Completed.Add(V->AsString());
    for(const auto& V:Array(Existing,TEXT("events"))){auto G=V->AsObject();const FString Key=ReceiptKey(Text(G,TEXT("participant_id")),Text(G,TEXT("message_id")));if(DurableReceipts.Contains(Key))Completed.Add(Key);}
    TArray<TSharedPtr<FJsonValue>> Events;TSet<FString> EventKeys;
    for(const auto& V:Array(Existing,TEXT("events")))
    {
        auto G=V->AsObject();const FString Key=ReceiptKey(Text(G,TEXT("participant_id")),Text(G,TEXT("message_id")));
        if(!Completed.Contains(Key)){Events.Add(V);EventKeys.Add(Key);}
    }
    for(const auto& Pair:PendingGifts)for(const auto& G:Pair.Value)
    {
        const FString Key=ReceiptKey(G.ParticipantId,G.MessageId);if(DurableReceipts.Contains(Key))Completed.Add(Key);if(Completed.Contains(Key)||EventKeys.Contains(Key))continue;
        auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("participant_id"),G.ParticipantId);V->SetStringField(TEXT("message_id"),G.MessageId);V->SetStringField(TEXT("source_room"),G.SourceRoomId.IsEmpty()?RoomId:G.SourceRoomId);V->SetNumberField(TEXT("gift_id"),uint8(G.GiftId));V->SetNumberField(TEXT("count"),G.Count);V->SetNumberField(TEXT("timestamp"),G.EventUtcSeconds);V->SetBoolField(TEXT("development"),G.bDevelopment);Events.Add(MakeShared<FJsonValueObject>(V));
    }
    TArray<TSharedPtr<FJsonValue>> CompletedValues;for(const auto& Key:Completed)CompletedValues.Add(MakeShared<FJsonValueString>(Key));Root->SetArrayField(TEXT("completed"),CompletedValues);Root->SetArrayField(TEXT("events"),Events);return WriteOutbox(Path,Root);
}
void UCultivationPrototypeClient::LoadOutbox()
{
    LoadOutboxData(ReadOutbox(OutboxPath()));
}
void UCultivationPrototypeClient::LoadParticipantOutboxes(const FString& Id)
{
    TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(OutboxDirectory()/TEXT("service-outbox-*.json")),true,false);
    for(const auto& File:Files){auto Data=ReadOutbox(OutboxDirectory()/File);if(Data&&Text(Data,TEXT("app_id"))==AppId&&Text(Data,TEXT("namespace"))==Namespace)LoadOutboxData(Data,Id);}
}
void UCultivationPrototypeClient::LoadOutboxData(const TSharedPtr<FJsonObject>& Data,const FString& OnlyId)
{
    if(!Data)return;for(const auto& V:Array(Data,TEXT("completed"))){const FString Key=V->AsString();if(OnlyId.IsEmpty()||Key.StartsWith(OnlyId+TEXT("|")))DurableReceipts.Add(Key);}
    for(const auto& V:Array(Data,TEXT("events")))
    {
        auto O=V->AsObject();const double Count=Number(O,TEXT("count")),Gift=Number(O,TEXT("gift_id"));if(Count<1||Count>MAX_int32||Gift<0||Gift>7)continue;
        FCultivationGiftEvent G;G.ParticipantId=Text(O,TEXT("participant_id"));G.MessageId=Text(O,TEXT("message_id"));G.SourceRoomId=Text(O,TEXT("source_room"));if(G.SourceRoomId.IsEmpty())G.SourceRoomId=Text(Data,TEXT("room_id"));if(G.SourceRoomId.IsEmpty())G.SourceRoomId=RoomId;
        G.GiftId=ECultivationGift(int32(Gift));G.Count=int32(Count);G.EventUtcSeconds=Number(O,TEXT("timestamp"));O->TryGetBoolField(TEXT("development"),G.bDevelopment);
        if(G.ParticipantId.IsEmpty()||G.MessageId.IsEmpty()||(!OnlyId.IsEmpty()&&OnlyId!=G.ParticipantId)||DurableReceipts.Contains(ReceiptKey(G.ParticipantId,G.MessageId)))continue;
        if(Cultivation->GetParticipantSnapshot(G.ParticipantId).ParticipantId.IsEmpty()){Cultivation->RegisterParticipant(G.ParticipantId,TEXT(""),TEXT(""));Cultivation->LeaveParticipant(G.ParticipantId);}
        auto& Gifts=PendingGifts.FindOrAdd(G.ParticipantId);if(!Gifts.ContainsByPredicate([&](const auto& Pending){return Pending.MessageId==G.MessageId;}))Gifts.Add(G);
        auto& Owner=Ownership.FindOrAdd(G.ParticipantId);if(Owner.JoinedUtc<=0)Owner.JoinedUtc=G.EventUtcSeconds;Joins.Add(G.ParticipantId);
    }
}
void UCultivationPrototypeClient::ClearConfirmedOutboxes()const
{
    if(DurableReceipts.IsEmpty())return;TArray<FString> Files;IFileManager::Get().FindFiles(Files,*(OutboxDirectory()/TEXT("service-outbox-*.json")),true,false);
    for(const auto& File:Files)
    {
        const FString Path=OutboxDirectory()/File;FSystemWideCriticalSection Lock(OutboxMutex(Path),FTimespan::FromMilliseconds(500));if(!Lock.IsValid())continue;
        auto Data=ReadOutbox(Path);if(!Data||Text(Data,TEXT("app_id"))!=AppId||Text(Data,TEXT("namespace"))!=Namespace)continue;
        TSet<FString> Completed;for(const auto& V:Array(Data,TEXT("completed")))Completed.Add(V->AsString());TArray<TSharedPtr<FJsonValue>> Keep;bool Changed=false;
        for(const auto& V:Array(Data,TEXT("events"))){auto G=V->AsObject();const FString Key=ReceiptKey(Text(G,TEXT("participant_id")),Text(G,TEXT("message_id")));if(DurableReceipts.Contains(Key)){Completed.Add(Key);Changed=true;}else Keep.Add(V);}
        if(Changed){TArray<TSharedPtr<FJsonValue>> Values;for(const auto& Key:Completed)Values.Add(MakeShared<FJsonValueString>(Key));Data->SetArrayField(TEXT("completed"),Values);Data->SetArrayField(TEXT("events"),Keep);WriteOutbox(Path,Data.ToSharedRef());}
    }
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCultivationOutboxTransferTest,"BoundlessImmortality.Cultivation.SharedOutboxRoomRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCultivationOutboxTransferTest::RunTest(const FString&)
{
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Codex/LiveCultivation20261020/OutboxUnit")/FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const auto MakeClient=[&]()
    {
        auto* GI=NewObject<UGameInstance>();auto* C=NewObject<UCultivationSubsystem>(GI);C->bPersistLocalRecords=false;
        auto* Client=NewObject<UCultivationPrototypeClient>(GI);Client->Cultivation=C;Client->bEnabled=true;Client->OutboxDirectoryOverride=Folder;return Client;
    };
    FLiveSession A;A.AppId=TEXT("outbox-unit-app");A.RoomId=TEXT("room-a");A.bLocalTest=false;
    auto* First=MakeClient();First->BeginRoom(A);First->TrackParticipant(TEXT("unit-user"));
    FCultivationGiftEvent Gift;Gift.ParticipantId=TEXT("unit-user");Gift.MessageId=TEXT("pending-before-move");Gift.GiftId=ECultivationGift::LifePotion;Gift.Count=2;Gift.EventUtcSeconds=100;Gift.bDevelopment=false;
    TestTrue(TEXT("gift event is durable before local grant"),First->QueueGiftForRecovery(Gift));const FString OriginalPath=First->OutboxPath();
    FLiveSession B=A;B.RoomId=TEXT("room-b");auto* Second=MakeClient();Second->BeginRoom(B);Second->TrackParticipant(TEXT("unit-user"));
    const auto* Restored=Second->PendingGifts.Find(TEXT("unit-user"));TestTrue(TEXT("new process room restores old room pending event"),Restored&&Restored->Num()==1);
    if(Restored&&Restored->Num()==1){TestEqual(TEXT("original room retained for receipt dedupe"),(*Restored)[0].SourceRoomId,A.RoomId);TestEqual(TEXT("UTC never restarts during transfer"),(*Restored)[0].EventUtcSeconds,100.0);TestEqual(TEXT("all gifted units retained"),(*Restored)[0].Count,2);}
    // Same-process room handoff must use the identical durable recovery route.
    First->BeginRoom(B);First->TrackParticipant(TEXT("unit-user"));TestTrue(TEXT("same process handoff also restores pending event"),First->PendingGifts.Contains(TEXT("unit-user")));
    auto* Stale=MakeClient();Stale->BeginRoom(A); // Keeps a pre-confirmation in-memory snapshot in the source room.
    const auto OfflineRecovery=Stale->BuildSync();
    const auto& OfflineJoins=Array(OfflineRecovery,TEXT("joins"));TestEqual(TEXT("absent outbox owner still queues durable recovery"),OfflineJoins.Num(),1);
    if(OfflineJoins.Num()==1){bool Present=true;OfflineJoins[0]->AsObject()->TryGetBoolField(TEXT("present"),Present);TestFalse(TEXT("outbox-only recovery does not assert online presence"),Present);}
    Second->DurableReceipts.Add(ReceiptKey(Gift.ParticipantId,Gift.MessageId));Second->PendingGifts.Reset();TestTrue(TEXT("new owner saves completed outbox"),Second->SaveOutbox());Second->ClearConfirmedOutboxes();
    TestTrue(TEXT("source file drained after shared confirmation"),Array(ReadOutbox(OriginalPath),TEXT("events")).IsEmpty());
    TestTrue(TEXT("old instance may safely save stale snapshot"),Stale->SaveOutbox());TestTrue(TEXT("completion tombstone prevents resurrected gift"),Array(ReadOutbox(OriginalPath),TEXT("events")).IsEmpty());
    Second->Ownership.FindChecked(Gift.ParticipantId).bRevoked=true;
    Second->BeginRoom(B);Second->TrackParticipant(Gift.ParticipantId,false);
    TestTrue(TEXT("automatic menu/session restoration cannot reclaim a moved identity"),Second->IsParticipantRevoked(Gift.ParticipantId));
    Second->TrackParticipant(Gift.ParticipantId,true);
    TestFalse(TEXT("explicit new presence may request a fresh identity claim"),Second->IsParticipantRevoked(Gift.ParticipantId));
    auto OldReply=MakeShared<FJsonObject>();OldReply->SetNumberField(TEXT("schema"),1);OldReply->SetBoolField(TEXT("prototype"),true);
    auto OldParticipant=MakeShared<FJsonObject>();OldParticipant->SetStringField(TEXT("participant_id"),Gift.ParticipantId);OldParticipant->SetStringField(TEXT("lease"),TEXT("old-write-lease"));OldParticipant->SetNumberField(TEXT("server_version"),1);
    auto OldRecord=MakeShared<FJsonObject>();OldRecord->SetStringField(TEXT("participant_id"),Gift.ParticipantId);OldParticipant->SetObjectField(TEXT("record"),OldRecord);
    OldReply->SetArrayField(TEXT("participants"),{MakeShared<FJsonValueObject>(OldParticipant)});
    // Simulate a write/leave request already in flight when Start queues re-entry.
    Second->Receive(OldReply,false,MakeShared<FJsonObject>());
    TestTrue(TEXT("late pre-menu write response preserves newer resume join"),Second->Joins.Contains(Gift.ParticipantId));
    Second->Cultivation->RegisterParticipant(Gift.ParticipantId,TEXT("new-presence"),TEXT(""));
    OldParticipant->SetStringField(TEXT("status"),TEXT("revoked"));
    OldParticipant->SetArrayField(TEXT("receipts"),{MakeShared<FJsonValueString>(TEXT("unit-user|old-confirmed-receipt"))});
    Second->Receive(OldReply,false,MakeShared<FJsonObject>());
    TestTrue(TEXT("late revoked response preserves newer join"),Second->Joins.Contains(Gift.ParticipantId));
    TestFalse(TEXT("late revoked response cannot revoke new explicit presence"),Second->IsParticipantRevoked(Gift.ParticipantId));
    TestTrue(TEXT("late revoked response cannot remove new local presence"),Second->Cultivation->GetParticipantSnapshot(Gift.ParticipantId).bPresent);
    TestTrue(TEXT("late revoked response still confirms durable receipts"),Second->DurableReceipts.Contains(TEXT("unit-user|old-confirmed-receipt")));
    // Menu L1 is in flight; resume J2 cancels it; menu L3 must survive L1's reply.
    Second->LeaveParticipant(Gift.ParticipantId);const auto LeaveOne=Second->BuildSync();
    Second->TrackParticipant(Gift.ParticipantId);Second->LeaveParticipant(Gift.ParticipantId);
    auto EmptyReply=MakeShared<FJsonObject>();EmptyReply->SetNumberField(TEXT("schema"),1);EmptyReply->SetBoolField(TEXT("prototype"),true);
    Second->Receive(EmptyReply,false,LeaveOne);
    TestTrue(TEXT("old leave reply cannot clear newer leave intent"),Second->Leaves.Contains(Gift.ParticipantId));
    const auto LeaveThree=Second->BuildSync();
    for(const auto& V:Array(LeaveThree,TEXT("joins"))){bool Present=true;V->AsObject()->TryGetBoolField(TEXT("present"),Present);TestFalse(TEXT("queued resume join reflects latest leave presence"),Present);}
    Second->Receive(EmptyReply,false,LeaveThree);
    TestFalse(TEXT("matching leave confirmation drains its own intent"),Second->Leaves.Contains(Gift.ParticipantId));
    const FString NewPath=Second->OutboxPath();
    for(const FString& Path:{OriginalPath,NewPath,OriginalPath+TEXT(".pending"),NewPath+TEXT(".pending")})if(FPaths::GetPath(Path)==Folder)IFileManager::Get().Delete(*Path,false,true);
    IFileManager::Get().DeleteDirectory(*Folder,false,false);return true;
}
#endif
