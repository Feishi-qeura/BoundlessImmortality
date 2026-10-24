#include "InkActivitiesBatch2Panels.h"

#include "Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h"
#include "Frontend/WulingFrontendData.h"
#include "Components/CanvasPanel.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <initializer_list>

namespace InkActivitiesBatch2
{
using FObject = TSharedPtr<FJsonObject>;

FObject Object(const FObject& Owner, const TCHAR* Key)
{
    const FObject* Value = nullptr;
    return Owner && Owner->TryGetObjectField(Key, Value) ? *Value : nullptr;
}

FString String(const FObject& Owner, const TCHAR* Key)
{
    FString Value;
    if (Owner) Owner->TryGetStringField(Key, Value);
    return Value;
}

bool Number(const FObject& Owner, const TCHAR* Key, double& Out)
{
    return Owner && Owner->TryGetNumberField(Key, Out);
}

TArray<FObject> Rows(const FObject& Owner, const TCHAR* Key)
{
    TArray<FObject> Result;
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (Owner && Owner->TryGetArrayField(Key, Values))
        for (const auto& Value : *Values)
            if (Value && Value->Type == EJson::Object) Result.Add(Value->AsObject());
    return Result;
}

FString FieldOrPending(const FObject& Owner, const TCHAR* Key)
{
    const FString Value = String(Owner, Key);
    return Value.IsEmpty() ? TEXT("待同步") : Value;
}

FString TaskConditionLabel(const FString& Id)
{
    static const TMap<FString, FString> Names = {
        {TEXT("join_and_free_response"), TEXT("正式加入 · 免费响应")},
        {TEXT("complete_world_rounds"), TEXT("完整参与世界历练")},
        {TEXT("correct_guard_response"), TEXT("第一阶段 · 正确护阵")},
        {TEXT("correct_break_response"), TEXT("第二阶段 · 正确破障")},
        {TEXT("route_vote_in_distinct_rounds"), TEXT("不同轮次 · 路线选择")}};
    if (const FString* Name = Names.Find(Id)) return *Name;
    return TEXT("任务条件待说明");
}

FString NumberOrPending(const FObject& Owner, const TCHAR* Key)
{
    double Value = 0;
    return Number(Owner, Key, Value) ? FString::SanitizeFloat(Value) : TEXT("待同步");
}

FString ArrayCountOrPending(const FObject& Owner, const TCHAR* Key)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    return Owner && Owner->TryGetArrayField(Key, Values) && Values
        ? FString::FromInt(Values->Num()) : TEXT("待同步");
}

FString Json(const FObject& Value)
{
    if (!Value) return TEXT("<missing>");
    FString Encoded;
    const auto Writer = TJsonWriterFactory<>::Create(&Encoded);
    FJsonSerializer::Serialize(Value.ToSharedRef(), Writer);
    return Encoded;
}

void CopyField(const FObject& Source, const FObject& Destination, const TCHAR* Key)
{
    if (Source && Destination)
        if (const auto* Value = Source->Values.Find(Key)) Destination->SetField(Key, *Value);
}

FObject Projection(const FObject& Source, std::initializer_list<const TCHAR*> Keys)
{
    FObject Result = MakeShared<FJsonObject>();
    for (const TCHAR* Key : Keys) CopyField(Source, Result, Key);
    return Result;
}

FString RefreshKey(const UWulingPrototypeSubsystem* Service, const FObject& Snapshot,
    bool bActive, FName Route, const FString& SelectedId, const FString& ServiceDay, const FObject& Data)
{
    double Version = -1;
    if (Snapshot) Snapshot->TryGetNumberField(TEXT("version"), Version);
    return FString::Printf(TEXT("%s|active=%d|version=%.0f|enabled=%d|connected=%d|busy=%d|status=%s|selected=%s|day=%s|data=%s"),
        *Route.ToString(), bActive ? 1 : 0, Version,
        Service && Service->IsEnabled() ? 1 : 0,
        Service && Service->IsConnected() ? 1 : 0,
        Service && Service->IsBusy() ? 1 : 0,
        Service ? *Service->GetStatus() : TEXT("service-unavailable"),
        *SelectedId, *ServiceDay, *Json(Data));
}

FString ActiveEventBoundary(const FObject& Snapshot)
{
    const FObject Event = Object(Snapshot, TEXT("launch"));
    if (String(Event, TEXT("status")) != TEXT("Active")) return TEXT("not-active");
    double EndUtc = 0, NowUtc = 0;
    if (!Number(Event, TEXT("end"), EndUtc) || !Number(Snapshot, TEXT("server_utc"), NowUtc) || NowUtc <= 0)
        return TEXT("time-pending");
    return EndUtc > NowUtc ? TEXT("active") : TEXT("expired");
}

FString LaunchWindowBoundary(const FObject& Snapshot)
{
    double EndUtc = 0, NowUtc = 0;
    if (!Number(Object(Snapshot, TEXT("manifest")), TEXT("window_end"), EndUtc) ||
        !Number(Snapshot, TEXT("server_utc"), NowUtc) || NowUtc <= 0) return TEXT("time-pending");
    return EndUtc > NowUtc ? TEXT("open") : TEXT("expired");
}

FString ServerDateOrPending(const FObject& Owner, const TCHAR* Key)
{
    double Value = 0;
    if (!Number(Owner, Key, Value) || Value <= 0) return TEXT("待同步");
    return (FDateTime::FromUnixTimestamp(static_cast<int64>(Value)) + FTimespan::FromHours(8))
        .ToString(TEXT("%Y-%m-%d %H:%M")) + TEXT("（香港）");
}

FObject FindTemplate(const InkJourneyAdapter::FView& View, const FString& Id, const FString& Cycle)
{
    for (const FObject& Row : View.Templates.Items)
    {
        const FString RowId = String(Row, TEXT("activity_id")).IsEmpty()
            ? String(Row, TEXT("id")) : String(Row, TEXT("activity_id"));
        if (RowId == Id && String(Row, TEXT("cycle")).Equals(Cycle, ESearchCase::IgnoreCase)) return Row;
    }
    return nullptr;
}
}

void UInkLaunchEventBatch2Panel::RefreshLiveData()
{
    using namespace InkActivitiesBatch2;
    const bool bActive = IsRolloutPageActive();
    if (!bActive) return;
    auto* Service = WulingFrontendData::Service(this);
    const FObject Snapshot = Service ? Service->Snapshot() : nullptr;
    const FObject Data = Projection(Snapshot, {TEXT("launch"), TEXT("identity"), TEXT("guild"), TEXT("manifest")});
    const FString Key = InkActivitiesBatch2::RefreshKey(Service, Snapshot, bActive, PageId,
        SelectedTab + TEXT("|") + ActiveEventBoundary(Snapshot) + TEXT("|") + LaunchWindowBoundary(Snapshot), FString(), Data);
    if (Key != AuthorityContentKey)
    {
        AuthorityContentKey = Key;
        RebuildPanel();
    }
}

void UInkLaunchEventBatch2Panel::BuildPanelContent()
{
    using namespace InkActivitiesBatch2;
    const float W = BodyWidth;
    auto* Service = WulingFrontendData::Service(this);
    const FObject Snapshot = Service ? Service->Snapshot() : nullptr;
    const FObject Launch = Object(Snapshot, TEXT("launch"));
    const FObject Personal = Object(Launch, TEXT("personal"));
    const float HeaderY = 0;
    Surface(BodyCanvas, TEXT("LaunchHeroPaper"), InkPresentationStyle::ESurface::FeatureCard, 0, HeaderY, W, 206);
    Label(BodyCanvas, TEXT("LaunchTitle"), TEXT("青云开服 · 共赴仙途"), 22, 20, W-44, 44, 32);
    Label(BodyCanvas, TEXT("LaunchPeriod"),
        TEXT("活动时间：") + ServerDateOrPending(Launch, TEXT("start")) + TEXT(" — ") + ServerDateOrPending(Launch, TEXT("end")),
        22, 74, W-44, 36, 19);
    Label(BodyCanvas, TEXT("LaunchPersonal"),
        TEXT("我的贡献：") + NumberOrPending(Personal, TEXT("score")) + TEXT("    完整歷练：") + NumberOrPending(Personal, TEXT("complete_count")),
        22, 118, W-44, 36, 21);

    const TPair<const TCHAR*, const TCHAR*> Tabs[] = {
        {TEXT("overview"), TEXT("总览")}, {TEXT("tasks"), TEXT("任务")},
        {TEXT("guild"), TEXT("组织")}, {TEXT("honors"), TEXT("荣誉")}};
    const float TabW = W / 4.f;
    for (int32 I = 0; I < UE_ARRAY_COUNT(Tabs); ++I)
        Action(BodyCanvas, FString(TEXT("tab_")) + Tabs[I].Key, Tabs[I].Value,
            I * TabW, 222, TabW - 6, 58, false, false, SelectedTab == Tabs[I].Key);

    const float Y = 304;
    if (SelectedTab == TEXT("overview"))
    {
        Label(BodyCanvas, TEXT("LaunchOverviewState"),
            Launch ? TEXT("活动状态由服务端确认") : TEXT("活动资料待同步"), 18, Y, W-36, 54, 24);
        Label(BodyCanvas, TEXT("LaunchOverviewRules"),
            TEXT("贡献进度与结算以服务端回执为准。任务完成后自动结算；此页不提供客户端领取按钮。"),
            18, Y+62, W-36, 96, 20);
        Action(BodyCanvas, TEXT("Open_LaunchRank"), TEXT("查看贡献榜"), 18, Y+174, W-36, 68, true);
        double EndUtc = 0, NowUtc = 0;
        const bool bCanEnterActiveEvent = Launch && String(Launch, TEXT("status")) == TEXT("Active") &&
            Number(Launch, TEXT("end"), EndUtc) && Number(Snapshot, TEXT("server_utc"), NowUtc) && EndUtc > NowUtc;
        if (bCanEnterActiveEvent)
            Action(BodyCanvas, TEXT("EnterLaunchWorld"), TEXT("进入活动历练"), 18, Y+254, W-36, 68, true, true);
        else
        {
            const auto Context = InkJourneyAdapter::Capture(Service, IsRolloutPageActive());
            InkJourneyAdapter::FRequest StartRequest;
            StartRequest.Action = InkJourneyAdapter::EAction::StartLaunch;
            StartRequest.bConfirmed = true;
            const auto Gate = InkJourneyAdapter::Evaluate(Context, StartRequest);
            if (Gate.bAllowed)
                Action(BodyCanvas, TEXT("StartLaunch"), bConfirmStart ? TEXT("再次确认开启活动") : TEXT("开启开服活动"),
                    18, Y+254, W-36, 68, true, true);
            else
                Label(BodyCanvas, TEXT("LaunchStartGate"), Gate.Reason, 18, Y+254, W-36, 54, 18);
        }
    }
    else if (SelectedTab == TEXT("tasks"))
    {
        const auto Tasks = Rows(Launch, TEXT("tasks"));
        if (Tasks.IsEmpty()) Label(BodyCanvas, TEXT("LaunchTasksPending"), TEXT("任务清单待服务端同步"), 18, Y, W-36, 58, 22);
        for (int32 I = 0; I < Tasks.Num(); ++I)
        {
            const FObject& Task = Tasks[I];
            const float RowY = Y + I * 142.f;
            Surface(BodyCanvas, FName(*FString::Printf(TEXT("LaunchTaskCard%d"), I)), InkPresentationStyle::ESurface::FeatureCard, 8, RowY, W-16, 132);
            Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchTaskTitle%d"), I)),
                TaskConditionLabel(String(Task, TEXT("condition_type"))), 20, RowY+10, W-40, 34, 22);
            bool bConfirmed = false;
            Task->TryGetBoolField(TEXT("confirmed"), bConfirmed);
            Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchTaskProgress%d"), I)),
                (bConfirmed ? TEXT("服务已确认 · ") : TEXT("进度：")) + NumberOrPending(Task, TEXT("progress")) + TEXT(" / ") + NumberOrPending(Task, TEXT("target")),
                20, RowY+50, W-40, 32, 19);
            const FObject Permissions = Object(Launch, TEXT("permissions"));
            FString RewardText = TEXT("奖励资料待同步");
            bool bCanShowNumericReward = false;
            if (Permissions) Permissions->TryGetBoolField(TEXT("task_numeric_rewards"), bCanShowNumericReward);
            double RewardCount = 0;
            const FString RewardItem = String(Task, TEXT("reward_item"));
            if (bCanShowNumericReward && !RewardItem.IsEmpty() && Number(Task, TEXT("reward_count"), RewardCount))
                RewardText = FString::Printf(TEXT("%s %.0f · 服务自动结算"), *RewardItem, RewardCount);
            else if (!bCanShowNumericReward)
                RewardText = TEXT("数值奖励未获服务端展示许可 · 自动结算");
            Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchTaskSettlement%d"), I)),
                RewardText, 20, RowY+84, W-40, 30, 17);
        }
    }
    else if (SelectedTab == TEXT("guild"))
    {
        const auto Milestones = Rows(Launch, TEXT("guild_milestones"));
        if (Milestones.IsEmpty()) Label(BodyCanvas, TEXT("LaunchGuildPending"), TEXT("组织里程碑待服务端同步"), 18, Y, W-36, 54, 22);
        for (int32 I = 0; I < Milestones.Num(); ++I)
        {
            const float RowY = Y + I * 102.f;
            Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchGuildMilestone%d"), I)),
                TEXT("共修目标 · ") + NumberOrPending(Milestones[I], TEXT("complete_rounds")) + TEXT(" 轮"), 18, RowY, W-36, 36, 21);
            Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchGuildProgress%d"), I)),
                TEXT("已确认轮次：") + ArrayCountOrPending(Launch, TEXT("complete_rounds")),
                18, RowY+38, W-36, 34, 18);
        }
    }
    else
    {
        Label(BodyCanvas, TEXT("LaunchHonorTitle"), TEXT("荣誉名册由活动快照提供"), 18, Y, W-36, 44, 22);
        Action(BodyCanvas, TEXT("Open_LaunchRank"), TEXT("查看贡献榜"), 18, Y+54, W-36, 66, true);
    }

    const float OfferY = BodyHeight - 132;
    Surface(BodyCanvas, TEXT("LaunchOfferDisabledCard"), InkPresentationStyle::ESurface::FeatureCard, 8, OfferY, W-16, 112);
    Label(BodyCanvas, TEXT("LaunchOfferDisabled"), TEXT("开服特惠 · 商品资料 / SKU / 价格 / 购买接口待接入"),
        20, OfferY+14, W-40, 44, 18);
    Label(BodyCanvas, TEXT("LaunchOfferGate"), TEXT("当前不可购买"), 20, OfferY+62, W-40, 30, 17);
}

void UInkLaunchEventBatch2Panel::HandlePanelAction(const FString& ActionId)
{
    if (ActionId.StartsWith(TEXT("tab_")))
    {
        SelectedTab = ActionId.Mid(4);
        bConfirmStart = false;
        RebuildPanel();
        return;
    }
    if (ActionId == TEXT("Open_LaunchRank")) { Open(TEXT("LaunchRank")); return; }
    if (ActionId == TEXT("EnterLaunchWorld")) { Open(TEXT("WulingWorld")); return; }
    if (ActionId == TEXT("StartLaunch"))
    {
        if (!bConfirmStart)
        {
            bConfirmStart = true;
            FeedbackMessage(TEXT("请再次确认开启开服活动"));
            RebuildPanel();
            return;
        }
        InkJourneyAdapter::FRequest Request;
        Request.Action = InkJourneyAdapter::EAction::StartLaunch;
        Request.bConfirmed = true;
        FString ActionFeedback;
        InkJourneyAdapter::Submit(WulingFrontendData::Service(this), IsRolloutPageActive(), Request, ActionFeedback);
        FeedbackMessage(ActionFeedback);
        bConfirmStart = false;
        RefreshLiveData();
        return;
    }
    // Product offer remains disabled until the service publishes a SKU and purchase command.
    Super::HandlePanelAction(ActionId);
}

void UInkLaunchRankBatch2Panel::BuildPanelContent()
{
    using namespace InkActivitiesBatch2;
    const float W = BodyWidth;
    auto* Service = WulingFrontendData::Service(this);
    const FObject Snapshot = Service ? Service->Snapshot() : nullptr;
    const FObject Launch = Object(Snapshot, TEXT("launch"));
    const auto Leaders = Rows(Launch, TEXT("leaderboard"));
    const auto Personal = Object(Launch, TEXT("personal"));
    Surface(BodyCanvas, TEXT("LaunchRankHeader"), InkPresentationStyle::ESurface::FeatureCard, 0, 0, W, 144);
    Label(BodyCanvas, TEXT("LaunchRankHeading"), TEXT("青云贡献榜"), 18, 16, W-36, 42, 30);
    Label(BodyCanvas, TEXT("LaunchRankScope"),
        TEXT("榜单周期：") + ServerDateOrPending(Launch, TEXT("start")) + TEXT(" — ") + ServerDateOrPending(Launch, TEXT("end")),
        18, 68, W-36, 34, 17);
    Label(BodyCanvas, TEXT("LaunchRankMyContribution"),
        TEXT("我的贡献：") + NumberOrPending(Personal, TEXT("score")) + TEXT(" · 完整历练：") + NumberOrPending(Personal, TEXT("complete_count")),
        18, 106, W-36, 30, 18);
    if (Leaders.IsEmpty())
    {
        Label(BodyCanvas, TEXT("LaunchRankEmpty"), TEXT("贡献榜待服务端同步"), 18, 164, W-36, 58, 22);
        return;
    }
    for (int32 I = 0; I < Leaders.Num(); ++I)
    {
        const float Y = 160 + I * 86.f;
        Surface(BodyCanvas, FName(*FString::Printf(TEXT("LaunchRankRow%d"), I)), InkPresentationStyle::ESurface::FeatureCard, 8, Y, W-16, 78);
        Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchRankPlace%d"), I)),
            TEXT("第 ") + NumberOrPending(Leaders[I], TEXT("rank")) + TEXT(" 名"), 18, Y+12, 106, 48, 22, true);
        Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchRankName%d"), I)),
            FieldOrPending(Leaders[I], TEXT("name")), 132, Y+12, W-280, 48, 21);
        Label(BodyCanvas, FName(*FString::Printf(TEXT("LaunchRankScore%d"), I)),
            TEXT("贡献 ") + NumberOrPending(Leaders[I], TEXT("score")), W-142, Y+12, 124, 48, 17, true);
    }
}

void UInkLaunchRankBatch2Panel::RefreshLiveData()
{
    using namespace InkActivitiesBatch2;
    const bool bActive = IsRolloutPageActive();
    if (!bActive) return;
    auto* Service = WulingFrontendData::Service(this);
    const FObject Snapshot = Service ? Service->Snapshot() : nullptr;
    const FObject Data = InkActivitiesBatch2::Projection(Snapshot,
        {TEXT("launch"), TEXT("identity"), TEXT("guild"), TEXT("manifest")});
    const FString Key = InkActivitiesBatch2::RefreshKey(Service, Snapshot, bActive, PageId,
        FString(), FString(), Data);
    if (Key != AuthorityContentKey)
    {
        AuthorityContentKey = Key;
        RebuildPanel();
    }
}

void UInkLaunchRankBatch2Panel::HandlePanelAction(const FString& ActionId)
{
    Super::HandlePanelAction(ActionId);
}

FText UInkActivityDetailBatch2PanelBase::PanelTitle() const
{
    return FText::FromString(SelectedEntry.IsEmpty() ? TEXT("活动详情 · 待同步") : SelectedEntry);
}

void UInkActivityDetailBatch2PanelBase::BuildPanelContent()
{
    using namespace InkActivitiesBatch2;
    const float W = BodyWidth;
    const FString Cycle(ExpectedCycle());
    const auto Context = InkJourneyAdapter::Capture(WulingFrontendData::Service(this), IsRolloutPageActive());
    const auto View = InkJourneyAdapter::Read(Context);
    CapturedServiceDay = View.ServiceDay;
    const FObject Row = FindTemplate(View, SelectedEntry, Cycle);
    Surface(BodyCanvas, TEXT("ActivityDetailShell"), InkPresentationStyle::ESurface::FeatureCard, 0, 0, W, BodyHeight);
    Label(BodyCanvas, TEXT("ActivityDetailHeading"), Cycle == TEXT("daily") ? TEXT("日常活动详情") : TEXT("限时活动详情"),
        18, 14, W-36, 42, 27);
    if (!Row)
    {
        FString State;
        switch (View.State)
        {
        case InkJourneyAdapter::EState::Disabled: State = TEXT("活动服务未启用"); break;
        case InkJourneyAdapter::EState::Offline: State = TEXT("服务离线，暂不可操作"); break;
        case InkJourneyAdapter::EState::Loading: State = TEXT("活动资料正在同步"); break;
        case InkJourneyAdapter::EState::Pending: State = TEXT("活动资料待同步"); break;
        case InkJourneyAdapter::EState::Ready: State = TEXT("所选活动未在本周期清单中"); break;
        }
        Label(BodyCanvas, TEXT("ActivityDetailState"), State, 18, 68, W-36, 44, 21);
        Label(BodyCanvas, TEXT("ActivityDetailEmpty"), TEXT("活动开放并同步后，可在此查看条件、进度、时限与奖励信息。"),
            18, 122, W-36, 88, 19);
        return;
    }

    Label(BodyCanvas, TEXT("ActivityDetailName"), FieldOrPending(Row, TEXT("name")), 18, 68, W-36, 48, 26);
    Label(BodyCanvas, TEXT("ActivityDetailDescription"), FieldOrPending(Row, TEXT("description")), 18, 122, W-36, 68, 19);
    Label(BodyCanvas, TEXT("ActivityDetailCycle"), TEXT("周期：") + FieldOrPending(Row, TEXT("cycle")), 18, 198, W-36, 34, 18);
    Label(BodyCanvas, TEXT("ActivityDetailEligibility"), TEXT("参与条件：") + FieldOrPending(Row, TEXT("profile_requirement")), 18, 236, W-36, 52, 18);
    Label(BodyCanvas, TEXT("ActivityDetailProgress"),
        TEXT("进度：") + NumberOrPending(Row, TEXT("progress")) + TEXT(" / ") + NumberOrPending(Row, TEXT("target")),
        18, 294, W-36, 40, 18);
    Label(BodyCanvas, TEXT("ActivityDetailDeadline"), TEXT("截止时间：") + FieldOrPending(Row, TEXT("end_at")), 18, 340, W-36, 40, 18);
    Label(BodyCanvas, TEXT("ActivityDetailBudget"), TEXT("奖励预算：") + NumberOrPending(Row, TEXT("reward_budget")), 18, 386, W-36, 40, 18);
    Label(BodyCanvas, TEXT("ActivityDetailRewardStatus"), TEXT("奖励与结算状态以活动服务回执为准"), 18, 432, W-36, 42, 17);
    if (SelectedEntry == TEXT("ACT_SIGN") && Cycle == TEXT("daily") && View.Signed.IsSet() && !View.Signed.GetValue())
        Action(BodyCanvas, TEXT("ActivityDetailSignIn"), TEXT("每日签到"), 18, BodyHeight-96, W-36, 70, true);
    else if (SelectedEntry == TEXT("ACT_SIGN") && Cycle == TEXT("daily") && View.Signed.IsSet() && View.Signed.GetValue())
        Label(BodyCanvas, TEXT("ActivityDetailSignInDone"), TEXT("今日已签到"), 18, BodyHeight-96, W-36, 58, 18);
    else if (SelectedEntry == TEXT("ACT_SIGN") && Cycle == TEXT("daily"))
        Label(BodyCanvas, TEXT("ActivityDetailSignInPending"), TEXT("签到状态待同步 · 暂不可操作"), 18, BodyHeight-96, W-36, 58, 17);
    else
        Label(BodyCanvas, TEXT("ActivityDetailActionPending"), TEXT("参与入口待接入活动服务接口"), 18, BodyHeight-96, W-36, 58, 17);
}

void UInkActivityDetailBatch2PanelBase::RefreshLiveData()
{
    using namespace InkActivitiesBatch2;
    const bool bActive = IsRolloutPageActive();
    if (!bActive) return;
    auto* Service = WulingFrontendData::Service(this);
    const FObject Snapshot = Service ? Service->Snapshot() : nullptr;
    const FObject Data = InkActivitiesBatch2::Projection(Snapshot, {TEXT("activities")});
    const auto Context = InkJourneyAdapter::Capture(Service, bActive);
    const auto View = InkJourneyAdapter::Read(Context);
    const FString Key = InkActivitiesBatch2::RefreshKey(Service, Snapshot, bActive, PageId,
        SelectedEntry + TEXT("|") + ExpectedCycle(), View.ServiceDay, Data);
    if (Key != AuthorityContentKey)
    {
        AuthorityContentKey = Key;
        RebuildPanel();
    }
}

void UInkActivityDetailBatch2PanelBase::HandlePanelAction(const FString& ActionId)
{
    if (ActionId == TEXT("ActivityDetailSignIn") && SelectedEntry == TEXT("ACT_SIGN") && FString(ExpectedCycle()) == TEXT("daily"))
    {
        InkJourneyAdapter::FRequest Request;
        Request.Action = InkJourneyAdapter::EAction::SignIn;
        Request.ExpectedDay = CapturedServiceDay;
        FString ActionFeedback;
        InkJourneyAdapter::Submit(WulingFrontendData::Service(this), IsRolloutPageActive(), Request, ActionFeedback);
        FeedbackMessage(ActionFeedback);
        RefreshLiveData();
        return;
    }
    Super::HandlePanelAction(ActionId);
}

