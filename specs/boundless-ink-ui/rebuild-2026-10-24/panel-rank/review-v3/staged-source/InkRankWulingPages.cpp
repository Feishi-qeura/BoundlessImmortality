#include "InkRankWulingPages.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

namespace
{
    const FLinearColor Ink(0.106f, 0.161f, 0.145f, 1.f);       // #1B2925
    const FLinearColor Slate(0.259f, 0.373f, 0.404f, 1.f);     // #425F67
    const FLinearColor OldGold(0.604f, 0.451f, 0.204f, 1.f);   // #9A7334
    const FLinearColor Paper(0.957f, 0.929f, 0.867f, 0.98f);   // #F4ECDD
    const FLinearColor PaperLight(0.984f, 0.969f, 0.933f, 1.f);// #FBF7EE
    const FLinearColor Jade(0.196f, 0.435f, 0.388f, 1.f);      // #326F63
    const FLinearColor Warning(0.612f, 0.263f, 0.212f, 1.f);   // #9C4336
    const FLinearColor Muted(0.322f, 0.349f, 0.310f, 1.f);     // #52594F

    UTextBlock* AddText(UInkRankWulingPageBase* Owner, UWidgetTree* Tree, UPanelWidget* Parent, const FString& Value,
        int32 Size, FLinearColor Color = Ink, ETextJustify::Type Align = ETextJustify::Left)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(FText::FromString(Value));
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(Owner->MakeThemeFont(Size, Size >= 44));
        Text->SetAutoWrapText(true);
        Text->SetJustification(Align);
        Parent->AddChild(Text);
        return Text;
    }

    UBorder* AddPanel(UWidgetTree* Tree, UPanelWidget* Parent, UWidget* Child,
        FLinearColor Fill = Paper, float Padding = 16.f)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Border->SetBrushColor(Fill);
        Border->SetPadding(FMargin(Padding));
        if (Child) Border->SetContent(Child);
        Parent->AddChild(Border);
        return Border;
    }

    UButton* AddButton(UInkRankWulingPageBase* Owner, UWidgetTree* Tree, UPanelWidget* Parent, const FText& Label,
        bool bEnabled, TFunction<void()>&& OnPressed,
        FLinearColor Fill = FLinearColor(0.91f,0.87f,0.78f,1.f), FLinearColor TextColor = Ink)
    {
        auto* Button = Tree->ConstructWidget<UInkRankWulingCallbackButton>(UInkRankWulingCallbackButton::StaticClass());
        Button->SetBackgroundColor(Fill);
        Button->SetIsEnabled(bEnabled);
        auto* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(Label);
        Text->SetFont(Owner->MakeThemeFont(28));
        Text->SetColorAndOpacity(FSlateColor(bEnabled ? TextColor : Muted));
        Text->SetJustification(ETextJustify::Center);
        if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
            ButtonSlot->SetPadding(FMargin(4.f));
        Button->SetAction(MoveTemp(OnPressed));
        auto* HitTarget = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        HitTarget->SetMinDesiredWidth(88.f);
        HitTarget->SetMinDesiredHeight(88.f);
        HitTarget->SetContent(Button);
        Parent->AddChild(HitTarget);
        return Button;
    }

    void AddSpacer(UWidgetTree* Tree, UPanelWidget* Parent, float Height)
    {
        auto* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Box->SetHeightOverride(Height);
        Parent->AddChild(Box);
    }

    FString StateLabel(EInkRankPageState State)
    {
        switch (State)
        {
        case EInkRankPageState::Unknown: return TEXT("榜单状态未知");
        case EInkRankPageState::Unavailable: return TEXT("榜单服务不可用");
        case EInkRankPageState::Loading: return TEXT("正在读取当前榜单");
        case EInkRankPageState::Empty: return TEXT("当前范围暂无榜单数据");
        case EInkRankPageState::Pending: return TEXT("请求已提交，等待榜单服务确认");
        case EInkRankPageState::Offline: return TEXT("当前离线，显示内容由服务状态决定");
        case EInkRankPageState::Error: return TEXT("榜单暂时无法载入");
        case EInkRankPageState::Disabled: return TEXT("当前操作不可用");
        case EInkRankPageState::Data: return FString();
        default: return TEXT("榜单状态未知");
        }
    }

    void SetWeightedFill(UHorizontalBoxSlot* Slot, float Weight)
    {
        if (!Slot) return;
        FSlateChildSize Size(ESlateSizeRule::Fill);
        Size.Value = Weight;
        Slot->SetSize(Size);
    }

    void SetVerticalFill(UVerticalBoxSlot* Slot)
    {
        if (!Slot) return;
        FSlateChildSize Size(ESlateSizeRule::Fill);
        Slot->SetSize(Size);
    }

    UTextBlock* ButtonLabel(UButton* Button)
    {
        USizeBox* HitTarget = Button ? Cast<USizeBox>(Button->GetParent()) : nullptr;
        UButton* InnerButton = HitTarget ? Cast<UButton>(HitTarget->GetContent()) : nullptr;
        return InnerButton ? Cast<UTextBlock>(InnerButton->GetContent()) : nullptr;
    }

    void AddRowCell(UInkRankWulingPageBase* Owner, UWidgetTree* Tree, UHorizontalBox* Row, const FString& Text,
        float Fill, FLinearColor Color, int32 FontSize, ETextJustify::Type Align = ETextJustify::Left)
    {
        auto* Cell = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Cell->SetText(FText::FromString(Text));
        Cell->SetColorAndOpacity(FSlateColor(Color));
        Cell->SetFont(Owner->MakeThemeFont(FontSize));
        Cell->SetJustification(Align);
        Cell->SetAutoWrapText(true);
        SetWeightedFill(Row->AddChildToHorizontalBox(Cell), Fill);
    }
}

void UInkRankBoardPage::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPage();
}

void UInkRankBoardPage::SetBoardData(const FInkNewRankBoardData& InData)
{
    Data = InData;
    if (Content) BuildPage();
}

void UInkRankBoardPage::SetParticipantAvatar(const FString& ParticipantId, UTexture2D* Avatar)
{
    if (ParticipantId.IsEmpty()) return;
    AvatarRequestsInFlight.Remove(ParticipantId);
    if (!Avatar) return;
    ParticipantAvatarCache.Add(ParticipantId, Avatar);
    if (TObjectPtr<UImage>* Image = AvatarImagesByParticipant.Find(ParticipantId))
        (*Image)->SetBrushFromTexture(Avatar, true);
}

void UInkRankBoardPage::RetryParticipantAvatarAfterHostBackoff(const FString& ParticipantId)
{
    if (ParticipantId.IsEmpty() || !OnAvatarRequested.IsBound() ||
        AvatarRequestsInFlight.Contains(ParticipantId) || ParticipantAvatarCache.Contains(ParticipantId) ||
        Data.State != EInkRankPageState::Data)
        return;

    const bool bParticipantVisible = Data.Rows.ContainsByPredicate([&ParticipantId](const FInkNewRankRow& Row)
    {
        return Row.ParticipantId == ParticipantId && !Row.Avatar;
    });
    if (!bParticipantVisible) return;

    AvatarRequestsInFlight.Add(ParticipantId);
    OnAvatarRequested.Broadcast(ParticipantId);
}

void UInkRankBoardPage::BuildPage()
{
    if (!WidgetTree || !EnsureStableRoot()) return;
    Content->ClearChildren();
    AvatarImagesByParticipant.Reset();
    auto* Header = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    TitleText->SetText(FText::FromString(TEXT("天下榜")));
    TitleText->SetFont(MakeThemeFont(48, true));
    TitleText->SetColorAndOpacity(FSlateColor(Ink));
    TitleText->SetJustification(ETextJustify::Center);
    Header->AddChildToOverlay(TitleText)->SetHorizontalAlignment(HAlign_Fill);
    auto* HeaderSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    HeaderSize->SetHeightOverride(176.f);
    HeaderSize->SetContent(Header);
    Content->AddChild(HeaderSize);
    AddText(this, WidgetTree, Content, TEXT("以当前服务返回的名次与指标为准"), 20, Slate, ETextJustify::Center);
    AddSpacer(WidgetTree, Content, 16.f);

    auto* Categories = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
    Categories->SetInnerSlotPadding(FVector2D(8.f, 8.f));
    Content->AddChild(Categories);
    const TArray<TPair<FName,FText>> Tabs = {{TEXT("Damage"),FText::FromString(TEXT("伤害榜"))},
        {TEXT("CultivationSpeed"),FText::FromString(TEXT("修炼速度"))}, {TEXT("Anchor"),FText::FromString(TEXT("主播剑灵"))}};
    CategoryTabs.Reset();
    for (const auto& Tab : Tabs)
    {
        const FName Category = Tab.Key;
        CategoryTabs.Add(AddButton(this, WidgetTree, Categories, Tab.Value, true, [this,Category]() { OnCategoryRequested.Broadcast(Category); },
            Data.Category == Category ? Jade : Paper, Data.Category == Category ? PaperLight : Ink));
    }
    auto* Scope = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
    Scope->SetInnerSlotPadding(FVector2D(8.f, 8.f));
    Content->AddChild(Scope);
    ScopeTabs.Reset();
    ScopeTabs.Add(AddButton(this, WidgetTree, Scope, FText::FromString(TEXT("本地")), true,
        [this]() { OnScopeRequested.Broadcast(TEXT("Local")); }, Data.Scope == TEXT("Local") ? Jade : Paper, Data.Scope == TEXT("Local") ? PaperLight : Ink));
    ScopeTabs.Add(AddButton(this, WidgetTree, Scope, FText::FromString(TEXT("全服")), true,
        [this]() { OnScopeRequested.Broadcast(TEXT("Global")); }, Data.Scope == TEXT("Global") ? Jade : Paper, Data.Scope == TEXT("Global") ? PaperLight : Ink));

    AddSpacer(WidgetTree, Content, 18.f);
    MetricHeaderText = AddText(this, WidgetTree, Content,
        FString::Printf(TEXT("名次与身份由榜单服务提供 · %s"), *Data.MetricLabel.ToString()), 22, Slate);
    auto* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    AddPanel(WidgetTree, Content, Head, FLinearColor(0.86f,0.82f,0.74f,1.f), 12.f);
    AddRowCell(this, WidgetTree, Head, TEXT("名次"), .18f, Ink, 22, ETextJustify::Center);
    AddRowCell(this, WidgetTree, Head, TEXT("身份"), .48f, Ink, 22);
    MetricColumnText = AddText(this, WidgetTree, Head,
        Data.MetricLabel.IsEmpty() ? TEXT("榜单指标") : Data.MetricLabel.ToString(), 22, Ink, ETextJustify::Right);

    StatusText = AddText(this, WidgetTree, Content, FString(), 24, Muted, ETextJustify::Center);
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    SetVerticalFill(Content->AddChildToVerticalBox(Scroll));
    RowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Scroll->AddChild(RowsBox);
    AddSpacer(WidgetTree, Content, 12.f);
    auto* SelfPanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    AddPanel(WidgetTree, Content, SelfPanel, FLinearColor(0.84f,0.88f,0.87f,1.f), 18.f);
    SelfText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    SelfText->SetColorAndOpacity(FSlateColor(Ink));
    SelfText->SetFont(MakeThemeFont(25));
    SetWeightedFill(SelfPanel->AddChildToHorizontalBox(SelfText), .58f);
    SelfMetricText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    SelfMetricText->SetColorAndOpacity(FSlateColor(Slate));
    SelfMetricText->SetFont(MakeThemeFont(23));
    SelfMetricText->SetJustification(ETextJustify::Right);
    SetWeightedFill(SelfPanel->AddChildToHorizontalBox(SelfMetricText), .42f);
    RefreshPage();
}

void UInkRankBoardPage::RefreshPage()
{
    if (!WidgetTree || !RowsBox) return;
    const FString State = StateLabel(Data.State);
    StatusText->SetText(FText::FromString(State.IsEmpty() ? Data.ProviderStatus.ToString() :
        (Data.ProviderStatus.IsEmpty() ? State : State + TEXT(" · ") + Data.ProviderStatus.ToString())));
    TitleText->SetText(Data.Title.IsEmpty() ? FText::FromString(TEXT("天下榜")) : Data.Title);
    StatusText->SetColorAndOpacity(FSlateColor(Data.State == EInkRankPageState::Error ? Warning : Muted));
    MetricHeaderText->SetText(FText::FromString(FString::Printf(TEXT("名次与身份由榜单服务提供 · %s"),
        Data.MetricLabel.IsEmpty() ? TEXT("榜单指标") : *Data.MetricLabel.ToString())));
    MetricColumnText->SetText(Data.MetricLabel.IsEmpty() ? FText::FromString(TEXT("榜单指标")) : Data.MetricLabel);
    const TArray<FName> CategoryNames = {TEXT("Damage"),TEXT("CultivationSpeed"),TEXT("Anchor")};
    for (int32 I = 0; I < CategoryTabs.Num(); ++I)
    {
        const bool bSelected = Data.Category == CategoryNames[I];
        CategoryTabs[I]->SetBackgroundColor(bSelected ? Jade : Paper);
        if (auto* Label = ButtonLabel(CategoryTabs[I]))
            Label->SetColorAndOpacity(FSlateColor(bSelected ? PaperLight : Ink));
    }
    for (int32 I = 0; I < ScopeTabs.Num(); ++I)
    {
        const bool bSelected = Data.Scope == (I == 0 ? FName(TEXT("Local")) : FName(TEXT("Global")));
        ScopeTabs[I]->SetBackgroundColor(bSelected ? Jade : Paper);
        if (auto* Label = ButtonLabel(ScopeTabs[I]))
            Label->SetColorAndOpacity(FSlateColor(bSelected ? PaperLight : Ink));
    }
    RowsBox->ClearChildren();
    if (Data.State == EInkRankPageState::Data || Data.State == EInkRankPageState::Offline)
    {
        for (const FInkNewRankRow& Entry : Data.Rows)
        {
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            auto* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            RowSize->SetMinDesiredHeight(120.f);
            RowSize->SetContent(Row);
            AddPanel(WidgetTree, RowsBox, RowSize, Entry.Rank == 1 ? FLinearColor(0.96f,0.91f,0.79f,1.f) : PaperLight, 12.f);
            AddRowCell(this, WidgetTree, Row, Entry.Rank > 0 ? FString::FromInt(Entry.Rank) : TEXT("—"), .18f,
                Entry.Rank == 1 ? OldGold : Slate, 28, ETextJustify::Center);
            auto* Identity = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            SetWeightedFill(Row->AddChildToHorizontalBox(Identity), .48f);
            auto* AvatarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            AvatarBox->SetWidthOverride(64.f); AvatarBox->SetHeightOverride(64.f);
            auto* Avatar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            UTexture2D* ResolvedAvatar = Entry.Avatar;
            if (!ResolvedAvatar)
                if (const TObjectPtr<UTexture2D>* Cached = ParticipantAvatarCache.Find(Entry.ParticipantId)) ResolvedAvatar = Cached->Get();
            if (ResolvedAvatar) Avatar->SetBrushFromTexture(ResolvedAvatar, true);
            else
            {
                Avatar->SetColorAndOpacity(FLinearColor(0.78f,0.75f,0.67f,1.f));
                Avatar->SetToolTipText(FText::FromString(TEXT("榜单未提供头像")));
            }
            if (!Entry.ParticipantId.IsEmpty())
            {
                AvatarImagesByParticipant.Add(Entry.ParticipantId, Avatar);
                if (!ResolvedAvatar && OnAvatarRequested.IsBound() && !AvatarRequestsInFlight.Contains(Entry.ParticipantId))
                {
                    AvatarRequestsInFlight.Add(Entry.ParticipantId);
                    OnAvatarRequested.Broadcast(Entry.ParticipantId);
                }
            }
            AvatarBox->AddChild(Avatar);
            Identity->AddChildToHorizontalBox(AvatarBox);
            auto* IdentityText = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
            SetWeightedFill(Identity->AddChildToHorizontalBox(IdentityText), 1.f);
            AddText(this, WidgetTree, IdentityText, Entry.DisplayName.IsEmpty() ? TEXT("身份资料缺失") : Entry.DisplayName.ToString(), 25, Ink);
            if (!Entry.EarnedTitle.IsEmpty()) AddText(this, WidgetTree, IdentityText, Entry.EarnedTitle.ToString(), 18, Slate);
            if (!Entry.Realm.IsEmpty()) AddText(this, WidgetTree, IdentityText, Entry.Realm.ToString(), 18, Muted);
            AddRowCell(this, WidgetTree, Row, Entry.Metric.IsEmpty() ? TEXT("—") : Entry.Metric.ToString(), .34f, Ink, 24, ETextJustify::Right);
        }
        if (Data.Rows.IsEmpty()) StatusText->SetText(FText::FromString(TEXT("服务返回空榜单")));
    }
    if (Data.State == EInkRankPageState::Error || Data.State == EInkRankPageState::Unavailable)
    {
        AddButton(this, WidgetTree, RowsBox, FText::FromString(TEXT("重新读取")), true,
            [this]() { OnRetryRequested.Broadcast(); }, Paper);
    }
    SelfText->SetText(FText::FromString(Data.SelfPosition.IsEmpty() ? TEXT("我的排名：当前响应未提供") :
        TEXT("我的排名：") + Data.SelfPosition.ToString()));
    SelfMetricText->SetText(Data.SelfMetric.IsEmpty() ? FText::FromString(TEXT("暂无本人指标")) : Data.SelfMetric);
}

void UInkRankBoardPage::RebuildForThemeChange()
{
    if (WidgetTree) BuildPage();
}

void UInkWulingOverviewPage::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPage();
}

void UInkWulingOverviewPage::SetOverviewData(const FInkWulingOverviewData& InData)
{
    Data = InData; if (Content) BuildPage();
}

void UInkWulingOverviewPage::BuildPage()
{
    if (!WidgetTree || !EnsureStableRoot()) return;
    Content->ClearChildren();
    AddText(this, WidgetTree, Content, TEXT("武灵"), 48, Ink, ETextJustify::Center);
    AddText(this, WidgetTree, Content, TEXT("组织总览"), 28, Slate, ETextJustify::Center);
    AddSpacer(WidgetTree, Content, 8.f);
    auto* Identity = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    AddPanel(WidgetTree, Content, Identity, Paper, 20.f);
    auto* Emblem = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    Emblem->SetDesiredSizeOverride(FVector2D(160.f, 160.f));
    if (Data.Insignia) Emblem->SetBrushFromTexture(Data.Insignia, true);
    Identity->AddChild(Emblem);
    AddText(this, WidgetTree, Identity, Data.SectName.IsEmpty() ? TEXT("组织名称待同步") : Data.SectName.ToString(), 34, Ink, ETextJustify::Center);
    AddText(this, WidgetTree, Identity, Data.ServiceStatus.IsEmpty() ? TEXT("组织状态未知") : Data.ServiceStatus.ToString(), 22, Jade, ETextJustify::Center);

    auto* Counts = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
    Counts->SetInnerSlotPadding(FVector2D(8.f, 8.f));
    AddPanel(WidgetTree, Content, Counts, FLinearColor(0.90f,0.87f,0.81f,1.f), 14.f);
    const TArray<TPair<FText,FText>> CountFields = {
        {FText::FromString(TEXT("成员")),Data.MemberCount},
        {FText::FromString(TEXT("参与")),Data.ParticipatingCount},
        {FText::FromString(TEXT("助战")),Data.HelperCount}};
    for (const auto& Field : CountFields)
    {
        auto* Cell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        auto* CellSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        CellSize->SetWidthOverride(200.f);
        CellSize->SetMinDesiredHeight(88.f);
        CellSize->SetContent(Cell);
        Counts->AddChildToWrapBox(CellSize);
        AddText(this, WidgetTree, Cell, Field.Key.ToString(), 21, Slate, ETextJustify::Center);
        AddText(this, WidgetTree, Cell, Field.Value.IsEmpty() ? TEXT("—") : Field.Value.ToString(), 30, Ink, ETextJustify::Center);
    }
    auto* Summary = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    AddPanel(WidgetTree, Content, Summary, PaperLight, 18.f);
    AddRowCell(this, WidgetTree, Summary, TEXT("公共資材  ") + (Data.PublicSupply.IsEmpty() ? TEXT("—") : Data.PublicSupply.ToString()), .5f, Ink, 24);
    AddRowCell(this, WidgetTree, Summary, TEXT("武灵声望  ") + (Data.Reputation.IsEmpty() ? TEXT("—") : Data.Reputation.ToString()), .5f, Ink, 24);
    AddSpacer(WidgetTree, Content, 12.f);
    AddText(this, WidgetTree, Content, TEXT("现有组织操作"), 28, Ink);
    for (const FInkWulingActionView& Action : Data.ExistingActions)
    {
        const FName ActionId = Action.ActionId;
        AddButton(this, WidgetTree, Content, Action.Label, Action.bEnabled,
            [this,ActionId]() { OnActionRequested.Broadcast(ActionId); }, Action.bEnabled ? Paper : FLinearColor(0.84f,0.84f,0.80f,1.f));
        if (!Action.bEnabled && !Action.DisabledReason.IsEmpty())
            AddText(this, WidgetTree, Content, Action.DisabledReason.ToString(), 18, Muted);
    }
}

void UInkWulingOverviewPage::RefreshPage()
{
    BuildPage();
}

void UInkWulingOverviewPage::RebuildForThemeChange()
{
    if (WidgetTree) BuildPage();
}

void UInkWulingMembersPage::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPage();
}

void UInkWulingMembersPage::SetMembersData(const FInkWulingMembersData& InData)
{
    bRemoveSubmitIssued = InData.bRemovePending;
    if (bConfirmRemove && (PendingRemoveUserId != InData.SelectedUserId || InData.State == EInkRankPageState::Pending))
    {
        bConfirmRemove = false;
        PendingRemoveUserId.Reset();
    }
    Data = InData;
    if (Content) BuildPage();
}

void UInkWulingMembersPage::BuildPage()
{
    if (!WidgetTree || !EnsureStableRoot()) return;
    Content->ClearChildren();
    AddText(this, WidgetTree, Content, TEXT("武灵成员"), 44, Ink, ETextJustify::Center);
    AddText(this, WidgetTree, Content, Data.ServiceStatus.IsEmpty() ? TEXT("成员列表由组织服务分页提供") : Data.ServiceStatus.ToString(), 20, Slate, ETextJustify::Center);
    auto* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    AddPanel(WidgetTree, Content, Head, FLinearColor(0.86f,0.82f,0.74f,1.f), 12.f);
    AddRowCell(this, WidgetTree, Head, TEXT("选择"), .16f, Ink, 20, ETextJustify::Center);
    AddRowCell(this, WidgetTree, Head, TEXT("成员身份"), .84f, Ink, 20);
    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    SetVerticalFill(Content->AddChildToVerticalBox(Scroll));
    auto* RowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Scroll->AddChild(RowsBox);
    const int32 VisibleRowCount = Data.PageSize > 0 ? FMath::Min(Data.Rows.Num(), Data.PageSize) : 0;
    for (int32 RowIndex = 0; RowIndex < VisibleRowCount &&
        (Data.State == EInkRankPageState::Data || Data.State == EInkRankPageState::Offline); ++RowIndex)
    {
        const FInkWulingMemberRow& Member = Data.Rows[RowIndex];
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        auto* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        RowSize->SetMinDesiredHeight(112.f);
        RowSize->SetContent(Row);
        const bool bSelected = Member.UserId == Data.SelectedUserId;
        AddPanel(WidgetTree, RowsBox, RowSize, bSelected ? Jade : PaperLight, 12.f);
        const FString UserId = Member.UserId;
        AddButton(this, WidgetTree, Row, Member.UserId == Data.SelectedUserId ? FText::FromString(TEXT("已选")) : FText::FromString(TEXT("选择")),
            Data.State == EInkRankPageState::Data && !bRemoveSubmitIssued,
            [this,UserId]() { OnMemberSelected.Broadcast(UserId); }, bSelected ? OldGold : Paper);
        auto* Identity = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        SetWeightedFill(Row->AddChildToHorizontalBox(Identity), .84f);
        AddText(this, WidgetTree, Identity, Member.DisplayName.IsEmpty() ? TEXT("身份资料缺失") : Member.DisplayName.ToString(), 24, bSelected ? PaperLight : Ink);
    }
    if ((Data.State != EInkRankPageState::Data && Data.State != EInkRankPageState::Offline) || Data.Rows.IsEmpty())
    {
        const FString State = Data.State == EInkRankPageState::Data ? TEXT("当前成员页暂无记录") : StateLabel(Data.State);
        AddText(this, WidgetTree, RowsBox, State.IsEmpty() ? TEXT("成员列表状态未知") : State, 24,
            Data.State == EInkRankPageState::Error ? Warning : Muted, ETextJustify::Center);
        if (Data.State == EInkRankPageState::Error || Data.State == EInkRankPageState::Unavailable)
            AddButton(this, WidgetTree, RowsBox, FText::FromString(TEXT("重新读取")), true,
                [this]() { OnRetryRequested.Broadcast(); }, Paper);
    }
    auto* Pager = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Content->AddChild(Pager);
    AddButton(this, WidgetTree, Pager, FText::FromString(TEXT("上一页")), Data.PreviousCursor != INDEX_NONE && Data.State == EInkRankPageState::Data && !bRemoveSubmitIssued,
        [this]() { OnPageRequested.Broadcast(Data.PreviousCursor); }, Paper);
    AddText(this, WidgetTree, Pager, FString::Printf(TEXT("起始位置 %d"), Data.Cursor + 1), 20, Slate, ETextJustify::Center);
    AddButton(this, WidgetTree, Pager, FText::FromString(TEXT("下一页")), Data.NextCursor != INDEX_NONE && Data.State == EInkRankPageState::Data && !bRemoveSubmitIssued,
        [this]() { OnPageRequested.Broadcast(Data.NextCursor); }, Paper);
    AddButton(this, WidgetTree, Content, FText::FromString(TEXT("移除已选成员")),
        Data.bCanRemoveSelected && !Data.SelectedUserId.IsEmpty() && Data.State == EInkRankPageState::Data && !bRemoveSubmitIssued,
        [this]()
        {
            if (!Data.bCanRemoveSelected || Data.SelectedUserId.IsEmpty() || Data.State != EInkRankPageState::Data || bRemoveSubmitIssued) return;
            PendingRemoveUserId = Data.SelectedUserId;
            bConfirmRemove = true;
            RefreshPage();
        },
        Data.bCanRemoveSelected ? FLinearColor(0.88f,0.75f,0.70f,1.f) : FLinearColor(0.84f,0.84f,0.80f,1.f));
    if (!Data.bCanRemoveSelected && !Data.RemoveDisabledReason.IsEmpty())
        AddText(this, WidgetTree, Content, Data.RemoveDisabledReason.ToString(), 18, Muted, ETextJustify::Center);
    if (bConfirmRemove)
    {
        FString ConfirmName;
        for (const FInkWulingMemberRow& Member : Data.Rows)
            if (Member.UserId == PendingRemoveUserId) { ConfirmName = Member.DisplayName.ToString(); break; }
        auto* Confirm = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        AddPanel(WidgetTree, Content, Confirm, FLinearColor(0.91f,0.84f,0.80f,1.f), 24.f);
        AddText(this, WidgetTree, Confirm, ConfirmName.IsEmpty()
            ? TEXT("确认移除所选成员？")
            : FString::Printf(TEXT("确认移除成员“%s”？"), *ConfirmName), 28, Warning, ETextJustify::Center);
        auto* ConfirmActions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        Confirm->AddChild(ConfirmActions);
        AddButton(this, WidgetTree, ConfirmActions, FText::FromString(TEXT("取消")), true,
            [this]() { CancelRemove(); }, Paper);
        AddButton(this, WidgetTree, ConfirmActions, FText::FromString(TEXT("确认移除")),
            Data.bCanRemoveSelected && Data.SelectedUserId == PendingRemoveUserId && Data.State == EInkRankPageState::Data && !bRemoveSubmitIssued,
            [this]() { ConfirmRemove(); }, Warning);
    }
}

void UInkWulingMembersPage::RefreshPage()
{
    BuildPage();
}

void UInkWulingMembersPage::RebuildForThemeChange()
{
    if (WidgetTree) BuildPage();
}

void UInkWulingMembersPage::ConfirmRemove()
{
    if (!bConfirmRemove || PendingRemoveUserId.IsEmpty() ||
        PendingRemoveUserId != Data.SelectedUserId || !Data.bCanRemoveSelected ||
        Data.State != EInkRankPageState::Data || bRemoveSubmitIssued)
        return;
    const FString UserId = PendingRemoveUserId;
    bConfirmRemove = false;
    PendingRemoveUserId.Reset();
    bRemoveSubmitIssued = true;
    OnRemoveRequested.Broadcast(UserId);
}

void UInkWulingMembersPage::CancelRemove()
{
    bConfirmRemove = false;
    PendingRemoveUserId.Reset();
    RefreshPage();
}
