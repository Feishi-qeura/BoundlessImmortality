#include "InkActivityRolloutPanels.h"

#include "Dom/JsonObject.h"
#include "Frontend/WulingFrontendData.h"
#include "InkPresentationStyle.h"
#include "Wuling/WulingPrototypeSubsystem.h"

namespace
{
using ESurface = InkPresentationStyle::ESurface;

const TCHAR* CycleTitle(const FString& Cycle)
{
    if (Cycle == TEXT("daily")) return TEXT("日常活动");
    if (Cycle == TEXT("limited")) return TEXT("限时活动");
    if (Cycle == TEXT("permanent")) return TEXT("常驻活动");
    if (Cycle == TEXT("launch")) return TEXT("开服活动");
    return TEXT("活动");
}

FString FieldOrPending(const TSharedPtr<FJsonObject>& Row, const TCHAR* Key)
{
    FString Value;
    return Row && Row->TryGetStringField(Key, Value) && !Value.IsEmpty() ? Value : TEXT("待同步");
}

FString EnabledState(const TSharedPtr<FJsonObject>& Row)
{
    bool bEnabled = false;
    if (!Row || !Row->TryGetBoolField(TEXT("enabled"), bEnabled)) return TEXT("启用状态待同步");
    return bEnabled ? TEXT("已启用") : TEXT("当前未启用");
}

TArray<TSharedPtr<FJsonObject>> TemplatesForCycle(const TSharedPtr<FJsonObject>& Activities, const FString& Cycle)
{
    TArray<TSharedPtr<FJsonObject>> Matches;
    const TArray<TSharedPtr<FJsonValue>>* Templates = nullptr;
    if (!Activities || !Activities->TryGetArrayField(TEXT("templates"), Templates) || !Templates) return Matches;
    for (const TSharedPtr<FJsonValue>& Value : *Templates)
    {
        const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
        if (Row && WulingFrontendData::String(Row, TEXT("cycle")) == Cycle) Matches.Add(Row);
    }
    return Matches;
}

FString SnapshotState(const UWulingPrototypeSubsystem* Service, const TSharedPtr<FJsonObject>& Activities)
{
    if (!Service || !Service->IsEnabled()) return TEXT("活动服务未启用，当前无法读取活动配置。");
    if (!Service->IsConnected()) return TEXT("活动服务未连接；保留当前界面，恢复连接后可重新读取。");
    if (Service->IsBusy()) return TEXT("正在读取活动状态…");
    if (!Activities.IsValid()) return TEXT("活动快照尚未提供或读取失败；请稍后重试。");
    const TArray<TSharedPtr<FJsonValue>>* Templates = nullptr;
    if (!Activities->TryGetArrayField(TEXT("templates"), Templates) || !Templates)
        return TEXT("服务端尚未提供活动配置。");
    return FString();
}

}

void UInkActivitiesRolloutPanel::BuildPanelContent()
{
    const UWulingPrototypeSubsystem* Service = WulingFrontendData::Service(this);
    const TSharedPtr<FJsonObject> Activities = WulingFrontendData::Activity(this);
    const FString State = SnapshotState(Service, Activities);

    Surface(BodyCanvas, TEXT("ActivitiesRolloutTitlePaper"), ESurface::FeatureHeading, 0, 0, BodyWidth, 56);
    Label(BodyCanvas, TEXT("ActivitiesRolloutTitle"), TEXT("活动分类"), 12, 4, BodyWidth - 24, 48, 32);

    const TCHAR* Cycles[] = { TEXT("daily"), TEXT("limited"), TEXT("permanent"), TEXT("launch") };
    const float TabGap = 8.f;
    const float TabWidth = (BodyWidth - TabGap * 3) / 4.f;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cycles); ++Index)
    {
        const FString Cycle(Cycles[Index]);
        Action(BodyCanvas, FString(TEXT("ActivityCycle_")) + Cycle, CycleTitle(Cycle), Index * (TabWidth + TabGap), 64, TabWidth, 88,
            false, false, SelectedCycle == Cycle);
    }

    Surface(BodyCanvas, TEXT("ActivitiesRolloutAuthorityPaper"), ESurface::FeatureCard, 0, 164, BodyWidth, 96);
    const FString AuthorityMessage = State.IsEmpty()
        ? TEXT("活动内容、开放时间和状态以服务端快照为准。未提供的进度、奖励和领取状态不会显示。")
        : State;
    Label(BodyCanvas, TEXT("ActivitiesRolloutAuthority"), AuthorityMessage, 16, 174, BodyWidth - 32, 76, 20);

    float Y = 276.f;
    if (SelectedCycle == TEXT("launch"))
    {
        Surface(BodyCanvas, TEXT("ActivitiesRolloutLaunchPaper"), ESurface::FeatureCard, 0, Y, BodyWidth, 120);
        Label(BodyCanvas, TEXT("ActivitiesRolloutLaunchState"), TEXT("开服任务、贡献榜与活动期限由开服活动页按权威快照展示。"), 16, Y + 12, BodyWidth - 32, 54, 20);
        Action(BodyCanvas, TEXT("Open_LaunchEvent"), TEXT("查看开服活动"), 12, Y + 68, BodyWidth - 24, 88, true);
        Surface(BodyCanvas, TEXT("ActivitiesRolloutOfferPaper"), ESurface::FeatureCard, 0, Y + 136, BodyWidth, 112);
        Label(BodyCanvas, TEXT("ActivitiesRolloutOfferStatus"), TEXT("特惠商品入口暂不可用：活动快照未提供可核实的 SKU 与价格。"), 16, Y + 144, BodyWidth - 32, 52, 19);
        Action(BodyCanvas, TEXT("OfferUnavailable"), TEXT("特惠资料待同步"), 12, Y + 198, BodyWidth - 24, 88)->SetIsEnabled(false);
        return;
    }

    if (!State.IsEmpty())
    {
        Surface(BodyCanvas, TEXT("ActivitiesRolloutStatePaper"), ESurface::FeatureCard, 0, Y, BodyWidth, 96);
        Label(BodyCanvas, TEXT("ActivitiesRolloutState"), State, 16, Y + 10, BodyWidth - 32, 76, 21);
        return;
    }

    const TArray<TSharedPtr<FJsonObject>> Rows = TemplatesForCycle(Activities, SelectedCycle);
    if (Rows.IsEmpty())
    {
        Surface(BodyCanvas, TEXT("ActivitiesRolloutEmptyPaper"), ESurface::FeatureCard, 0, Y, BodyWidth, 112);
        Label(BodyCanvas, TEXT("ActivitiesRolloutEmpty"), FString(TEXT("当前分类暂无")) + CycleTitle(SelectedCycle) + TEXT("配置。"), 16, Y + 12, BodyWidth - 32, 88, 23);
        return;
    }

    const bool bWide = !bPortraitLayout && BodyWidth >= 800;
    const int32 Columns = bWide ? 2 : 1;
    const float Gap = 16.f;
    const float CardWidth = (BodyWidth - (Columns - 1) * Gap) / Columns;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const float X = (Index % Columns) * (CardWidth + Gap);
        const float CardY = Y + (Index / Columns) * 200.f;
        const TSharedPtr<FJsonObject>& Row = Rows[Index];
        const FString Prefix = FString::Printf(TEXT("ActivityRollout_%d_"), Index);
        Surface(BodyCanvas, NAME_None, ESurface::FeatureCard, X, CardY, CardWidth, 184);
        Surface(BodyCanvas, NAME_None, ESurface::FeatureHeading, X + 8, CardY + 8, CardWidth - 16, 48);
        Label(BodyCanvas, FName(*(Prefix + TEXT("Title"))), FieldOrPending(Row, TEXT("name")), X + 16, CardY + 12, CardWidth - 32, 40, 27);
        const FString Id = WulingFrontendData::String(Row, TEXT("activity_id"));
        Label(BodyCanvas, FName(*(Prefix + TEXT("Status"))), FString(TEXT("配置：")) + EnabledState(Row) + TEXT(" · 编号：") + (Id.IsEmpty() ? TEXT("待同步") : Id), X + 16, CardY + 64, CardWidth - 32, 36, 19);
        const FString End = SelectedCycle == TEXT("limited") ? FieldOrPending(Row, TEXT("end_at")) : FString(TEXT("不适用/待同步"));
        Label(BodyCanvas, FName(*(Prefix + TEXT("Time"))), FString(TEXT("开始：")) + FieldOrPending(Row, TEXT("start_at")) + TEXT(" · 截止：") + End, X + 16, CardY + 108, CardWidth - 32, 52, 18);
    }
}

void UInkActivitiesRolloutPanel::HandlePanelAction(const FString& ActionId)
{
    if (ActionId.StartsWith(TEXT("ActivityCycle_")))
    {
        SelectedCycle = ActionId.RightChop(14);
        RebuildPanel();
        return;
    }
    if (ActionId == TEXT("Open_LaunchEvent"))
    {
        Open(TEXT("LaunchEvent"));
        return;
    }
    Super::HandlePanelAction(ActionId);
}

FText UInkActivityCycleRolloutPanel::PanelTitle() const
{
    return FText::FromString(CycleTitle(FString(ActivityCycle())));
}

void UInkActivityCycleRolloutPanel::BuildPanelContent()
{
    const FString Cycle(ActivityCycle());
    const UWulingPrototypeSubsystem* Service = WulingFrontendData::Service(this);
    const TSharedPtr<FJsonObject> Activities = WulingFrontendData::Activity(this);
    const FString State = SnapshotState(Service, Activities);

    Surface(BodyCanvas, TEXT("ActivityCycleTitlePaper"), ESurface::FeatureHeading, 0, 0, BodyWidth, 56);
    Label(BodyCanvas, TEXT("ActivityCycleTitle"), CycleTitle(Cycle), 12, 4, BodyWidth - 24, 48, 32);
    Surface(BodyCanvas, TEXT("ActivityCycleAuthorityPaper"), ESurface::FeatureCard, 0, 64, BodyWidth, 96);
    const FString CategoryNotice = Cycle == TEXT("daily")
        ? TEXT("仅显示 cycle=daily 的明确分类；不推测玩法进度、预算或奖励。")
        : Cycle == TEXT("limited")
            ? TEXT("仅显示 cycle=limited 的明确分类；服务端时间戳与活动状态决定期限。")
            : TEXT("仅显示 cycle=permanent 的明确分类；不把未知周期归为常驻。");
    Label(BodyCanvas, TEXT("ActivityCycleAuthority"), State.IsEmpty() ? CategoryNotice : State, 16, 74, BodyWidth - 32, 76, 20);

    if (!State.IsEmpty())
    {
        Surface(BodyCanvas, TEXT("ActivityCycleStatePaper"), ESurface::FeatureCard, 0, 176, BodyWidth, 96);
        Label(BodyCanvas, TEXT("ActivityCycleState"), State, 16, 186, BodyWidth - 32, 76, 21);
        return;
    }

    const TArray<TSharedPtr<FJsonObject>> Rows = TemplatesForCycle(Activities, Cycle);
    float Y = 176.f;
    if (Rows.IsEmpty())
    {
        Surface(BodyCanvas, TEXT("ActivityCycleEmptyPaper"), ESurface::FeatureCard, 0, Y, BodyWidth, 112);
        Label(BodyCanvas, TEXT("ActivityCycleEmpty"), FString(TEXT("当前配置中暂无")) + CycleTitle(Cycle) + TEXT("条目。"), 16, Y + 12, BodyWidth - 32, 88, 23);
        return;
    }

    const bool bWide = !bPortraitLayout && BodyWidth >= 800;
    const int32 Columns = bWide ? 2 : 1;
    const float Gap = 16.f;
    const float CardWidth = (BodyWidth - (Columns - 1) * Gap) / Columns;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const float X = (Index % Columns) * (CardWidth + Gap);
        const float CardY = Y + (Index / Columns) * 200.f;
        const TSharedPtr<FJsonObject>& Row = Rows[Index];
        const FString Prefix = FString::Printf(TEXT("ActivityCycle_%d_"), Index);
        Surface(BodyCanvas, NAME_None, ESurface::FeatureCard, X, CardY, CardWidth, 184);
        Surface(BodyCanvas, NAME_None, ESurface::FeatureHeading, X + 8, CardY + 8, CardWidth - 16, 48);
        Label(BodyCanvas, FName(*(Prefix + TEXT("Title"))), FieldOrPending(Row, TEXT("name")), X + 16, CardY + 12, CardWidth - 32, 40, 27);
        const FString Id = WulingFrontendData::String(Row, TEXT("activity_id"));
        Label(BodyCanvas, FName(*(Prefix + TEXT("Status"))), FString(TEXT("配置：")) + EnabledState(Row) + TEXT(" · 编号：") + (Id.IsEmpty() ? TEXT("待同步") : Id), X + 16, CardY + 64, CardWidth - 32, 36, 19);
        const FString End = Cycle == TEXT("limited") ? FieldOrPending(Row, TEXT("end_at")) : FString(TEXT("不适用/待同步"));
        Label(BodyCanvas, FName(*(Prefix + TEXT("Time"))), FString(TEXT("开始：")) + FieldOrPending(Row, TEXT("start_at")) + TEXT(" · 截止：") + End, X + 16, CardY + 108, CardWidth - 32, 52, 18);
    }
}

void UInkActivityCycleRolloutPanel::HandlePanelAction(const FString& ActionId)
{
    Super::HandlePanelAction(ActionId);
}
