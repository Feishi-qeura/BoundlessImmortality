#include "RebuildEconomyWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"
#include "Misc/DefaultValueHelper.h"

namespace RebuildEconomyUI
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
    FMargin Padding = FMargin(18.f))
{
    auto* Result = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Result->SetBrushColor(Fill);
    Result->SetPadding(Padding);
    Result->SetContent(Child);
    return Result;
}

UVerticalBox* Vertical(UWidgetTree* Tree) { return Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass()); }
UHorizontalBox* Horizontal(UWidgetTree* Tree) { return Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass()); }

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

URebuildActivityIntentButton* Button(UWidgetTree* Tree, UWidget* Child, URebuildActivityPageWidget* Owner,
    FName Action, FName EntryId = NAME_None, FName Cycle = NAME_None, bool bEnabled = true, bool bRelay = true)
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
    Result->Request.EntryId = EntryId;
    Result->Request.Cycle = Cycle;
    if (bRelay && Owner)
        Result->OnIntentRequested.AddUniqueDynamic(Owner, &URebuildActivityPageWidget::RelayIntent);
    return Result;
}

FRebuildActivityActionRequest Request(FName Action, FName EntryId = NAME_None, bool bConfirmed = false)
{
    FRebuildActivityActionRequest Result;
    Result.Action = Action;
    Result.EntryId = EntryId;
    Result.bConfirmed = bConfirmed;
    return Result;
}

UBorder* StableRoot(UWidgetTree* Tree, UVerticalBox*& Root, UBorder*& Frame)
{
    if (!Root)
    {
        Root = Vertical(Tree);
        Root->SetClipping(EWidgetClipping::ClipToBounds);
        Frame = Card(Tree, Root, InkRebuildUI::Background(), FMargin(0.f));
        Tree->RootWidget = Frame;
    }
    return Frame;
}

void Header(UWidgetTree* Tree, UVerticalBox* Root, const FText& Title, const FText& Status)
{
    auto* Column = Vertical(Tree);
    Add(Column, Text(Tree, Title, 40, InkRebuildUI::ETextRole::Display), 4.f);
    if (!Status.IsEmpty()) Add(Column, Text(Tree, Status, 20, InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 0.f);
    Add(Root, Card(Tree, Column, InkRebuildUI::Background(), FMargin(24.f, 16.f)), 10.f);
}

UWidget* StatePanel(UWidgetTree* Tree, ERebuildActivityPageState State, const FText& Detail)
{
    FText Heading;
    switch (State)
    {
    case ERebuildActivityPageState::Loading: Heading = FText::FromString(TEXT("资料正在同步")); break;
    case ERebuildActivityPageState::Offline: Heading = FText::FromString(TEXT("服务暂时离线")); break;
    case ERebuildActivityPageState::Empty: Heading = FText::FromString(TEXT("当前没有可显示的记录")); break;
    case ERebuildActivityPageState::Error: Heading = FText::FromString(TEXT("资料暂时无法读取")); break;
    case ERebuildActivityPageState::Pending: Heading = FText::FromString(TEXT("等待服务确认")); break;
    case ERebuildActivityPageState::Disabled: Heading = FText::FromString(TEXT("当前功能不可用")); break;
    default: Heading = FText::FromString(TEXT("状态待同步")); break;
    }
    auto* Body = Vertical(Tree);
    Add(Body, Text(Tree, Heading, 24, InkRebuildUI::ETextRole::Section), 8.f);
    if (!Detail.IsEmpty()) Add(Body, Text(Tree, Detail, 19, InkRebuildUI::ETextRole::Body, InkRebuildUI::Muted()), 0.f);
    return Card(Tree, Body, InkRebuildUI::Paper());
}

UWidget* DataRow(UWidgetTree* Tree, const FText& Key, const FText& Value, bool bKnown)
{
    auto* Row = Horizontal(Tree);
    Add(Row, Text(Tree, Key, 19, InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 14.f);
    Add(Row, Text(Tree, bKnown ? Value : FText::FromString(TEXT("待同步")), 21,
        InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 0.f);
    return Card(Tree, Row, InkRebuildUI::Paper(), FMargin(14.f, 10.f));
}

void AddScroll(UWidgetTree* Tree, UVerticalBox* Root, UScrollBox*& ActiveScroll, UWidget* Content, float PreviousOffset)
{
    auto* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    Scroll->SetAllowOverscroll(false);
    Scroll->AddChild(Card(Tree, Content, InkRebuildUI::Background(), FMargin(InkRebuildUI::SafePadding, 8.f)));
    Add(Root, Scroll, 0.f);
    if (auto* Slot = Cast<UVerticalBoxSlot>(Scroll->Slot))
    {
        FSlateChildSize Fill(ESlateSizeRule::Fill);
        Fill.Value = 1.f;
        Slot->SetSize(Fill);
        Slot->SetPadding(FMargin(InkRebuildUI::SafePadding, 8.f));
    }
    Scroll->SetScrollOffset(PreviousOffset);
    ActiveScroll = Scroll;
}

bool ReadPositiveCount(const FString& Raw, int32& OutCount)
{
    const FString Trimmed = Raw.TrimStartAndEnd();
    return FDefaultValueHelper::ParseInt(Trimmed, OutCount) && OutCount > 0;
}
}

// Shop -----------------------------------------------------------------------

void URebuildShopWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildShopWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const auto Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNowWide = Size.X / Size.Y >= 1.15f;
    if (!bMeasured || bNowWide != bWideLayout) { bMeasured = true; bWideLayout = bNowWide; BuildPage(); }
}

void URebuildShopWidget::ApplyViewModel(const FRebuildShopViewModel& Model)
{
    const auto* NewSelected = Model.Entries.FindByPredicate([&Model](const auto& Entry) { return Entry.Sku == Model.SelectedSku; });
    if (bConfirmingPurchase && (Model.SelectedSku != ConfirmedSku || !NewSelected ||
        NewSelected->Sku != ConfirmedSku || !NewSelected->bSkuKnown || !NewSelected->bPriceKnown || !NewSelected->bCanPurchase ||
        NewSelected->QuoteVersion != ConfirmedQuoteVersion || NewSelected->PriceText.ToString() != ConfirmedPriceText.ToString()))
    {
        bConfirmingPurchase = false;
        ConfirmedSku = NAME_None;
        ConfirmedQuoteVersion.Reset();
        ConfirmedPriceText = FText();
    }
    ViewModel = Model;
    BuildPage();
}

void URebuildShopWidget::HandleShopIntent(FRebuildActivityActionRequest RequestIn)
{
    if (RequestIn.Action.IsNone())
    {
        if (ViewModel.SelectedSku != RequestIn.EntryId && bConfirmingPurchase)
        {
            bConfirmingPurchase = false;
            ConfirmedSku = NAME_None;
            ConfirmedQuoteVersion.Reset();
            ConfirmedPriceText = FText();
        }
        ViewModel.SelectedSku = RequestIn.EntryId;
        BuildPage();
        return;
    }
    if (ViewModel.bBusy || !ViewModel.bConnected) return;
    if (RequestIn.Action == TEXT("query-shop"))
    {
        if (ViewModel.bQueryAllowed) OnActionRequested.Broadcast(RequestIn);
        return;
    }
    if (RequestIn.Action != TEXT("purchase")) return;
    const auto* Selected = ViewModel.Entries.FindByPredicate([this](const auto& Entry) { return Entry.Sku == ViewModel.SelectedSku; });
    if (!Selected || !Selected->bSkuKnown || Selected->Sku.IsNone() || !Selected->bPriceKnown ||
        Selected->PriceText.IsEmpty() || Selected->QuoteVersion.IsEmpty() || !Selected->bCanPurchase) return;
    if (!bConfirmingPurchase)
    {
        ConfirmedSku = Selected->Sku;
        ConfirmedQuoteVersion = Selected->QuoteVersion;
        ConfirmedPriceText = Selected->PriceText;
        bConfirmingPurchase = true;
        BuildPage();
        return;
    }
    if (Selected->Sku != ConfirmedSku || Selected->QuoteVersion != ConfirmedQuoteVersion ||
        Selected->PriceText.ToString() != ConfirmedPriceText.ToString()) return;
    auto Purchase = RebuildEconomyUI::Request(TEXT("purchase"), ConfirmedSku, true);
    Purchase.Version = ConfirmedQuoteVersion;
    Purchase.PriceText = ConfirmedPriceText.ToString();
    OnActionRequested.Broadcast(Purchase);
    bConfirmingPurchase = false;
    ConfirmedSku = NAME_None;
    ConfirmedQuoteVersion.Reset();
    ConfirmedPriceText = FText();
    BuildPage();
}

void URebuildShopWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace RebuildEconomyUI;
    RebuildEconomyUI::StableRoot(WidgetTree, StableRoot, StableFrame);
    const float PreviousOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StableRoot->ClearChildren();
    Header(WidgetTree, StableRoot, FText::FromString(TEXT("原型商店")), ViewModel.StatusText);

    auto* Controls = Horizontal(WidgetTree);
    const FText OwnedLabel = ViewModel.bOwnedOnly ? FText::FromString(TEXT("仅看已拥有")) : FText::FromString(TEXT("查看全部"));
    auto* Owned = Button(WidgetTree, Text(WidgetTree, OwnedLabel, 20, InkRebuildUI::ETextRole::Button),
        this, TEXT("query-shop"), NAME_None, NAME_None, ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy);
    Owned->Request.Offset = 0;
    Owned->Request.bOwnedOnly = !ViewModel.bOwnedOnly;
    Add(Controls, Owned, 12.f);
    auto* Refresh = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("刷新目录")), 20,
        InkRebuildUI::ETextRole::Button), this, TEXT("query-shop"), NAME_None, NAME_None,
        ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy);
    Refresh->Request.Offset = ViewModel.Offset;
    Refresh->Request.bOwnedOnly = ViewModel.bOwnedOnly;
    Add(Controls, Refresh, 0.f);
    if (ViewModel.PageSize > 0)
    {
        auto* PageControls = Horizontal(WidgetTree);
        auto* Previous = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("上一页")), 19,
            InkRebuildUI::ETextRole::Button), this, TEXT("query-shop"), NAME_None, NAME_None,
            ViewModel.Offset >= ViewModel.PageSize && ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy);
        Previous->Request.Offset = FMath::Max(0, ViewModel.Offset - ViewModel.PageSize);
        Previous->Request.bOwnedOnly = ViewModel.bOwnedOnly;
        Add(PageControls, Previous, 12.f);
        auto* Next = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("下一页")), 19,
            InkRebuildUI::ETextRole::Button), this, TEXT("query-shop"), NAME_None, NAME_None,
            ViewModel.bHasNextPage && ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy);
        Next->Request.Offset = ViewModel.Offset + ViewModel.PageSize;
        Next->Request.bOwnedOnly = ViewModel.bOwnedOnly;
        Add(PageControls, Next, 0.f);
        Add(Controls, PageControls, 0.f);
    }

    auto* Rows = Vertical(WidgetTree);
    if (!ViewModel.bEntriesProvided || ViewModel.Entries.IsEmpty())
    {
        const auto PageState = ViewModel.bEntriesProvided ? ERebuildActivityPageState::Empty :
            (ViewModel.State == ERebuildActivityPageState::Unknown || ViewModel.State == ERebuildActivityPageState::Ready
                ? ERebuildActivityPageState::Loading : ViewModel.State);
        Add(Rows, StatePanel(WidgetTree, PageState, ViewModel.StatusText), 0.f);
    }
    else
    {
        for (const auto& Entry : ViewModel.Entries)
        {
            const bool bSelected = Entry.Sku == ViewModel.SelectedSku;
            auto* Copy = Vertical(WidgetTree);
            Add(Copy, Text(WidgetTree, Entry.bSkuKnown && !Entry.Name.IsEmpty() ? Entry.Name : FText::FromString(TEXT("商品资料待同步")),
                21, InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 4.f);
            Add(Copy, Text(WidgetTree, Entry.bPriceKnown ? Entry.PriceText : FText::FromString(TEXT("价格待同步")),
                18, InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 0.f);
            auto* Row = Horizontal(WidgetTree);
            Add(Row, Copy, 10.f);
            Add(Row, Text(WidgetTree, Entry.StateText.IsEmpty() ? FText::FromString(TEXT("查看详情")) : Entry.StateText,
                17, InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Jade()), 0.f);
            auto* Select = Button(WidgetTree, Card(WidgetTree, Row, bSelected ? InkRebuildUI::Paper() : FLinearColor::White),
                this, NAME_None, NAME_None, NAME_None, Entry.bSkuKnown && !Entry.Sku.IsNone(), false);
            Select->Request.EntryId = Entry.Sku;
            Select->OnIntentRequested.AddUniqueDynamic(this, &URebuildShopWidget::HandleShopIntent);
            Add(Rows, Select, 8.f);
        }
    }

    auto* Detail = Vertical(WidgetTree);
    const auto* Selected = ViewModel.Entries.FindByPredicate([this](const auto& Entry) { return Entry.Sku == ViewModel.SelectedSku; });
    if (!Selected)
        Add(Detail, StatePanel(WidgetTree, ERebuildActivityPageState::Unknown, FText::FromString(TEXT("选择服务目录中的商品查看详情"))), 0.f);
    else
    {
        Add(Detail, Text(WidgetTree, Selected->Name.IsEmpty() ? FText::FromString(TEXT("商品资料待同步")) : Selected->Name,
            28, InkRebuildUI::ETextRole::Title, InkRebuildUI::Ink()), 10.f);
        if (!Selected->Description.IsEmpty()) Add(Detail, Text(WidgetTree, Selected->Description, 20,
            InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 10.f);
        Add(Detail, DataRow(WidgetTree, FText::FromString(TEXT("费用")), Selected->CostText, !Selected->CostText.IsEmpty()), 8.f);
        Add(Detail, DataRow(WidgetTree, FText::FromString(TEXT("产出")), Selected->OutputText, !Selected->OutputText.IsEmpty()), 8.f);
        const bool bCanBuy = Selected->bSkuKnown && !Selected->Sku.IsNone() && Selected->bPriceKnown &&
            !Selected->PriceText.IsEmpty() && !Selected->QuoteVersion.IsEmpty() && Selected->bCanPurchase &&
            ViewModel.bConnected && !ViewModel.bBusy;
        if (bCanBuy)
        {
            auto* Buy = Button(WidgetTree, Text(WidgetTree, bConfirmingPurchase ? FText::FromString(TEXT("再次确认购买")) : FText::FromString(TEXT("确认购买")),
                22, InkRebuildUI::ETextRole::Button), this, TEXT("purchase"), Selected->Sku, NAME_None, true, false);
            Buy->Request.EntryId = Selected->Sku;
            Buy->OnIntentRequested.AddUniqueDynamic(this, &URebuildShopWidget::HandleShopIntent);
            Add(Detail, Buy, 0.f);
        }
        else Add(Detail, StatePanel(WidgetTree, ViewModel.bBusy ? ERebuildActivityPageState::Pending : ERebuildActivityPageState::Disabled,
            FText::FromString(TEXT("当前商品或购买资料不可用"))), 0.f);
    }

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    Scroll->SetAllowOverscroll(false);
    auto* Body = Vertical(WidgetTree);
    Add(Body, Controls, 10.f);
    if (bWideLayout)
    {
        auto* Columns = Horizontal(WidgetTree);
        auto* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        ListSize->SetMinDesiredWidth(480.f); ListSize->SetContent(Rows); Add(Columns, ListSize, 18.f);
        auto* DetailSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        DetailSize->SetMinDesiredWidth(440.f); DetailSize->SetContent(Card(WidgetTree, Detail, InkRebuildUI::Background())); Add(Columns, DetailSize, 0.f);
        Add(Body, Columns, 0.f);
    }
    else
    {
        Add(Body, Card(WidgetTree, Detail, InkRebuildUI::Background()), 10.f);
        Add(Body, Rows, 0.f);
    }
    AddScroll(WidgetTree, StableRoot, ActiveScroll, Body, PreviousOffset);
    auto* Footer = Vertical(WidgetTree);
    Add(Footer, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("返回")), 22,
        InkRebuildUI::ETextRole::Button), this, TEXT("back")), 0.f);
    Add(StableRoot, Card(WidgetTree, Footer, InkRebuildUI::Background(), FMargin(24.f, 12.f)), 0.f);
}

// Consignment ----------------------------------------------------------------

void URebuildConsignmentWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void URebuildConsignmentWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const auto Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNowWide = Size.X / Size.Y >= 1.15f;
    if (!bMeasured || bNowWide != bWideLayout) { bMeasured = true; bWideLayout = bNowWide; BuildPage(); }
}

void URebuildConsignmentWidget::ApplyViewModel(const FRebuildConsignmentViewModel& Model)
{
    int32 ParsedDraftCount = 0;
    const auto* NewInventoryItem = Model.InventoryEntries.FindByPredicate([&Model](const auto& Row) { return Row.ItemId == ConfirmedItemId; });
    if (bConfirmingCreate && (Model.SelectedInventoryItemId != ConfirmedItemId || !NewInventoryItem || !NewInventoryItem->bCanList ||
        !Model.bPriceRulesKnown || !Model.bCreateAllowed || Model.DraftUnitPriceText != ConfirmedPriceText ||
        !RebuildEconomyUI::ReadPositiveCount(Model.DraftQuantityText, ParsedDraftCount) || ParsedDraftCount != ConfirmedQuantity))
    {
        bConfirmingCreate = false;
        ConfirmedItemId = NAME_None;
        ConfirmedQuantity = 0;
        ConfirmedPriceText.Reset();
    }
    const auto* NewSelectedListing = Model.Listings.FindByPredicate([&Model](const auto& Row) { return Row.ListingId == Model.SelectedListingId; });
    if (bConfirmingBuy && (Model.SelectedListingId != ConfirmedListingId || !NewSelectedListing || !NewSelectedListing->bCanBuy ||
        NewSelectedListing->ListingId != ConfirmedListingId || NewSelectedListing->ListingVersion != ConfirmedListingVersion ||
        NewSelectedListing->PriceText.ToString() != ConfirmedBuyPriceText))
    {
        bConfirmingBuy = false;
        ConfirmedListingId = NAME_None;
        ConfirmedListingVersion = NAME_None;
        ConfirmedBuyPriceText.Reset();
    }
    ViewModel = Model;
    BuildPage();
}

void URebuildConsignmentWidget::HandleQuantityChanged(const FText& TextIn)
{
    ViewModel.DraftQuantityText = TextIn.ToString();
    bConfirmingCreate = false;
    ConfirmedItemId = NAME_None; ConfirmedQuantity = 0; ConfirmedPriceText.Reset();
}

void URebuildConsignmentWidget::HandlePriceChanged(const FText& TextIn)
{
    ViewModel.DraftUnitPriceText = TextIn.ToString();
    bConfirmingCreate = false;
    ConfirmedItemId = NAME_None; ConfirmedQuantity = 0; ConfirmedPriceText.Reset();
}

void URebuildConsignmentWidget::HandleConsignmentIntent(FRebuildActivityActionRequest RequestIn)
{
    if (RequestIn.Action.IsNone())
    {
        if (RequestIn.Cycle == TEXT("inventory"))
        {
            if (ViewModel.SelectedInventoryItemId != RequestIn.EntryId && bConfirmingCreate)
            {
                bConfirmingCreate = false; ConfirmedItemId = NAME_None; ConfirmedQuantity = 0; ConfirmedPriceText.Reset();
            }
            ViewModel.SelectedInventoryItemId = RequestIn.EntryId;
        }
        else if (RequestIn.Cycle == TEXT("listing"))
        {
            if (ViewModel.SelectedListingId != RequestIn.EntryId && bConfirmingBuy)
            {
                bConfirmingBuy = false; ConfirmedListingId = NAME_None; ConfirmedListingVersion = NAME_None; ConfirmedBuyPriceText.Reset();
            }
            ViewModel.SelectedListingId = RequestIn.EntryId;
        }
        BuildPage();
        return;
    }
    if (ViewModel.bBusy || !ViewModel.bConnected) return;
    if (RequestIn.Action == TEXT("query-market"))
    {
        if (ViewModel.bQueryAllowed) OnActionRequested.Broadcast(RequestIn);
        return;
    }
    if (RequestIn.Action == TEXT("buy-listing"))
    {
        const auto* Selected = ViewModel.Listings.FindByPredicate([this](const auto& Row) { return Row.ListingId == ViewModel.SelectedListingId; });
        if (!Selected || !Selected->bCanBuy || Selected->ListingId.IsNone() || Selected->ListingVersion.IsNone()) return;
        if (!bConfirmingBuy)
        {
            ConfirmedListingId = Selected->ListingId; ConfirmedListingVersion = Selected->ListingVersion; ConfirmedBuyPriceText = Selected->PriceText.ToString();
            bConfirmingBuy = true; BuildPage(); return;
        }
        if (ViewModel.SelectedListingId != ConfirmedListingId || Selected->ListingId != ConfirmedListingId ||
            Selected->ListingVersion != ConfirmedListingVersion || Selected->PriceText.ToString() != ConfirmedBuyPriceText) return;
        auto Buy = RebuildEconomyUI::Request(TEXT("buy-listing"), ConfirmedListingId, true);
        Buy.Version = ConfirmedListingVersion.ToString();
        OnActionRequested.Broadcast(Buy);
        bConfirmingBuy = false; ConfirmedListingId = NAME_None; ConfirmedListingVersion = NAME_None; ConfirmedBuyPriceText.Reset();
        BuildPage(); return;
    }
    if (RequestIn.Action == TEXT("create-listing"))
    {
        const auto* Item = ViewModel.InventoryEntries.FindByPredicate([this](const auto& Row) { return Row.ItemId == ViewModel.SelectedInventoryItemId; });
        int32 Count = 0;
        if (!Item || !Item->bCanList || Item->ItemId.IsNone() || !ViewModel.bPriceRulesKnown || !ViewModel.bCreateAllowed ||
            !ReadPositiveCount(ViewModel.DraftQuantityText, Count) || ViewModel.DraftUnitPriceText.TrimStartAndEnd().IsEmpty()) return;
        if (!bConfirmingCreate)
        {
            ConfirmedItemId = Item->ItemId; ConfirmedQuantity = Count; ConfirmedPriceText = ViewModel.DraftUnitPriceText;
            bConfirmingCreate = true; BuildPage(); return;
        }
        if (Item->ItemId != ConfirmedItemId || Count != ConfirmedQuantity || ViewModel.DraftUnitPriceText != ConfirmedPriceText) return;
        auto Create = RebuildEconomyUI::Request(TEXT("create-listing"), ConfirmedItemId, true);
        Create.Quantity = ConfirmedQuantity;
        Create.QuantityText = ViewModel.DraftQuantityText;
        Create.PriceText = ConfirmedPriceText;
        OnActionRequested.Broadcast(Create);
        bConfirmingCreate = false; ConfirmedItemId = NAME_None; ConfirmedQuantity = 0; ConfirmedPriceText.Reset();
        BuildPage();
    }
}

void URebuildConsignmentWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace RebuildEconomyUI;
    RebuildEconomyUI::StableRoot(WidgetTree, StableRoot, StableFrame);
    const float PreviousOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StableRoot->ClearChildren();
    Header(WidgetTree, StableRoot, FText::FromString(TEXT("寄售")), ViewModel.StatusText);
    auto* Controls = Horizontal(WidgetTree);
    auto* Filter = Button(WidgetTree, Text(WidgetTree, ViewModel.bOwnedOnly ? FText::FromString(TEXT("仅看我的寄售")) : FText::FromString(TEXT("查看市场")),
        19, InkRebuildUI::ETextRole::Button), this, TEXT("query-market"), NAME_None, NAME_None,
        ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy, false);
    Filter->Request.Offset = 0; Filter->Request.bOwnedOnly = !ViewModel.bOwnedOnly;
    Filter->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
    Add(Controls, Filter, 12.f);
    auto* Refresh = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("刷新")), 19,
        InkRebuildUI::ETextRole::Button), this, TEXT("query-market"), NAME_None, NAME_None,
        ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy, false);
    Refresh->Request.Offset = ViewModel.Offset; Refresh->Request.bOwnedOnly = ViewModel.bOwnedOnly;
    Refresh->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
    Add(Controls, Refresh, 0.f);
    if (ViewModel.PageSize > 0)
    {
        auto* PageControls = Horizontal(WidgetTree);
        auto* Previous = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("上一页")), 19,
            InkRebuildUI::ETextRole::Button), this, TEXT("query-market"), NAME_None, NAME_None,
            ViewModel.Offset >= ViewModel.PageSize && ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy, false);
        Previous->Request.Offset = FMath::Max(0, ViewModel.Offset - ViewModel.PageSize);
        Previous->Request.bOwnedOnly = ViewModel.bOwnedOnly;
        Previous->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
        Add(PageControls, Previous, 12.f);
        auto* Next = Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("下一页")), 19,
            InkRebuildUI::ETextRole::Button), this, TEXT("query-market"), NAME_None, NAME_None,
            ViewModel.bHasNextPage && ViewModel.bQueryAllowed && ViewModel.bConnected && !ViewModel.bBusy, false);
        Next->Request.Offset = ViewModel.Offset + ViewModel.PageSize;
        Next->Request.bOwnedOnly = ViewModel.bOwnedOnly;
        Next->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
        Add(PageControls, Next, 0.f);
        Add(Controls, PageControls, 0.f);
    }

    auto* Listings = Vertical(WidgetTree);
    if (!ViewModel.bListingsProvided || ViewModel.Listings.IsEmpty())
    {
        const auto State = ViewModel.bListingsProvided ? ERebuildActivityPageState::Empty :
            (ViewModel.State == ERebuildActivityPageState::Unknown || ViewModel.State == ERebuildActivityPageState::Ready ? ERebuildActivityPageState::Loading : ViewModel.State);
        Add(Listings, StatePanel(WidgetTree, State, ViewModel.StatusText), 0.f);
    }
    else for (const auto& Listing : ViewModel.Listings)
    {
        auto* Row = Horizontal(WidgetTree);
        auto* Copy = Vertical(WidgetTree);
        Add(Copy, Text(WidgetTree, Listing.bNameKnown ? Listing.Name : FText::FromString(TEXT("商品待同步")),
            20, InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), 4.f);
        Add(Copy, DataRow(WidgetTree, FText::FromString(TEXT("价格")), Listing.PriceText, Listing.bPriceKnown), 0.f);
        Add(Row, Copy, 12.f);
        if (!Listing.StateText.IsEmpty()) Add(Row, Text(WidgetTree, Listing.StateText, 17,
            InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 0.f);
        auto* Select = Button(WidgetTree, Card(WidgetTree, Row,
            Listing.ListingId == ViewModel.SelectedListingId ? InkRebuildUI::Paper() : FLinearColor::White),
            this, NAME_None, Listing.ListingId, TEXT("listing"), !Listing.ListingId.IsNone(), false);
        Select->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
        Add(Listings, Select, 8.f);
    }

    auto* SelectedDetail = Vertical(WidgetTree);
    const auto* Listing = ViewModel.Listings.FindByPredicate([this](const auto& Row) { return Row.ListingId == ViewModel.SelectedListingId; });
    if (Listing)
    {
        Add(SelectedDetail, DataRow(WidgetTree, FText::FromString(TEXT("商品")), Listing->Name, Listing->bNameKnown), 8.f);
        Add(SelectedDetail, DataRow(WidgetTree, FText::FromString(TEXT("价格")), Listing->PriceText, Listing->bPriceKnown), 8.f);
        const bool bCanBuy = Listing->bCanBuy && !Listing->ListingId.IsNone() && !Listing->ListingVersion.IsNone() && ViewModel.bConnected && !ViewModel.bBusy;
        if (bCanBuy)
        {
            auto* Buy = Button(WidgetTree, Text(WidgetTree, bConfirmingBuy ? FText::FromString(TEXT("再次确认购买")) : FText::FromString(TEXT("确认购买")),
                21, InkRebuildUI::ETextRole::Button), this, TEXT("buy-listing"), Listing->ListingId, NAME_None, true, false);
            Buy->Request.Version = Listing->ListingVersion.ToString();
            Buy->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
            Add(SelectedDetail, Buy, 10.f);
        }
    }
    else Add(SelectedDetail, StatePanel(WidgetTree, ERebuildActivityPageState::Unknown, FText::FromString(TEXT("选择真实寄售条目查看详情"))), 0.f);

    auto* Editor = Vertical(WidgetTree);
    Add(Editor, Text(WidgetTree, FText::FromString(TEXT("发布寄售")), 24,
        InkRebuildUI::ETextRole::Section, InkRebuildUI::Ink()), 8.f);
    auto* InventoryRows = Vertical(WidgetTree);
    if (!ViewModel.bInventoryProvided || ViewModel.InventoryEntries.IsEmpty())
    {
        const auto State = ViewModel.bInventoryProvided ? ERebuildActivityPageState::Empty : ERebuildActivityPageState::Loading;
        Add(InventoryRows, StatePanel(WidgetTree, State, FText::FromString(TEXT("可寄售物品待同步"))), 0.f);
    }
    else for (const auto& Entry : ViewModel.InventoryEntries)
    {
        const FText Name = Entry.bNameKnown ? Entry.Name : FText::FromString(TEXT("物品资料待同步"));
        const FText CountText = Entry.bCountKnown ? Entry.AvailableCountText : FText::FromString(TEXT("数量待同步"));
        auto* Choose = Button(WidgetTree, Text(WidgetTree,
            FText::FromString(FString::Printf(TEXT("%s   ·   %s"), *Name.ToString(), *CountText.ToString())), 19,
            InkRebuildUI::ETextRole::Body, InkRebuildUI::Ink()), this, NAME_None, Entry.ItemId, TEXT("inventory"), Entry.bCanList, false);
        Choose->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
        Add(InventoryRows, Choose, 6.f);
    }
    Add(Editor, InventoryRows, 8.f);

    QuantityInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    QuantityInput->SetFont(InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, 20));
    QuantityInput->SetHintText(FText::FromString(TEXT("数量")));
    QuantityInput->SetText(FText::FromString(ViewModel.DraftQuantityText));
    QuantityInput->OnTextChanged.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleQuantityChanged);
    Add(Editor, Card(WidgetTree, QuantityInput, InkRebuildUI::Paper()), 8.f);
    PriceInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
    PriceInput->SetFont(InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, 20));
    PriceInput->SetHintText(FText::FromString(TEXT("输入单价")));
    PriceInput->SetText(FText::FromString(ViewModel.DraftUnitPriceText));
    PriceInput->OnTextChanged.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandlePriceChanged);
    Add(Editor, Card(WidgetTree, PriceInput, InkRebuildUI::Paper()), 8.f);

    int32 ParsedCount = 0;
    const auto* InventoryItem = ViewModel.InventoryEntries.FindByPredicate([this](const auto& Entry) { return Entry.ItemId == ViewModel.SelectedInventoryItemId; });
    const bool bCanCreate = ViewModel.bPriceRulesKnown && ViewModel.bCreateAllowed && ViewModel.bConnected && !ViewModel.bBusy &&
        InventoryItem && InventoryItem->bCanList && ReadPositiveCount(ViewModel.DraftQuantityText, ParsedCount) &&
        !ViewModel.DraftUnitPriceText.TrimStartAndEnd().IsEmpty();
    if (bCanCreate)
    {
        auto* Create = Button(WidgetTree, Text(WidgetTree, bConfirmingCreate ? FText::FromString(TEXT("再次确认发布")) : FText::FromString(TEXT("发布寄售")),
            21, InkRebuildUI::ETextRole::Button), this, TEXT("create-listing"), NAME_None, NAME_None, true, false);
        Create->OnIntentRequested.AddUniqueDynamic(this, &URebuildConsignmentWidget::HandleConsignmentIntent);
        Add(Editor, Create, 0.f);
    }
    else Add(Editor, StatePanel(WidgetTree, ViewModel.bBusy ? ERebuildActivityPageState::Pending : ERebuildActivityPageState::Disabled,
        FText::FromString(TEXT("需等待真实库存与寄售规则"))), 0.f);

    auto* Content = Vertical(WidgetTree);
    Add(Content, Controls, 10.f);
    if (bWideLayout)
    {
        auto* Columns = Horizontal(WidgetTree);
        auto* ListColumn = Vertical(WidgetTree);
        Add(ListColumn, Listings, 10.f); Add(ListColumn, SelectedDetail, 0.f);
        auto* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()); ListSize->SetMinDesiredWidth(500.f); ListSize->SetContent(ListColumn); Add(Columns, ListSize, 18.f);
        auto* EditorSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()); EditorSize->SetMinDesiredWidth(440.f); EditorSize->SetContent(Card(WidgetTree, Editor, InkRebuildUI::Background())); Add(Columns, EditorSize, 0.f);
        Add(Content, Columns, 0.f);
    }
    else
    {
        Add(Content, Card(WidgetTree, SelectedDetail, InkRebuildUI::Background()), 10.f);
        Add(Content, Listings, 10.f);
        Add(Content, Card(WidgetTree, Editor, InkRebuildUI::Background()), 0.f);
    }
    AddScroll(WidgetTree, StableRoot, ActiveScroll, Content, PreviousOffset);
    auto* Footer = Vertical(WidgetTree);
    Add(Footer, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("返回")), 22,
        InkRebuildUI::ETextRole::Button), this, TEXT("back")), 0.f);
    Add(StableRoot, Card(WidgetTree, Footer, InkRebuildUI::Background(), FMargin(24.f, 12.f)), 0.f);
}

// Orders ---------------------------------------------------------------------

void URebuildOrdersWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPage();
}

void URebuildOrdersWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const auto Size = Geometry.GetLocalSize();
    if (Size.X <= 0.f || Size.Y <= 0.f) return;
    const bool bNowWide = Size.X / Size.Y >= 1.15f;
    if (!bMeasured || bNowWide != bWideLayout) { bMeasured = true; bWideLayout = bNowWide; BuildPage(); }
}

void URebuildOrdersWidget::ApplyViewModel(const FRebuildOrdersViewModel& Model)
{
    if (bConfirmingCancel)
    {
        const auto* NewRow = Model.Orders.FindByPredicate([this](const auto& Row) { return Row.ListingId == ConfirmedListingId; });
        if (Model.SelectedListingId != ConfirmedListingId || !NewRow || !NewRow->bCanCancel ||
            NewRow->ListingVersion != ConfirmedListingVersion)
        { bConfirmingCancel = false; ConfirmedListingId = NAME_None; ConfirmedListingVersion = NAME_None; }
    }
    ViewModel = Model;
    BuildPage();
}

void URebuildOrdersWidget::HandleCancelIntent(FRebuildActivityActionRequest RequestIn)
{
    if (RequestIn.Action.IsNone())
    {
        if (ViewModel.SelectedListingId != RequestIn.EntryId && bConfirmingCancel)
        { bConfirmingCancel = false; ConfirmedListingId = NAME_None; ConfirmedListingVersion = NAME_None; }
        ViewModel.SelectedListingId = RequestIn.EntryId; BuildPage(); return;
    }
    if (RequestIn.Action != TEXT("cancel-listing") || !ViewModel.bConnected || ViewModel.bBusy) return;
    const auto* Selected = ViewModel.Orders.FindByPredicate([this](const auto& Row) { return Row.ListingId == ViewModel.SelectedListingId; });
    if (!Selected || !Selected->bCanCancel || Selected->ListingId.IsNone() || Selected->ListingVersion.IsNone()) return;
    if (!bConfirmingCancel)
    {
        ConfirmedListingId = Selected->ListingId; ConfirmedListingVersion = Selected->ListingVersion;
        bConfirmingCancel = true; BuildPage(); return;
    }
    if (Selected->ListingId != ConfirmedListingId || Selected->ListingVersion != ConfirmedListingVersion) return;
    auto Cancel = RebuildEconomyUI::Request(TEXT("cancel-listing"), ConfirmedListingId, true);
    Cancel.Version = ConfirmedListingVersion.ToString();
    OnActionRequested.Broadcast(Cancel);
    bConfirmingCancel = false; ConfirmedListingId = NAME_None; ConfirmedListingVersion = NAME_None;
    BuildPage();
}

void URebuildOrdersWidget::BuildPage()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    using namespace RebuildEconomyUI;
    RebuildEconomyUI::StableRoot(WidgetTree, StableRoot, StableFrame);
    const float PreviousOffset = ActiveScroll ? ActiveScroll->GetScrollOffset() : 0.f;
    StableRoot->ClearChildren();
    Header(WidgetTree, StableRoot, FText::FromString(TEXT("我的订单")), ViewModel.StatusText);
    auto* Rows = Vertical(WidgetTree);
    if (!ViewModel.bOrdersProvided || ViewModel.Orders.IsEmpty())
    {
        const auto State = ViewModel.bOrdersProvided ? ERebuildActivityPageState::Empty :
            (ViewModel.State == ERebuildActivityPageState::Unknown || ViewModel.State == ERebuildActivityPageState::Ready ? ERebuildActivityPageState::Loading : ViewModel.State);
        Add(Rows, StatePanel(WidgetTree, State, ViewModel.StatusText), 0.f);
    }
    else for (const auto& Order : ViewModel.Orders)
    {
        const bool bSelected = Order.ListingId == ViewModel.SelectedListingId;
        auto* Details = Vertical(WidgetTree);
        Add(Details, DataRow(WidgetTree, FText::FromString(TEXT("商品")), Order.ItemText, Order.bItemKnown), 5.f);
        Add(Details, DataRow(WidgetTree, FText::FromString(TEXT("价格")), Order.PriceText, Order.bPriceKnown), 5.f);
        Add(Details, DataRow(WidgetTree, FText::FromString(TEXT("时间")), Order.TimeText, Order.bTimeKnown), 5.f);
        if (!Order.StateText.IsEmpty()) Add(Details, Text(WidgetTree, Order.StateText, 18,
            InkRebuildUI::ETextRole::Secondary, InkRebuildUI::Muted()), 0.f);
        auto* Select = Button(WidgetTree, Card(WidgetTree, Details, bSelected ? InkRebuildUI::Paper() : FLinearColor::White),
            this, NAME_None, Order.ListingId, NAME_None, !Order.ListingId.IsNone(), false);
        Select->OnIntentRequested.AddUniqueDynamic(this, &URebuildOrdersWidget::HandleCancelIntent);
        Add(Rows, Select, 8.f);
    }
    const auto* Selected = ViewModel.Orders.FindByPredicate([this](const auto& Order) { return Order.ListingId == ViewModel.SelectedListingId; });
    auto* Detail = Vertical(WidgetTree);
    if (Selected)
    {
        Add(Detail, Text(WidgetTree, Selected->bItemKnown ? Selected->ItemText : FText::FromString(TEXT("订单商品待同步")),
            24, InkRebuildUI::ETextRole::Section, InkRebuildUI::Ink()), 8.f);
        if (!Selected->StateText.IsEmpty()) Add(Detail, Text(WidgetTree, Selected->StateText, 20,
            InkRebuildUI::ETextRole::Body, InkRebuildUI::Muted()), 8.f);
        if (Selected->bCanCancel && !Selected->ListingId.IsNone() && !Selected->ListingVersion.IsNone() && ViewModel.bConnected && !ViewModel.bBusy)
        {
            auto* Cancel = Button(WidgetTree, Text(WidgetTree, bConfirmingCancel ? FText::FromString(TEXT("再次确认撤销")) : FText::FromString(TEXT("撤销寄售")),
                21, InkRebuildUI::ETextRole::Button), this, TEXT("cancel-listing"), Selected->ListingId, NAME_None, true, false);
            Cancel->Request.Version = Selected->ListingVersion.ToString();
            Cancel->OnIntentRequested.AddUniqueDynamic(this, &URebuildOrdersWidget::HandleCancelIntent);
            Add(Detail, Cancel, 0.f);
        }
    }
    else Add(Detail, StatePanel(WidgetTree, ERebuildActivityPageState::Unknown, FText::FromString(TEXT("选择真实订单查看可用操作"))), 0.f);

    auto* Content = Vertical(WidgetTree);
    if (bWideLayout)
    {
        auto* Columns = Horizontal(WidgetTree);
        auto* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()); ListSize->SetMinDesiredWidth(520.f); ListSize->SetContent(Rows); Add(Columns, ListSize, 18.f);
        auto* DetailSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()); DetailSize->SetMinDesiredWidth(440.f); DetailSize->SetContent(Card(WidgetTree, Detail, InkRebuildUI::Background())); Add(Columns, DetailSize, 0.f);
        Add(Content, Columns, 0.f);
    }
    else
    {
        Add(Content, Card(WidgetTree, Detail, InkRebuildUI::Background()), 10.f);
        Add(Content, Rows, 0.f);
    }
    AddScroll(WidgetTree, StableRoot, ActiveScroll, Content, PreviousOffset);
    auto* Footer = Vertical(WidgetTree);
    Add(Footer, Button(WidgetTree, Text(WidgetTree, FText::FromString(TEXT("返回")), 22,
        InkRebuildUI::ETextRole::Button), this, TEXT("back")), 0.f);
    Add(StableRoot, Card(WidgetTree, Footer, InkRebuildUI::Background(), FMargin(24.f, 12.f)), 0.f);
}
