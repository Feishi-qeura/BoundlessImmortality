#include "RebuildActivitySecondaryWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"

namespace ActivitySecondaryUI
{
UTextBlock* Text(UWidgetTree* Tree, const FText& Value, int32 Size,
    InkRebuildUI::ETextRole Role = InkRebuildUI::ETextRole::Body,
    FLinearColor Color = FLinearColor::White)
{
    auto* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Block->SetText(Value);
    Block->SetFont(InkRebuildUI::Font(Role, Size));
    Block->SetColorAndOpacity(Color);
    Block->SetAutoWrapText(true);
    return Block;
}

UBorder* Card(UWidgetTree* Tree, UWidget* Child, FLinearColor Fill = FLinearColor::White,
    FMargin Padding = FMargin(20.f))
{
    auto* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Border->SetBrushColor(Fill);
    Border->SetPadding(Padding);
    Border->SetContent(Child);
    return Border;
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
    if (Parent && Child)
        if (auto* Slot = Parent->AddChildToVerticalBox(Child)) Slot->SetPadding(FMargin(Left, 0.f, Right, Bottom));
}

void Add(UHorizontalBox* Parent, UWidget* Child, float Right = 8.f)
{
    if (Parent && Child)
        if (auto* Slot = Parent->AddChildToHorizontalBox(Child)) Slot->SetPadding(FMargin(0.f, 0.f, Right, 0.f));
}

URebuildActivityIntentButton* Button(UWidgetTree* Tree, UWidget* Child,
    URebuildActivityPageWidget* Owner, FName Action, bool bEnabled = true, bool bRelay = true)
{
    auto* Result = Tree->ConstructWidget<URebuildActivityIntentButton>(URebuildActivityIntentButton::StaticClass());
    auto* Target = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Target->SetMinDesiredHeight(InkRebuildUI::TouchSize);
    Target->SetContent(Child);
    Result->SetContent(Target);
    Result->SetIsEnabled(bEnabled);
    Result->SetStyle(InkRebuildUI::ButtonStyle(true, false));
    Result->SetColorAndOpacity(FLinearColor::White);
    Result->Request.Action = Action;
    if (bRelay && Owner)
        Result->OnIntentRequested.AddUniqueDynamic(Owner, &URebuildActivityPageWidget::RelayIntent);
    return Result;
}

UWidget* StatePanel(UWidgetTree* Tree, ERebuildActivityPageState State, const FText& Detail)
{
    FText Heading;
    switch (State)
    {
    case ERebuildActivityPageState::Loading: Heading = FText::FromString(TEXT("资料正在同步")); break;
    case ERebuildActivityPageState::Offline: Heading = FText::FromString(TEXT("服务暂时离线")); break;
    case ERebuildActivityPageState::Empty: Heading = FText::FromString(TEXT("当前范围没有记录")); break;
    case ERebuildActivityPageState::Disabled: Heading = FText::FromString(TEXT("当前服务不可用")); break;
    case ERebuildActivityPageState::Error: Heading = FText::FromString(TEXT("资料暂时无法读取")); break;
    case ERebuildActivityPageState::Pending: Heading = FText::FromString(TEXT("等待服务确认")); break;
    default: Heading = FText::FromString(TEXT("状态待同步")); break;
    }
    auto* Body = Vertical(Tree);
    Add(Body, Text(Tree, Heading, 26, InkRebuildUI::ETextRole::Section), 8.f);
    if (!Detail.IsEmpty()) Add(Body, Text(Tree, Detail, 20), 0.f);
    return Card(Tree, Body, InkRebuildUI::Paper());
}

UWidget* KeyValue(UWidgetTree* Tree, const FText& Key, const FText& Value, bool bKnown)
{
    auto* Row = Horizontal(Tree);
    Add(Row, Text(Tree, Key, 20, InkRebuildUI::ETextRole::Body, InkRebuildUI::Muted()), 12.f);
    Add(Row, Text(Tree, bKnown ? Value : FText::FromString(TEXT("待同步")), 22, InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 0.f);
    return Card(Tree, Row, InkRebuildUI::Paper(), FMargin(16.f, 12.f));
}

FRebuildActivityActionRequest Request(FName Action, FName EntryId = NAME_None, bool bConfirmed = false)
{
    FRebuildActivityActionRequest Result;
    Result.Action = Action;
    Result.EntryId = EntryId;
    Result.bConfirmed = bConfirmed;
    return Result;
}

void AddHeader(UWidgetTree* Tree, UVerticalBox* Root, const FText& Title, const FText& Status)
{
    auto* Header = Vertical(Tree);
    Add(Header, Text(Tree, Title, 40, InkRebuildUI::ETextRole::Display), 6.f);
    if (!Status.IsEmpty()) Add(Header, Text(Tree, Status, 20, InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 0.f);
    Add(Root, Card(Tree, Header, InkRebuildUI::Background(), FMargin(24.f, 18.f)), 12.f);
}

void SetStableRoot(UWidgetTree* Tree, UVerticalBox*& Root, UBorder*& Frame)
{
    if (Root) return;
    Root = Vertical(Tree);
    Root->SetClipping(EWidgetClipping::ClipToBounds);
    Frame = Card(Tree, Root, InkRebuildUI::Background(), FMargin(0.f));
    Tree->RootWidget = Frame;
}
}

void URebuildLaunchRankWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildLaunchRankWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNowWide = Size.X / Size.Y >= 1.15f;
    if (!bMeasured || bWideLayout != bNowWide)
    {
        bMeasured = true;
        bWideLayout = bNowWide;
        BuildPage();
    }
}

void URebuildLaunchRankWidget::ApplyViewModel(const FRebuildLaunchRankViewModel& Model)
{
    ViewModel = Model;
    BuildPage();
}

void URebuildLaunchRankWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace ActivitySecondaryUI;
    SetStableRoot(WidgetTree, StableRoot, StableFrame);
    const float PreviousOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StableRoot->ClearChildren();
    AddHeader(WidgetTree, StableRoot, FText::FromString(TEXT("开服贡献榜")), ViewModel.StatusText);

    auto* Summary = Vertical(WidgetTree);
    Add(Summary, Text(WidgetTree, ViewModel.ScopeText.IsEmpty() ? FText::FromString(TEXT("范围待同步")) : ViewModel.ScopeText,
        24, InkRebuildUI::ETextRole::Section, InkRebuildUI::Jade()), 8.f);
    if (!ViewModel.PeriodText.IsEmpty()) Add(Summary, Text(WidgetTree, ViewModel.PeriodText, 19,
        InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 8.f);
    Add(Summary, KeyValue(WidgetTree, FText::FromString(TEXT("我的名次")), ViewModel.OwnRankText, ViewModel.bOwnRankKnown), 6.f);
    Add(Summary, KeyValue(WidgetTree, FText::FromString(TEXT("我的贡献")), ViewModel.OwnContributionText,
        ViewModel.bOwnContributionKnown), 0.f);

    auto* Rows = Vertical(WidgetTree);
    if (!ViewModel.bRowsProvided || ViewModel.Rows.IsEmpty())
    {
        const auto MissingState = ViewModel.bRowsProvided ? ERebuildActivityPageState::Empty :
            (ViewModel.State == ERebuildActivityPageState::Unknown || ViewModel.State == ERebuildActivityPageState::Ready
                ? ERebuildActivityPageState::Loading : ViewModel.State);
        Add(Rows, StatePanel(WidgetTree, MissingState, ViewModel.StatusText), 0.f);
    }
    else
    {
        for (const auto& RowModel : ViewModel.Rows) // Preserve service ordering and tied ranks exactly.
        {
            auto* Row = Horizontal(WidgetTree);
            Add(Row, Text(WidgetTree, RowModel.bRankKnown ? RowModel.RankText : FText::FromString(TEXT("待同步")),
                24, InkRebuildUI::ETextRole::Number, InkRebuildUI::Gold()), 14.f);
            auto* Identity = Vertical(WidgetTree);
            Add(Identity, Text(WidgetTree, RowModel.bNameKnown ? RowModel.DisplayName : FText::FromString(TEXT("姓名待同步")),
                21, InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 4.f);
            if (RowModel.bIsCurrentParticipant)
                Add(Identity, Text(WidgetTree, FText::FromString(TEXT("当前参与者")), 17,
                    InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Jade()), 0.f);
            Add(Row, Identity, 14.f);
            Add(Row, Text(WidgetTree, RowModel.bContributionKnown ? RowModel.ContributionText : FText::FromString(TEXT("待同步")),
                20, InkRebuildUI::ETextRole::Number, InkRebuildUI::Ink()), 0.f);
            Add(Rows, Card(WidgetTree, Row, RowModel.bIsCurrentParticipant ? InkRebuildUI::Paper() : FLinearColor::White,
                FMargin(16.f, 12.f)), 8.f);
        }
    }

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    Scroll->SetAllowOverscroll(false);
    ActiveScroll = Scroll;
    auto* Content = Vertical(WidgetTree);
    auto* Footer = Vertical(WidgetTree);
    auto* Back = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("返回")), 23,
        InkRebuildUI::ETextRole::Button), this, TEXT("back"));
    Add(Footer, Back, 0.f);

    if (bWideLayout)
    {
        auto* Columns = Horizontal(WidgetTree);
        auto* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ListSize->SetMinDesiredWidth(560.f);
        ListSize->SetContent(Rows);
        Add(Columns, ListSize, 20.f);
        auto* SummarySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SummarySize->SetMinDesiredWidth(360.f);
        SummarySize->SetContent(Card(WidgetTree, Summary, InkRebuildUI::Background()));
        Add(Columns, SummarySize, 0.f);
        Add(Content, Columns, 0.f);
    }
    else
    {
        Add(Content, Card(WidgetTree, Summary, InkRebuildUI::Background()), 12.f);
        Add(Content, Rows, 0.f);
    }
    Scroll->AddChild(Card(WidgetTree, Content, InkRebuildUI::Background(), FMargin(InkRebuildUI::SafePadding, 8.f)));
    Add(StableRoot, Scroll, 0.f);
    if (auto* ScrollSlot = Cast<UVerticalBoxSlot>(Scroll->Slot))
    {
        FSlateChildSize FillSize(ESlateSizeRule::Fill);
        FillSize.Value = 1.f;
        ScrollSlot->SetSize(FillSize);
        ScrollSlot->SetPadding(FMargin(InkRebuildUI::SafePadding, 8.f));
    }
    Scroll->SetScrollOffset(PreviousOffset);
    Add(StableRoot, Card(WidgetTree, Footer, InkRebuildUI::Background(), FMargin(24.f, 12.f)), 0.f);
}

void URebuildLaunchSpecialOfferWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildLaunchSpecialOfferWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNowWide = Size.X / Size.Y >= 1.15f;
    if (!bMeasured || bWideLayout != bNowWide)
    {
        bMeasured = true;
        bWideLayout = bNowWide;
        BuildPage();
    }
}

void URebuildLaunchSpecialOfferWidget::ApplyViewModel(const FRebuildLaunchOfferViewModel& Model)
{
    if (bConfirmingPurchase && (Model.OfferId != ConfirmedOfferId || Model.Sku != ConfirmedSku || Model.QuoteVersion != ConfirmedQuoteVersion ||
        Model.PriceText.ToString() != ConfirmedPriceText.ToString() || !Model.bSkuKnown || !Model.bPriceKnown ||
        !Model.bEntitlementKnown || Model.bOwnsEntitlement || !Model.bPlatformPaymentAvailable ||
        !Model.bServicePurchaseAvailable || !Model.bConnected || Model.bBusy))
    {
        bConfirmingPurchase = false;
        ConfirmedOfferId = NAME_None;
        ConfirmedSku = NAME_None;
        ConfirmedQuoteVersion.Reset();
        ConfirmedPriceText = FText();
    }
    ViewModel = Model;
    BuildPage();
}

void URebuildLaunchSpecialOfferWidget::HandleOfferButton(FRebuildActivityActionRequest RequestIn)
{
    HandlePurchaseIntent(RequestIn);
}

void URebuildLaunchSpecialOfferWidget::HandlePurchaseIntent(FRebuildActivityActionRequest RequestIn)
{
    const bool bPurchaseGates = !ViewModel.OfferId.IsNone() && ViewModel.bSkuKnown && !ViewModel.Sku.IsNone() &&
        !ViewModel.QuoteVersion.IsEmpty() && ViewModel.bPriceKnown &&
        !ViewModel.PriceText.IsEmpty() && ViewModel.bEntitlementKnown && !ViewModel.bOwnsEntitlement &&
        ViewModel.bPlatformPaymentAvailable && ViewModel.bServicePurchaseAvailable && ViewModel.bConnected && !ViewModel.bBusy;
    if (!bPurchaseGates) return;
    if (!bConfirmingPurchase)
    {
        ConfirmedOfferId = ViewModel.OfferId;
        ConfirmedSku = ViewModel.Sku;
        ConfirmedQuoteVersion = ViewModel.QuoteVersion;
        ConfirmedPriceText = ViewModel.PriceText;
        bConfirmingPurchase = true;
        BuildPage();
        return;
    }
    if (ConfirmedOfferId != ViewModel.OfferId || ConfirmedSku != ViewModel.Sku || ConfirmedQuoteVersion != ViewModel.QuoteVersion ||
        ConfirmedPriceText.ToString() != ViewModel.PriceText.ToString()) return;
    bConfirmingPurchase = false;
    auto Purchase = ActivitySecondaryUI::Request(TEXT("purchase"), ConfirmedSku, true);
    Purchase.OfferId = ConfirmedOfferId;
    Purchase.Version = ConfirmedQuoteVersion;
    Purchase.PriceText = ConfirmedPriceText.ToString();
    OnActionRequested.Broadcast(Purchase);
    ConfirmedOfferId = NAME_None;
    ConfirmedSku = NAME_None;
    ConfirmedQuoteVersion.Reset();
    ConfirmedPriceText = FText();
    BuildPage();
}

void URebuildLaunchSpecialOfferWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace ActivitySecondaryUI;
    SetStableRoot(WidgetTree, StableRoot, StableFrame);
    StableRoot->ClearChildren();
    AddHeader(WidgetTree, StableRoot, FText::FromString(TEXT("开服特惠")), ViewModel.StatusText);
    auto* Details = Vertical(WidgetTree);
    if (ViewModel.State != ERebuildActivityPageState::Ready && ViewModel.State != ERebuildActivityPageState::Unknown)
        Add(Details, StatePanel(WidgetTree, ViewModel.State, ViewModel.StatusText), 8.f);
    Add(Details, Text(WidgetTree, ViewModel.Title.IsEmpty() ? FText::FromString(TEXT("商品资料待同步")) : ViewModel.Title,
        28, InkRebuildUI::ETextRole::Title, InkRebuildUI::Ink()), 8.f);
    if (!ViewModel.Description.IsEmpty()) Add(Details, Text(WidgetTree, ViewModel.Description, 20,
        InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 12.f);
    Add(Details, KeyValue(WidgetTree, FText::FromString(TEXT("商品编号")), FText::FromName(ViewModel.Sku), ViewModel.bSkuKnown), 8.f);
    Add(Details, KeyValue(WidgetTree, FText::FromString(TEXT("商品价格")), ViewModel.PriceText, ViewModel.bPriceKnown), 8.f);
    Add(Details, KeyValue(WidgetTree, FText::FromString(TEXT("权益状态")), ViewModel.EntitlementText, ViewModel.bEntitlementKnown), 12.f);
    const bool bCanPurchase = !ViewModel.OfferId.IsNone() && ViewModel.bSkuKnown && !ViewModel.Sku.IsNone() &&
        !ViewModel.QuoteVersion.IsEmpty() && ViewModel.bPriceKnown &&
        !ViewModel.PriceText.IsEmpty() && ViewModel.bEntitlementKnown && !ViewModel.bOwnsEntitlement &&
        ViewModel.bPlatformPaymentAvailable && ViewModel.bServicePurchaseAvailable && ViewModel.bConnected && !ViewModel.bBusy;
    if (bCanPurchase)
    {
        const FText Caption = bConfirmingPurchase ? FText::FromString(TEXT("再次确认购买")) : FText::FromString(TEXT("确认购买"));
        auto* Confirm = Button(WidgetTree, Text(WidgetTree, Caption, 23, InkRebuildUI::ETextRole::Button),
            this, NAME_None, true, false);
        Confirm->Request = Request(TEXT("purchase"), ViewModel.Sku, true);
        Confirm->OnIntentRequested.AddUniqueDynamic(this, &URebuildLaunchSpecialOfferWidget::HandleOfferButton);
        Add(Details, Confirm, 0.f);
    }
    else
    {
        const FText DisabledText = !ViewModel.StatusText.IsEmpty() ? ViewModel.StatusText : FText::FromString(TEXT("SKU、权威价格与购买接口待同步 · 暂不可用"));
        Add(Details, Card(WidgetTree, Text(WidgetTree, DisabledText, 20,
            InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), InkRebuildUI::Background()), 0.f);
    }
    if (bWideLayout)
    {
        auto* Columns = Horizontal(WidgetTree);
        auto* StatusColumn = Vertical(WidgetTree);
        Add(StatusColumn, Text(WidgetTree, FText::FromString(TEXT("特惠资料")), 30,
            InkRebuildUI::ETextRole::Section, InkRebuildUI::Jade()), 10.f);
        Add(StatusColumn, Text(WidgetTree, ViewModel.StatusText.IsEmpty()
            ? FText::FromString(TEXT("商品信息待同步")) : ViewModel.StatusText, 21,
            InkRebuildUI::ETextRole::Body, InkRebuildUI::Muted()), 0.f);
        auto* StatusSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        StatusSize->SetMinDesiredWidth(360.f);
        StatusSize->SetContent(Card(WidgetTree, StatusColumn, InkRebuildUI::Paper()));
        Add(Columns, StatusSize, 20.f);
        auto* DetailsSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        DetailsSize->SetMinDesiredWidth(560.f);
        DetailsSize->SetContent(Card(WidgetTree, Details, InkRebuildUI::Background()));
        Add(Columns, DetailsSize, 0.f);
        Add(StableRoot, Card(WidgetTree, Columns, InkRebuildUI::Background(), FMargin(24.f)), 0.f);
    }
    else
    {
        Add(StableRoot, Card(WidgetTree, Details, InkRebuildUI::Background(), FMargin(24.f)), 0.f);
    }
}

void URebuildOfflineRewardsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildOfflineRewardsWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNowWide = Size.X / Size.Y >= 1.15f;
    if (!bMeasured || bWideLayout != bNowWide)
    {
        bMeasured = true;
        bWideLayout = bNowWide;
        BuildPage();
    }
}

void URebuildOfflineRewardsWidget::ApplyViewModel(const FRebuildOfflineRewardsViewModel& Model)
{
    ViewModel = Model;
    BuildPage();
}

void URebuildOfflineRewardsWidget::HandleAcknowledge(FRebuildActivityActionRequest RequestIn)
{
    if (!ViewModel.bCanAcknowledge || !ViewModel.bConnected || ViewModel.bBusy) return;
    OnActionRequested.Broadcast(ActivitySecondaryUI::Request(TEXT("ack-offline")));
}

void URebuildOfflineRewardsWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace ActivitySecondaryUI;
    SetStableRoot(WidgetTree, StableRoot, StableFrame);
    const float PreviousOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StableRoot->ClearChildren();
    AddHeader(WidgetTree, StableRoot, FText::FromString(TEXT("离线记录")), ViewModel.StatusText);

    auto* Summary = Vertical(WidgetTree);
    Add(Summary, KeyValue(WidgetTree, FText::FromString(TEXT("已入账")), ViewModel.OfflineTotalQiText, ViewModel.bTotalQiKnown), 8.f);
    Add(Summary, KeyValue(WidgetTree, FText::FromString(TEXT("已确认阅读")), ViewModel.AcknowledgedQiText,
        ViewModel.bAcknowledgedQiKnown), 0.f);

    auto* Rows = Vertical(WidgetTree);
    if (!ViewModel.bReceiptsProvided || ViewModel.Receipts.IsEmpty())
    {
        const auto MissingState = ViewModel.bReceiptsProvided ? ERebuildActivityPageState::Empty :
            (ViewModel.State == ERebuildActivityPageState::Unknown || ViewModel.State == ERebuildActivityPageState::Ready
                ? ERebuildActivityPageState::Loading : ViewModel.State);
        Add(Rows, StatePanel(WidgetTree, MissingState, ViewModel.StatusText), 0.f);
    }
    else
    {
        for (const auto& Receipt : ViewModel.Receipts)
        {
            auto* Row = Vertical(WidgetTree);
            Add(Row, Text(WidgetTree, Receipt.bSourceKnown ? Receipt.SourceText : FText::FromString(TEXT("来源待同步")),
                23, InkRebuildUI::ETextRole::Section, InkRebuildUI::Ink()), 6.f);
            Add(Row, KeyValue(WidgetTree, FText::FromString(TEXT("时间")), Receipt.TimeText, Receipt.bTimeKnown), 6.f);
            Add(Row, KeyValue(WidgetTree, FText::FromString(TEXT("灵气")), Receipt.QiText, Receipt.bQiKnown), 6.f);
            if (!Receipt.StateText.IsEmpty()) Add(Row, Text(WidgetTree, Receipt.StateText, 18,
                InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Jade()), 0.f);
            Add(Rows, Card(WidgetTree, Row, InkRebuildUI::Paper()), 10.f);
        }
    }

    auto* Body = Vertical(WidgetTree);
    if (bWideLayout)
    {
        auto* Columns = Horizontal(WidgetTree);
        auto* SummarySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        SummarySize->SetMinDesiredWidth(340.f);
        SummarySize->SetContent(Card(WidgetTree, Summary, InkRebuildUI::Background()));
        Add(Columns, SummarySize, 16.f);
        auto* RowsSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        RowsSize->SetMinDesiredWidth(560.f);
        RowsSize->SetContent(Rows);
        Add(Columns, RowsSize, 0.f);
        Add(Body, Columns, 0.f);
    }
    else
    {
        Add(Body, Card(WidgetTree, Summary, InkRebuildUI::Background()), 10.f);
        Add(Body, Rows, 0.f);
    }

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    Scroll->SetAllowOverscroll(false);
    ActiveScroll = Scroll;
    Scroll->AddChild(Card(WidgetTree, Body, InkRebuildUI::Background(), FMargin(InkRebuildUI::SafePadding, 8.f)));
    Add(StableRoot, Scroll, 0.f);
    if (auto* Slot = Cast<UVerticalBoxSlot>(Scroll->Slot))
    {
        FSlateChildSize FillSize(ESlateSizeRule::Fill);
        FillSize.Value = 1.f;
        Slot->SetSize(FillSize);
        Slot->SetPadding(FMargin(InkRebuildUI::SafePadding, 8.f));
    }
    Scroll->SetScrollOffset(PreviousOffset);

    const bool bCanAcknowledge = ViewModel.bCanAcknowledge && ViewModel.bConnected && !ViewModel.bBusy;
    const FText AckLabel = ViewModel.bBusy ? FText::FromString(TEXT("等待当前请求完成")) : FText::FromString(TEXT("确认已读"));
    auto* Ack = Button(WidgetTree, Text(WidgetTree, AckLabel, 22, InkRebuildUI::ETextRole::Button),
        this, NAME_None, bCanAcknowledge, false);
    Ack->OnIntentRequested.AddUniqueDynamic(this, &URebuildOfflineRewardsWidget::HandleAcknowledge);
    Add(StableRoot, Card(WidgetTree, Ack, InkRebuildUI::Background(), FMargin(24.f, 12.f)), 0.f);
}
