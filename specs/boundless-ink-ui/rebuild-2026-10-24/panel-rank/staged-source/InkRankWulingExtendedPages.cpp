#include "InkRankWulingExtendedPages.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
    const FLinearColor ExtInk(0.106f,0.161f,0.145f,1.f);
    const FLinearColor ExtSlate(0.259f,0.373f,0.404f,1.f);
    const FLinearColor ExtGold(0.604f,0.451f,0.204f,1.f);
    const FLinearColor ExtPaper(0.957f,0.929f,0.867f,0.98f);
    const FLinearColor ExtPaperLight(0.984f,0.969f,0.933f,1.f);
    const FLinearColor ExtJade(0.196f,0.435f,0.388f,1.f);
    const FLinearColor ExtWarning(0.612f,0.263f,0.212f,1.f);
    const FLinearColor ExtMuted(0.322f,0.349f,0.310f,1.f);

    UTextBlock* ExtText(UWidgetTree* Tree, UPanelWidget* Parent, const FText& Value,
        int32 Size, FLinearColor Color = ExtInk, ETextJustify::Type Align = ETextJustify::Left)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(Value);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(NAME_None, Size, FName(TEXT("Regular"))));
        Text->SetAutoWrapText(true);
        Text->SetJustification(Align);
        Parent->AddChild(Text);
        return Text;
    }

    UBorder* ExtPanel(UWidgetTree* Tree, UPanelWidget* Parent, UWidget* Child,
        FLinearColor Fill = ExtPaper, float Padding = 24.f)
    {
        auto* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
        Border->SetBrushColor(Fill);
        Border->SetPadding(FMargin(Padding));
        if (Child) Border->SetContent(Child);
        Parent->AddChild(Border);
        return Border;
    }

    UButton* ExtButton(UWidgetTree* Tree, UPanelWidget* Parent, const FText& Label,
        bool bEnabled, TFunction<void()>&& Handler, FLinearColor Fill = ExtPaper,
        FLinearColor TextColor = ExtInk)
    {
        auto* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
        Button->SetBackgroundColor(Fill);
        Button->SetPadding(FMargin(20.f,28.f));
        Button->SetIsEnabled(bEnabled);
        auto* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Text->SetText(Label);
        Text->SetFont(FSlateFontInfo(NAME_None,30,FName(TEXT("Regular"))));
        Text->SetColorAndOpacity(FSlateColor(bEnabled ? TextColor : ExtMuted));
        Text->SetJustification(ETextJustify::Center);
        Button->AddChild(Text);
        Parent->AddChild(Button);
        Button->OnClicked.AddLambda([Handler = MoveTemp(Handler)]() mutable { if (Handler) Handler(); });
        return Button;
    }

    void ExtSpacer(UWidgetTree* Tree, UPanelWidget* Parent, float Height)
    {
        auto* Spacer = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Spacer->SetHeightOverride(Height);
        Parent->AddChild(Spacer);
    }

    FString ExtState(EInkRankPageState State)
    {
        switch (State)
        {
        case EInkRankPageState::Unknown: return TEXT("资料状态未知");
        case EInkRankPageState::Unavailable: return TEXT("当前服务不可用");
        case EInkRankPageState::Loading: return TEXT("正在读取服务资料");
        case EInkRankPageState::Empty: return TEXT("当前范围暂无记录");
        case EInkRankPageState::Pending: return TEXT("请求已提交，等待权威服务确认");
        case EInkRankPageState::Offline: return TEXT("当前离线");
        case EInkRankPageState::Error: return TEXT("读取失败");
        case EInkRankPageState::Disabled: return TEXT("当前操作不可用");
        case EInkRankPageState::Data: return FString();
        default: return TEXT("状态未提供");
        }
    }

    void ExtRoot(UUserWidget* Owner, UVerticalBox*& Content)
    {
        Content = Owner->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        Owner->WidgetTree->RootWidget = Content;
        Content->SetClipping(EWidgetClipping::ClipToBounds);
    }
}

void UInkGuildBuildPage::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPage(); RefreshPage();
}

void UInkGuildBuildPage::SetBuildPageData(const FInkGuildBuildPageData& InData)
{
    if (bConfirmBuild && (InData.SnapshotVersion != Data.SnapshotVersion || InData.bBuildPending))
    {
        bConfirmBuild = false;
        ConfirmBuildingId.Reset();
        ConfirmTargetLevel = 0;
    }
    Data = InData;
    bBuildSubmitIssued = InData.bBuildPending;
    if (Content) RefreshPage();
}

void UInkGuildBuildPage::BuildPage()
{
    if (!WidgetTree || Content) return;
    ExtRoot(this, Content);
    ExtText(WidgetTree, Content, FText::FromString(TEXT("武灵建设")), 48, ExtInk, ETextJustify::Center);
    ExtText(WidgetTree, Content, Data.ServiceStatus.IsEmpty() ? FText::FromString(TEXT("建筑资料由组织服务提供")) : Data.ServiceStatus,
        22, ExtSlate, ETextJustify::Center);
    auto* Filters = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Content->AddChild(Filters);
    ExtButton(WidgetTree, Filters, FText::FromString(TEXT("全部建筑")), Data.bCanQuery && !Data.bBuildPending,
        [this]() { OnQueryRequested.Broadcast(0, false); }, Data.bOwnedOnly ? ExtPaper : ExtJade,
        Data.bOwnedOnly ? ExtInk : ExtPaperLight);
    ExtButton(WidgetTree, Filters, FText::FromString(TEXT("已有建筑")), Data.bCanQuery && !Data.bBuildPending,
        [this]() { OnQueryRequested.Broadcast(0, true); }, Data.bOwnedOnly ? ExtJade : ExtPaper,
        Data.bOwnedOnly ? ExtPaperLight : ExtInk);

    auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Scroll->AddChild(List);
    if (Data.State == EInkRankPageState::Data || Data.State == EInkRankPageState::Offline)
    {
        for (const FInkGuildBuildingCardView& Building : Data.Buildings)
        {
            auto* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
            ExtPanel(WidgetTree, List, Card, ExtPaperLight, 24.f);
            ExtText(WidgetTree, Card, Building.DisplayName.IsEmpty() ? FText::FromString(TEXT("建筑名称未提供")) : Building.DisplayName, 32, ExtInk);
            const FText LevelText = Building.bHasCurrentLevel
                ? FText::FromString(FString::Printf(TEXT("当前等级 %d"), Building.CurrentLevel))
                : FText::FromString(TEXT("当前等级未知"));
            ExtText(WidgetTree, Card, LevelText, 24, ExtSlate);
            if (Building.bHasTargetLevel)
                ExtText(WidgetTree, Card, FText::FromString(FString::Printf(TEXT("可查看目标等级 %d"), Building.TargetLevel)), 24, ExtInk);
            else
                ExtText(WidgetTree, Card, FText::FromString(TEXT("目标等级未由目录提供")), 22, ExtMuted);
            ExtText(WidgetTree, Card, Building.AuthoritativeCosts.IsEmpty()
                ? FText::FromString(TEXT("费用资料未提供")) : FText::FromString(TEXT("服务费用：") + Building.AuthoritativeCosts.ToString()), 22, ExtSlate);
            if (!Building.StateText.IsEmpty()) ExtText(WidgetTree, Card, Building.StateText, 20, ExtMuted);
            if (Building.bCanBuild && Building.bHasTargetLevel && !Data.SnapshotVersion.IsEmpty() && Data.State == EInkRankPageState::Data && !bBuildSubmitIssued)
            {
                const FString Id = Building.BuildingId;
                const int32 TargetLevel = Building.TargetLevel;
                ExtButton(WidgetTree, Card, FText::FromString(TEXT("建设")), true,
                    [this,Id,TargetLevel]()
                    {
                        if (Data.State != EInkRankPageState::Data || bBuildSubmitIssued) return;
                        ConfirmBuildingId = Id;
                        ConfirmTargetLevel = TargetLevel;
                        bConfirmBuild = true;
                        RefreshPage();
                    }, ExtJade, ExtPaperLight);
            }
            else if (!Building.DisabledReason.IsEmpty())
                ExtText(WidgetTree, Card, Building.DisabledReason, 19, ExtMuted);
            else if (Building.bCanBuild && Data.SnapshotVersion.IsEmpty())
                ExtText(WidgetTree, Card, FText::FromString(TEXT("版本资料未知，暂不能提交")), 19, ExtMuted);
        }
        if (Data.Buildings.IsEmpty()) ExtText(WidgetTree, List, FText::FromString(TEXT("当前建筑目录为空")), 26, ExtMuted, ETextJustify::Center);
    }
    else
    {
        ExtPanel(WidgetTree, List, nullptr, Data.State == EInkRankPageState::Error ? FLinearColor(0.95f,0.87f,0.84f,1.f) : ExtPaper, 32.f);
        const FString State = ExtState(Data.State);
        ExtText(WidgetTree, List, FText::FromString(Data.ServiceStatus.IsEmpty() ? State : State + TEXT(" · ") + Data.ServiceStatus.ToString()),
            26, Data.State == EInkRankPageState::Error ? ExtWarning : ExtMuted, ETextJustify::Center);
        if ((Data.State == EInkRankPageState::Error || Data.State == EInkRankPageState::Unavailable || Data.State == EInkRankPageState::Offline) && Data.bCanQuery)
            ExtButton(WidgetTree, List, FText::FromString(TEXT("重新读取")), true,
                [this]() { OnRetryRequested.Broadcast(); }, ExtPaper);
    }
    auto* Pager = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Content->AddChild(Pager);
    ExtButton(WidgetTree, Pager, FText::FromString(TEXT("上一页")), Data.PreviousOffset != INDEX_NONE && !bBuildSubmitIssued,
        [this]() { OnQueryRequested.Broadcast(Data.PreviousOffset, Data.bOwnedOnly); }, ExtPaper);
    ExtText(WidgetTree, Pager, FText::FromString(FString::Printf(TEXT("目录位置 %d"), Data.Offset)), 20, ExtSlate, ETextJustify::Center);
    ExtButton(WidgetTree, Pager, FText::FromString(TEXT("下一页")), Data.NextOffset != INDEX_NONE && !bBuildSubmitIssued,
        [this]() { OnQueryRequested.Broadcast(Data.NextOffset, Data.bOwnedOnly); }, ExtPaper);
    if (bConfirmBuild)
    {
        const FInkGuildBuildingCardView* Selected = Data.Buildings.FindByPredicate([this](const FInkGuildBuildingCardView& B)
            { return B.BuildingId == ConfirmBuildingId && B.bHasTargetLevel && B.TargetLevel == ConfirmTargetLevel; });
        if (Selected)
        {
            auto* Confirm = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
            ExtPanel(WidgetTree, Content, Confirm, FLinearColor(0.91f,0.84f,0.80f,1.f), 24.f);
            ExtText(WidgetTree, Confirm, FText::FromString(FString::Printf(TEXT("确认建设“%s”至等级 %s？"),
                *Selected->DisplayName.ToString(), *FString::FromInt(ConfirmTargetLevel))), 28, ExtWarning, ETextJustify::Center);
            if (!Selected->AuthoritativeCosts.IsEmpty())
                ExtText(WidgetTree, Confirm, FText::FromString(TEXT("服务费用：") + Selected->AuthoritativeCosts.ToString()), 22, ExtInk, ETextJustify::Center);
            auto* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            Confirm->AddChild(Buttons);
            ExtButton(WidgetTree, Buttons, FText::FromString(TEXT("取消")), true, [this]() { CancelBuild(); }, ExtPaper);
            ExtButton(WidgetTree, Buttons, FText::FromString(TEXT("确认建设")), Selected->bCanBuild && !bBuildSubmitIssued,
                [this]() { ConfirmBuild(); }, ExtWarning, ExtPaperLight);
        }
    }
}

void UInkGuildBuildPage::RefreshPage()
{
    if (!WidgetTree || !Content) return;
    Content->ClearChildren(); Content = nullptr; BuildPage();
}

void UInkGuildBuildPage::ConfirmBuild()
{
    if (!bConfirmBuild || bBuildSubmitIssued || Data.State != EInkRankPageState::Data || ConfirmBuildingId.IsEmpty()) return;
    const FInkGuildBuildingCardView* Selected = Data.Buildings.FindByPredicate([this](const FInkGuildBuildingCardView& B)
        { return B.BuildingId == ConfirmBuildingId && B.bHasTargetLevel && B.TargetLevel == ConfirmTargetLevel; });
    if (!Selected || !Selected->bCanBuild || Data.SnapshotVersion.IsEmpty()) return;
    const FString Id = ConfirmBuildingId;
    const int32 TargetLevel = Selected->TargetLevel;
    const FString Version = Data.SnapshotVersion;
    bConfirmBuild = false; bBuildSubmitIssued = true;
    ConfirmBuildingId.Reset(); ConfirmTargetLevel = 0;
    OnBuildRequested.Broadcast(Id, TargetLevel, Version);
}

void UInkGuildBuildPage::CancelBuild()
{
    bConfirmBuild = false; ConfirmBuildingId.Reset(); ConfirmTargetLevel = 0; RefreshPage();
}

void UInkSwordSpiritPage::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPage(); RefreshPage();
}

void UInkSwordSpiritPage::SetSwordSpiritData(const FInkSwordSpiritPageData& InData)
{
    Data = InData; if (Content) RefreshPage();
}

void UInkSwordSpiritPage::BuildPage()
{
    if (!WidgetTree || Content) return;
    ExtRoot(this, Content);
    ExtText(WidgetTree, Content, FText::FromString(TEXT("我的剑灵")), 48, ExtInk, ETextJustify::Center);
    const FString State = ExtState(Data.State);
    ExtPanel(WidgetTree, Content, nullptr, Data.State == EInkRankPageState::Error ? FLinearColor(0.95f,0.87f,0.84f,1.f) : ExtPaper, 24.f);
    ExtText(WidgetTree, Content, FText::FromString(Data.ServiceStatus.IsEmpty()
        ? (State.IsEmpty() ? TEXT("个人剑灵资料未提供") : State)
        : (State.IsEmpty() ? Data.ServiceStatus.ToString() : State + TEXT(" · ") + Data.ServiceStatus.ToString())),
        24, Data.State == EInkRankPageState::Error ? ExtWarning : ExtMuted, ETextJustify::Center);
    if ((Data.State == EInkRankPageState::Data || Data.State == EInkRankPageState::Offline) && !Data.ParticipantId.IsEmpty())
    {
        if (Data.Portrait)
        {
            auto* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
            Portrait->SetBrushFromTexture(Data.Portrait, true);
            Portrait->SetDesiredSizeOverride(FVector2D(120.f,120.f));
            Content->AddChild(Portrait);
        }
        if (!Data.DisplayName.IsEmpty()) ExtText(WidgetTree, Content, Data.DisplayName, 34, ExtInk, ETextJustify::Center);
        if (!Data.Realm.IsEmpty()) ExtText(WidgetTree, Content, Data.Realm, 26, ExtJade, ETextJustify::Center);
        if (!Data.Honor.IsEmpty()) ExtText(WidgetTree, Content, Data.Honor, 22, ExtGold, ETextJustify::Center);
        for (const FInkSwordSpiritAttributeView& Attribute : Data.Attributes)
        {
            if (Attribute.Label.IsEmpty() && Attribute.Value.IsEmpty()) continue;
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
            ExtPanel(WidgetTree, Content, Row, ExtPaperLight, 16.f);
            ExtText(WidgetTree, Row, Attribute.Label.IsEmpty() ? FText::FromString(TEXT("属性名称未知")) : Attribute.Label, 24, ExtInk);
            ExtText(WidgetTree, Row, Attribute.Value.IsEmpty() ? FText::FromString(TEXT("未知")) : Attribute.Value, 24, ExtSlate, ETextJustify::Right);
        }
    }
    else if ((Data.State == EInkRankPageState::Data || Data.State == EInkRankPageState::Offline) && Data.ParticipantId.IsEmpty())
    {
        ExtText(WidgetTree, Content, FText::FromString(TEXT("准确参与者身份未提供，个人剑灵资料保持未知")), 24, ExtMuted, ETextJustify::Center);
    }
    ExtSpacer(WidgetTree, Content, 12.f);
    ExtButton(WidgetTree, Content, FText::FromString(TEXT("查看礼物效果说明")), true,
        [this]() { OnOpenGiftRulesRequested.Broadcast(); }, ExtJade, ExtPaperLight);
}

void UInkSwordSpiritPage::RefreshPage()
{
    if (!WidgetTree || !Content) return;
    Content->ClearChildren(); Content = nullptr; BuildPage();
}

void UInkGiftRulesPage::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPage(); RefreshPage();
}

void UInkGiftRulesPage::SetGiftEffectData(const FInkGiftEffectPageData& InData)
{
    Data = InData; if (Content) RefreshPage();
}

void UInkGiftRulesPage::BuildPage()
{
    if (!WidgetTree || Content) return;
    ExtRoot(this, Content);
    ExtText(WidgetTree, Content, FText::FromString(TEXT("礼物效果说明")), 48, ExtInk, ETextJustify::Center);
    const FString State = ExtState(Data.State);
    ExtText(WidgetTree, Content, FText::FromString(Data.ServiceStatus.IsEmpty()
        ? (State.IsEmpty() ? TEXT("已同步效果说明") : State)
        : (State.IsEmpty() ? Data.ServiceStatus.ToString() : State + TEXT(" · ") + Data.ServiceStatus.ToString())),
        22, Data.State == EInkRankPageState::Error ? ExtWarning : ExtMuted, ETextJustify::Center);
    if (Data.State == EInkRankPageState::Data || Data.State == EInkRankPageState::Offline)
    {
        if (Data.bRulesProvided && Data.Rules.IsEmpty())
            ExtText(WidgetTree, Content, FText::FromString(TEXT("服务已确认当前说明目录为空")), 24, ExtMuted, ETextJustify::Center);
        else if (!Data.bRulesProvided)
            ExtText(WidgetTree, Content, FText::FromString(TEXT("详细礼物效果目录尚未同步")), 24, ExtMuted, ETextJustify::Center);
        for (const FInkGiftRuleView& Rule : Data.Rules)
        {
            auto* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
            ExtPanel(WidgetTree, Content, Card, ExtPaperLight, 24.f);
            ExtText(WidgetTree, Card, Rule.Name.IsEmpty() ? FText::FromString(TEXT("规则名称未知")) : Rule.Name, 28, ExtInk);
            if (!Rule.ActualDescription.IsEmpty()) ExtText(WidgetTree, Card, Rule.ActualDescription, 24, ExtSlate);
        }
        if (!Data.ActiveEffects.IsEmpty())
        {
            ExtSpacer(WidgetTree, Content, 16.f);
            ExtText(WidgetTree, Content, FText::FromString(TEXT("当前已生效效果 · 服务记录")), 28, ExtJade);
            for (const FInkGiftRuleView& Effect : Data.ActiveEffects)
            {
                auto* Card = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
                ExtPanel(WidgetTree, Content, Card, ExtPaper, 18.f);
                ExtText(WidgetTree, Card, Effect.Name.IsEmpty() ? FText::FromString(TEXT("效果名称未知")) : Effect.Name, 24, ExtInk);
                if (!Effect.ActualDescription.IsEmpty()) ExtText(WidgetTree, Card, Effect.ActualDescription, 21, ExtSlate);
            }
        }
    }
    else if (Data.State != EInkRankPageState::Data)
    {
        ExtPanel(WidgetTree, Content, nullptr, Data.State == EInkRankPageState::Error ? FLinearColor(0.95f,0.87f,0.84f,1.f) : ExtPaper, 28.f);
        const FText HonestState = Data.bRulesProvided && Data.State == EInkRankPageState::Empty
            ? FText::FromString(TEXT("服务已确认规则目录为空"))
            : FText::FromString(TEXT("详细礼物效果目录尚未同步"));
        ExtText(WidgetTree, Content, HonestState, 24, Data.State == EInkRankPageState::Error ? ExtWarning : ExtMuted, ETextJustify::Center);
    }
}

void UInkGiftRulesPage::RefreshPage()
{
    if (!WidgetTree || !Content) return;
    Content->ClearChildren(); Content = nullptr; BuildPage();
}
