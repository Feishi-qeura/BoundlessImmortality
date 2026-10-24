#include "InkWulingSectPanel.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/WulingFrontendData.h"
#include "Frontend/UI/InkUiTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
using namespace WulingFrontendData;

FString Amount(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
{
    double Value = 0;
    return Object && Object->TryGetNumberField(Field, Value)
        ? FString::Printf(TEXT("%.0f"), Value) : FString(TEXT("—"));
}

FString RoundState(const TSharedPtr<FJsonObject>& Round)
{
    const FString Phase = String(Round, TEXT("status"));
    if (Phase == TEXT("Preparing")) return TEXT("历练准备中");
    if (Phase == TEXT("Running")) return TEXT("历练进行中");
    if (Phase == TEXT("Settling")) return TEXT("阶段结算中");
    if (Phase == TEXT("Completed")) return TEXT("本轮历练已结算");
    if (Phase == TEXT("Interrupted")) return TEXT("本轮历练已中断");
    return TEXT("可从仙途地图选择历练区域");
}

// Only fields used by this overview participate. A ticking world clock, wallet,
// and unrelated progression updates never invalidate the editable name widget.
FString PresentationKey(const TSharedPtr<FJsonObject>& State, const UWulingPrototypeSubsystem* Service)
{
    FString Key;
    auto Append = [&Key](const FString& Value)
    {
        Key += FString::FromInt(Value.Len()) + TEXT(":") + Value;
    };
    Append(Service ? FString::Printf(TEXT("%d%d%d"), Service->IsEnabled(), Service->IsConnected(), Service->IsBusy()) : TEXT("none"));
    Append(Service ? Service->GetStatus() : FString());
    Append(State ? TEXT("snapshot") : TEXT("no snapshot"));
    const auto Guild = Object(State, TEXT("guild"));
    Append(Guild ? TEXT("guild") : TEXT("no guild"));
    Append(String(Guild, TEXT("name")));
    Append(Amount(Guild, TEXT("supply")));
    Append(Amount(Guild, TEXT("reputation")));
    const auto Counts = Object(State, TEXT("counts"));
    for (const TCHAR* Field : {TEXT("members"), TEXT("participating_members"), TEXT("helpers")})
        Append(Amount(Counts, Field));
    Append(Amount(State, TEXT("member_cursor")));
    const TArray<TSharedPtr<FJsonValue>>* Members = nullptr;
    if (State && State->TryGetArrayField(TEXT("members"), Members))
    {
        Append(FString::FromInt(Members->Num()));
        for (int32 Index = 0; Index < FMath::Min(4, Members->Num()); ++Index)
            Append(String((*Members)[Index]->AsObject(), TEXT("name")));
    }
    else Append(TEXT("no roster"));
    const auto Round = Object(State, TEXT("round"));
    Append(String(Round, TEXT("status")));
    Append(String(Round, TEXT("region_name")));
    return Key;
}
}

FText UInkWulingSectPanel::PanelTitle() const
{
    return FText::FromString(TEXT("武灵 · 山门"));
}

void UInkWulingSectPanel::BuildPanelContent()
{
    // Layout changes may recreate the body; snapshot changes only edit text below.
    if (GuildNameInput) DraftName = GuildNameInput->GetText().ToString();
    GuildNameInput = nullptr;
    CreateButton = nullptr;
    auto* Service = WulingFrontendData::Service(this);
    bBuiltWithGuild = Service && Service->IsEnabled() && Object(Service->Snapshot(), TEXT("guild")).IsValid();
    const float Gap = 24.f;
    const bool TwoColumns = !bPortraitLayout && BodyWidth >= 800.f;
    const float ColumnWidth = TwoColumns ? (BodyWidth - Gap) * .5f : BodyWidth;
    // The organization snapshot has no banner or identity image field. Do not
    // reserve an empty hero surface until real artwork is available.
    const float HeroHeight = 0.f;
    const float IdentityY = HeroHeight + 12.f;
    GuildTitle = Label(BodyCanvas, TEXT("SectGuildName"), TEXT("武灵山门"), 0, IdentityY, ColumnWidth, 64, 40);
    ConnectionText = Label(BodyCanvas, TEXT("SectConnection"), TEXT("等待组织数据"), 0, IdentityY + 72, ColumnWidth, 96, 24);

    const float CountsY = IdentityY + 184;
    const float StatWidth = (ColumnWidth - 24.f) / 3.f;
    Surface(BodyCanvas, TEXT("SectStatsCard"), InkPresentationStyle::ESurface::FeatureCard, 0, CountsY - 8, ColumnWidth, 210);
    auto Count = [&](const TCHAR* Name, const TCHAR* Caption, int32 Index)
    {
        const float X = Index * (StatWidth + 12.f);
        Label(BodyCanvas, NAME_None, Caption, X, CountsY, StatWidth, 36, 24);
        return Label(BodyCanvas, Name, TEXT("—"), X, CountsY + 40, StatWidth, 48, 32);
    };
    MemberCount = Count(TEXT("SectMembersCount"), TEXT("正式成员"), 0);
    ParticipatingCount = Count(TEXT("SectParticipatingCount"), TEXT("参与成员"), 1);
    HelperCount = Count(TEXT("SectHelpersCount"), TEXT("临时助战"), 2);
    SupplyText = Label(BodyCanvas, TEXT("SectSupply"), TEXT("公共资材 —"), 0, CountsY + 108, ColumnWidth, 44, 28);
    ReputationText = Label(BodyCanvas, TEXT("SectReputation"), TEXT("武灵声望 —"), 0, CountsY + 156, ColumnWidth, 44, 28);

    const float CreateY = CountsY + 228;
    const float HintY = CreateY + (bBuiltWithGuild ? 60.f : 264.f);
    Surface(BodyCanvas, TEXT("SectCreationCard"), InkPresentationStyle::ESurface::FeatureCard, 0, CreateY - 8, ColumnWidth, HintY - CreateY + 120);
    CreationTitle = Label(BodyCanvas, TEXT("SectCreationTitle"), TEXT("立下山门"), 0, CreateY, ColumnWidth, 48, 32);
    if (!bBuiltWithGuild)
    {
        GuildNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("SectNameInput"));
        GuildNameInput->SetText(FText::FromString(DraftName));
        GuildNameInput->SetHintText(FText::FromString(TEXT("输入武灵名称")));
        auto InputStyle = GuildNameInput->GetWidgetStyle();
        InputStyle.SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Body, 28));
        GuildNameInput->SetWidgetStyle(InputStyle);
        auto* InputSlot = BodyCanvas->AddChildToCanvas(GuildNameInput);
        InputSlot->SetPosition(FVector2D(0, CreateY + 60));
        InputSlot->SetSize(FVector2D(ColumnWidth, 88));
        InputSlot->SetZOrder(5);
        CreateButton = Action(BodyCanvas, TEXT("SectCreate"), TEXT("创建武灵"), 0, CreateY + 160, ColumnWidth, 88, true);
    }
    CreationHint = Label(BodyCanvas, TEXT("SectCreationHint"), TEXT("名称由服务校验，创建后同步真实组织状态。"), 0, HintY, ColumnWidth, 112, 24);

    const float LeftEnd = HintY + 124.f;
    const float RightX = TwoColumns ? ColumnWidth + Gap : 0;
    const float RightY = TwoColumns ? 0 : LeftEnd + 20;
    Surface(BodyCanvas, TEXT("SectRosterCard"), InkPresentationStyle::ESurface::FeatureCard, RightX, RightY, ColumnWidth, 416);
    Surface(BodyCanvas, TEXT("SectRosterHeadingCard"), InkPresentationStyle::ESurface::FeatureHeading, RightX, RightY, ColumnWidth, 48);
    Label(BodyCanvas, TEXT("SectRosterHeading"), TEXT("成员名册"), RightX, RightY, ColumnWidth, 48, 32);
    RosterStatus = Label(BodyCanvas, TEXT("SectRosterStatus"), TEXT("等待成员同步"), RightX, RightY + 56, ColumnWidth, 64, 24);
    RosterText = Label(BodyCanvas, TEXT("SectRosterPreview"), TEXT(""), RightX, RightY + 128, ColumnWidth, 176, 26);
    Action(BodyCanvas, TEXT("Open_WulingGuild"), TEXT("查看名册与组织管理"), RightX, RightY + 316, ColumnWidth, 88);

    Surface(BodyCanvas, TEXT("SectOperationsCard"), InkPresentationStyle::ESurface::FeatureCard, RightX, RightY + 420, ColumnWidth, 500);
    Surface(BodyCanvas, TEXT("SectOperationsHeadingCard"), InkPresentationStyle::ESurface::FeatureHeading, RightX, RightY + 428, ColumnWidth, 48);
    Label(BodyCanvas, TEXT("SectOrganizationHeading"), TEXT("组织事务"), RightX, RightY + 428, ColumnWidth, 48, 32);
    const float HalfWidth = (ColumnWidth - 12) * .5f;
    Action(BodyCanvas, TEXT("Open_WulingConstruction"), TEXT("门派建设"), RightX, RightY + 488, HalfWidth, 88);
    Action(BodyCanvas, TEXT("Open_WulingRights"), TEXT("剑灵权益"), RightX + HalfWidth + 12, RightY + 488, HalfWidth, 88);
    Action(BodyCanvas, TEXT("Open_WulingEquipment"), TEXT("装备成长"), RightX, RightY + 588, HalfWidth, 88);
    Action(BodyCanvas, TEXT("Open_WulingOffline"), TEXT("离线记录"), RightX + HalfWidth + 12, RightY + 588, HalfWidth, 88);
    Action(BodyCanvas, TEXT("Open_WulingOrders"), TEXT("订单与收据"), RightX, RightY + 688, HalfWidth, 88);
    Action(BodyCanvas, TEXT("Open_WulingActivity"), TEXT("活动与签到"), RightX + HalfWidth + 12, RightY + 688, HalfWidth, 88);
    Action(BodyCanvas, TEXT("Open_GiftRules"), TEXT("礼物效果规则"), RightX, RightY + 788, ColumnWidth, 88);

    Surface(BodyCanvas, TEXT("SectWorldCard"), InkPresentationStyle::ESurface::FeatureCard, RightX, RightY + 892, ColumnWidth, 250);
    Surface(BodyCanvas, TEXT("SectWorldHeadingCard"), InkPresentationStyle::ESurface::FeatureHeading, RightX, RightY + 900, ColumnWidth, 48);
    Label(BodyCanvas, TEXT("SectWorldHeading"), TEXT("下山历练"), RightX, RightY + 900, ColumnWidth, 48, 32);
    WorldStatus = Label(BodyCanvas, TEXT("SectWorldStatus"), TEXT(""), RightX, RightY + 956, ColumnWidth, 80, 24);
    Action(BodyCanvas, TEXT("Open_WorldCultivation"), TEXT("前往仙途地图"), RightX, RightY + 1048, ColumnWidth, 88, true);
    Action(BodyCanvas, TEXT("Open_WulingWorld"), TEXT("历练准备与进度"), RightX, RightY + 1148, ColumnWidth, 88);

    SectStateKey.Reset();
    RefreshLiveData();
}

void UInkWulingSectPanel::RefreshLiveData()
{
    if (!GuildTitle) return;
    auto* Service = WulingFrontendData::Service(this);
    const auto Snapshot = Service && Service->IsEnabled() ? Service->Snapshot() : nullptr;
    const auto Guild = Object(Snapshot, TEXT("guild"));
    // A confirmed organization transition changes the authored layout once.
    // Ordinary snapshots retain the same editable widget and its typing focus.
    if (bAwaitingCreationConfirmation && Guild)
    {
        bAwaitingCreationConfirmation = false;
        FeedbackMessage(TEXT("武灵创建已确认 · ") + String(Guild, TEXT("name")));
    }
    else if (bAwaitingCreationConfirmation && Service && Service->IsConnected() && !Service->IsBusy())
    {
        bAwaitingCreationConfirmation = false;
        FeedbackMessage(Service->GetStatus());
    }
    if (Snapshot && Guild.IsValid() != bBuiltWithGuild)
    {
        RebuildPanel();
        if(ResponsiveScroll)ResponsiveScroll->ScrollToStart();
        return;
    }
    const FString Key = PresentationKey(Snapshot, Service);
    if (Key == SectStateKey) return;
    SectStateKey = Key;

    const auto Counts = Object(Snapshot, TEXT("counts"));
    const bool Connected = Service && Service->IsEnabled() && Service->IsConnected();
    const bool Ready = Connected && !Service->IsBusy() && Snapshot.IsValid();
    auto Set = [](UTextBlock* Text, const FString& Value)
    {
        if (Text && Text->GetText().ToString() != Value) Text->SetText(FText::FromString(Value));
    };
    FString Name = String(Guild, TEXT("name"));
    Set(GuildTitle, Guild ? (Name.IsEmpty() ? FString(TEXT("武灵名称待同步")) : Name) : Snapshot ? FString(TEXT("尚未创建武灵")) : FString(TEXT("武灵山门")));
    FString Status;
    if (!Service || !Service->IsEnabled()) Status = TEXT("武灵服务未启用\n可浏览组织与历练入口；创建待服务开启。");
    else if (!Service->IsConnected()) Status = Snapshot ? TEXT("服务未连接 · 以下为上次同步\n重连后更新组织状态。") : TEXT("服务未连接 · 组织数据待同步\n创建暂不可用。");
    else if (!Snapshot) Status = TEXT("服务已连接 · 等待组织数据\n同步完成后可创建武灵。");
    else if (Service->IsBusy()) Status = TEXT("操作确认中 · 请稍候\n组织状态以服务确认结果为准。");
    else Status = Guild ? TEXT("山门已立 · 组织状态已同步\n正式成员与临时助战分别统计。") : TEXT("尚未立下山门\n主播创建武灵，观众剑灵可加入组织。");
    Set(ConnectionText, Status);
    Set(MemberCount, Amount(Counts, TEXT("members")));
    Set(ParticipatingCount, Amount(Counts, TEXT("participating_members")));
    Set(HelperCount, Amount(Counts, TEXT("helpers")));
    Set(SupplyText, TEXT("公共资材 ") + Amount(Guild, TEXT("supply")));
    Set(ReputationText, TEXT("武灵声望 ") + Amount(Guild, TEXT("reputation")));
    if (GuildNameInput) GuildNameInput->SetIsEnabled(Ready && !Guild);
    if (CreateButton) CreateButton->SetIsEnabled(Ready && !Guild);
    Set(CreationTitle, Guild ? TEXT("共守山门") : TEXT("立下山门"));
    Set(CreationHint, Guild ? TEXT("武灵已创建。可从名册管理组织，从建设与权益查看共同成长。\n公共资材与声望以服务快照为准。") : TEXT("名称由服务校验；创建请求确认后显示组织名称。\n临时助战不计为正式成员。"));

    const TArray<TSharedPtr<FJsonValue>>* Members = nullptr;
    const bool HasRoster = Snapshot && Snapshot->TryGetArrayField(TEXT("members"), Members);
    FString Roster;
    if (Guild && HasRoster)
    {
        const int32 MemberOffset = FMath::Max(0, Number(Snapshot, TEXT("member_cursor")));
        for (int32 Index = 0; Index < FMath::Min(4, Members->Num()); ++Index)
        {
            FString MemberName = String((*Members)[Index]->AsObject(), TEXT("name"));
            if (MemberName.IsEmpty()) MemberName = TEXT("名称待同步");
            if (MemberName.Len() > 18) MemberName = MemberName.Left(17) + TEXT("…");
            if (!Roster.IsEmpty()) Roster += TEXT("\n");
            Roster += FString::Printf(TEXT("%02d  %s"), MemberOffset + Index + 1, *MemberName);
        }
    }
    if (!Guild) Roster = Snapshot ? TEXT("创建武灵后，在此查看组织成员。") : TEXT("连接服务后显示真实成员。 ");
    else if (!HasRoster) Roster = TEXT("成员名单待服务同步。");
    else if (Roster.IsEmpty()) Roster = TEXT("本页暂无成员，完整名册以组织管理为准。");
    Set(RosterText, Roster);
    Set(RosterStatus, !Connected ? TEXT("离线浏览 · 名册待同步") : !Guild ? TEXT("正式成员、参与成员与助战各自统计") : TEXT("当前名册页预览 · 完整名单进入组织管理"));
    const auto Round = Object(Snapshot, TEXT("round"));
    const FString Region = String(Round, TEXT("region_name"));
    Set(WorldStatus, !Connected ? TEXT("连接服务后可确认历练进度。\n仙途地图可先浏览区域与入口。") : (Region.IsEmpty() ? FString() : Region + TEXT(" · ")) + RoundState(Round));
}

void UInkWulingSectPanel::HandlePanelAction(const FString& Id)
{
    if (Id != TEXT("SectCreate"))
    {
        Super::HandlePanelAction(Id);
        return;
    }
    auto* Service = WulingFrontendData::Service(this);
    if (!Service || !Service->IsEnabled() || !Service->IsConnected() || Service->IsBusy() || !Service->Snapshot())
    {
        FeedbackMessage(TEXT("服务未就绪或有操作待确认，暂不能创建武灵。"));
        RefreshLiveData();
        return;
    }
    if (Object(Service->Snapshot(), TEXT("guild")))
    {
        FeedbackMessage(TEXT("武灵已创建，可从名册进入组织管理。"));
        return;
    }
    DraftName = GuildNameInput ? GuildNameInput->GetText().ToString() : DraftName;
    const FString SubmittedName = DraftName.TrimStartAndEnd();
    if (SubmittedName.IsEmpty())
    {
        FeedbackMessage(TEXT("请输入武灵名称。"));
        return;
    }
    bAwaitingCreationConfirmation = Service->CreateGuild(SubmittedName);
    FeedbackMessage(bAwaitingCreationConfirmation ? TEXT("创建请求已记录，等待服务确认。") : Service->GetStatus());
    RefreshLiveData();
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkWulingSectSnapshotIsolationTest,
    "Boundless.InkUI.WulingSect.SnapshotIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInkWulingSectSnapshotIsolationTest::RunTest(const FString& Parameters)
{
    const auto State = MakeShared<FJsonObject>();
    const auto Guild = MakeShared<FJsonObject>();
    Guild->SetStringField(TEXT("name"), TEXT("测试武灵"));
    Guild->SetNumberField(TEXT("supply"), 0);
    State->SetObjectField(TEXT("guild"), Guild);
    const auto Counts = MakeShared<FJsonObject>();
    Counts->SetNumberField(TEXT("members"), 2);
    Counts->SetNumberField(TEXT("participating_members"), 1);
    Counts->SetNumberField(TEXT("helpers"), 3);
    State->SetObjectField(TEXT("counts"), Counts);
    const FString InitialKey = PresentationKey(State, nullptr);
    State->SetNumberField(TEXT("version"), 123);
    State->SetNumberField(TEXT("server_utc"), 9999);
    const auto Wallet = MakeShared<FJsonObject>();
    Wallet->SetNumberField(TEXT("CUR_STONE"), 888);
    State->SetObjectField(TEXT("wallet"), Wallet);
    TestEqual(TEXT("Unrelated polling/progression does not invalidate the sect editor"), PresentationKey(State, nullptr), InitialKey);
    TestEqual(TEXT("Unknown reputation stays unknown"), Amount(Guild, TEXT("reputation")), FString(TEXT("—")));
    TestEqual(TEXT("A real zero supply remains a real zero"), Amount(Guild, TEXT("supply")), FString(TEXT("0")));
    Counts->SetNumberField(TEXT("helpers"), 4);
    TestNotEqual(TEXT("Temporary helper updates invalidate the displayed count"), PresentationKey(State, nullptr), InitialKey);
    Counts->SetNumberField(TEXT("helpers"), 3);
    TestEqual(TEXT("Helper updates do not alter formal members"), Amount(Counts, TEXT("members")), FString(TEXT("2")));
    Guild->SetStringField(TEXT("name"), TEXT("另一武灵"));
    TestNotEqual(TEXT("Actual guild identity changes refresh the title"), PresentationKey(State, nullptr), InitialKey);
    TestNotEqual(TEXT("Missing state is distinguishable from an empty synchronized state"), PresentationKey(nullptr, nullptr), PresentationKey(MakeShared<FJsonObject>(), nullptr));
    return true;
}
#endif
