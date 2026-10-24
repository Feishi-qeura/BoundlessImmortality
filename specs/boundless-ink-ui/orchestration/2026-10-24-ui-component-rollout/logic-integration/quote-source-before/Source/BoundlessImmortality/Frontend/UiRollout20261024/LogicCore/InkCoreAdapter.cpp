#include "InkCoreAdapter.h"
#include "Frontend/InkFrontendWidget.h"
#include "Cultivation/CultivationPrototypeClient.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Wuling/WulingInventoryProjection.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Serialization/JsonSerializer.h"

namespace
{
TSharedPtr<FJsonObject> CoreAdapterObject(const TSharedPtr<FJsonObject>& O,const TCHAR* Key)
{const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(Key,V)?*V:nullptr;}
FString CoreAdapterString(const TSharedPtr<FJsonObject>& O,const TCHAR* Key)
{FString V;if(O)O->TryGetStringField(Key,V);return V;}
bool CoreAdapterNumber(const TSharedPtr<FJsonObject>& O,const TCHAR* Key,double& Value)
{return O&&O->TryGetNumberField(Key,Value)&&FMath::IsFinite(Value);}
bool CoreAdapterField(const TSharedPtr<FJsonObject>& O,const TCHAR* Key)
{const auto V=O?O->TryGetField(Key):nullptr;return V&&V->Type!=EJson::Null;}
bool CoreAdapterSameCounts(const TMap<FString,int32>& A,const TMap<FString,int32>& B)
{if(A.Num()!=B.Num())return false;for(const auto& P:A){const auto* N=B.Find(P.Key);if(!N||*N!=P.Value)return false;}return true;}
bool CoreAdapterSelectionValid(const TMap<FString,int32>& Selection)
{if(Selection.IsEmpty())return false;for(const auto& P:Selection)if(P.Key.IsEmpty()||!InkCorePolicy::ValidCount(P.Value))return false;return true;}
TArray<TSharedPtr<FJsonValue>> CoreAdapterSelectionRows(const TMap<FString,int32>& Selection)
{
    TArray<FString> Keys;Selection.GetKeys(Keys);Keys.Sort();TArray<TSharedPtr<FJsonValue>> Rows;
    for(const auto& Key:Keys){auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("item_id"),Key);O->SetNumberField(TEXT("count"),Selection[Key]);Rows.Add(MakeShared<FJsonValueObject>(O));}
    return Rows;
}
FString CoreAdapterJson(const TSharedRef<FJsonObject>& Object)
{FString Text;FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));return Text;}
FString CoreAdapterSelectionJson(const TMap<FString,int32>& Selection)
{FString Text;FJsonSerializer::Serialize(CoreAdapterSelectionRows(Selection),TJsonWriterFactory<>::Create(&Text));return Text;}
InkCore::FActionResult CoreAdapterReject(const FString& Reason){return {false,false,Reason};}
InkCore::FActionResult CoreAdapterSubmitted(bool Accepted,const UWulingPrototypeSubsystem* W)
{return {Accepted,Accepted,Accepted?FString(TEXT("请求已记录，等待服务确认")):(W?W->GetStatus():FString(TEXT("服务不可用")))};}
bool CoreAdapterActiveAttempt(const TSharedPtr<FJsonObject>& T)
{const auto Phase=CoreAdapterString(T,TEXT("status"));return Phase==TEXT("Prepare")||Phase==TEXT("Trial")||Phase==TEXT("Reconcile");}
bool CoreAdapterChance(const TSharedPtr<FJsonObject>& O)
{
    double V=0;
    return (CoreAdapterNumber(O,TEXT("tribulation_chance"),V)&&V>=0&&V<=100)||
        (CoreAdapterNumber(O,TEXT("probability_basis_points"),V)&&V>=0&&V<=10000);
}
bool CoreAdapterInventoryRowsValid(const TArray<TSharedPtr<FJsonValue>>& Rows)
{
    TSet<FGuid> Ids;
    for(const auto& Value:Rows)
    {
        const auto O=Value&&Value->Type==EJson::Object?Value->AsObject():nullptr;FGuid Id;double Count=0,Expiry=0;
        if(!O||!CoreAdapterObject(O,TEXT("definition"))||!FGuid::Parse(CoreAdapterString(O,TEXT("id")),Id)||Ids.Contains(Id)||CoreAdapterString(O,TEXT("item")).IsEmpty()||
            !CoreAdapterNumber(O,TEXT("count"),Count)||!InkCorePolicy::ValidCount(Count,MAX_int32)||!CoreAdapterNumber(O,TEXT("expiry"),Expiry)||Expiry<=0||Expiry>253402300799.)return false;
        Ids.Add(Id);
    }
    return true;
}
bool CoreAdapterDispatchReady(const UUserWidget* Context,bool Active)
{
    if(!InkCorePolicy::CanDispatch(InkCore::Transport(Context,Active)))return false;
    auto* W=InkCore::Service(Context);
    return CoreAdapterString(CoreAdapterObject(W->Snapshot(),TEXT("identity")),TEXT("role"))==TEXT("anchor");
}
}

UWulingPrototypeSubsystem* InkCore::Service(const UUserWidget* Context)
{return Context&&Context->GetGameInstance()?Context->GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;}
InkCorePolicy::FTransport InkCore::Transport(const UUserWidget* Context,bool Active)
{auto* W=Service(Context);return {W!=nullptr,W&&W->IsEnabled(),W&&W->IsConnected(),W&&W->IsBusy(),Active};}

InkCore::FView InkCore::Read(UInkFrontendWidget* Frontend,bool Active)
{
    FView V;V.Transport=Transport(Frontend,Active);auto* W=Service(Frontend);V.WulingAuthority=W&&W->IsEnabled();
    V.CanAct=V.WulingAuthority?CoreAdapterDispatchReady(Frontend,Active):Frontend&&Active;
    V.Status=W?W->GetStatus():FString(TEXT("服务不可用"));if(!Frontend)return V;
    auto* Model=Frontend->GetInventoryModel();
    if(V.WulingAuthority)
    {
        const auto S=V.Transport.Connected?W->Snapshot():nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Items=nullptr;
        const bool HasItems=S&&S->TryGetArrayField(TEXT("item_instances"),Items);
        const bool ValidItems=HasItems&&CoreAdapterInventoryRowsValid(*Items);
        if(ValidItems&&Model)WulingInventoryProjection::Apply(*Model,S);
        V.InventoryState=InkCorePolicy::State(V.Transport,HasItems,HasItems&&!Items->IsEmpty(),S&&S->HasField(TEXT("item_instances"))&&!ValidItems);
        if(ValidItems&&Model)V.Inventory=Model->GetEntries();
        V.Guild=CoreAdapterObject(S,TEXT("guild"));V.Growth=CoreAdapterObject(S,TEXT("growth"));V.Quote=CoreAdapterObject(S,TEXT("breakthrough_quote"));V.Tribulation=CoreAdapterObject(S,TEXT("tribulation"));
        V.GuildState=InkCorePolicy::State(V.Transport,S&&S->HasField(TEXT("guild")),V.Guild.IsValid(),CoreAdapterField(S,TEXT("guild"))&&!V.Guild);
        V.CharacterState=InkCorePolicy::State(V.Transport,V.Growth.IsValid(),V.Growth.IsValid(),CoreAdapterField(S,TEXT("growth"))&&!V.Growth);
        // Character attributes are supplied by the existing authoritative bridge, never rebuilt from guessed defaults.
        if(!V.Growth)return V;
    }
    else
    {
        // Explicit legacy local mode. Do not allow its model to mutate an old remote projection.
        if(Model&&!Model->IsAuthoritative()){V.Inventory=Model->GetEntries();V.InventoryState=V.Inventory.IsEmpty()?InkCorePolicy::EState::Empty:InkCorePolicy::EState::Data;}
        V.CharacterState=Frontend->GetPresentedCharacterAttributes().IsEmpty()?InkCorePolicy::EState::Loading:InkCorePolicy::EState::Data;
    }
    if(V.CharacterState==InkCorePolicy::EState::Data)
    {V.CharacterName=Frontend->GetPresentedCharacterName();V.Attributes=Frontend->GetPresentedCharacterAttributes();V.Disciplines=Frontend->GetPresentedCharacterDisciplines();if(V.Attributes.IsEmpty())V.CharacterState=InkCorePolicy::EState::Loading;}
    return V;
}

InkCore::FBoardView InkCore::ReadBoard(const UUserWidget* Context,const TArray<FInkLeaderboardEntry>& RoomRows,FName Scope,FName Category,bool RoomDataProvided)
{
    FBoardView V;V.Scope=Scope;V.Category=Category;
    if(Category!=TEXT("Anchor")&&Category!=TEXT("CultivationSpeed")&&Category!=TEXT("Damage"))
    {V.State=InkCorePolicy::EState::Error;V.Status=TEXT("未知榜单类别");return V;}
    TArray<FInkLeaderboardEntry> Rows;
    if(Scope==TEXT("Local"))
    {
        if(!RoomDataProvided){V.State=InkCorePolicy::EState::Loading;V.Status=TEXT("等待房间榜单同步");return V;}
        Rows=RoomRows;V.Status=TEXT("当前房间榜单");
    }
    else if(Scope==TEXT("Global"))
    {
        auto* Shared=Context&&Context->GetGameInstance()?Context->GetGameInstance()->GetSubsystem<UCultivationPrototypeClient>():nullptr;
        V.Status=Shared?Shared->GetStatus():FString(TEXT("共享榜单服务不可用"));
        if(!Shared||!Shared->IsEnabled())return V;
        if(!Shared->IsConnected()){V.State=InkCorePolicy::EState::Offline;return V;}
        Rows=Shared->GetLeaderboardEntries();
    }
    else{V.State=InkCorePolicy::EState::Error;V.Status=TEXT("未知榜单范围");return V;}
    TSet<int32> Ranks;TSet<FString> Ids;
    for(const auto& E:Rows)if(E.Category==Category)
    {
        if(E.ParticipantId.IsEmpty()||E.Rank<1||!FMath::IsFinite(E.NumericMetric)||Ranks.Contains(E.Rank)||Ids.Contains(E.ParticipantId))
        {V.Entries.Reset();V.State=InkCorePolicy::EState::Error;V.Status=TEXT("榜单记录缺失身份、名次或存在重复，等待重新同步");return V;}
        Ranks.Add(E.Rank);Ids.Add(E.ParticipantId);V.Entries.Add(E);
    }
    // Preserve authoritative ranks (including gaps); ordering alone never grants a rank.
    V.Entries.Sort([](const auto& A,const auto& B){return A.Rank<B.Rank;});
    V.State=V.Entries.IsEmpty()?InkCorePolicy::EState::Empty:InkCorePolicy::EState::Data;return V;
}

InkCore::FQuoteDecision InkCore::EvaluateQuote(const TSharedPtr<FJsonObject>& S,const TMap<FString,int32>& Selection,bool Rework)
{
    FQuoteDecision D;const auto Q=CoreAdapterObject(S,TEXT("breakthrough_quote")),T=CoreAdapterObject(S,TEXT("tribulation"));
    const FString Phase=CoreAdapterString(T,TEXT("status"));const bool HasId=!CoreAdapterString(T,TEXT("id")).IsEmpty();double Now=0,Start=0;
    const bool HasNow=CoreAdapterNumber(S,TEXT("server_utc"),Now),HasStart=CoreAdapterNumber(T,TEXT("trial_start"),Start);
    D.CanCancel=InkCorePolicy::CanCancel(Phase==TEXT("Prepare"),HasId,HasNow,Now,HasStart,Start);
    D.CanReconcile=Phase==TEXT("Reconcile")&&HasId;D.Facts.AttemptActive=CoreAdapterActiveAttempt(T);
    D.Facts.HasSelection=CoreAdapterSelectionValid(Selection);
    if(!Q){D.Reason=TEXT("突破报价尚未同步");return D;}
    const auto Readiness=CoreAdapterObject(Q,TEXT("readiness"));bool Ready=false;
    const bool HasReady=Q->TryGetBoolField(TEXT("readiness"),Ready)||(Readiness&&Readiness->TryGetBoolField(TEXT("ready"),Ready));D.Facts.ServiceReady=HasReady&&Ready;
    const bool IsRework=CoreAdapterString(Q,TEXT("kind"))==TEXT("rework");D.Facts.KindMatches=Rework==IsRework;
    TMap<FString,int32> Echo,Slots;const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    bool EchoValid=Q->TryGetArrayField(TEXT("selection"),Rows);
    if(EchoValid)for(const auto& V:*Rows)
    {
        const auto O=V&&V->Type==EJson::Object?V->AsObject():nullptr;const auto Item=CoreAdapterString(O,TEXT("item_id"));double Count=0;
        if(Item.IsEmpty()||!CoreAdapterNumber(O,TEXT("count"),Count)||!InkCorePolicy::ValidCount(Count)||Echo.Contains(Item)){EchoValid=false;break;}
        Echo.Add(Item,int32(Count));
    }
    D.Facts.SelectionMatches=EchoValid&&CoreAdapterSameCounts(Selection,Echo);
    Rows=nullptr;bool SlotsValid=Q->TryGetArrayField(TEXT("slots"),Rows)&&!Rows->IsEmpty();int32 Cores=0;
    if(SlotsValid)for(const auto& V:*Rows)
    {
        const auto O=V&&V->Type==EJson::Object?V->AsObject():nullptr;const auto Item=CoreAdapterString(O,TEXT("item_id"));
        auto Category=CoreAdapterString(O,TEXT("category"));if(Category.IsEmpty())Category=CoreAdapterString(O,TEXT("slot_category"));double Count=0;
        if(Item.IsEmpty()||Category.IsEmpty()||!CoreAdapterNumber(O,O->HasField(TEXT("count"))?TEXT("count"):TEXT("quantity"),Count)||!InkCorePolicy::ValidCount(Count)){SlotsValid=false;break;}
        // Sum in double first so hostile repeated slots cannot overflow an int32.
        const double Sum=double(Slots.FindRef(Item))+Count;if(!InkCorePolicy::ValidCount(Sum)){SlotsValid=false;break;}
        Slots.Add(Item,int32(Sum));if(Category.Equals(TEXT("core"),ESearchCase::IgnoreCase)){Cores+=int32(Count);if(Cores>1){SlotsValid=false;break;}}
    }
    D.Facts.SlotsMatch=SlotsValid&&CoreAdapterSameCounts(Selection,Slots);D.Facts.CoreSlotsValid=SlotsValid&&(Rework?Cores<=1:Cores==1);
    const auto Stage=CoreAdapterObject(Q,TEXT("stage")),Growth=CoreAdapterObject(S,TEXT("growth"));
    FString Current=CoreAdapterString(Q,TEXT("current_stage"));if(Current.IsEmpty())Current=CoreAdapterString(Growth,TEXT("name"));
    FString Target=CoreAdapterString(Q,TEXT("target_stage"));if(Target.IsEmpty())Target=CoreAdapterString(Stage,TEXT("target_stage"));
    double Quality=0,Normalized=0,Min=0,Max=0;
    const bool Numbers=CoreAdapterNumber(Q,TEXT("q"),Quality)&&CoreAdapterNumber(Q,TEXT("q_normalized"),Normalized)&&CoreAdapterNumber(Q,TEXT("growth_min"),Min)&&CoreAdapterNumber(Q,TEXT("growth_max"),Max)&&Min<=Max;
    const TArray<TSharedPtr<FJsonValue>>* Sources=nullptr;const bool HasSources=Q->TryGetArrayField(TEXT("sources"),Sources)&&!Sources->IsEmpty();
    D.Facts.Complete=Numbers&&!Current.IsEmpty()&&!Target.IsEmpty()&&HasReady&&HasSources&&(CoreAdapterField(Q,TEXT("quality"))||CoreAdapterField(Q,TEXT("band"))||!CoreAdapterString(Q,TEXT("quality_band")).IsEmpty())&&(CoreAdapterChance(Q)||CoreAdapterChance(T));
    D.CanSubmit=InkCorePolicy::CanSubmitQuote(D.Facts);
    if(!D.CanSubmit)D.Reason=D.Facts.AttemptActive?TEXT("当前已有准备、天劫或对账中的突破"):!D.Facts.SelectionMatches?TEXT("报价材料与当前选择不一致，请重新报价"):!D.Facts.KindMatches?TEXT("报价类型与当前操作不一致"):TEXT("报价字段不完整、材料槽位无效或服务尚未确认准备度");
    return D;
}

InkCore::FActionResult InkCore::InventoryAction(UInkFrontendWidget* Frontend,FGuid Id,InkCorePolicy::EInventoryAction Action,int32 Quantity,bool Active)
{
    using namespace InkCorePolicy;if(!Frontend||!Active)return CoreAdapterReject(TEXT("当前页面不可操作"));
    auto* Model=Frontend->GetInventoryModel();const auto* E=Model?Model->FindEntry(Id):nullptr;
    if(!E)return CoreAdapterReject(TEXT("物品已移除或尚未同步"));
    auto* W=Service(Frontend);const bool Remote=Model->IsAuthoritative();
    // Never fall back to local consumption while the Wuling authority is enabled or after a remote projection.
    if((W&&W->IsEnabled())&&!Remote)return CoreAdapterReject(TEXT("权威背包尚未同步"));
    if(Remote&&!CoreAdapterDispatchReady(Frontend,Active))return CoreAdapterReject(TEXT("服务未连接、缺少主播权限、页面未激活或请求处理中"));
    FInventoryFacts Facts{true,E->bEquipped,E->Item.bUsable,E->Item.bSellable,E->Item.bDecomposable,Model->CanEquipEntry(Id),E->Item.Count,Quantity};
    if(!Remote)Facts.Usable=Model->CanUseEntry(Id);
    else
    {
        // Re-read the current authority instance at click time; reserved/expired/stale copies are inert.
        const auto S=W->Snapshot();const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;TSharedPtr<FJsonObject> Instance;
        if(S&&S->TryGetArrayField(TEXT("item_instances"),Rows))for(const auto& Value:*Rows)
        {const auto O=Value&&Value->Type==EJson::Object?Value->AsObject():nullptr;if(CoreAdapterString(O,TEXT("id"))==E->AuthorityInstanceId){Instance=O;break;}}
        double Count=0,Now=0,Expiry=0;
        if(!Instance||CoreAdapterString(Instance,TEXT("item"))!=E->Item.ItemId.ToString()||!CoreAdapterNumber(Instance,TEXT("count"),Count)||!InkCorePolicy::ValidCount(Count,MAX_int32)||Quantity>Count||!CoreAdapterString(Instance,TEXT("reserved")).IsEmpty()||!CoreAdapterNumber(S,TEXT("server_utc"),Now)||!CoreAdapterNumber(Instance,TEXT("expiry"),Expiry)||Expiry<=Now)
            return CoreAdapterReject(TEXT("物品已变化、预留、过期或缺少服务确认，请等待背包同步"));
        // Authority equip validity remains server-owned; use the existing category restriction.
        Facts.Equippable=E->Item.Category==TEXT("Equipment")||E->Item.Category==TEXT("Treasure");Facts.Decomposable=false;
    }
    if(!InventoryAllowed(Facts,Action))return CoreAdapterReject(TEXT("数量超出持有量或当前物品不支持该操作"));
    if(Remote)
    {
        const auto Item=E->Item.ItemId.ToString();FString Op;
        switch(Action){case EInventoryAction::Use:Op=Item.StartsWith(TEXT("BOOK_"))?TEXT("LearnMethod"):TEXT("UseProp");break;
        case EInventoryAction::Sell:Op=TEXT("Recycle");break;case EInventoryAction::Equip:Op=TEXT("Equip");break;case EInventoryAction::Unequip:Op=TEXT("Unequip");break;default:return CoreAdapterReject(TEXT("当前规则未开放分解"));}
        auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("item_id"),Item);Args->SetNumberField(TEXT("count"),Quantity);
        return CoreAdapterSubmitted(W->SubmitPrototypeCommand(Op,CoreAdapterJson(Args)),W);
    }
    bool Applied=false;
    switch(Action){case EInventoryAction::Use:Applied=Model->UseItem(Id,Quantity)>0;break;case EInventoryAction::Sell:Applied=Model->SellItem(Id,Quantity)>0;break;
    case EInventoryAction::Decompose:Applied=Model->DecomposeItem(Id,Quantity)>0;break;case EInventoryAction::Equip:Applied=Model->EquipItem(Id);break;case EInventoryAction::Unequip:Applied=Model->UnequipItem(Id);break;}
    return {Applied,false,Applied?FString(TEXT("本地操作完成")):FString(TEXT("当前物品无法执行此操作"))};
}

InkCore::FActionResult InkCore::QuoteBreakthrough(const UUserWidget* Context,const TMap<FString,int32>& Selection,bool Rework,bool Active)
{
    if(!CoreAdapterDispatchReady(Context,Active))return CoreAdapterReject(TEXT("服务未连接、缺少主播权限、页面未激活或请求处理中"));
    if(!CoreAdapterSelectionValid(Selection))return CoreAdapterReject(TEXT("请选择合法的材料数量"));
    auto Args=MakeShared<FJsonObject>();Args->SetArrayField(TEXT("selection"),CoreAdapterSelectionRows(Selection));auto* W=Service(Context);
    return CoreAdapterSubmitted(W->SubmitPrototypeCommand(Rework?TEXT("QuoteReworkQuality"):TEXT("QuoteBreakthrough"),CoreAdapterJson(Args)),W);
}
InkCore::FActionResult InkCore::PrepareBreakthrough(const UUserWidget* Context,const TMap<FString,int32>& Selection,bool Rework,bool Active)
{
    if(!CoreAdapterDispatchReady(Context,Active))return CoreAdapterReject(TEXT("服务未连接、缺少主播权限、页面未激活或请求处理中"));
    auto* W=Service(Context);const auto Decision=EvaluateQuote(W->Snapshot(),Selection,Rework);if(!Decision.CanSubmit)return CoreAdapterReject(Decision.Reason);
    if(!Rework)return CoreAdapterSubmitted(W->PrepareBreakthrough(CoreAdapterSelectionJson(Selection)),W);
    auto Args=MakeShared<FJsonObject>();Args->SetArrayField(TEXT("selection"),CoreAdapterSelectionRows(Selection));return CoreAdapterSubmitted(W->SubmitPrototypeCommand(TEXT("ReworkQuality"),CoreAdapterJson(Args)),W);
}
InkCore::FActionResult InkCore::CancelOrReconcile(const UUserWidget* Context,bool Active)
{
    if(!CoreAdapterDispatchReady(Context,Active))return CoreAdapterReject(TEXT("服务未连接、缺少主播权限、页面未激活或请求处理中"));
    auto* W=Service(Context);const auto D=EvaluateQuote(W->Snapshot(),{});
    if(D.CanCancel)return CoreAdapterSubmitted(W->CancelBreakthrough(),W);
    if(!D.CanReconcile)return CoreAdapterReject(TEXT("仅试炼开始前可取消；对账需要有效的尝试编号"));
    auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("attempt_id"),CoreAdapterString(CoreAdapterObject(W->Snapshot(),TEXT("tribulation")),TEXT("id")));
    return CoreAdapterSubmitted(W->SubmitPrototypeCommand(TEXT("ReconcileBreakthrough"),CoreAdapterJson(Args)),W);
}
InkCore::FActionResult InkCore::CreateGuild(const UUserWidget* Context,const FString& Name,bool Active)
{
    if(!CoreAdapterDispatchReady(Context,Active))return CoreAdapterReject(TEXT("服务未连接、缺少主播权限、页面未激活或请求处理中"));
    auto* W=Service(Context);if(!W->Snapshot()||!W->Snapshot()->HasField(TEXT("guild")))return CoreAdapterReject(TEXT("武灵信息尚未同步"));
    if(CoreAdapterObject(W->Snapshot(),TEXT("guild")))return CoreAdapterReject(TEXT("武灵已存在，请使用现有武灵入口"));
    const auto Trimmed=Name.TrimStartAndEnd();if(Trimmed.IsEmpty())return CoreAdapterReject(TEXT("请输入武灵名称"));
    return CoreAdapterSubmitted(W->CreateGuild(Trimmed),W);
}
InkCore::FActionResult InkCore::RequestMembers(const UUserWidget* Context,int32 Cursor,bool Active)
{
    if(!CoreAdapterDispatchReady(Context,Active))return CoreAdapterReject(TEXT("服务未连接、缺少主播权限、页面未激活或请求处理中"));
    auto* W=Service(Context);if(Cursor<0||!CoreAdapterObject(W->Snapshot(),TEXT("guild")))return CoreAdapterReject(TEXT("成员页尚不可用"));
    W->RequestMemberPage(Cursor);return {true,true,TEXT("已请求成员页，等待服务同步")};
}
