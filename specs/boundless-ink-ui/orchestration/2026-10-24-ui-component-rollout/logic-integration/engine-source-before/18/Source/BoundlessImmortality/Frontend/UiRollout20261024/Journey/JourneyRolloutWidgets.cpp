#include "JourneyRolloutWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Frontend/InkFeaturePanel.h"
#include "Frontend/InkPresentationStyle.h"
#include "Frontend/UI/InkUiTheme.h"

namespace
{
FString JourneyGlyphTexturePath(EJourneyGlyphFamily Family, FName State)
{
    const TCHAR* TextureName = nullptr;
    switch (Family)
    {
    case EJourneyGlyphFamily::WorldNode:
        if (State == TEXT("explored")) TextureName = TEXT("T_Journey_WorldNodeExplored");
        else if (State == TEXT("available")) TextureName = TEXT("T_Journey_WorldNodeAvailable");
        else if (State == TEXT("locked")) TextureName = TEXT("T_Journey_WorldNodeLocked");
        break;
    case EJourneyGlyphFamily::ResourcePin:
        if (State == TEXT("free")) TextureName = TEXT("T_Journey_ResourcePinFree");
        else if (State == TEXT("temporary")) TextureName = TEXT("T_Journey_ResourcePinTemporary");
        else if (State == TEXT("contest-open")) TextureName = TEXT("T_Journey_ResourcePinContestOpen");
        else if (State == TEXT("expired")) TextureName = TEXT("T_Journey_ResourcePinExpired");
        break;
    case EJourneyGlyphFamily::FortuneSeal:
        if (State == TEXT("great")) TextureName = TEXT("T_Journey_FortuneGreat");
        else if (State == TEXT("good")) TextureName = TEXT("T_Journey_FortuneGood");
        else if (State == TEXT("neutral")) TextureName = TEXT("T_Journey_FortuneNeutral");
        else if (State == TEXT("bad")) TextureName = TEXT("T_Journey_FortuneBad");
        break;
    }
    return TextureName
        ? FString::Printf(TEXT("/Game/UI/Ink/ComponentRollout20261024/Journey/%s.%s"), TextureName, TextureName)
        : FString();
}

FLinearColor CalendarFill(EJourneyCalendarState State)
{
    switch (State)
    {
    case EJourneyCalendarState::Signed: return FLinearColor(.23f, .55f, .49f, .16f);
    case EJourneyCalendarState::TodayClaimable: return FLinearColor(.72f, .54f, .22f, .19f);
    case EJourneyCalendarState::Future: return FLinearColor(.48f, .49f, .45f, .07f);
    case EJourneyCalendarState::Missed: return FLinearColor(.62f, .28f, .25f, .12f);
    default: return FLinearColor(.96f, .94f, .88f, .48f);
    }
}

FLinearColor CalendarInk(EJourneyCalendarState State)
{
    switch (State)
    {
    case EJourneyCalendarState::Signed: return FLinearColor(.12f, .43f, .36f, 1.f);
    case EJourneyCalendarState::TodayClaimable: return FLinearColor(.63f, .40f, .11f, 1.f);
    case EJourneyCalendarState::Future: return FLinearColor(.43f, .45f, .42f, 1.f);
    case EJourneyCalendarState::Missed: return FLinearColor(.58f, .25f, .22f, 1.f);
    default: return FLinearColor(.20f, .24f, .21f, 1.f);
    }
}

FText CalendarStateLabel(EJourneyCalendarState State)
{
    switch (State)
    {
    case EJourneyCalendarState::Signed: return FText::FromString(TEXT("已签到"));
    case EJourneyCalendarState::TodayClaimable: return FText::FromString(TEXT("今日可领"));
    case EJourneyCalendarState::Future: return FText::FromString(TEXT("未到"));
    case EJourneyCalendarState::Missed: return FText::FromString(TEXT("漏签"));
    default: return FText::FromString(TEXT("待同步"));
    }
}

void AddFillSlot(UOverlay* Overlay, UWidget* Child)
{
    if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Child))
    {
        Slot->SetHorizontalAlignment(HAlign_Fill);
        Slot->SetVerticalAlignment(VAlign_Fill);
    }
}
}

void UJourneyStateBadgeWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
    {
        HitTarget = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JourneyGlyphHitTarget"));
        FButtonStyle GlyphButton;
        FSlateBrush EmptyBrush;
        EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
        FSlateBrush HoverBrush;
        HoverBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        HoverBrush.TintColor = FLinearColor(.29f, .57f, .68f, .12f);
        HoverBrush.OutlineSettings.Color = FLinearColor(.29f, .57f, .68f, .9f);
        HoverBrush.OutlineSettings.Width = 1.5f;
        GlyphButton.SetNormal(EmptyBrush).SetHovered(HoverBrush).SetPressed(HoverBrush).SetDisabled(EmptyBrush);
        HitTarget->SetStyle(GlyphButton);
        WidgetTree->RootWidget = HitTarget;
        UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("JourneyGlyphOverlay"));
        HitTarget->AddChild(Overlay);
        GlyphImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("JourneyGlyphImage"));
        GlyphImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        AddFillSlot(Overlay, GlyphImage);
        CaptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JourneyGlyphCaption"));
        CaptionText->SetVisibility(ESlateVisibility::Collapsed);
        CaptionText->SetJustification(ETextJustify::Center);
        CaptionText->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Button, 22));
        AddFillSlot(Overlay, CaptionText);
        HitTarget->OnClicked.AddDynamic(this, &UJourneyStateBadgeWidget::HandlePressed);
    }
    RefreshGlyph();
}

void UJourneyStateBadgeWidget::SetGlyphState(EJourneyGlyphFamily Family, FName State)
{
    GlyphFamily = Family;
    GlyphState = State;
    if (HitTarget) HitTarget->SetIsEnabled(Family != EJourneyGlyphFamily::FortuneSeal);
    RefreshGlyph();
}

void UJourneyStateBadgeWidget::SetCaption(const FText& Caption)
{
    if (CaptionText)
    {
        CaptionText->SetText(Caption);
        CaptionText->SetVisibility(Caption.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
}

void UJourneyStateBadgeWidget::SetBadgeEnabled(bool bEnabled)
{
    if (HitTarget) HitTarget->SetIsEnabled(bEnabled);
}

void UJourneyStateBadgeWidget::RefreshGlyph()
{
    if (!GlyphImage) return;
    const FString Path = JourneyGlyphTexturePath(GlyphFamily, GlyphState);
    UTexture2D* Texture = Path.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *Path);
    if (Texture) GlyphImage->SetBrushFromTexture(Texture, true);
    GlyphImage->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UJourneyStateBadgeWidget::HandlePressed()
{
    OnGlyphPressed.Broadcast(GlyphState);
}

void UJourneyCalendarDayWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
    {
        HitTarget = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JourneyCalendarDayHitTarget"));
        WidgetTree->RootWidget = HitTarget;
        CellOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("JourneyCalendarDayOverlay"));
        HitTarget->AddChild(CellOverlay);
        UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JourneyCalendarDayBackground"));
        Background->SetBrushColor(FLinearColor::White);
        AddFillSlot(CellOverlay, Background);
        DayText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JourneyCalendarDayNumber"));
        DayText->SetJustification(ETextJustify::Center);
        DayText->SetVisibility(ESlateVisibility::HitTestInvisible);
        DayText->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Data, 25));
        AddFillSlot(CellOverlay, DayText);
        if (UOverlaySlot* DaySlot = Cast<UOverlaySlot>(DayText->Slot))
        {
            DaySlot->SetHorizontalAlignment(HAlign_Center);
            DaySlot->SetVerticalAlignment(VAlign_Top);
        }
        StateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JourneyCalendarDayState"));
        StateText->SetJustification(ETextJustify::Center);
        StateText->SetVisibility(ESlateVisibility::HitTestInvisible);
        StateText->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Secondary, 18));
        AddFillSlot(CellOverlay, StateText);
        if (UOverlaySlot* StateSlot = Cast<UOverlaySlot>(StateText->Slot))
        {
            StateSlot->SetHorizontalAlignment(HAlign_Center);
            StateSlot->SetVerticalAlignment(VAlign_Bottom);
        }
        HitTarget->OnClicked.AddDynamic(this, &UJourneyCalendarDayWidget::HandlePressed);
    }
    RefreshCell();
}

void UJourneyCalendarDayWidget::SetDayState(int32 Day, EJourneyCalendarState State, bool bInSelected)
{
    ServerDayNumber = FMath::Clamp(Day, 0, 31);
    ServerState = State;
    bSelected = bInSelected;
    RefreshCell();
}

void UJourneyCalendarDayWidget::SetDayEnabled(bool bEnabled)
{
    if (HitTarget) HitTarget->SetIsEnabled(bEnabled && ServerDayNumber > 0);
}

void UJourneyCalendarDayWidget::RefreshCell()
{
    if (!DayText || !StateText || !CellOverlay) return;
    DayText->SetText(ServerDayNumber > 0 ? FText::AsNumber(ServerDayNumber) : FText::GetEmpty());
    DayText->SetColorAndOpacity(FSlateColor(CalendarInk(ServerState)));
    StateText->SetText(CalendarStateLabel(ServerState));
    StateText->SetColorAndOpacity(FSlateColor(CalendarInk(ServerState)));
    for (UWidget* Child : CellOverlay->GetAllChildren())
    {
        if (UBorder* Background = Cast<UBorder>(Child))
        {
            Background->SetBrushColor(CalendarFill(ServerState));
            Background->SetPadding(FMargin(bSelected ? 3.f : 5.f));
        }
    }
}

void UJourneyCalendarDayWidget::HandlePressed()
{
    if (ServerDayNumber > 0) OnDayPressed.Broadcast(ServerDayNumber);
}

void UJourneyClaimStateWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
    {
        ClaimButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JourneySignInClaimButton"));
        ClaimButton->SetStyle(InkPresentationStyle::FeatureButton(true, true, false));
        WidgetTree->RootWidget = ClaimButton;
        ClaimLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JourneySignInClaimLabel"));
        ClaimLabel->SetJustification(ETextJustify::Center);
        ClaimLabel->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Button, 28));
        ClaimButton->AddChild(ClaimLabel);
        ClaimButton->OnClicked.AddDynamic(this, &UJourneyClaimStateWidget::HandlePressed);
    }
    RefreshClaimState();
}

void UJourneyClaimStateWidget::SetClaimState(EDailySignInClaimState State, const FText& Reason)
{
    ClaimState = State;
    ClaimReason = Reason;
    bClaimEventSent = State == EDailySignInClaimState::Pending;
    RefreshClaimState();
}

void UJourneyClaimStateWidget::RefreshClaimState()
{
    if (!ClaimButton || !ClaimLabel) return;
    FText Caption;
    FLinearColor Color(.45f, .46f, .43f, 1.f);
    switch (ClaimState)
    {
    case EDailySignInClaimState::Claimable:
        Caption = FText::FromString(TEXT("今日签到")); Color = FLinearColor(.15f, .48f, .39f, 1.f); break;
    case EDailySignInClaimState::Pending:
        Caption = FText::FromString(TEXT("提交中，等待确认")); Color = FLinearColor(.38f, .43f, .43f, 1.f); break;
    case EDailySignInClaimState::Claimed:
        Caption = FText::FromString(TEXT("今日已签到")); Color = FLinearColor(.46f, .48f, .44f, 1.f); break;
    case EDailySignInClaimState::Disconnected:
        Caption = FText::FromString(TEXT("签到服务未连接")); Color = FLinearColor(.55f, .34f, .28f, 1.f); break;
    case EDailySignInClaimState::Unavailable:
        Caption = ClaimReason.IsEmpty() ? FText::FromString(TEXT("签到暂不可用")) : ClaimReason; break;
    case EDailySignInClaimState::Error:
        Caption = ClaimReason.IsEmpty() ? FText::FromString(TEXT("签到未确认")) : ClaimReason; Color = FLinearColor(.55f, .34f, .28f, 1.f); break;
    default:
        Caption = ClaimReason.IsEmpty() ? FText::FromString(TEXT("签到状态待同步")) : ClaimReason; break;
    }
    ClaimLabel->SetText(Caption);
    ClaimLabel->SetColorAndOpacity(FSlateColor(ClaimState == EDailySignInClaimState::Claimable
        ? FLinearColor(.98f, .96f, .90f, 1.f)
        : FLinearColor(.15f, .18f, .16f, 1.f)));
    ClaimButton->SetStyle(InkPresentationStyle::FeatureButton(
        ClaimState == EDailySignInClaimState::Claimable,
        ClaimState == EDailySignInClaimState::Claimable,
        false));
    ClaimButton->SetColorAndOpacity(Color);
    ClaimButton->SetIsEnabled(ClaimState == EDailySignInClaimState::Claimable);
    ClaimButton->SetToolTipText(ClaimReason);
}

void UJourneyClaimStateWidget::HandlePressed()
{
    if (ClaimState == EDailySignInClaimState::Claimable && !bClaimEventSent)
    {
        bClaimEventSent = true;
        ClaimButton->SetIsEnabled(false);
        OnClaimRequested.Broadcast();
    }
}
