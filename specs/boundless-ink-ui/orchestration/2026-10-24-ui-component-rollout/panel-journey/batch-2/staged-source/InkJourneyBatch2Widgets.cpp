#include "InkJourneyBatch2Widgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Frontend/InkPresentationStyle.h"
#include "Frontend/UI/InkUiTheme.h"

namespace InkJourneyBatch2
{
    UTextBlock* AddText(UWidgetTree* Tree, UVerticalBox* Parent, const FText& Text, int32 FontSize, EInkUiTextRole Role, FMargin Padding = FMargin(0))
    {
        if (!Tree || !Parent) return nullptr;
        auto* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Block->SetText(Text);
        Block->SetFont(InkUiTypography::MakeFont(Role, FontSize));
        Block->SetAutoWrapText(true);
        Block->SetColorAndOpacity(InkUiTypography::Theme()->InkTint);
        Parent->AddChildToVerticalBox(Block)->SetPadding(Padding);
        return Block;
    }

    UButton* AddButton(UWidgetTree* Tree, UVerticalBox* Parent, const FText& Text)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
        Button->SetStyle(InkPresentationStyle::FeatureButton(false, true));
        auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(Text);
        Label->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Button, 26));
        Label->SetJustification(ETextJustify::Center);
        Button->AddChild(Label);
        Parent->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 8, 0, 0));
        return Button;
    }

    UBorder* MakePaper(UWidgetTree* Tree)
    {
        auto* Paper = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Paper->SetBrush(InkPresentationStyle::Surface(InkPresentationStyle::ESurface::FeatureCard, FVector2D(680, 900)));
        return Paper;
    }

    UTextBlock* AddScrollText(UWidgetTree* Tree, UScrollBox* Parent, const FText& Text, int32 FontSize, EInkUiTextRole Role)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Block->SetText(Text);
        Block->SetFont(InkUiTypography::MakeFont(Role, FontSize));
        Block->SetAutoWrapText(true);
        Block->SetColorAndOpacity(InkUiTypography::Theme()->InkTint);
        Parent->AddChild(Block);
        return Block;
    }

    FString RowLine(const FInkJourneyDisplayRow& Row)
    {
        const FString Label = Row.Label.ToString();
        const FString Value = Row.Value.ToString();
        if (Label.IsEmpty()) return Value;
        if (Value.IsEmpty()) return Label;
        return Label + TEXT("  ·  ") + Value;
    }
}

void UInkJourneySettlementCardWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    EnsureTree();
}

void UInkJourneySettlementCardWidget::EnsureTree()
{
    if (!WidgetTree || WidgetTree->RootWidget) return;
    auto* Paper = InkJourneyBatch2::MakePaper(WidgetTree);
    Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Paper->SetContent(Content);
    WidgetTree->RootWidget = Paper;
    HeadingText = InkJourneyBatch2::AddText(WidgetTree, Content, HeadingValue, 34, EInkUiTextRole::Display, FMargin(22, 18, 22, 8));
    StatusText = InkJourneyBatch2::AddText(WidgetTree, Content, StatusValue, 23, EInkUiTextRole::Secondary, FMargin(22, 0, 22, 14));
    RowsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    RowsScroll->SetAllowOverscroll(false);
    RowsScroll->SetScrollbarThickness(FVector2D(5, 5));
    Content->AddChildToVerticalBox(RowsScroll)->SetPadding(FMargin(8, 0, 8, 12));
    RebuildRows();
}

void UInkJourneySettlementCardWidget::SetCardContent(const FText& Heading, const FText& Status, const TArray<FInkJourneyDisplayRow>& Rows)
{
    HeadingValue = Heading;
    StatusValue = Status;
    DisplayRows = Rows;
    EnsureTree();
    RebuildRows();
}

void UInkJourneySettlementCardWidget::RebuildRows()
{
    if (!WidgetTree || !RowsScroll) return;
    if (HeadingText) HeadingText->SetText(HeadingValue);
    if (StatusText) StatusText->SetText(StatusValue);
    RowsScroll->ClearChildren();
    for (const FInkJourneyDisplayRow& Row : DisplayRows)
    {
        const int32 Font = Row.bEmphasized ? 28 : 23;
        InkJourneyBatch2::AddScrollText(WidgetTree, RowsScroll, FText::FromString(InkJourneyBatch2::RowLine(Row)), Font,
            Row.bEmphasized ? EInkUiTextRole::Data : EInkUiTextRole::Body);
    }
}

void UInkResourceDetailBatch2Widget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    EnsureTree();
}

void UInkResourceDetailBatch2Widget::EnsureTree()
{
    if (!WidgetTree || WidgetTree->RootWidget) return;
    auto* Paper = InkJourneyBatch2::MakePaper(WidgetTree);
    auto* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Paper->SetContent(Root);
    WidgetTree->RootWidget = Paper;
    TitleText = InkJourneyBatch2::AddText(WidgetTree, Root, FText::GetEmpty(), 38, EInkUiTextRole::Display, FMargin(22, 18, 22, 8));
    StatusText = InkJourneyBatch2::AddText(WidgetTree, Root, FText::GetEmpty(), 23, EInkUiTextRole::Secondary, FMargin(22, 0, 22, 12));
    RowsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    RowsScroll->SetAllowOverscroll(false);
    RowsScroll->SetScrollbarThickness(FVector2D(5, 5));
    Root->AddChildToVerticalBox(RowsScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PrimaryButton = InkJourneyBatch2::AddButton(WidgetTree, Root, FText::FromString(TEXT("查看参与确认")));
    BackButton = InkJourneyBatch2::AddButton(WidgetTree, Root, FText::FromString(TEXT("返回资源地图")));
    PrimaryText = Cast<UTextBlock>(PrimaryButton ? PrimaryButton->GetContent() : nullptr);
    if (PrimaryButton) PrimaryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePrimaryAction);
    if (BackButton) BackButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackAction);
    RefreshText();
}

void UInkResourceDetailBatch2Widget::SetResourceDetail(const FString& EntryId, const FText& Name, const FText& Status, const TArray<FInkJourneyDisplayRow>& Rows)
{
    EnsureTree();
    const bool bEntryChanged = SelectedEntryId != EntryId;
    SelectedEntryId = EntryId;
    ResourceName = Name;
    ResourceStatus = Status;
    DisplayRows = Rows;
    if (bEntryChanged)
    {
        bConfirmationOpen = false;
        bPending = false;
        TravelFeedback = FText::GetEmpty();
    }
    RefreshText();
}

void UInkResourceDetailBatch2Widget::SetTravelFeedback(const FText& Feedback, bool bRequestPending)
{
    EnsureTree();
    TravelFeedback = Feedback;
    bPending = bRequestPending;
    bConfirmationOpen = false;
    RefreshText();
}

void UInkResourceDetailBatch2Widget::ResetInteractionForReopen()
{
    bConfirmationOpen = false;
    bPending = false;
    TravelFeedback = FText::GetEmpty();
    RefreshText();
}

void UInkResourceDetailBatch2Widget::RefreshText()
{
    if (!RowsScroll) return;
    if (TitleText) TitleText->SetText(ResourceName.IsEmpty() ? FText::FromString(TEXT("资源详情待同步")) : ResourceName);
    if (StatusText)
    {
        FText Combined = ResourceStatus;
        if (!TravelFeedback.IsEmpty()) Combined = FText::FromString(Combined.ToString() + TEXT("\n") + TravelFeedback.ToString());
        else if (bConfirmationOpen) Combined = FText::FromString(Combined.ToString() + TEXT("\n确认前将重新检查开放状态、身份、组织与连接。"));
        StatusText->SetText(Combined);
    }
    RowsScroll->ClearChildren();
    for (const FInkJourneyDisplayRow& Row : DisplayRows)
        InkJourneyBatch2::AddScrollText(WidgetTree, RowsScroll, FText::FromString(InkJourneyBatch2::RowLine(Row)), Row.bEmphasized ? 27 : 23,
            Row.bEmphasized ? EInkUiTextRole::Data : EInkUiTextRole::Body);
    if (PrimaryText) PrimaryText->SetText(FText::FromString(bPending ? TEXT("请求处理中") : bConfirmationOpen ? TEXT("确认前往竞采") : TEXT("查看参与确认")));
    if (PrimaryButton) PrimaryButton->SetIsEnabled(!bPending && !SelectedEntryId.IsEmpty());
}

void UInkResourceDetailBatch2Widget::HandlePrimaryAction()
{
    if (bPending || SelectedEntryId.IsEmpty()) return;
    if (!bConfirmationOpen)
    {
        bConfirmationOpen = true;
        TravelFeedback = FText::GetEmpty();
        RefreshText();
        return;
    }
    bPending = true;
    RefreshText();
    OnTravelRequested.Broadcast(SelectedEntryId);
}

void UInkResourceDetailBatch2Widget::HandleBackAction()
{
    OnBackRequested.Broadcast();
}

void UInkJourneyOutcomeBatch2Widget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    EnsureTree();
}

void UInkJourneyOutcomeBatch2Widget::EnsureTree()
{
    if (!WidgetTree || WidgetTree->RootWidget) return;
    RootContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    WidgetTree->RootWidget = RootContent;
    SettlementCard = WidgetTree->ConstructWidget<UInkJourneySettlementCardWidget>(UInkJourneySettlementCardWidget::StaticClass());
    RootContent->AddChildToVerticalBox(SettlementCard)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    BackButton = InkJourneyBatch2::AddButton(WidgetTree, RootContent, FText::FromString(TEXT("返回")));
    if (BackButton) BackButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleBackAction);
    if (!bHasAuthorityPayload) SettlementCard->SetCardContent(DefaultHeading(), FText::FromString(TEXT("权威结果待同步")), TArray<FInkJourneyDisplayRow>());
    else SettlementCard->SetCardContent(HeadingValue, StatusValue, DisplayRows);
}

void UInkJourneyOutcomeBatch2Widget::SetOutcome(const FText& Heading, const FText& Status, const TArray<FInkJourneyDisplayRow>& Rows)
{
    HeadingValue = Heading.IsEmpty() ? DefaultHeading() : Heading;
    StatusValue = Status;
    DisplayRows = Rows;
    bHasAuthorityPayload = true;
    EnsureTree();
    if (SettlementCard) SettlementCard->SetCardContent(HeadingValue, StatusValue, DisplayRows);
}

void UInkJourneyOutcomeBatch2Widget::HandleBackAction()
{
    OnBackRequested.Broadcast();
}


