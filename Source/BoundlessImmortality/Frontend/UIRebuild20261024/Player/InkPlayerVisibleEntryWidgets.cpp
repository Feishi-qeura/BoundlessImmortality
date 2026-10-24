#include "InkPlayerVisibleEntryWidgets.h"
#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"

#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(32, 40, 35));
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(243, 235, 221));
	const FLinearColor PaperLight = FLinearColor::FromSRGBColor(FColor(251, 247, 238));
	const FLinearColor Jade = FLinearColor::FromSRGBColor(FColor(58, 141, 131));
	const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(184, 139, 69));
	const FLinearColor SecondaryInk = FLinearColor::FromSRGBColor(FColor(61, 88, 97));
	const FLinearColor MutedInk = FLinearColor::FromSRGBColor(FColor(112, 114, 105));

	FSlateBrush BoxBrush(const FLinearColor Fill, const FLinearColor Outline = Gold, const float OutlineWidth = 1.5f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(4.f);
		Brush.TintColor = Fill;
		Brush.OutlineSettings.Color = Outline;
		Brush.OutlineSettings.Width = OutlineWidth;
		return Brush;
	}

	UTextBlock* MakeLabel(UWidgetTree* Tree, const FText& Text, const int32 Size,
		const FName Name = NAME_None, const FLinearColor Color = Ink)
	{
		auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		const FSlateFontInfo Font = InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, Size);
		Label->SetFont(Font);
		Label->SetColorAndOpacity(Color);
		Label->SetAutoWrapText(true);
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	}

	FButtonStyle ActionStyle(const bool bPrimary, const bool bEnabled)
	{
		const FLinearColor Fill = bPrimary ? Ink : PaperLight;
		const FLinearColor Hover = bPrimary ? Jade : Paper;
		const FLinearColor Pressed = bPrimary ? SecondaryInk : FLinearColor::FromSRGBColor(FColor(229, 220, 203));
		const FLinearColor Disabled = FLinearColor::FromSRGBColor(FColor(224, 220, 209));
		FButtonStyle Style;
		Style.SetNormal(BoxBrush(bEnabled ? Fill : Disabled, bEnabled ? Gold : MutedInk));
		Style.SetHovered(BoxBrush(Hover, Gold, 2.f));
		Style.SetPressed(BoxBrush(Pressed, Gold, 2.f));
		Style.SetDisabled(BoxBrush(Disabled, MutedInk));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		return Style;
	}

	UInkFrontendActionButton* MakeActionButton(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		const FName WidgetName, const FName Action, const FText& Caption,
		const bool bPrimary, const bool bEnabled, const float MinWidth = 88.f,
		const FName LabelName = NAME_None, const float MinHeight = 88.f,
		const int32 CompactLabelSize = 0)
	{
		auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(
			UInkFrontendActionButton::StaticClass(), WidgetName);
		Button->Action = Action;
		Button->BindFrontend(Frontend);
		Button->SetIsEnabled(bEnabled);
		Button->SetStyle(ActionStyle(bPrimary, bEnabled));
		auto* Sizing = Tree->ConstructWidget<USizeBox>();
		Sizing->SetMinDesiredHeight(FMath::Max(InkRebuildUI::TouchSize,MinHeight));
		Sizing->SetMinDesiredWidth(FMath::Max(InkRebuildUI::TouchSize,MinWidth));
		auto* Label = MakeLabel(Tree, Caption, CompactLabelSize > 0 ? CompactLabelSize : bPrimary ? 30 : 24, LabelName,
			bPrimary && bEnabled ? PaperLight : Ink);
		Sizing->SetContent(Label);
		if(auto* Slot=Cast<USizeBoxSlot>(Label->Slot))Slot->SetVerticalAlignment(VAlign_Center);
		Button->SetContent(Sizing);
		if(auto* Slot=Cast<UButtonSlot>(Sizing->Slot))
		{Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(12.f,4.f));}
		return Button;
	}

	void AddEqualAction(UHorizontalBox* Row, UInkFrontendActionButton* Button)
	{
		if (auto* Slot = Row->AddChildToHorizontalBox(Button))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(4.f, 2.f));
		}
	}
}

UWidget* InkPlayerVisibleEntryWidgets::BuildMainMenuSurface(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, const bool bCanContinue, const FText& ContinueStatus)
{
	if (!Tree || !Frontend) return nullptr;

	// This surface occupies only the existing bottom menu viewport. The top title and
	// the central camera/character region remain outside its hit-test and paint bounds.
	auto* Surface = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rebuild_MainSurfaceV1"));
	Surface->SetBrush(BoxBrush(Paper.CopyWithNewOpacity(.92f), Gold, 1.5f));
	Surface->SetPadding(FMargin(12.f));
	auto* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rebuild_MainActionsV1"));
	Surface->SetContent(Content);

	const auto AddPrimary = [Tree, Frontend, Content](const FName Name, const FName Action,
		const FText& Caption, const bool bEnabled)
	{
		auto* Button = MakeActionButton(Tree, Frontend, Name, Action, Caption, true, bEnabled);
		Content->AddChildToVerticalBox(Button)->SetPadding(FMargin(2.f, 3.f));
	};
	AddPrimary(TEXT("Rebuild_Main_StartGame"), TEXT("StartGame"), NSLOCTEXT("InkPlayerVisibleEntry", "StartGame", "开始游戏"), true);
	AddPrimary(TEXT("Rebuild_Main_ContinueGame"), TEXT("ContinueGame"), NSLOCTEXT("InkPlayerVisibleEntry", "ContinueGame", "继续游戏"), bCanContinue);

	auto* Status = MakeLabel(Tree, ContinueStatus, 22, TEXT("Rebuild_MainContinueStatus"), SecondaryInk);
	Content->AddChildToVerticalBox(Status)->SetPadding(FMargin(6.f, 2.f, 6.f, 6.f));

	auto* Utilities = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Rebuild_MainUtilitiesV1"));
	AddEqualAction(Utilities, MakeActionButton(Tree, Frontend, TEXT("Rebuild_Main_OpenSaves"), TEXT("OpenSaves"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Saves", "存档"), false, true));
	AddEqualAction(Utilities, MakeActionButton(Tree, Frontend, TEXT("Rebuild_Main_OpenSettings"), TEXT("OpenSettings"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Settings", "设置"), false, true));
	AddEqualAction(Utilities, MakeActionButton(Tree, Frontend, TEXT("Rebuild_Main_RequestQuit"), TEXT("RequestQuit"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Quit", "退出"), false, true));
	Content->AddChildToVerticalBox(Utilities)->SetPadding(FMargin(0.f, 4.f));

	auto* Feedback = MakeLabel(Tree, FText::GetEmpty(), 20, TEXT("Rebuild_MainFeedback"),
		FLinearColor::FromSRGBColor(FColor(170, 73, 55)));
	Feedback->SetVisibility(ESlateVisibility::Collapsed);
	Content->AddChildToVerticalBox(Feedback);
	return Surface;
}

namespace
{
	UProgressBar* MakeMeter(UWidgetTree* Tree, const FName Name, const FLinearColor Fill)
	{
		auto* Meter = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
		FProgressBarStyle Style;
		Style.SetBackgroundImage(BoxBrush(FLinearColor(.12f, .15f, .14f, .95f), Gold, 1.f));
		Style.SetFillImage(BoxBrush(Fill, Fill, 0.f));
		Meter->SetWidgetStyle(Style);
		Meter->SetPercent(0.f);
		return Meter;
	}

	UWidget* AddStatusLine(UWidgetTree* Tree, UVerticalBox* Body, const FText& Label,
		const FName Name, const FLinearColor Fill)
	{
		// The live value already includes the semantic label; avoid a duplicate line.
		auto* Meter = MakeMeter(Tree, Name, Fill);
		auto* MeterSize=Tree->ConstructWidget<USizeBox>();MeterSize->SetHeightOverride(10.f);MeterSize->SetContent(Meter);
		Body->AddChildToVerticalBox(MeterSize)->SetPadding(FMargin(2.f, 2.f, 2.f, 6.f));
		return Meter;
	}
}

void InkPlayerVisibleEntryWidgets::BuildHudSurface(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, UCanvasPanel* HudPage, UInkAnchorStatusWidget* LiveStatus,
	const FText& ActivityStatus)
{
	if (!Tree || !Frontend || !HudPage) return;

	// The center stays untouched so the live camera and controlled character remain visible.
	auto* Identity = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rebuild_HudIdentityPanelV1"));
	Identity->SetBrush(BoxBrush(Ink.CopyWithNewOpacity(.92f), Gold, 1.5f));
	Identity->SetPadding(FMargin(14.f, 10.f));
	auto* IdentityBody = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rebuild_HudIdentityBodyV1"));
	Identity->SetContent(IdentityBody);
	auto* Name = MakeLabel(Tree, NSLOCTEXT("InkPlayerVisibleEntry", "AnchorLoading", "读取中"), 24,
		TEXT("Rebuild_HudAnchorName"), PaperLight);
	Name->SetJustification(ETextJustify::Left);
	IdentityBody->AddChildToVerticalBox(Name)->SetPadding(FMargin(2.f, 1.f, 2.f, 0.f));
	auto* Realm = MakeLabel(Tree, NSLOCTEXT("InkPlayerVisibleEntry", "RealmFallback", "境界读取中"), 19,
		TEXT("Rebuild_HudRealmName"), FLinearColor::FromSRGBColor(FColor(224, 190, 123)));
	Realm->SetJustification(ETextJustify::Left);
	IdentityBody->AddChildToVerticalBox(Realm)->SetPadding(FMargin(2.f, 0.f, 2.f, 4.f));
	auto* Health = MakeLabel(Tree, FText::GetEmpty(), 16, TEXT("Rebuild_HudHealthValue"), PaperLight);
	Health->SetJustification(ETextJustify::Left);
	IdentityBody->AddChildToVerticalBox(Health);
	AddStatusLine(Tree, IdentityBody, NSLOCTEXT("InkPlayerVisibleEntry", "Health", "气血"),
		TEXT("Rebuild_HudHealthMeter"), FLinearColor::FromSRGBColor(FColor(174, 75, 63)));
	auto* Cultivation = MakeLabel(Tree, FText::GetEmpty(), 16, TEXT("Rebuild_HudCultivationValue"), PaperLight);
	Cultivation->SetJustification(ETextJustify::Left);
	IdentityBody->AddChildToVerticalBox(Cultivation);
	AddStatusLine(Tree, IdentityBody, NSLOCTEXT("InkPlayerVisibleEntry", "Cultivation", "修为"),
		TEXT("Rebuild_HudCultivationMeter"), Jade);
	auto* Activity = MakeLabel(Tree, ActivityStatus, 15, TEXT("Rebuild_HudActivityStatus"),
		FLinearColor::FromSRGBColor(FColor(224, 190, 123)));
	Activity->SetJustification(ETextJustify::Left);
	IdentityBody->AddChildToVerticalBox(Activity)->SetPadding(FMargin(2.f, 0.f));
	auto* IdentitySlot = HudPage->AddChildToCanvas(Identity);
	IdentitySlot->SetAnchors(FAnchors(0.f, 0.f));
	IdentitySlot->SetOffsets(FMargin(24.f, 24.f, 344.f, 220.f));
	IdentitySlot->SetAutoSize(false);
	IdentitySlot->SetZOrder(5);

	// Existing shortcut actions are kept as a compact column at the far edge.
	auto* Shortcuts = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rebuild_HudShortcutsPanelV1"));
	Shortcuts->SetBrush(BoxBrush(Ink.CopyWithNewOpacity(.88f), Gold, 1.f));
	Shortcuts->SetPadding(FMargin(6.f));
	auto* ShortcutBody = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rebuild_HudShortcutActionsV1"));
	Shortcuts->SetContent(ShortcutBody);
	const FName ShortcutActions[] = {TEXT("OpenActivities"), TEXT("OpenSignIn"), TEXT("OpenExtensions"), TEXT("OpenLaunchEvent"), TEXT("OpenWuling")};
	const FText Labels[] = {
		NSLOCTEXT("InkPlayerVisibleEntry", "Activities", "活动"),
		NSLOCTEXT("InkPlayerVisibleEntry", "SignIn", "签到"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Extensions", "仙途"),
		NSLOCTEXT("InkPlayerVisibleEntry", "LaunchEvent", "开服"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Wuling", "武灵")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ShortcutActions); ++Index)
	{
		const FName WidgetName(*FString::Printf(TEXT("Rebuild_Hud_%s"), *ShortcutActions[Index].ToString()));
		ShortcutBody->AddChildToVerticalBox(MakeActionButton(Tree, Frontend, WidgetName,
			ShortcutActions[Index], Labels[Index], false, true, 104.f))->SetPadding(FMargin(1.f, 2.f));
	}
	auto* ShortcutSlot = HudPage->AddChildToCanvas(Shortcuts);
	ShortcutSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	ShortcutSlot->SetAlignment(FVector2D(1.f, 0.f));
	ShortcutSlot->SetOffsets(FMargin(-24.f, 24.f, 0.f, 0.f));
	ShortcutSlot->SetAutoSize(true);
	ShortcutSlot->SetZOrder(5);

	// Five gameplay actions sit in a distinct bottom rail, with the center scene visible above.
	auto* Rail = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rebuild_HudRailV1"));
	Rail->SetBrush(BoxBrush(Paper.CopyWithNewOpacity(.92f), Gold, 1.5f));
	Rail->SetPadding(FMargin(6.f));
	auto* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Rebuild_HudActionsV1"));
	Rail->SetContent(Row);
	const FName Actions[] = {TEXT("OpenCharacter"), TEXT("OpenInventory"), TEXT("ToggleCultivation"), TEXT("OpenLeaderboard"), TEXT("ToggleHUDMenu")};
	const FText ActionLabels[] = {
		NSLOCTEXT("InkPlayerVisibleEntry", "Character", "角色"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Inventory", "背包"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Cultivation", "修炼"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Leaderboard", "排行"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Menu", "菜单")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
	{
		const FName WidgetName(*FString::Printf(TEXT("Rebuild_Hud_%s"), *Actions[Index].ToString()));
		AddEqualAction(Row, MakeActionButton(Tree, Frontend, WidgetName, Actions[Index], ActionLabels[Index],
		Index == 0, true, 56.f, Index == 2 ? FName(TEXT("Rebuild_HudCultivationLabel")) : NAME_None, 72.f, 18));
	}
	auto* RailSlot = HudPage->AddChildToCanvas(Rail);
	RailSlot->SetAnchors(FAnchors(.5f, 1.f));
	RailSlot->SetAlignment(FVector2D(.5f, 1.f));
	RailSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, -24.f));
	RailSlot->SetAutoSize(true);
	RailSlot->SetZOrder(5);

	// Existing menu toggle logic addresses this exact name; the user-visible choices remain real routes.
	auto* Menu = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HUDRuntimeMenu"));
	Menu->SetBrush(BoxBrush(Paper.CopyWithNewOpacity(.96f), Gold, 1.5f));
	Menu->SetPadding(FMargin(8.f));
	Menu->SetVisibility(ESlateVisibility::Collapsed);
	auto* MenuBody = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rebuild_HudRuntimeActionsV1"));
	auto* MenuScroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewHudFeatureMenuScroll"));MenuScroll->AddChild(MenuBody);
    auto* MenuBounds=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewHudFeatureMenuBounds"));MenuBounds->SetWidthOverride(300);MenuBounds->SetHeightOverride(420);MenuBounds->SetContent(MenuScroll);Menu->SetContent(MenuBounds);
	const FName MenuActions[] = {TEXT("OpenExtensions"),TEXT("OpenActivities"),TEXT("OpenSignIn"),TEXT("OpenWuling"),TEXT("OpenSwordSpirit"),TEXT("OpenSaves"), TEXT("OpenSettings"), TEXT("ReturnToMainMenu")};
	const FText MenuLabels[] = {FText::FromString(TEXT("仙途")),FText::FromString(TEXT("活动")),FText::FromString(TEXT("签到")),FText::FromString(TEXT("武灵")),FText::FromString(TEXT("剑灵")),
		NSLOCTEXT("InkPlayerVisibleEntry", "Saves", "存档"),
		NSLOCTEXT("InkPlayerVisibleEntry", "Settings", "设置"),
		NSLOCTEXT("InkPlayerVisibleEntry", "ReturnToMainMenu", "返回主菜单")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(MenuActions); ++Index)
	{
		const FName WidgetName(*FString::Printf(TEXT("Rebuild_Hud_%s"), *MenuActions[Index].ToString()));
		MenuBody->AddChildToVerticalBox(MakeActionButton(Tree, Frontend, WidgetName,
			MenuActions[Index], MenuLabels[Index], false, true, 184.f))->SetPadding(FMargin(2.f));
	}
	auto* MenuSlot = HudPage->AddChildToCanvas(Menu);
	MenuSlot->SetAnchors(FAnchors(.5f, 1.f));
	MenuSlot->SetAlignment(FVector2D(.5f, 1.f));
	MenuSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, -148.f));
	MenuSlot->SetAutoSize(true);
	MenuSlot->SetZOrder(10);
	RefreshHudStatus(Tree, LiveStatus, ActivityStatus);
}

void InkPlayerVisibleEntryWidgets::RefreshHudStatus(UWidgetTree* Tree,
	UInkAnchorStatusWidget* LiveStatus, const FText& ActivityStatus)
{
	if (!Tree) return;
	if (auto* Text = Cast<UTextBlock>(Tree->FindWidget(TEXT("Rebuild_HudAnchorName"))))
		Text->SetText(LiveStatus && LiveStatus->bHasAnchor && !LiveStatus->AnchorName.IsEmpty()
			? FText::FromString(LiveStatus->AnchorName) : NSLOCTEXT("InkPlayerVisibleEntry", "AnchorLoading", "读取中"));
	if (auto* Text = Cast<UTextBlock>(Tree->FindWidget(TEXT("Rebuild_HudRealmName"))))
		Text->SetText(LiveStatus && LiveStatus->bHasAnchor && !LiveStatus->RealmName.IsEmpty()
			? LiveStatus->RealmName : NSLOCTEXT("InkPlayerVisibleEntry", "RealmFallback", "境界读取中"));
	if (auto* Text = Cast<UTextBlock>(Tree->FindWidget(TEXT("Rebuild_HudHealthValue"))))
		Text->SetText(LiveStatus && LiveStatus->bHasAnchor ? FText::Format(NSLOCTEXT("InkPlayerVisibleEntry", "HealthValue", "气血  {0} / {1}"),
			FText::AsNumber(LiveStatus->Health), FText::AsNumber(LiveStatus->MaxHealth)) : FText::GetEmpty());
	if (auto* Meter = Cast<UProgressBar>(Tree->FindWidget(TEXT("Rebuild_HudHealthMeter"))))
		Meter->SetPercent(LiveStatus && LiveStatus->bHasAnchor ? LiveStatus->DisplayedHealthFraction : 0.f);
	if (auto* Text = Cast<UTextBlock>(Tree->FindWidget(TEXT("Rebuild_HudCultivationValue"))))
		Text->SetText(LiveStatus && LiveStatus->bHasAnchor ? FText::Format(NSLOCTEXT("InkPlayerVisibleEntry", "CultivationValue", "修为  {0} / {1}"),
			FText::AsNumber(LiveStatus->Cultivation), FText::AsNumber(LiveStatus->RequiredCultivation)) : FText::GetEmpty());
	if (auto* Meter = Cast<UProgressBar>(Tree->FindWidget(TEXT("Rebuild_HudCultivationMeter"))))
		Meter->SetPercent(LiveStatus && LiveStatus->bHasAnchor ? LiveStatus->DisplayedCultivationFraction : 0.f);
	if (auto* Text = Cast<UTextBlock>(Tree->FindWidget(TEXT("Rebuild_HudActivityStatus"))))
		Text->SetText(LiveStatus && !LiveStatus->ActionLabel.IsEmpty()
			? FText::FromString(LiveStatus->ActionLabel) : ActivityStatus);
}
