#include "InkJourneyBatch2FeaturePanels.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameInstance.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/WulingFrontendData.h"
#include "Wuling/WulingPrototypeSubsystem.h"

namespace InkJourneyBatch2Feature
{
    using FObject = InkJourneyAdapter::FObject;

    FString String(const FObject& Object, const TCHAR* Key)
    {
        FString Value;
        if (Object) Object->TryGetStringField(Key, Value);
        return Value;
    }

    bool Number(const FObject& Object, const TCHAR* Key, double& Out)
    {
        return Object && Object->TryGetNumberField(Key, Out) && FMath::IsFinite(Out);
    }

    bool Flag(const FObject& Object, const TCHAR* Key, bool& Out)
    {
        return Object && Object->TryGetBoolField(Key, Out);
    }

    FInkJourneyDisplayRow Row(const FString& Label, const FString& Value, bool bEmphasized = false)
    {
        FInkJourneyDisplayRow Result;
        Result.Label = FText::FromString(Label);
        Result.Value = FText::FromString(Value);
        Result.bEmphasized = bEmphasized;
        return Result;
    }

    FString NumberText(double Value)
    {
        return FText::AsNumber(Value).ToString();
    }

    FString Availability(const InkJourneyAdapter::FContext& Context)
    {
        if (!Context.bEnabled) return TEXT("服务未启用");
        if (!Context.bConnected) return TEXT("暂时离线 · 当前快照仅供查看");
        if (!Context.Snapshot) return TEXT("正在等待权威数据同步");
        if (Context.bBusy) return TEXT("请求处理中 · 等待服务确认");
        return TEXT("资料已同步");
    }

    FString SnapshotKey(const InkJourneyAdapter::FContext& Context, const FString& EntryId)
    {
        double Version = 0;
        if (Context.Snapshot) Context.Snapshot->TryGetNumberField(TEXT("version"), Version);
        return FString::Printf(TEXT("%.0f|%d|%d|%d|%s|%s"), Version, Context.bEnabled, Context.bConnected,
            Context.bBusy, *EntryId, *Availability(Context));
    }

    FObject FindRow(const InkJourneyAdapter::FRows& Rows, const FString& Id)
    {
        if (!Rows.bProvided || Id.IsEmpty()) return nullptr;
        for (const FObject& RowObject : Rows.Items)
            if (String(RowObject, TEXT("id")) == Id) return RowObject;
        return nullptr;
    }

    FString ReceiptDescription(const FObject& Receipt)
    {
        FString Description = String(Receipt, TEXT("source_label"));
        const auto Items = InkJourneyAdapter::Rows(Receipt, TEXT("items"));
        for (const FObject& Item : Items.Items)
        {
            double Quantity = 0;
            if (!Number(Item, TEXT("quantity"), Quantity) || Quantity <= 0) continue;
            if (!Description.IsEmpty()) Description += TEXT(" · ");
            Description += String(Item, TEXT("name")) + TEXT(" × ") + NumberText(Quantity);
        }
        return Description;
    }

    void AppendExactReceipts(const FObject& Snapshot, const FString& ExactSourcePrefix, TArray<FInkJourneyDisplayRow>& Rows)
    {
        const FObject Inbox = Snapshot.IsValid() ? WulingFrontendData::Object(Snapshot, TEXT("reward_receipts")) : nullptr;
        const auto Receipts = InkJourneyAdapter::Rows(Inbox, TEXT("entries"));
        for (const FObject& Receipt : Receipts.Items)
        {
            const FString SourceId = String(Receipt, TEXT("source_id"));
            if (!SourceId.Equals(ExactSourcePrefix, ESearchCase::CaseSensitive) &&
                !SourceId.StartsWith(ExactSourcePrefix + TEXT(":"), ESearchCase::CaseSensitive)) continue;
            Rows.Add(Row(TEXT("已记录回执"), ReceiptDescription(Receipt)));
        }
    }

    FObject FindRaceTeam(const FObject& Race, const FObject& Identity)
    {
        const FString User = String(Identity, TEXT("user"));
        const FString Room = String(Identity, TEXT("room"));
        const TArray<TSharedPtr<FJsonValue>>* Teams = nullptr;
        if (!Race || User.IsEmpty() || !Race->TryGetArrayField(TEXT("teams"), Teams) || !Teams) return nullptr;
        for (const auto& Value : *Teams)
        {
            const FObject Team = Value.IsValid() ? Value->AsObject() : nullptr;
            if (String(Team, TEXT("anchor")) == User && String(Team, TEXT("room")) == Room) return Team;
        }
        return nullptr;
    }

    FString Lifecycle(const FString& Status)
    {
        if (Status == TEXT("Settled")) return TEXT("结算已确认");
        if (Status.IsEmpty()) return TEXT("权威结果待同步");
        return TEXT("结算确认中 · ") + Status;
    }

    void AttachToCanvas(UCanvasPanel* Body, UWidget* Child)
    {
        if (!Body || !Child) return;
        if (Child->GetParent()) Child->RemoveFromParent();
        auto* Slot = Body->AddChildToCanvas(Child);
        Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        Slot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
        Slot->SetZOrder(5);
        Child->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    }

    FString IntegerArrayText(const FObject& Object, const TCHAR* Key)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object || !Object->TryGetArrayField(Key, Values) || !Values) return TEXT("服务未提供");
        TArray<FString> Parts;
        for (const auto& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::Number) return TEXT("服务未提供");
            Parts.Add(FString::FromInt(int32(Value->AsNumber())));
        }
        return Parts.IsEmpty() ? TEXT("0") : FString::Join(Parts, TEXT("、"));
    }

    bool ArrayCount(const FObject& Object, const TCHAR* Key, int32& OutCount)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object || !Object->TryGetArrayField(Key, Values) || !Values) return false;
        for (const auto& Value : *Values) if (!Value.IsValid()) return false;
        OutCount = Values->Num();
        return true;
    }
}

void UInkJourneyResourceDetailFeaturePanel::BuildPanelContent()
{
    if (!DetailWidget) DetailWidget = CreateWidget<UInkResourceDetailBatch2Widget>(GetOwningPlayer());
    if (!DetailWidget) return;
    DetailWidget->OnTravelRequested.AddUniqueDynamic(this, &ThisClass::HandleTravelRequested);
    DetailWidget->OnBackRequested.AddUniqueDynamic(this, &ThisClass::HandleChildBackRequested);
    InkJourneyBatch2Feature::AttachToCanvas(BodyCanvas, DetailWidget);
    RefreshLiveData();
}

void UInkJourneyResourceDetailFeaturePanel::RefreshLiveData()
{
    if (!DetailWidget || !IsRolloutPageActive()) return;
    auto* Service = WulingFrontendData::Service(this);
    const auto Context = InkJourneyAdapter::Capture(Service, true);
    const auto View = InkJourneyAdapter::Read(Context);
    const FString Key = InkJourneyBatch2Feature::SnapshotKey(Context, SelectedEntry);
    if (Key == AuthorityContentKey) return;
    AuthorityContentKey = Key;

    const auto Entry = InkJourneyBatch2Feature::FindRow(View.Resources, SelectedEntry);
    TArray<FInkJourneyDisplayRow> Rows;
    FString Status = InkJourneyBatch2Feature::Availability(Context);
    if (Entry)
    {
        bool bOpen = false;
        if (InkJourneyBatch2Feature::Flag(Entry, TEXT("open"), bOpen))
            Status = bOpen ? TEXT("资源开放状态由服务确认") : TEXT("当前资源未开放");
        else Status = TEXT("开放状态待服务确认");
        double NumberValue = 0;
        const FString ItemName = InkJourneyBatch2Feature::String(Entry, TEXT("item_name"));
        if (!ItemName.IsEmpty()) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("产出"), ItemName, true));
        if (InkJourneyBatch2Feature::Number(Entry, TEXT("quantity"), NumberValue)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("单次数量"), InkJourneyBatch2Feature::NumberText(NumberValue)));
        if (InkJourneyBatch2Feature::Number(Entry, TEXT("interval_seconds"), NumberValue)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("结算间隔（秒）"), InkJourneyBatch2Feature::NumberText(NumberValue)));
        if (InkJourneyBatch2Feature::Number(Entry, TEXT("qi_additive"), NumberValue)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("修为加成"), InkJourneyBatch2Feature::NumberText(NumberValue)));
        if (InkJourneyBatch2Feature::Number(Entry, TEXT("lease_max_seconds"), NumberValue)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("开采权最长时长（秒）"), InkJourneyBatch2Feature::NumberText(NumberValue)));
        if (InkJourneyBatch2Feature::Number(Entry, TEXT("entry_count"), NumberValue)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("同类地点"), InkJourneyBatch2Feature::NumberText(NumberValue)));
        const auto Owners = InkJourneyAdapter::Rows(Entry, TEXT("owners"));
        if (Owners.bProvided)
        {
            FString Names;
            for (const auto& Owner : Owners.Items)
            {
                const FString Name = InkJourneyBatch2Feature::String(Owner, TEXT("name"));
                if (Name.IsEmpty()) continue;
                if (!Names.IsEmpty()) Names += TEXT("、");
                Names += Name;
            }
            Rows.Add(InkJourneyBatch2Feature::Row(TEXT("当前归属"), Names.IsEmpty() ? TEXT("当前无归属记录") : Names));
        }
    }
    else if (Context.Snapshot && View.Resources.bProvided)
        Status = TEXT("所选资源条目已不在当前权威目录中");
    else Status += TEXT(" · 资源详情待同步");

    const FString Name = InkJourneyBatch2Feature::String(Entry, TEXT("name"));
    DetailWidget->SetResourceDetail(SelectedEntry, FText::FromString(Name.IsEmpty() ? TEXT("资源详情待同步") : Name), FText::FromString(Status), Rows);

    if (DetailWidget->IsRequestPending() && Entry && Context.Snapshot)
    {
        const auto Queue = WulingFrontendData::Object(Context.Snapshot, TEXT("race_queue"));
        const FString QueueNode = InkJourneyBatch2Feature::String(Queue, TEXT("node_id"));
        const FString QueueStatus = InkJourneyBatch2Feature::String(Queue, TEXT("status"));
        if (QueueNode == InkJourneyBatch2Feature::String(Entry, TEXT("node_id")))
        {
            if (QueueStatus == TEXT("Queued")) DetailWidget->SetTravelFeedback(FText::FromString(TEXT("服务已确认 · 等待匹配")), false);
            else if (QueueStatus == TEXT("Matched")) DetailWidget->SetTravelFeedback(FText::FromString(TEXT("服务已确认 · 已匹配")), false);
            else if (QueueStatus == TEXT("Cancelled")) DetailWidget->SetTravelFeedback(FText::FromString(TEXT("匹配请求已取消")), false);
        }
    }
}

void UInkJourneyResourceDetailFeaturePanel::HandleTravelRequested(const FString& EntryId)
{
    if (!DetailWidget) return;
    if (EntryId != SelectedEntry)
    {
        DetailWidget->SetTravelFeedback(FText::FromString(TEXT("所选地点已变化，请重新查看详情")), false);
        return;
    }
    auto* Service = WulingFrontendData::Service(this);
    const InkJourneyAdapter::FContext Fresh = InkJourneyAdapter::Capture(Service, IsRolloutPageActive());
    InkJourneyAdapter::FRequest Request;
    Request.Action = InkJourneyAdapter::EAction::ResourceTravel;
    Request.EntryId = EntryId;
    Request.bConfirmed = true;
    const auto Gate = InkJourneyAdapter::Evaluate(Fresh, Request);
    if (!Gate.bAllowed)
    {
        DetailWidget->SetTravelFeedback(FText::FromString(Gate.Reason), false);
        return;
    }
    FString Feedback;
    const bool bEnqueued = InkJourneyAdapter::Submit(Service, IsRolloutPageActive(), Request, Feedback);
    DetailWidget->SetTravelFeedback(FText::FromString(Feedback), bEnqueued);
}

void UInkJourneyResourceDetailFeaturePanel::HandleChildBackRequested()
{
    HandleAction(FName(TEXT("Back")));
}

void UInkJourneyResourceDetailFeaturePanel::HandlePanelAction(const FString& Action)
{
    if (Action == TEXT("ResetInteraction") && DetailWidget)
    {
        DetailWidget->ResetInteractionForReopen();
        return;
    }
    Super::HandlePanelAction(Action);
}

void UInkJourneyResourceDetailFeaturePanel::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    if (!IsRolloutPageActive()) return;
    RefreshElapsed += DeltaSeconds;
    if (RefreshElapsed >= .35f)
    {
        RefreshElapsed = 0.f;
        RefreshLiveData();
    }
}

void UInkJourneyOutcomeFeaturePanel::BuildPanelContent()
{
    const auto WidgetClass = OutcomeWidgetClass();
    if (!OutcomeWidget && WidgetClass) OutcomeWidget = CreateWidget<UInkJourneyOutcomeBatch2Widget>(GetOwningPlayer(), WidgetClass);
    if (!OutcomeWidget) return;
    OutcomeWidget->OnBackRequested.AddUniqueDynamic(this, &ThisClass::HandleChildBackRequested);
    InkJourneyBatch2Feature::AttachToCanvas(BodyCanvas, OutcomeWidget);
    RefreshLiveData();
}

void UInkJourneyOutcomeFeaturePanel::RefreshLiveData()
{
    if (!OutcomeWidget || !IsRolloutPageActive()) return;
    auto* Service = WulingFrontendData::Service(this);
    const auto Context = InkJourneyAdapter::Capture(Service, true);
    const FString Key = InkJourneyBatch2Feature::SnapshotKey(Context, SelectedEntry);
    if (Key == AuthorityContentKey) return;
    AuthorityContentKey = Key;
    PopulateOutcome(Context, OutcomeWidget);
}

void UInkJourneyOutcomeFeaturePanel::HandleChildBackRequested()
{
    HandleAction(FName(TEXT("Back")));
}

void UInkJourneyOutcomeFeaturePanel::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    if (!IsRolloutPageActive()) return;
    RefreshElapsed += DeltaSeconds;
    if (RefreshElapsed >= .35f)
    {
        RefreshElapsed = 0.f;
        RefreshLiveData();
    }
}

void UInkWorldResultFeaturePanel::PopulateOutcome(const InkJourneyAdapter::FContext& Context, UInkJourneyOutcomeBatch2Widget* Widget)
{
    if (!Widget) return;
    const auto Round = Context.Snapshot ? WulingFrontendData::Object(Context.Snapshot, TEXT("round")) : nullptr;
    const FString RoundId = InkJourneyBatch2Feature::String(Round, TEXT("id"));
    if (!SelectedEntry.IsEmpty() && RoundId != SelectedEntry)
    {
        Widget->SetOutcome(FText::FromString(TEXT("世界历练结果")), FText::FromString(TEXT("当前权威轮次与所选结果不一致")), TArray<FInkJourneyDisplayRow>());
        return;
    }
    if (!Round)
    {
        Widget->SetOutcome(FText::FromString(TEXT("世界历练结果")), FText::FromString(InkJourneyBatch2Feature::Availability(Context)), TArray<FInkJourneyDisplayRow>());
        return;
    }
    TArray<FInkJourneyDisplayRow> Rows;
    const FString Status = InkJourneyBatch2Feature::String(Round, TEXT("status"));
    const FString Completed = InkJourneyBatch2Feature::IntegerArrayText(Round, TEXT("completed"));
    Rows.Add(InkJourneyBatch2Feature::Row(TEXT("已完成阶段"), Completed, true));
    Rows.Add(InkJourneyBatch2Feature::Row(TEXT("轮次状态"), Status.IsEmpty() ? TEXT("服务未提供") : Status));
    const auto Participants = WulingFrontendData::Object(Round, TEXT("participants"));
    if (Participants)
    {
        for (const auto& Pair : Participants->Values)
        {
            const auto Participant = Pair.Value.IsValid() ? Pair.Value->AsObject() : nullptr;
            int32 StageCountValue = 0;
            const FString StageCount = InkJourneyBatch2Feature::ArrayCount(Participant, TEXT("stages"), StageCountValue)
                ? FString::FromInt(StageCountValue) : TEXT("服务未提供");
            Rows.Add(InkJourneyBatch2Feature::Row(Pair.Key + TEXT(" · 已参与阶段"), StageCount));
        }
    }
    InkJourneyBatch2Feature::AppendExactReceipts(Context.Snapshot, RoundId, Rows);
    Widget->SetOutcome(FText::FromString(TEXT("世界历练结果")), FText::FromString(InkJourneyBatch2Feature::Lifecycle(Status)), Rows);
}

void UInkRaceResultFeaturePanel::PopulateOutcome(const InkJourneyAdapter::FContext& Context, UInkJourneyOutcomeBatch2Widget* Widget)
{
    if (!Widget) return;
    const auto Race = Context.Snapshot ? WulingFrontendData::Object(Context.Snapshot, TEXT("race")) : nullptr;
    const auto Identity = Context.Snapshot ? WulingFrontendData::Object(Context.Snapshot, TEXT("identity")) : nullptr;
    const FString RaceId = InkJourneyBatch2Feature::String(Race, TEXT("id"));
    if (!SelectedEntry.IsEmpty() && RaceId != SelectedEntry)
    {
        Widget->SetOutcome(FText::FromString(TEXT("灵脉竞采结果")), FText::FromString(TEXT("当前权威竞采与所选结果不一致")), TArray<FInkJourneyDisplayRow>());
        return;
    }
    const auto Team = InkJourneyBatch2Feature::FindRaceTeam(Race, Identity);
    if (!Race || !Team)
    {
        Widget->SetOutcome(FText::FromString(TEXT("灵脉竞采结果")), FText::FromString(InkJourneyBatch2Feature::Availability(Context) + TEXT(" · 竞采结果待同步")), TArray<FInkJourneyDisplayRow>());
        return;
    }
    TArray<FInkJourneyDisplayRow> Rows;
    const FString Status = InkJourneyBatch2Feature::String(Race, TEXT("status"));
    const FString NodeId = InkJourneyBatch2Feature::String(Race, TEXT("node_id"));
    if (!NodeId.IsEmpty()) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("竞采节点"), NodeId));
    double Score = 0;
    if (InkJourneyBatch2Feature::Number(Team, TEXT("score"), Score)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("团队总分"), InkJourneyBatch2Feature::NumberText(Score), true));
    bool bEligible = false;
    if (InkJourneyBatch2Feature::Flag(Team, TEXT("eligible"), bEligible)) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("参与资格"), bEligible ? TEXT("符合服务记录") : TEXT("未符合服务记录")));
    bool bForfeited = false;
    if (InkJourneyBatch2Feature::Flag(Team, TEXT("rank_forfeited"), bForfeited) && bForfeited) Rows.Add(InkJourneyBatch2Feature::Row(TEXT("排名状态"), TEXT("服务记录为弃权")));

    int32 CommonCount = 0;
    if (InkJourneyBatch2Feature::ArrayCount(Team, TEXT("common"), CommonCount))
        Rows.Add(InkJourneyBatch2Feature::Row(TEXT("护阵 / 破障协同分"), InkJourneyBatch2Feature::NumberText(CommonCount * 100.0)));
    int32 AnchorActionCount = 0;
    if (InkJourneyBatch2Feature::ArrayCount(Team, TEXT("anchor_actions"), AnchorActionCount))
        Rows.Add(InkJourneyBatch2Feature::Row(TEXT("锚点应对分"), InkJourneyBatch2Feature::NumberText(AnchorActionCount * 100.0)));
    const auto Responses = WulingFrontendData::Object(Team, TEXT("responses"));
    if (Responses)
    {
        int32 ResponseCount = 0;
        for (const auto& Pair : Responses->Values)
        {
            const auto& Value = Pair.Value;
            if (Value.IsValid() && Value->Type == EJson::Array) ResponseCount += Value->AsArray().Num();
        }
        int32 RepresentativeCount = 0;
        if (InkJourneyBatch2Feature::ArrayCount(Team, TEXT("representatives"), RepresentativeCount) && RepresentativeCount > 0)
            Rows.Add(InkJourneyBatch2Feature::Row(TEXT("代表应答分"), InkJourneyBatch2Feature::NumberText(200.0 * ResponseCount / (2.0 * RepresentativeCount))));
        Rows.Add(InkJourneyBatch2Feature::Row(TEXT("代表响应数"), FString::FromInt(ResponseCount)));
    }

    const auto Lease = Context.Snapshot ? WulingFrontendData::Object(Context.Snapshot, TEXT("lease")) : nullptr;
    if (Lease && InkJourneyBatch2Feature::String(Lease, TEXT("race_id")) == RaceId)
    {
        const FString LeaseStatus = InkJourneyBatch2Feature::String(Lease, TEXT("status"));
        double End = 0;
        const FString EndText = InkJourneyBatch2Feature::Number(Lease, TEXT("end"), End) ? InkJourneyBatch2Feature::NumberText(End) : TEXT("服务未提供");
        Rows.Add(InkJourneyBatch2Feature::Row(TEXT("开采权状态"), LeaseStatus.IsEmpty() ? TEXT("服务未提供") : LeaseStatus));
        Rows.Add(InkJourneyBatch2Feature::Row(TEXT("开采权结束时间（服务时间戳）"), EndText));
    }
    const FString Anchor = InkJourneyBatch2Feature::String(Team, TEXT("anchor"));
    if (!Anchor.IsEmpty()) InkJourneyBatch2Feature::AppendExactReceipts(Context.Snapshot, TEXT("race-") + RaceId + TEXT(":") + Anchor, Rows);
    Widget->SetOutcome(FText::FromString(TEXT("灵脉竞采结果")), FText::FromString(InkJourneyBatch2Feature::Lifecycle(Status)), Rows);
}

