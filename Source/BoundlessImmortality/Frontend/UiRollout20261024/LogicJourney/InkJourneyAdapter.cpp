#include "InkJourneyAdapter.h"
#include "Wuling/WulingPrototypeSubsystem.h"

namespace InkJourneyAdapter
{
namespace
{
FObject Object(const FObject& O, const TCHAR* Key)
{
    const FObject* V = nullptr;
    return O && O->TryGetObjectField(Key,V) ? *V : nullptr;
}
FString Text(const FObject& O, const TCHAR* Key)
{
    FString V; if (O) O->TryGetStringField(Key,V); return V;
}
TOptional<bool> Flag(const FObject& O, const TCHAR* Key)
{
    bool V = false;
    return O && O->TryGetBoolField(Key,V) ? TOptional<bool>(V) : TOptional<bool>();
}
TOptional<double> Number(const FObject& O, const TCHAR* Key)
{
    double V = 0;
    return O && O->TryGetNumberField(Key,V) && FMath::IsFinite(V) ? TOptional<double>(V) : TOptional<double>();
}
bool True(const TOptional<bool>& V) { return V.IsSet() && V.GetValue(); }
bool False(const TOptional<bool>& V) { return V.IsSet() && !V.GetValue(); }
FObject Find(const FRows& List, const FString& Id)
{
    if (!Id.IsEmpty()) for (const auto& Row : List.Items) if (Text(Row,TEXT("id")) == Id) return Row;
    return nullptr;
}
FGate Denied(const TCHAR* Reason) { return {false,Reason}; }
}

FContext Capture(UWulingPrototypeSubsystem* Service, bool bActivePage)
{
    FContext C;
    C.bActivePage = bActivePage;
    if (Service)
    {
        C.Snapshot = Service->Snapshot(); C.bEnabled = Service->IsEnabled();
        C.bConnected = Service->IsConnected(); C.bBusy = Service->IsBusy();
    }
    return C;
}

FRows Rows(const FObject& O, const TCHAR* Key)
{
    FRows Result; const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    Result.bProvided = O && O->TryGetArrayField(Key,Values) && Values;
    if (Result.bProvided) for (const auto& V : *Values)
    {
        if (V.IsValid() && V->Type == EJson::Object && V->AsObject()) Result.Items.Add(V->AsObject());
        else Result.bProvided = false; // Malformed lists are not a successful empty response.
    }
    return Result;
}

FView Read(const FContext& C)
{
    FView V;
    V.State = !C.bEnabled ? EState::Disabled : !C.bConnected ? EState::Offline :
        !C.Snapshot ? EState::Loading : C.bBusy ? EState::Pending : EState::Ready;
    // Retain genuine cached data for offline inspection; Evaluate always rechecks connection.
    V.Activities = Object(C.Snapshot,TEXT("activities")); V.Journey = Object(C.Snapshot,TEXT("journey"));
    V.Launch = Object(C.Snapshot,TEXT("launch"));
    V.ServiceDay = Text(V.Activities,TEXT("day")); V.Verse = Text(V.Activities,TEXT("verse"));
    V.Signed = Flag(V.Activities,TEXT("signed")); V.MonthSignCount = Number(V.Activities,TEXT("month_sign_count"));
    V.Worlds = Rows(V.Journey,TEXT("worlds")); V.Resources = Rows(V.Journey,TEXT("resources"));
    V.WeeklyChests = Rows(V.Activities,TEXT("weekly_chests")); V.MonthlyChests = Rows(V.Activities,TEXT("monthly_chests"));
    V.Templates = Rows(V.Activities,TEXT("templates"));
    const auto* LaunchValue = C.Snapshot ? C.Snapshot->Values.Find(TEXT("launch")) : nullptr;
    V.bLaunchKnownAbsent = LaunchValue && LaunchValue->IsValid() && (*LaunchValue)->Type == EJson::Null;
    return V;
}

FRows ActivitiesForCycle(const FView& V, const FString& Cycle)
{
    FRows Result;
    if (Cycle != TEXT("daily") && Cycle != TEXT("limited") && Cycle != TEXT("permanent")) return Result;
    const auto Categories = Rows(V.Activities,TEXT("categories"));
    for (const auto& Category : Categories.Items)
    {
        FString Id = Text(Category,TEXT("id"));
        if (Id.IsEmpty()) Id = Text(Category,TEXT("type"));
        if (Id.IsEmpty()) Id = Text(Category,TEXT("key"));
        if (!Id.Equals(Cycle,ESearchCase::IgnoreCase) && !(Cycle == TEXT("permanent") && Id == TEXT("常驻"))) continue;
        const auto Items = Rows(Category,TEXT("items"));
        if (Category->HasField(TEXT("items"))) return Items;
        return Rows(Category,TEXT("activities"));
    }
    Result.bProvided = V.Templates.bProvided;
    for (const auto& Template : V.Templates.Items)
        if (Text(Template,TEXT("cycle")) == Cycle || (Cycle == TEXT("permanent") && Text(Template,TEXT("cycle")).Equals(Cycle,ESearchCase::IgnoreCase))) Result.Items.Add(Template);
    return Result;
}

FGate Evaluate(const FContext& C, const FRequest& R)
{
    if (!C.bActivePage) return Denied(TEXT("页面已关闭，请重新进入后操作"));
    if (!C.bEnabled) return Denied(TEXT("服务未启用"));
    if (!C.bConnected) return Denied(TEXT("服务未连接，保留快照仅供查看"));
    if (!C.Snapshot) return Denied(TEXT("权威数据待同步"));
    if (C.bBusy) return Denied(TEXT("请求处理中，请等待服务确认"));
    const FView V = Read(C);
    switch (R.Action)
    {
    case EAction::SignIn:
    {
        if (R.ExpectedDay.IsEmpty() || R.ExpectedDay != V.ServiceDay) return Denied(TEXT("服务日期已变化或尚未同步，请刷新后操作"));
        if (!V.Signed.IsSet()) return Denied(TEXT("签到状态待同步"));
        if (V.Signed.GetValue()) return Denied(TEXT("今日已签到"));
        bool bTemplate = false;
        for (const auto& Row : V.Templates.Items)
            if (Text(Row,TEXT("activity_id")) == TEXT("ACT_SIGN") && Text(Row,TEXT("cycle")) == TEXT("daily") && True(Flag(Row,TEXT("enabled")))) bTemplate = true;
        if (!V.Templates.bProvided || !bTemplate) return Denied(TEXT("签到模板未提供或未启用"));
        break;
    }
    case EAction::MonthlyChest:
    {
        if (!R.bConfirmed) return Denied(TEXT("请先查看奖励详情并确认领取"));
        if (R.ExpectedDay.IsEmpty() || R.ExpectedDay != V.ServiceDay) return Denied(TEXT("服务日期已变化或尚未同步，请重新查看奖励"));
        if (R.Milestone != 3 && R.Milestone != 7 && R.Milestone != 14 && R.Milestone != 21) return Denied(TEXT("无效月签里程碑"));
        bool bEligible = false;
        for (const auto& Chest : V.MonthlyChests.Items)
        {
            const auto Milestone = Number(Chest,TEXT("milestone"));
            if (Milestone.IsSet() && Milestone.GetValue() == R.Milestone)
                bEligible = True(Flag(Chest,TEXT("eligible"))) && False(Flag(Chest,TEXT("claimed")));
        }
        if (!V.MonthlyChests.bProvided || !bEligible) return Denied(TEXT("领取条件未满足、已领取或状态待同步"));
        break;
    }
    case EAction::WorldTravel:
    case EAction::ResourceTravel:
    {
        if (!R.bConfirmed) return Denied(TEXT("请先确认前往地点"));
        const bool bResource = R.Action == EAction::ResourceTravel;
        const auto Identity = Object(C.Snapshot,TEXT("identity"));
        if (Text(Identity,TEXT("role")) != TEXT("anchor") || Text(Identity,TEXT("user")).IsEmpty()) return Denied(TEXT("仅主播可发起前往请求"));
        if (bResource && Text(Object(C.Snapshot,TEXT("guild")),TEXT("id")).IsEmpty()) return Denied(TEXT("武灵组织资料待同步"));
        const FRows& List = bResource ? V.Resources : V.Worlds;
        const auto Entry = Find(List,R.EntryId);
        if (!List.bProvided || !Entry || !True(Flag(Entry,TEXT("open")))) return Denied(TEXT("地点未开放或资料待同步"));
        if (bResource && Text(Entry,TEXT("node_id")).IsEmpty()) return Denied(TEXT("资源节点编号待同步"));
        break;
    }
    case EAction::StartLaunch:
    {
        if (!R.bConfirmed) return Denied(TEXT("请确认开启活动"));
        const auto Identity = Object(C.Snapshot,TEXT("identity")), Guild = Object(C.Snapshot,TEXT("guild"));
        const auto Manifest = Object(C.Snapshot,TEXT("manifest"));
        const auto Now = Number(C.Snapshot,TEXT("server_utc")), End = Number(Manifest,TEXT("window_end"));
        if (Text(Manifest,TEXT("profile")) != TEXT("prototype_full")) return Denied(TEXT("当前服务配置未开放开服活动"));
        if (!V.bLaunchKnownAbsent) return Denied(TEXT("活动已存在或状态待同步"));
        const FString User = Text(Identity,TEXT("user"));
        if (Text(Identity,TEXT("role")) != TEXT("anchor") || User.IsEmpty() || User != Text(Guild,TEXT("anchor")) || Text(Guild,TEXT("id")).IsEmpty()) return Denied(TEXT("仅所属武灵主播可开启活动"));
        if (!Now.IsSet() || !End.IsSet() || Now.GetValue() <= 0 || End.GetValue() <= Now.GetValue()) return Denied(TEXT("活动授权已过期或时间待同步"));
        break;
    }
    default: return Denied(TEXT("不支持的操作"));
    }
    return {true,FString()};
}

bool Submit(UWulingPrototypeSubsystem* Service, bool bActivePage, const FRequest& R, FString& Feedback)
{
    const FContext C = Capture(Service,bActivePage);
    const FGate Gate = Evaluate(C,R);
    if (!Gate.bAllowed) { Feedback = Gate.Reason; return false; }
    bool bSent = false;
    switch (R.Action)
    {
    case EAction::SignIn: bSent = Service->SignIn(); break;
    case EAction::MonthlyChest:
        bSent = Service->SubmitPrototypeCommand(TEXT("ClaimSignInChest"),FString::Printf(TEXT("{\"milestone\":%d}"),R.Milestone)); break;
    case EAction::WorldTravel: bSent = Service->StartWorldRound(TEXT("ENC_SOLO"),TEXT("guard"),R.EntryId); break;
    case EAction::ResourceTravel:
        bSent = Service->QueueRace(Text(Find(Read(C).Resources,R.EntryId),TEXT("node_id"))); break;
    case EAction::StartLaunch: bSent = Service->StartLaunchEvent(); break;
    }
    Feedback = bSent ? TEXT("请求已提交，等待服务确认") : Service->GetStatus();
    return bSent;
}
}
