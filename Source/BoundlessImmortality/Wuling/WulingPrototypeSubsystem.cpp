#include "WulingPrototypeSubsystem.h"
#include "WulingWorldTarget.h"
#include "WulingSnapshotProjection.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
FString Encode(const TSharedPtr<FJsonObject>& O){FString S;if(O)FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&S));return S;}
FString Str(const TSharedPtr<FJsonObject>& O,const TCHAR* K){FString V;if(O)O->TryGetStringField(K,V);return V;}
double Num(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;if(O)O->TryGetNumberField(K,V);return V;}
TSharedPtr<FJsonObject> Obj(const TSharedPtr<FJsonObject>& O,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(K,V)?*V:nullptr;}
const TArray<TSharedPtr<FJsonValue>>& Arr(const TSharedPtr<FJsonObject>& O,const TCHAR* K){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return O&&O->TryGetArrayField(K,V)?*V:Empty;}
FString ErrorText(const FString& Code)
{
    static TMap<FString,FString> Messages={{TEXT("STALE_VERSION"),TEXT("状态已更新，正在重新确认")},{TEXT("ALREADY_MEMBER"),TEXT("已经有正式武灵归属")},
        {TEXT("REJOIN_WAIT"),TEXT("退出后需等待24小时再加入")},{TEXT("INSUFFICIENT_FUNDS"),TEXT("灵石或破境石不足")},
        {TEXT("INSUFFICIENT_QI"),TEXT("修为尚不足以突破")},{TEXT("NAME_REJECTED"),TEXT("武灵名需为2至12个汉字")},
        {TEXT("FEATURE_DISABLED"),TEXT("该功能尚未开放")},{TEXT("ACTIVITY_BUSY"),TEXT("请先结束当前历练")},
        {TEXT("ASSET_EXPIRED"),TEXT("本原型成长窗口已结束")},{TEXT("ALREADY_CREATED"),TEXT("已经创建武灵")},
        {TEXT("REGION_NOT_OPEN"),TEXT("该区域尚未开放，请选择青云山、落霞谷或玄铁岭")},
        {TEXT("GATHER_ROUTE_REQUIRED"),TEXT("药田封印未解，请选择采药路线")},{TEXT("WEAKPOINT_CLOSED"),TEXT("请先破障，开启矿脉弱点")}};
    if(const auto* Found=Messages.Find(Code))return *Found;
    return TEXT("操作未确认：")+Code;
}
}

void UWulingPrototypeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
    bEnabled=FParse::Param(FCommandLine::Get(),TEXT("WulingPrototype"))&&FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest"));
    FParse::Value(FCommandLine::Get(),TEXT("WulingUrl="),Endpoint);
    FParse::Value(FCommandLine::Get(),TEXT("WulingApp="),App);
    FParse::Value(FCommandLine::Get(),TEXT("WulingRoom="),Room);
    FParse::Value(FCommandLine::Get(),TEXT("WulingUser="),User);
    FString Port=Endpoint;Port.RemoveFromStart(TEXT("http://127.0.0.1:"));int32 N=0;
    bool Valid=Endpoint.StartsWith(TEXT("http://127.0.0.1:"))&&!Port.IsEmpty();
    for(TCHAR C:Port)Valid&=C>='0'&&C<='9';
    Valid&=LexTryParseString(N,*Port)&&N>=1024&&N<=65535;
    if(!Valid){bEnabled=false;Status=TEXT("武灵服务地址无效（仅允许本机端口）");}
#endif
    if(!bEnabled)return;
    Collection.InitializeDependency<ULiveInteractionSubsystem>();
    auto* Host=GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>();
    Host->OnComment.AddDynamic(this,&ThisClass::HandleComment);Host->OnPresence.AddDynamic(this,&ThisClass::HandlePresence);Host->OnSessionChanged.AddDynamic(this,&ThisClass::HandleSessionChanged);
    Outbox=FPaths::ProjectSavedDir()/TEXT("WulingPrototype/v1.4")/(FMD5::HashAnsiString(*(App+TEXT("|prototype_full|wuling-prototype-v1.4.0|")+Room+TEXT("|")+User))+TEXT(".json"));
    LoadOutbox();Status=TEXT("封闭原型 · 等待进入游戏");
}
void UWulingPrototypeSubsystem::Deinitialize()
{
    bEnding=true;++Generation;TribulationPresentation.Clear();EffectPresentation.Clear();
    if(Request){Request->CancelRequest();Request.Reset();}
    if(auto* Host=GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>())
    {Host->OnComment.RemoveDynamic(this,&ThisClass::HandleComment);Host->OnPresence.RemoveDynamic(this,&ThisClass::HandlePresence);Host->OnSessionChanged.RemoveDynamic(this,&ThisClass::HandleSessionChanged);}
    for(auto& Pair:Targets)if(auto* T=Pair.Value.Get())T->Destroy();Targets.Reset();
    Super::Deinitialize();
}
bool UWulingPrototypeSubsystem::IsTickable()const{return bEnabled&&!bEnding&&!HasAnyFlags(RF_ClassDefaultObject)&&GetWorld()&&GetWorld()->IsGameWorld();}
TStatId UWulingPrototypeSubsystem::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UWulingPrototypeSubsystem,STATGROUP_Tickables);}
UWorld* UWulingPrototypeSubsystem::GetTickableGameObjectWorld()const{return GetWorld();}
FString UWulingPrototypeSubsystem::GetSnapshotJson()const{return Encode(State);}
FString UWulingPrototypeSubsystem::TakeNextRewardReceiptJson(){return RewardReceipts.TakeNext();}
bool UWulingPrototypeSubsystem::IsBusy()const
{
    return Commands.ContainsByPredicate([this](const FCommand& C){const auto* P=Participants.Find(C.Actor);return C.Actor==User||(P&&P->bPresent);});
}
bool UWulingPrototypeSubsystem::IsWorldActive()const{const auto Round=Obj(State,TEXT("round"));const auto S=Str(Round,TEXT("status"));return bEnabled&&bGameplayActive&&Round&&S!=TEXT("Completed")&&S!=TEXT("Interrupted");}
void UWulingPrototypeSubsystem::SetGameplayActive(bool Active)
{
    if(bGameplayActive==Active)return;
    ++Generation;if(Request){Request->CancelRequest();Request.Reset();}
    bGameplayActive=Active;Poll=10;if(!Active)for(auto& Pair:Participants)Pair.Value.bPresent=false;
    ReconcileTargets();
}
void UWulingPrototypeSubsystem::RequestMemberPage(int32 Cursor){MemberCursor=FMath::Max(0,Cursor);bNeedMemberPage=true;Poll=10;}

bool UWulingPrototypeSubsystem::Enqueue(const FString& Operation,const TSharedRef<FJsonObject>& Args,const FString& Actor,const FString& SourceEvent)
{
    if(!bEnabled||!bGameplayActive||Commands.Num()>=128)return false;
    FCommand C;C.Actor=Actor.IsEmpty()?User:Actor;C.Data=MakeShared<FJsonObject>();
    const FString Id=SourceEvent.IsEmpty()?FGuid::NewGuid().ToString(EGuidFormats::Digits):FMD5::HashAnsiString(*(Room+TEXT("|")+C.Actor+TEXT("|")+SourceEvent+TEXT("|")+Operation));
    if(Commands.ContainsByPredicate([&](const FCommand& Pending){return Str(Pending.Data,TEXT("request_id"))==Id;}))return true;
    C.Data->SetStringField(TEXT("request_id"),Id);C.Data->SetStringField(TEXT("profile"),TEXT("prototype_full"));
    C.Data->SetStringField(TEXT("rule_version"),TEXT("wuling-prototype-v1.4.0"));C.Data->SetStringField(TEXT("command"),Operation);C.Data->SetObjectField(TEXT("args"),Args);
    Commands.Add(C);if(!SaveOutbox()){Commands.Pop();Status=TEXT("本机请求记录写入失败，操作未发送");return false;}Poll=10;return true;
}
bool UWulingPrototypeSubsystem::CreateGuild(const FString& Name){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("name"),Name);return !IsBusy()&&Enqueue(TEXT("CreateGuild"),A);}
bool UWulingPrototypeSubsystem::StartWorldRound(const FString& Preset,const FString& Route,const FString& RegionId){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("preset"),Preset);A->SetStringField(TEXT("route"),Route);A->SetStringField(TEXT("region_id"),RegionId);return !IsBusy()&&Enqueue(TEXT("StartWorldRound"),A);}
bool UWulingPrototypeSubsystem::InterruptWorldRound(){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("round_id"),Str(Obj(State,TEXT("round")),TEXT("id")));return Enqueue(TEXT("InterruptWorldRound"),A);}
bool UWulingPrototypeSubsystem::RespondToRegionMechanic()
{
    const auto Round=Obj(State,TEXT("round"));const FString Kind=Str(Obj(Round,TEXT("region_mechanic")),TEXT("kind"));const int32 Stage=int32(Num(Round,TEXT("stage")));
    if(!bConnected||IsBusy()||Str(Round,TEXT("status"))!=TEXT("Running"))return false;
    auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("round_id"),Str(Round,TEXT("id")));
    if(Kind==TEXT("gather_route")&&Stage==1){A->SetStringField(TEXT("action"),TEXT("route"));A->SetStringField(TEXT("choice"),TEXT("gather"));}
    else if(Kind==TEXT("weakpoint")&&Stage==2)A->SetStringField(TEXT("action"),TEXT("破障"));
    else return false;
    return Enqueue(TEXT("SubmitAction"),A);
}
bool UWulingPrototypeSubsystem::Breakthrough(){return !IsBusy()&&Enqueue(TEXT("Breakthrough"),MakeShared<FJsonObject>());}
bool UWulingPrototypeSubsystem::Craft(const FString& RecipeId){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("recipe_id"),RecipeId);return !IsBusy()&&Enqueue(TEXT("Craft"),A);}
bool UWulingPrototypeSubsystem::PrepareBreakthrough(const FString& Json)
{
    TArray<TSharedPtr<FJsonValue>> Selection;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Selection))return false;
    auto A=MakeShared<FJsonObject>();A->SetArrayField(TEXT("selection"),Selection);return !IsBusy()&&Enqueue(TEXT("PrepareBreakthrough"),A);
}
bool UWulingPrototypeSubsystem::CancelBreakthrough(){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("attempt_id"),Str(Obj(State,TEXT("tribulation")),TEXT("id")));return !IsBusy()&&Enqueue(TEXT("CancelBreakthrough"),A);}
bool UWulingPrototypeSubsystem::SignIn(){return !IsBusy()&&Enqueue(TEXT("SignIn"),MakeShared<FJsonObject>());}
bool UWulingPrototypeSubsystem::QueueRace(const FString& NodeId){auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("node_id"),NodeId);return !IsBusy()&&Enqueue(TEXT("QueueRace"),A);}
bool UWulingPrototypeSubsystem::StartLaunchEvent(){return !IsBusy()&&Enqueue(TEXT("StartLaunchEvent"),MakeShared<FJsonObject>());}
bool UWulingPrototypeSubsystem::SubmitPrototypeCommand(const FString& Operation,const FString& Json)
{
    static const TSet<FString> Allowed={TEXT("AcknowledgeOfflineReward"),TEXT("ClaimSignInChest"),TEXT("RenameGuild"),TEXT("RemoveMember"),TEXT("DissolveGuild"),TEXT("Recycle"),TEXT("Unequip"),TEXT("Equip"),TEXT("LearnMethod"),TEXT("ConfigureMethods"),TEXT("ActivateFabao"),TEXT("UseProp"),TEXT("ActivateMethod"),TEXT("Build"),TEXT("Purchase"),TEXT("CreateListing"),TEXT("BuyListing"),TEXT("CancelListing"),TEXT("BindMaterial"),TEXT("ChooseFocus"),TEXT("SubmitRaceAction"),TEXT("CancelRaceQueue"),TEXT("ReworkQuality"),TEXT("QuoteReworkQuality"),TEXT("QuoteBreakthrough"),TEXT("StartActivity"),TEXT("EndActivity"),TEXT("ReconcileBreakthrough")};
    TSharedPtr<FJsonObject> A;if(!Allowed.Contains(Operation)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),A)||!A)return false;
    return !IsBusy()&&Enqueue(Operation,A.ToSharedRef());
}
bool UWulingPrototypeSubsystem::RequestCatalog(const FString& Kind,int32 Offset,bool OwnedOnly)
{auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("kind"),Kind);A->SetNumberField(TEXT("offset"),FMath::Max(0,Offset));A->SetBoolField(TEXT("owned"),OwnedOnly);return !IsBusy()&&Enqueue(TEXT("QueryCatalog"),A);}
bool UWulingPrototypeSubsystem::SubmitStrike(const FString& Participant,const FString& Target)
{
    if(!bConnected||!IsWorldActive())return false;
    FString Actor=Participant;
    auto* C=GetGameInstance()->GetSubsystem<UCultivationSubsystem>();
    if(Participant.IsEmpty()||(C&&Participant==C->GetAnchorId()))Actor=User;
    if(Actor!=User&&(!Participants.Contains(Actor)||!Participants[Actor].bPresent))return false;
    const double Now=FPlatformTime::Seconds();if(Now-LastStrike.FindRef(Actor)<1.02)return false;
    if(Commands.ContainsByPredicate([&](const FCommand& Cmd){return Cmd.Actor==Actor&&Str(Cmd.Data,TEXT("command"))==TEXT("SubmitAction");}))return false;
    auto A=MakeShared<FJsonObject>();A->SetStringField(TEXT("action"),TEXT("strike"));A->SetStringField(TEXT("target_id"),Target);A->SetStringField(TEXT("round_id"),Str(Obj(State,TEXT("round")),TEXT("id")));
    if(!Enqueue(TEXT("SubmitAction"),A,Actor))return false;LastStrike.Add(Actor,Now);return true;
}

void UWulingPrototypeSubsystem::Tick(float Delta)
{
    Poll+=Delta;if(Request||FPlatformTime::Seconds()<RetryAt)return;
    if(!bGameplayActive&&Token.IsEmpty())return;
    if(Token.IsEmpty())
    {
        auto A=MakeShared<FJsonObject>();A->SetBoolField(TEXT("development_only"),true);A->SetBoolField(TEXT("simulated"),true);A->SetStringField(TEXT("profile"),TEXT("prototype_full"));
        A->SetStringField(TEXT("app"),App);A->SetStringField(TEXT("room"),Room);A->SetStringField(TEXT("user"),User);A->SetStringField(TEXT("role"),TEXT("anchor"));Post(TEXT("session"),A,User,true);return;
    }
    // Open simulated identities only after a current, session-validated local event.
    for(auto& Pair:Participants)if(Pair.Value.Token.IsEmpty()&&Pair.Value.bPresent)
    {
        auto A=MakeShared<FJsonObject>();A->SetBoolField(TEXT("development_only"),true);A->SetBoolField(TEXT("simulated"),true);A->SetStringField(TEXT("profile"),TEXT("prototype_full"));
        A->SetStringField(TEXT("app"),App);A->SetStringField(TEXT("room"),Room);A->SetStringField(TEXT("user"),Pair.Key);A->SetStringField(TEXT("role"),TEXT("viewer"));A->SetStringField(TEXT("display_name"),Pair.Value.DisplayName);Post(TEXT("session"),A,Pair.Key,true);return;
    }
    // Refresh presence between commands; a large queue must not stop the authority clock.
    if(Poll>=.75)
    {
        auto A=MakeShared<FJsonObject>();A->SetBoolField(TEXT("active"),bGameplayActive);A->SetNumberField(TEXT("cursor"),MemberCursor);TArray<TSharedPtr<FJsonValue>> People;
        for(auto& Pair:Participants)if(!Pair.Value.Token.IsEmpty()){auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("token"),Pair.Value.Token);P->SetBoolField(TEXT("active"),Pair.Value.bPresent&&bGameplayActive);People.Add(MakeShared<FJsonValueObject>(P));}
        A->SetArrayField(TEXT("participants"),People);Post(TEXT("sync"),A,User);Poll=0;return;
    }
    if(!Commands.IsEmpty()&&bGameplayActive)
    {
        const int32 Ready=Commands.IndexOfByPredicate([this](const FCommand& C){const auto* P=Participants.Find(C.Actor);return C.Actor==User||(P&&P->bPresent&&!P->Token.IsEmpty());});
        if(Ready==INDEX_NONE)return;
        if(Ready>0){Commands.Swap(0,Ready);SaveOutbox();}
        auto& C=Commands[0];if(C.Actor!=User&&(!Participants.Contains(C.Actor)||Participants[C.Actor].Token.IsEmpty()))return;
        if(!C.Data->HasField(TEXT("expected_version"))){C.Data->SetNumberField(TEXT("expected_version"),double(Version));if(!SaveOutbox()){Status=TEXT("请求落盘失败，等待重试");RetryAt=FPlatformTime::Seconds()+2;return;}}
        Post(TEXT("command"),C.Data.ToSharedRef(),C.Actor);return;
    }
    if(bNeedMemberPage){auto A=MakeShared<FJsonObject>();A->SetNumberField(TEXT("cursor"),MemberCursor);Post(TEXT("state"),A,User);bNeedMemberPage=false;}
}

void UWulingPrototypeSubsystem::Post(const FString& Route,const TSharedRef<FJsonObject>& Body,const FString& Actor,bool Session)
{
    auto R=FHttpModule::Get().CreateRequest();Request=R;R->SetURL(Endpoint+TEXT("/v1/wuling/")+Route);R->SetVerb(TEXT("POST"));R->SetTimeout(4);
    R->SetHeader(TEXT("Content-Type"),TEXT("application/json"));if(!Session)R->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+(Actor==User?Token:Participants.FindOrAdd(Actor).Token));
    R->SetContentAsString(Encode(Body));
    const TWeakObjectPtr<UWulingPrototypeSubsystem> Weak(this);
    const int64 RequestGeneration=Generation;
    R->OnProcessRequestComplete().BindLambda([Weak,Route,Actor,Session,RequestGeneration](FHttpRequestPtr,FHttpResponsePtr Reply,bool Success)
    {
        auto* Self=Weak.Get();if(!Self||Self->bEnding||Self->Generation!=RequestGeneration)return;Self->Request.Reset();TSharedPtr<FJsonObject> Data;
        if(!Success||!Reply||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Reply->GetContentAsString()),Data)||!Data)
        {Self->bConnected=false;Self->Status=TEXT("武灵服务未连接 · 待确认请求已保留");Self->RetryAt=FPlatformTime::Seconds()+2;Self->ReconcileTargets();return;}
        if(Reply->GetResponseCode()!=200)
        {
            Self->bConnected=false;Self->Status=ErrorText(Str(Data,TEXT("error_code")));Self->RetryAt=FPlatformTime::Seconds()+2;
            if(Reply->GetResponseCode()==401){if(Actor==Self->User)Self->Token.Reset();else Self->Participants.FindOrAdd(Actor).Token.Reset();}Self->ReconcileTargets();return;
        }
        const auto Incoming=Session?Obj(Data,TEXT("state")):(Route==TEXT("command")?Obj(Data,TEXT("result")):Data);
        if(Incoming)
        {
            const auto Manifest=Obj(Incoming,TEXT("manifest"));const FString Hash=Str(Manifest,TEXT("rules_hash"));
            if(Str(Manifest,TEXT("rule_version"))!=TEXT("wuling-prototype-v1.4.0")||Str(Manifest,TEXT("profile"))!=TEXT("prototype_full")||Hash.Len()!=64||(!Self->RulesHash.IsEmpty()&&Self->RulesHash!=Hash))
            {Self->bConnected=false;Self->Status=TEXT("规则版本不匹配 · 操作未确认");Self->RetryAt=FPlatformTime::Seconds()+5;return;}
            Self->RulesHash=Hash;
            // Only this player's authority projection can feed the overlay.
            // A replayed command result is safe even when its state is stale.
            if(Actor==Self->User)Self->RewardReceipts.Observe(Obj(Incoming,TEXT("reward_receipts")));
        }
        Self->bConnected=true;
        if(Session)
        {
            if(Actor==Self->User){Self->Token=Str(Data,TEXT("token"));Self->State=Obj(Data,TEXT("state"));}
            else Self->Participants.FindOrAdd(Actor).Token=Str(Data,TEXT("token"));
            Self->Version=int64(Num(Obj(Data,TEXT("state")),TEXT("version")));Self->Status=TEXT("封闭原型已连接");
        }
        else if(Route==TEXT("command"))
        {
            const FString Error=Str(Data,TEXT("error_code"));Self->Version=FMath::Max(Self->Version,int64(Num(Data,TEXT("authoritative_version"))));
            if(Error==TEXT("STALE_VERSION")&&!Self->Commands.IsEmpty()){Self->Commands[0].Data->RemoveField(TEXT("expected_version"));Self->SaveOutbox();Self->Poll=10;}
            else
            {
                if(Error.IsEmpty()&&Actor==Self->User&&Num(Incoming,TEXT("version"))>=Self->Version)Self->State=Incoming;
                if(!Self->Commands.IsEmpty())Self->Commands.RemoveAt(0);Self->SaveOutbox();
                Self->Poll=10;
            }
            if(Error==TEXT("ACTION_COOLDOWN")||Error==TEXT("TARGET_CLOSED")||Error==TEXT("ROUND_CLOSED"))Self->Status=TEXT("封闭原型已连接");
            else Self->Status=Error.IsEmpty()?TEXT("操作已确认 · 奖励以权威账本为准"):ErrorText(Error);
        }
        else
        {
            Self->State=Data;Self->Version=int64(Num(Data,TEXT("version")));
            for(const auto& V:Arr(Data,TEXT("revoked_participants")))
                if(auto* Person=Self->Participants.Find(V->AsString())){Person->Token.Reset();Person->bPresent=false;}
            if(Self->Status.Contains(TEXT("未连接")))Self->Status=TEXT("武灵服务已恢复");
        }
        if(auto* C=Self->GetGameInstance()->GetSubsystem<UCultivationSubsystem>())WulingSnapshotProjection::Apply(*C,Self->State,Self->User);
        Self->ReconcileTargets();
    });
    if(!R->ProcessRequest()){Request.Reset();bConnected=false;Status=TEXT("武灵服务未连接");RetryAt=FPlatformTime::Seconds()+2;}
}

void UWulingPrototypeSubsystem::ReconcileTargets()
{
    TribulationPresentation.Reconcile(GetWorld(),State,bGameplayActive&&bConnected,Generation);
    EffectPresentation.Reconcile(GetWorld(),State,bGameplayActive&&bConnected);
    TSet<FString> Visible;auto* Pawn=GetWorld()?UGameplayStatics::GetPlayerPawn(GetWorld(),0):nullptr;
    if(IsWorldActive()&&bConnected&&Pawn)
    {
        const auto Round=Obj(State,TEXT("round"));int32 Index=0;
        for(const auto& V:Arr(Round,TEXT("targets")))
        {
            auto O=V->AsObject();const FString Id=Str(O,TEXT("id"));Visible.Add(Id);auto* Target=Targets.FindRef(Id).Get();
            if(!Target)
            {
                const float Angle=Index*PI*.5f;const float Radius=Str(Round,TEXT("route"))==TEXT("gather")?420.f:560.f;
                const FVector Location=Pawn->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,-35);
                const FTransform Transform(FRotator::ZeroRotator,Location);
                Target=GetWorld()->SpawnActorDeferred<AWulingWorldTarget>(AWulingWorldTarget::StaticClass(),Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
                if(Target){Target->TargetId=Id;Target->Authority=this;Target->MaxHealth=float(Num(O,TEXT("max_health")));Target->AttackDamage=0;Target->MoveSpeed=55;Target->FollowTarget=Pawn;Target->FinishSpawning(Transform);Targets.Add(Id,Target);}
            }
            if(Target){bool Locked=false;O->TryGetBoolField(TEXT("mechanic_locked"),Locked);Target->bMechanicLocked=Locked;Target->Mirror(float(Num(O,TEXT("health"))),float(Num(O,TEXT("max_health"))));const double Speed=Num(O,TEXT("move_speed"));if(Speed>0)Target->MoveSpeed=float(Speed);}++Index;
        }
    }
    for(auto It=Targets.CreateIterator();It;++It)if(!Visible.Contains(It.Key())){if(auto* T=It.Value().Get())T->Destroy();It.RemoveCurrent();}
}

bool UWulingPrototypeSubsystem::SaveOutbox()const
{
    auto Root=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& C:Commands){auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("actor"),C.Actor);O->SetObjectField(TEXT("request"),C.Data);Rows.Add(MakeShared<FJsonValueObject>(O));}
    Root->SetArrayField(TEXT("commands"),Rows);IFileManager::Get().MakeDirectory(*FPaths::GetPath(Outbox),true);
    const FString Temp=Outbox+TEXT(".pending");return FFileHelper::SaveStringToFile(Encode(Root),*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)&&IFileManager::Get().Move(*Outbox,*Temp,true,true,false,true);
}
void UWulingPrototypeSubsystem::LoadOutbox()
{
    FString Json;TSharedPtr<FJsonObject> Root;if(!FFileHelper::LoadFileToString(Json,*Outbox)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root))return;
    for(const auto& V:Arr(Root,TEXT("commands"))){auto O=V->AsObject();auto D=Obj(O,TEXT("request"));const FString A=Str(O,TEXT("actor"));if(D&&!A.IsEmpty()&&Commands.Num()<128){Commands.Add({A,D});if(A!=User)Participants.FindOrAdd(A);}}
}
void UWulingPrototypeSubsystem::HandleSessionChanged()
{
    ++Generation;TribulationPresentation.Clear();EffectPresentation.Clear();if(Request){Request->CancelRequest();Request.Reset();}for(auto& Pair:Participants){Pair.Value.Token.Reset();Pair.Value.bPresent=false;}Poll=10;
    auto* Host=GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>();
    if(!Host||!Host->IsDevelopmentSimulation()){SetGameplayActive(false);Status=TEXT("武灵封闭原型等待本机模拟会话");}
}
void UWulingPrototypeSubsystem::HandlePresence(const FLivePresence& P)
{
    auto* Host=GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>();if(!bGameplayActive||!Host->IsDevelopmentSimulation()||!Host->IsEventFromCurrentSession(P.Session))return;
    auto& Person=Participants.FindOrAdd(Host->GetScopedUserId(P.UserId));Person.bPresent=P.EnterType!=2;Person.DisplayName=P.Nickname;Poll=10;
}
void UWulingPrototypeSubsystem::HandleComment(const FLiveComment& C)
{
    auto* Host=GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>();if(!bGameplayActive||!Host->IsDevelopmentSimulation()||!Host->IsEventFromCurrentSession(C.Session))return;
    const FString Actor=Host->GetScopedUserId(C.UserId);auto& Person=Participants.FindOrAdd(Actor);Person.bPresent=true;Person.DisplayName=C.Nickname;
    const FString Content=C.Content.TrimStartAndEnd();auto Args=MakeShared<FJsonObject>();
    if(Content==TEXT("加入")){const FString Guild=Str(Obj(State,TEXT("guild")),TEXT("id"));if(!Guild.IsEmpty()){Args->SetStringField(TEXT("guild_id"),Guild);Enqueue(TEXT("JoinGuild"),Args,Actor,C.MessageId);}}
    else if(Content==TEXT("退出武灵"))Enqueue(TEXT("LeaveGuild"),Args,Actor,C.MessageId);
    else if(Content==TEXT("确认离线收益"))Enqueue(TEXT("AcknowledgeOfflineReward"),Args,Actor,C.MessageId);
    else if(Content==TEXT("展示青云踏飞雁")||Content==TEXT("隐藏荣誉称号")){Args->SetStringField(TEXT("title_id"),Content==TEXT("展示青云踏飞雁")?TEXT("QingyunFeiyan"):TEXT(""));Enqueue(TEXT("SelectHonorTitle"),Args,Actor,C.MessageId);}
    else if(Content==TEXT("助力")){Args->SetStringField(TEXT("attempt_id"),Str(Obj(State,TEXT("tribulation")),TEXT("id")));Args->SetStringField(TEXT("event_id"),C.MessageId);Enqueue(TEXT("AssistTribulation"),Args,Actor,C.MessageId);}
    else if(Content==TEXT("护阵")||Content==TEXT("破障")||Content==TEXT("共修"))
    {
        const auto Race=Obj(State,TEXT("race"));Args->SetStringField(TEXT("action"),Content);
        if(Str(Race,TEXT("status"))==TEXT("Running")){Args->SetStringField(TEXT("race_id"),Str(Race,TEXT("id")));Args->SetStringField(TEXT("kind"),Content==TEXT("共修")?TEXT("representative"):TEXT("common"));Enqueue(TEXT("SubmitRaceAction"),Args,Actor,C.MessageId);}
        else{Args->SetStringField(TEXT("round_id"),Str(Obj(State,TEXT("round")),TEXT("id")));Enqueue(TEXT("SubmitAction"),Args,Actor,C.MessageId);}
    }
    else if(Content==TEXT("路线护阵")||Content==TEXT("路线采集")){Args->SetStringField(TEXT("action"),TEXT("route"));Args->SetStringField(TEXT("choice"),Content==TEXT("路线护阵")?TEXT("guard"):TEXT("gather"));Args->SetStringField(TEXT("round_id"),Str(Obj(State,TEXT("round")),TEXT("id")));Enqueue(TEXT("SubmitAction"),Args,Actor,C.MessageId);}
    Poll=10;
}
