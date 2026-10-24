#include "RebuildActivityWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"

namespace ActivityRebuildUI
{
const FLinearColor Ink(0.075f, 0.094f, 0.082f, 1.f);       // #202823
const FLinearColor SlateJade(0.239f, 0.345f, 0.380f, 1.f); // #3D5861
const FLinearColor Celadon(0.227f, 0.553f, 0.514f, 1.f);   // #3A8D83
const FLinearColor Gold(0.722f, 0.545f, 0.271f, 1.f);      // #B88B45
const FLinearColor Paper(0.953f, 0.922f, 0.843f, 1.f);     // #F3EBDD
const FLinearColor PaperLight(0.984f, 0.969f, 0.933f, 1.f);// #FBF7EE
const FLinearColor Muted(0.553f, 0.549f, 0.510f, 1.f);     // #8D8C82
const FLinearColor White(1.f, 0.984f, 0.929f, 1.f);
const FLinearColor Transparent(0.f, 0.f, 0.f, 0.f);

UTextBlock* Text(UWidgetTree* Tree, const FText& Value, int32 Size, FLinearColor Color,
    ETextJustify::Type Align = ETextJustify::Left)
{
    auto* Result = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Result->SetText(Value);
    const InkRebuildUI::ETextRole Role = Size >= 40 ? InkRebuildUI::ETextRole::Display :
        Size >= 30 ? InkRebuildUI::ETextRole::Title :
        Size >= 26 ? InkRebuildUI::ETextRole::Section : InkRebuildUI::ETextRole::Body;
    const FSlateFontInfo Font = InkRebuildUI::Font(Role, Size);
    Result->SetFont(Font);
    Result->SetColorAndOpacity(Color);
    Result->SetJustification(Align);
    Result->SetAutoWrapText(true);
    return Result;
}

UBorder* Card(UWidgetTree* Tree, UWidget* Child, FLinearColor Fill,
    FMargin Padding = FMargin(18.f))
{
    auto* Result = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Result->SetBrushColor(Fill);
    Result->SetPadding(Padding);
    Result->SetContent(Child);
    return Result;
}

UVerticalBox* Vertical(UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
}

UHorizontalBox* Horizontal(UWidgetTree* Tree)
{
    return Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
}

void Add(UVerticalBox* Parent, UWidget* Child, float Bottom = 8.f, float Left = 0.f, float Right = 0.f)
{
    if (!Parent || !Child) return;
    if (auto* Slot = Parent->AddChildToVerticalBox(Child)) Slot->SetPadding(FMargin(Left, 0.f, Right, Bottom));
}

void Add(UHorizontalBox* Parent, UWidget* Child, float Right = 8.f)
{
    if (!Parent || !Child) return;
    if (auto* Slot = Parent->AddChildToHorizontalBox(Child)) Slot->SetPadding(FMargin(0.f, 0.f, Right, 0.f));
}

URebuildActivityIntentButton* Button(UWidgetTree* Tree, UWidget* Child, const FRebuildActivityActionRequest& Request,
    URebuildActivityPageWidget* Owner, bool bEnabled = true, bool bEmitIntent = true)
{
    auto* Result = Tree->ConstructWidget<URebuildActivityIntentButton>(URebuildActivityIntentButton::StaticClass());
    auto* Target = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Target->SetMinDesiredHeight(88.f);
    Target->SetContent(Child);
    Result->SetContent(Target);
    Result->Request = Request;
    Result->SetIsEnabled(bEnabled);
    Result->SetStyle(InkRebuildUI::ButtonStyle(false, false));
    Result->SetColorAndOpacity(FLinearColor::White); // Child cards carry the surface color; never tint their text transparent.
    if (bEmitIntent && Owner) Result->OnIntentRequested.AddUniqueDynamic(Owner, &URebuildActivityPageWidget::RelayIntent);
    return Result;
}

UWidget* Chip(UWidgetTree* Tree, const FText& Label, FLinearColor Fill, FLinearColor Foreground = White)
{
    return Card(Tree, Text(Tree, Label, 18, Foreground, ETextJustify::Center), Fill, FMargin(12.f, 8.f));
}

void AddHeader(UWidgetTree* Tree, UVerticalBox* Root, const FText& Title, const FText& Status,
    const FText& ServiceDay)
{
    auto* Header = Horizontal(Tree);
    auto* TitleColumn = Vertical(Tree);
    Add(TitleColumn, Text(Tree, Title, 40, PaperLight), 4.f);
    if (!ServiceDay.IsEmpty()) Add(TitleColumn, Text(Tree, ServiceDay, 20, Paper), 0.f);
    Add(Header, TitleColumn, 12.f);
    if (!Status.IsEmpty()) Add(Header, Chip(Tree, Status, SlateJade), 0.f);
    Add(Root, Card(Tree, Header, Ink, FMargin(20.f, 16.f)), 12.f);
}

UWidget* StatePanel(UWidgetTree* Tree, ERebuildActivityPageState State, const FText& Extra)
{
    FText Main;
    switch (State)
    {
    case ERebuildActivityPageState::Loading: Main = FText::FromString(TEXT("活动资料正在同步")); break;
    case ERebuildActivityPageState::Offline: Main = FText::FromString(TEXT("服务暂时离线")); break;
    case ERebuildActivityPageState::Empty: Main = FText::FromString(TEXT("当前没有可显示的活动")); break;
    case ERebuildActivityPageState::Disabled: Main = FText::FromString(TEXT("活动服务未启用")); break;
    case ERebuildActivityPageState::Expired: Main = FText::FromString(TEXT("活动已结束")); break;
    case ERebuildActivityPageState::Complete: Main = FText::FromString(TEXT("服务已确认完成")); break;
    case ERebuildActivityPageState::Ready: Main = FText::FromString(TEXT("活动资料已同步")); break;
    case ERebuildActivityPageState::Pending: Main = FText::FromString(TEXT("状态待服务确认")); break;
    default: Main = FText::FromString(TEXT("活动状态待同步")); break;
    }
    auto* Column = Vertical(Tree);
    Add(Column, Text(Tree, Main, 24, PaperLight), 8.f);
    if (!Extra.IsEmpty()) Add(Column, Text(Tree, Extra, 19, Paper), 0.f);
    return Card(Tree, Column, SlateJade, FMargin(20.f));
}

UWidget* ActivityRow(UWidgetTree* Tree, const FRebuildActivityRowViewModel& Row,
    const FRebuildActivityActionRequest& Request, URebuildActivityPageWidget* Owner,
    bool bSelected = false)
{
    auto* Layout = Horizontal(Tree);
    auto* BadgeColumn = Vertical(Tree);
    Add(BadgeColumn, Text(Tree, Row.Cycle == TEXT("daily") ? FText::FromString(TEXT("日")) :
        Row.Cycle == TEXT("limited") ? FText::FromString(TEXT("限")) : FText::FromString(TEXT("常")),
        24, Gold, ETextJustify::Center), 0.f);
    Add(Layout, Card(Tree, BadgeColumn, SlateJade, FMargin(14.f)), 12.f);

    auto* Copy = Vertical(Tree);
    Add(Copy, Text(Tree, Row.Title.IsEmpty() ? FText::FromString(TEXT("活动资料待同步")) : Row.Title, 24, Ink), 4.f);
    if (!Row.Summary.IsEmpty()) Add(Copy, Text(Tree, Row.Summary, 18, SlateJade), 0.f);
    Add(Layout, Copy, 12.f);
    if (!Row.StateText.IsEmpty()) Add(Layout, Chip(Tree, Row.StateText, bSelected ? Celadon : SlateJade), 8.f);
    Add(Layout, Text(Tree, FText::FromString(TEXT("›")), 30, Gold, ETextJustify::Center), 0.f);
    auto* Content = Card(Tree, Layout, bSelected ? Paper : PaperLight, FMargin(14.f));
    return Button(Tree, Content, Request, Owner);
}

void AddProgress(UWidgetTree* Tree, UVerticalBox* Parent, const FText& Label, bool bHasProgress, float Fraction)
{
    Add(Parent, Text(Tree, Label, 18, Ink), 6.f);
    if (!bHasProgress) return;
    auto* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
    Bar->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
    Bar->SetFillColorAndOpacity(Celadon);
    FProgressBarStyle BarStyle = Bar->GetWidgetStyle();
    FSlateBrush BackgroundBrush;
    BackgroundBrush.TintColor = FSlateColor(SlateJade);
    BarStyle.SetBackgroundImage(BackgroundBrush);
    Bar->SetWidgetStyle(BarStyle);
    Add(Parent, Bar, 8.f);
}

FText CategoryName(const FRebuildActivityCategoryViewModel& Category)
{
    if (!Category.Title.IsEmpty()) return Category.Title;
    if (Category.Cycle == TEXT("daily")) return FText::FromString(TEXT("每日"));
    if (Category.Cycle == TEXT("limited")) return FText::FromString(TEXT("限时"));
    if (Category.Cycle == TEXT("permanent")) return FText::FromString(TEXT("常驻"));
    return FText::FromString(TEXT("活动"));
}

FRebuildActivityActionRequest MakeRequest(FName Action, FName Entry = NAME_None,
    FName Cycle = NAME_None, const FString& Day = FString(), bool bConfirmed = false)
{
    FRebuildActivityActionRequest Result;
    Result.Action = Action;
    Result.EntryId = Entry;
    Result.Cycle = Cycle;
    Result.ExpectedServiceDay = Day;
    Result.bConfirmed = bConfirmed;
    return Result;
}
}

URebuildActivityIntentButton::URebuildActivityIntentButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OnClicked.AddUniqueDynamic(this, &URebuildActivityIntentButton::HandleClicked);
}

void URebuildActivityIntentButton::HandleClicked()
{
    OnIntentRequested.Broadcast(Request);
}

void URebuildActivityPageWidget::RelayIntent(FRebuildActivityActionRequest Request)
{
    OnActionRequested.Broadcast(Request);
}

void URebuildActivitiesHubWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildActivitiesHubWidget::ApplyViewModel(const FRebuildActivitiesHubViewModel& Model)
{
    ViewModel = Model;
    if (!ViewModel.Categories.ContainsByPredicate([this](const auto& Row) { return Row.Cycle == SelectedCycle; }) && !ViewModel.Categories.IsEmpty())
        SelectedCycle = ViewModel.Categories[0].Cycle;
    BuildPage();
}

void URebuildActivitiesHubWidget::Emit(FName Action, FName EntryId, FName Cycle, const FString& Day)
{
    OnActionRequested.Broadcast(ActivityRebuildUI::MakeRequest(Action, EntryId, Cycle, Day));
}

void URebuildActivitiesHubWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace ActivityRebuildUI;
    if (!StablePageRoot)
    {
        StablePageRoot = Vertical(WidgetTree);
        StablePageRoot->SetClipping(EWidgetClipping::ClipToBounds);
        StablePageFrame = Card(WidgetTree, StablePageRoot, Ink, FMargin(0.f));
        WidgetTree->RootWidget = StablePageFrame;
    }
    const float PreserveScrollOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StablePageRoot->ClearChildren();
    auto* Root = StablePageRoot;
    Root->SetClipping(EWidgetClipping::ClipToBounds);
    AddHeader(WidgetTree, Root, FText::FromString(TEXT("活动")), ViewModel.ServiceStateText, ViewModel.ServiceDayText);

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    ActiveScroll = Scroll;
    Scroll->SetAllowOverscroll(false);
    auto* Body = Vertical(WidgetTree);

    auto* FeaturedCopy = Vertical(WidgetTree);
    Add(FeaturedCopy, Text(WidgetTree, ViewModel.bHasFeaturedLaunch && !ViewModel.FeaturedLaunch.Title.IsEmpty()
        ? ViewModel.FeaturedLaunch.Title : FText::FromString(TEXT("青云开服活动")), 28, PaperLight), 6.f);
    const FText FeaturedState = ViewModel.bHasFeaturedLaunch ? ViewModel.FeaturedLaunch.StateText :
        (ViewModel.State == ERebuildActivityPageState::Offline ? FText::FromString(TEXT("服务离线")) : FText::FromString(TEXT("活动状态待同步")));
    if (!FeaturedState.IsEmpty()) Add(FeaturedCopy, Text(WidgetTree, FeaturedState, 19, Paper), 0.f);
    const auto FeaturedRequest = MakeRequest(TEXT("open-launch"));
    auto* FeaturedContent = Horizontal(WidgetTree);
    Add(FeaturedContent, FeaturedCopy, 16.f);
    if (ViewModel.bHasFeaturedLaunch && ViewModel.FeaturedLaunch.bActionAllowed)
        Add(FeaturedContent, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("查看")), 21, White, ETextJustify::Center),
            FeaturedRequest, this), 0.f);
    Add(Body, Card(WidgetTree, FeaturedContent, SlateJade, FMargin(20.f)), 14.f);

    auto* CategoryRail = Horizontal(WidgetTree);
    const FName KnownCycles[] = {TEXT("daily"), TEXT("limited"), TEXT("permanent")};
    const FText KnownNames[] = {FText::FromString(TEXT("每日")), FText::FromString(TEXT("限时")), FText::FromString(TEXT("常驻"))};
    for (int32 I = 0; I < UE_ARRAY_COUNT(KnownCycles); ++I)
    {
        const FName Cycle = KnownCycles[I];
        FText Label = KnownNames[I];
        const auto* Category = ViewModel.Categories.FindByPredicate([Cycle](const auto& Item) { return Item.Cycle == Cycle; });
        if (Category && Category->bHasCount) Label = FText::FromString(FString::Printf(TEXT("%s  %d"), *Label.ToString(), Category->Count));
        const auto Request = MakeRequest(TEXT("activity-cycle"), NAME_None, Cycle);
        Add(CategoryRail, Button(WidgetTree,
            Card(WidgetTree, Text(WidgetTree, Label, 22, Cycle == SelectedCycle ? White : Ink, ETextJustify::Center),
                Cycle == SelectedCycle ? Celadon : PaperLight, FMargin(12.f)), Request, this,
            true), 6.f);
    }
    Add(Body, CategoryRail, 10.f);

    const auto* SelectedCategory = ViewModel.Categories.FindByPredicate([this](const auto& Item) { return Item.Cycle == SelectedCycle; });
    if (ViewModel.State == ERebuildActivityPageState::Offline || ViewModel.State == ERebuildActivityPageState::Loading ||
        ViewModel.State == ERebuildActivityPageState::Disabled || ViewModel.State == ERebuildActivityPageState::Empty)
    {
        Add(Body, StatePanel(WidgetTree, ViewModel.State, ViewModel.ServiceStateText), 8.f);
    }
    else if (!SelectedCategory || !SelectedCategory->bActivitiesProvided || SelectedCategory->Activities.IsEmpty())
    {
        const auto State = SelectedCategory
            ? (!SelectedCategory->bActivitiesProvided ? ERebuildActivityPageState::Loading :
                ERebuildActivityPageState::Empty)
            : (ViewModel.bCategoriesProvided ? ERebuildActivityPageState::Empty : ERebuildActivityPageState::Loading);
        Add(Body, StatePanel(WidgetTree, State, SelectedCategory ? SelectedCategory->Summary : ViewModel.ServiceStateText), 8.f);
    }
    else
    {
        Add(Body, Text(WidgetTree, CategoryName(*SelectedCategory), 30, PaperLight), 8.f);
        for (const auto& Activity : SelectedCategory->Activities)
        {
            const auto Request = MakeRequest(TEXT("open-activity-detail"), Activity.Id, SelectedCycle);
            Add(Body, ActivityRow(WidgetTree, Activity, Request, this), 8.f);
        }
    }
    Scroll->AddChild(Card(WidgetTree, Body, Transparent, FMargin(24.f, 8.f)));
    auto* ScrollSlot = Root->AddChildToVerticalBox(Scroll);
    if (ScrollSlot) ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill, 1.f));
    Scroll->SetScrollOffset(PreserveScrollOffset);
}

void URebuildActivityCategoryDetailWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildActivityCategoryDetailWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNewWideLayout = Size.X / Size.Y >= 1.05f;
    if (!bHasMeasuredLayout || bNewWideLayout != bWideLayout)
    {
        bHasMeasuredLayout = true;
        bWideLayout = bNewWideLayout;
        BuildPage();
    }
}

void URebuildActivityCategoryDetailWidget::ApplyViewModel(const FRebuildActivityCategoryDetailViewModel& Model)
{
    ViewModel = Model;
    BuildPage();
}

void URebuildActivityCategoryDetailWidget::Emit(FName Action, FName EntryId, FName Cycle, const FString& Day)
{
    OnActionRequested.Broadcast(ActivityRebuildUI::MakeRequest(Action, EntryId, Cycle, Day));
}

UWidget* URebuildActivityCategoryDetailWidget::BuildActivityList()
{
    using namespace ActivityRebuildUI;
    auto* List = Vertical(WidgetTree);
    const auto* Category = ViewModel.Categories.FindByPredicate([this](const auto& Item) { return Item.Cycle == ViewModel.SelectedCycle; });
    if (!ViewModel.bCategoriesProvided || !Category)
    {
        const auto MissingState = ViewModel.bCategoriesProvided ? ERebuildActivityPageState::Empty : ViewModel.State;
        Add(List, StatePanel(WidgetTree, MissingState, ViewModel.ServiceStateText), 0.f);
        return List;
    }
    if (!Category->bActivitiesProvided || Category->State == ERebuildActivityPageState::Loading || Category->State == ERebuildActivityPageState::Offline ||
        Category->State == ERebuildActivityPageState::Empty || Category->State == ERebuildActivityPageState::Disabled)
    {
        Add(List, StatePanel(WidgetTree, Category->State, Category->Summary), 0.f);
        return List;
    }
    Add(List, Text(WidgetTree, CategoryName(*Category), 30, PaperLight), 12.f);
    for (const auto& Row : Category->Activities)
    {
        const bool bSelected = Row.Id == ViewModel.SelectedEntryId;
        const auto Request = MakeRequest(TEXT("open-activity-detail"), Row.Id, ViewModel.SelectedCycle);
        Add(List, ActivityRow(WidgetTree, Row, Request, this, bSelected), 8.f);
    }
    if (Category->Activities.IsEmpty()) Add(List, StatePanel(WidgetTree, ERebuildActivityPageState::Empty, Category->Summary), 0.f);
    return List;
}

UWidget* URebuildActivityCategoryDetailWidget::BuildSelectedDetail()
{
    using namespace ActivityRebuildUI;
    auto* Detail = Vertical(WidgetTree);
    if (!ViewModel.bHasSelectedActivity)
    {
        Add(Detail, StatePanel(WidgetTree, ViewModel.State, ViewModel.ServiceStateText), 0.f);
        return Card(WidgetTree, Detail, Paper, FMargin(20.f));
    }
    const auto& Row = ViewModel.SelectedActivity;
    Add(Detail, Text(WidgetTree, Row.Title, 32, Ink), 8.f);
    if (!Row.Summary.IsEmpty()) Add(Detail, Text(WidgetTree, Row.Summary, 21, Ink), 10.f);
    if (!Row.StateText.IsEmpty()) Add(Detail, Chip(WidgetTree, Row.StateText, SlateJade), 8.f);
    if (!Row.EligibilityText.IsEmpty())
    {
        Add(Detail, Text(WidgetTree, FText::FromString(TEXT("参与条件")), 19, SlateJade), 4.f);
        Add(Detail, Text(WidgetTree, Row.EligibilityText, 20, Ink), 8.f);
    }
    if (!Row.DeadlineText.IsEmpty()) Add(Detail, Text(WidgetTree, Row.DeadlineText, 19, Ink), 6.f);
    if (!Row.RewardBudgetText.IsEmpty()) Add(Detail, Text(WidgetTree, Row.RewardBudgetText, 19, Ink), 6.f);
    if (!Row.ProgressText.IsEmpty()) AddProgress(WidgetTree, Detail, Row.ProgressText, Row.bHasProgress, Row.ProgressFraction);
    if (Row.bActionAllowed && !Row.AllowedAction.IsNone() && (Row.AllowedAction != TEXT("sign-in") || !ViewModel.ServiceDay.IsEmpty()))
    {
        const auto Request = MakeRequest(Row.AllowedAction, Row.Id, Row.Cycle, ViewModel.ServiceDay);
        const FText ActionText = Row.ActionLabel.IsEmpty()
            ? (Row.AllowedAction == TEXT("sign-in") ? FText::FromString(TEXT("每日签到")) : FText::FromString(TEXT("前往活动")))
            : Row.ActionLabel;
        Add(Detail, Button(WidgetTree, Text(WidgetTree, ActionText, 24, White, ETextJustify::Center),
            Request, this), 0.f);
    }
    else if (!Row.ActionDisabledReason.IsEmpty())
        Add(Detail, Chip(WidgetTree, Row.ActionDisabledReason, Muted), 0.f);
    return Card(WidgetTree, Detail, Paper, FMargin(20.f));
}

void URebuildActivityCategoryDetailWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace ActivityRebuildUI;
    if (!StablePageRoot)
    {
        StablePageRoot = Vertical(WidgetTree);
        StablePageFrame = Card(WidgetTree, StablePageRoot, Ink, FMargin(0.f));
        WidgetTree->RootWidget = StablePageFrame;
    }
    const float PreserveScrollOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StablePageRoot->ClearChildren();
    auto* Root = StablePageRoot;
    AddHeader(WidgetTree, Root, FText::FromString(TEXT("活动详情")), ViewModel.ServiceStateText,
        ViewModel.ServiceDay.IsEmpty() ? FText() : FText::FromString(ViewModel.ServiceDay));

    auto* Rail = Horizontal(WidgetTree);
    const FName Cycles[] = {TEXT("daily"), TEXT("limited"), TEXT("permanent")};
    for (const FName Cycle : Cycles)
    {
        const auto* Category = ViewModel.Categories.FindByPredicate([Cycle](const auto& Item) { return Item.Cycle == Cycle; });
        const FText Name = Category ? CategoryName(*Category) : Cycle == TEXT("daily") ? FText::FromString(TEXT("每日")) :
            Cycle == TEXT("limited") ? FText::FromString(TEXT("限时")) : FText::FromString(TEXT("常驻"));
        const auto Request = MakeRequest(TEXT("activity-cycle"), NAME_None, Cycle);
        Add(Rail, Button(WidgetTree,
            Card(WidgetTree, Text(WidgetTree, Name, 20, ViewModel.SelectedCycle == Cycle ? White : Ink, ETextJustify::Center),
                ViewModel.SelectedCycle == Cycle ? Celadon : PaperLight, FMargin(12.f)), Request, this,
            true), 4.f);
    }
    Add(Root, Card(WidgetTree, Rail, Ink, FMargin(8.f)), 8.f, 24.f, 24.f);

    auto* Content = Horizontal(WidgetTree);
    if (bWideLayout)
    {
        auto* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ListSize->SetWidthOverride(320.f);
        ListSize->SetContent(BuildActivityList());
        Add(Content, ListSize, 12.f);
        auto* DetailSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        DetailSize->SetMinDesiredWidth(400.f);
        DetailSize->SetContent(BuildSelectedDetail());
        Add(Content, DetailSize, 0.f);
    }
    else
    {
        auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
        ActiveScroll = Scroll;
        Scroll->SetAllowOverscroll(false);
        auto* PortraitContent = Vertical(WidgetTree);
        Add(PortraitContent, BuildSelectedDetail(), 12.f);
        Add(PortraitContent, BuildActivityList(), 0.f);
        Scroll->AddChild(Card(WidgetTree, PortraitContent, Transparent, FMargin(24.f, 12.f)));
        auto* Slot = Root->AddChildToVerticalBox(Scroll);
        if (Slot) Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill, 1.f));
        Scroll->SetScrollOffset(PreserveScrollOffset);
    }
    if (bWideLayout)
    {
        ActiveScroll = nullptr;
        auto* Slot = Root->AddChildToVerticalBox(Content);
        if (Slot)
        {
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill, 1.f));
            Slot->SetPadding(FMargin(24.f, 8.f));
        }
    }
}

void URebuildLaunchActivityWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildLaunchActivityWidget::ApplyViewModel(const FRebuildLaunchActivityViewModel& Model)
{
    if (bConfirmingStart && (Model.EventId != ConfirmedStartEventId || Model.ServiceDay != ConfirmedStartDay))
    {
        bConfirmingStart = false;
        ConfirmedStartDay.Reset();
        ConfirmedStartEventId = NAME_None;
    }
    const int32 PreserveTab = bHasSelectedTab ? SelectedTab : Model.SelectedTab;
    ViewModel = Model;
    SelectedTab = FMath::Clamp(PreserveTab, 0, 3);
    bHasSelectedTab = true;
    BuildPage();
}

void URebuildLaunchActivityWidget::Emit(FName Action, FName EntryId, FName Cycle, const FString& Day)
{
    OnActionRequested.Broadcast(ActivityRebuildUI::MakeRequest(Action, EntryId, Cycle, Day));
}

void URebuildLaunchActivityWidget::SelectTab(int32 Index)
{
    SelectedTab = FMath::Clamp(Index, 0, 3);
    bHasSelectedTab = true;
    BuildPage();
}

void URebuildLaunchActivityWidget::HandleLocalTab(FRebuildActivityActionRequest Request)
{
    SelectTab(FCString::Atoi(*Request.EntryId.ToString()));
}

void URebuildLaunchActivityWidget::HandleStartIntent(FRebuildActivityActionRequest Request)
{
    HandleStartClicked();
}

void URebuildLaunchActivityWidget::HandleStartClicked()
{
    if (!ViewModel.bCanStart || !ViewModel.bConnected || ViewModel.bBusy || ViewModel.EventId.IsNone() || ViewModel.ServiceDay.IsEmpty()) return;
    if (!bConfirmingStart)
    {
        ConfirmedStartDay = ViewModel.ServiceDay;
        ConfirmedStartEventId = ViewModel.EventId;
        bConfirmingStart = true;
        BuildPage();
        return;
    }
    if (ViewModel.EventId != ConfirmedStartEventId || ViewModel.ServiceDay != ConfirmedStartDay)
    {
        bConfirmingStart = false;
        ConfirmedStartDay.Reset();
        ConfirmedStartEventId = NAME_None;
        BuildPage();
        return;
    }
    bConfirmingStart = false;
    OnActionRequested.Broadcast(ActivityRebuildUI::MakeRequest(TEXT("start-launch"), ConfirmedStartEventId,
        NAME_None, ConfirmedStartDay, true));
    ConfirmedStartDay.Reset();
    ConfirmedStartEventId = NAME_None;
    BuildPage();
}

void URebuildLaunchActivityWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace ActivityRebuildUI;
    if (!StablePageRoot)
    {
        StablePageRoot = Vertical(WidgetTree);
        StablePageFrame = Card(WidgetTree, StablePageRoot, Ink, FMargin(0.f));
        WidgetTree->RootWidget = StablePageFrame;
    }
    const float PreserveScrollOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StablePageRoot->ClearChildren();
    auto* Root = StablePageRoot;
    AddHeader(WidgetTree, Root, FText::FromString(TEXT("青云初启")), ViewModel.StatusText, FText());

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    ActiveScroll = Scroll;
    Scroll->SetAllowOverscroll(false);
    auto* Body = Vertical(WidgetTree);
    auto* EventMeta = Vertical(WidgetTree);
    Add(EventMeta, Text(WidgetTree, FText::FromString(TEXT("活动时间")), 18, SlateJade), 4.f);
    const FText Range = FText::FromString(FString::Printf(TEXT("%s    —    %s"),
        ViewModel.StartTimeText.IsEmpty() ? TEXT("待同步") : *ViewModel.StartTimeText.ToString(),
        ViewModel.EndTimeText.IsEmpty() ? TEXT("待同步") : *ViewModel.EndTimeText.ToString()));
    Add(EventMeta, Text(WidgetTree, Range, 20, Ink), 0.f);
    Add(Body, Card(WidgetTree, EventMeta, Paper, FMargin(16.f)), 10.f);

    auto* Personal = Horizontal(WidgetTree);
    Add(Personal, Text(WidgetTree, FText::FromString(TEXT("我的贡献")), 18, PaperLight), 8.f);
    Add(Personal, Text(WidgetTree, ViewModel.bHasPersonalScore ? ViewModel.PersonalScoreText : FText::FromString(TEXT("待同步")), 26, PaperLight), 18.f);
    Add(Personal, Text(WidgetTree, FText::FromString(TEXT("共同轮次")), 18, PaperLight), 8.f);
    Add(Personal, Text(WidgetTree, ViewModel.bHasCompleteRounds ? ViewModel.CompleteRoundsText : FText::FromString(TEXT("待同步")), 24, PaperLight), 0.f);
    Add(Body, Card(WidgetTree, Personal, SlateJade, FMargin(16.f)), 12.f);

    const FText TabLabels[] = {FText::FromString(TEXT("总览")), FText::FromString(TEXT("个人任务")),
        FText::FromString(TEXT("组织目标")), FText::FromString(TEXT("荣誉"))};
    auto* Tabs = Horizontal(WidgetTree);
    for (int32 I = 0; I < UE_ARRAY_COUNT(TabLabels); ++I)
    {
        const int32 Index = I;
        auto* Tab = Button(WidgetTree, Card(WidgetTree, Text(WidgetTree, TabLabels[I], 18,
            SelectedTab == I ? White : Ink, ETextJustify::Center), SelectedTab == I ? Celadon : PaperLight,
            FMargin(9.f, 12.f)), MakeRequest(NAME_None, FName(*FString::FromInt(Index))), this,
            true, false);
        Tab->OnIntentRequested.AddUniqueDynamic(this, &URebuildLaunchActivityWidget::HandleLocalTab);
        Add(Tabs, Tab, 4.f);
    }
    Add(Body, Tabs, 12.f);

    if (SelectedTab == 0)
    {
        Add(Body, StatePanel(WidgetTree, ViewModel.State, ViewModel.StatusText), 10.f);
        auto* Preview = Vertical(WidgetTree);
        Add(Preview, Text(WidgetTree, FText::FromString(TEXT("本期任务")), 26, Ink), 6.f);
        Add(Preview, Text(WidgetTree, ViewModel.bTasksProvided
            ? FText::FromString(FString::Printf(TEXT("服务已提供 %d 项任务"), ViewModel.Tasks.Num()))
            : FText::FromString(TEXT("任务列表待同步")), 18, SlateJade), 8.f);
        const auto RankRequest = MakeRequest(TEXT("open-launch-rank"));
        Add(Preview, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("查看贡献榜")), 20, White, ETextJustify::Center),
            RankRequest, this), 0.f);
        Add(Body, Card(WidgetTree, Preview, Paper, FMargin(16.f)), 0.f);
    }
    else if (SelectedTab == 1)
    {
        if (ViewModel.Tasks.IsEmpty()) Add(Body, StatePanel(WidgetTree,
            ViewModel.bTasksProvided ? ERebuildActivityPageState::Empty : ERebuildActivityPageState::Loading,
            FText::FromString(TEXT("任务列表待同步"))), 0.f);
        for (const auto& Task : ViewModel.Tasks)
        {
            auto* TaskBody = Vertical(WidgetTree);
            Add(TaskBody, Text(WidgetTree, Task.ConditionText, 21, Ink), 6.f);
            if (!Task.StateText.IsEmpty()) Add(TaskBody, Chip(WidgetTree, Task.StateText, Task.bConfirmed ? Celadon : SlateJade), 6.f);
            AddProgress(WidgetTree, TaskBody, Task.ProgressText, Task.bHasProgress, Task.ProgressFraction);
            if (ViewModel.bTaskNumericRewardsAllowed && !Task.RewardText.IsEmpty())
                Add(TaskBody, Text(WidgetTree, Task.RewardText, 18, SlateJade), 0.f);
            else
                Add(TaskBody, Text(WidgetTree, FText::FromString(TEXT("奖励信息待服务同步")), 17, SlateJade), 0.f);
            Add(Body, Card(WidgetTree, TaskBody, PaperLight, FMargin(16.f)), 8.f);
        }
    }
    else if (SelectedTab == 2)
    {
        if (ViewModel.Milestones.IsEmpty()) Add(Body, StatePanel(WidgetTree,
            ViewModel.bMilestonesProvided ? ERebuildActivityPageState::Empty : ERebuildActivityPageState::Loading,
            FText::FromString(TEXT("组织目标待同步"))), 0.f);
        for (const auto& Milestone : ViewModel.Milestones)
        {
            auto* MilestoneBody = Vertical(WidgetTree);
            Add(MilestoneBody, Text(WidgetTree, Milestone.Title, 22, Ink), 6.f);
            if (!Milestone.ProgressText.IsEmpty()) Add(MilestoneBody, Text(WidgetTree, Milestone.ProgressText, 18, SlateJade), 6.f);
            if (!Milestone.StateText.IsEmpty()) Add(MilestoneBody, Chip(WidgetTree, Milestone.StateText, SlateJade), 0.f);
            Add(Body, Card(WidgetTree, MilestoneBody, PaperLight, FMargin(16.f)), 8.f);
        }
    }
    else
    {
        auto* Honor = Vertical(WidgetTree);
        Add(Honor, Text(WidgetTree, FText::FromString(TEXT("贡献榜单独展示服务返回的名次与贡献值。")), 20, Ink), 8.f);
        const auto RankRequest = MakeRequest(TEXT("open-launch-rank"));
        Add(Honor, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("查看贡献榜")), 20, White, ETextJustify::Center),
            RankRequest, this), 0.f);
        Add(Body, Card(WidgetTree, Honor, Paper, FMargin(16.f)), 0.f);
    }

    auto* Offer = Vertical(WidgetTree);
    Add(Offer, Text(WidgetTree, FText::FromString(TEXT("开服特惠")), 20, Muted), 4.f);
    Add(Offer, Text(WidgetTree, ViewModel.OfferDisabledReason.IsEmpty()
        ? FText::FromString(TEXT("商品 SKU、价格与购买接口待接入 · 暂不可用")) : ViewModel.OfferDisabledReason,
        18, Muted), 0.f);
    Add(Body, Card(WidgetTree, Offer, FLinearColor(0.16f, 0.17f, 0.16f, 1.f), FMargin(16.f)), 12.f);
    Scroll->AddChild(Card(WidgetTree, Body, Transparent, FMargin(24.f, 8.f)));
    auto* ScrollSlot = Root->AddChildToVerticalBox(Scroll);
    if (ScrollSlot) ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill, 1.f));
    Scroll->SetScrollOffset(PreserveScrollOffset);

    auto* Footer = Vertical(WidgetTree);
    if (ViewModel.bCanEnterWorld && ViewModel.bConnected && !ViewModel.bBusy)
    {
        const auto Request = MakeRequest(TEXT("open-world"));
        Add(Footer, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("进入活动历练")), 23, White, ETextJustify::Center),
            Request, this), 0.f);
    }
    else if (ViewModel.bCanStart)
    {
        const FText Label = bConfirmingStart ? FText::FromString(TEXT("再次确认开启活动")) : FText::FromString(TEXT("开启活动"));
        auto* Start = Button(WidgetTree, Text(WidgetTree, Label, 23, White, ETextJustify::Center),
            MakeRequest(TEXT("start-launch")), this, true, false);
        Start->OnIntentRequested.AddUniqueDynamic(this, &URebuildLaunchActivityWidget::HandleStartIntent);
        Add(Footer, Start, 0.f);
    }
    else
    {
        Add(Footer, Chip(WidgetTree, ViewModel.StartDisabledReason.IsEmpty()
            ? FText::FromString(TEXT("开启条件待服务确认")) : ViewModel.StartDisabledReason, Muted), 0.f);
    }
    Add(Root, Card(WidgetTree, Footer, Ink, FMargin(20.f, 12.f)), 0.f);
}
