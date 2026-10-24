#include "InkPlayerVisibleEntryWidgets.h"

#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(27, 41, 37));
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(244, 236, 221));
	const FLinearColor PaperLight = FLinearColor::FromSRGBColor(FColor(251, 247, 238));
	const FLinearColor Jade = FLinearColor::FromSRGBColor(FColor(50, 111, 99));
	const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(154, 115, 52));
	const FLinearColor SecondaryInk = FLinearColor::FromSRGBColor(FColor(66, 95, 103));
	const FLinearColor MutedInk = FLinearColor::FromSRGBColor(FColor(112, 110, 102));

	FSlateBrush BoxBrush(const FLinearColor Fill, const FLinearColor Outline = Gold, const float OutlineWidth = 1.5f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
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
		FSlateFontInfo Font;
		Font.Size = Size;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(Color);
		Label->SetAutoWrapText(true);
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	}

	FButtonStyle ActionStyle(const bool bPrimary, const bool bEnabled)
	{
		const FLinearColor Fill = bPrimary ? FLinearColor::FromSRGBColor(FColor(23, 62, 55)) : PaperLight;
		const FLinearColor Hover = bPrimary ? Jade : Paper;
		const FLinearColor Pressed = bPrimary ? SecondaryInk : FLinearColor::FromSRGBColor(FColor(229, 220, 203));
		const FLinearColor Disabled = MutedInk;
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
		const int32 CompactLabelSize = 0, UTexture2D* GlyphAtlas = nullptr,
		const int32 GlyphIndex = INDEX_NONE, const int32 GlyphColumns = 3, const int32 GlyphRows = 2)
	{
		auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(
			UInkFrontendActionButton::StaticClass(), WidgetName);
		Button->Action = Action;
		Button->BindFrontend(Frontend);
		Button->SetIsEnabled(bEnabled);
		Button->SetStyle(ActionStyle(bPrimary, bEnabled));
		auto* Sizing = Tree->ConstructWidget<USizeBox>();
		Sizing->SetMinDesiredHeight(MinHeight);
		Sizing->SetMinDesiredWidth(MinWidth);
		if (GlyphAtlas && GlyphIndex >= 0 && GlyphIndex < GlyphColumns * GlyphRows)
		{
			auto* Content = Tree->ConstructWidget<UVerticalBox>();
			FSlateBrush IconBrush;
			IconBrush.DrawAs = ESlateBrushDrawType::Image;
			IconBrush.SetResourceObject(GlyphAtlas);
			IconBrush.SetImageSize(FVector2D(40.f));
			const int32 Column = GlyphIndex % GlyphColumns;
			const int32 Row = GlyphIndex / GlyphColumns;
			IconBrush.SetUVRegion(FBox2D(FVector2D(float(Column) / GlyphColumns, float(Row) / GlyphRows),
				FVector2D(float(Column + 1) / GlyphColumns, float(Row + 1) / GlyphRows)));
			auto* Icon = Tree->ConstructWidget<UImage>();
			Icon->SetBrush(IconBrush);
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
			Content->AddChildToVerticalBox(Icon)->SetPadding(FMargin(0.f, 2.f, 0.f, 1.f));
			Content->AddChildToVerticalBox(MakeLabel(Tree, Caption,
				CompactLabelSize > 0 ? CompactLabelSize : bPrimary ? 30 : 26, LabelName,
				!bEnabled || bPrimary ? PaperLight : Ink));
			Sizing->SetContent(Content);
		}
		else
		{
			Sizing->SetContent(MakeLabel(Tree, Caption,
				CompactLabelSize > 0 ? CompactLabelSize : bPrimary ? 30 : 26, LabelName,
				!bEnabled || bPrimary ? PaperLight : Ink));
		}
		Button->SetContent(Sizing);
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
	UInkFrontendWidget* Frontend, const bool bCanContinue, const FText& ContinueStatus,
	UTexture2D* MainActionOrnament)
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
		FLinearColor::FromSRGBColor(FColor(156, 67, 54)));
	Feedback->SetVisibility(ESlateVisibility::Collapsed);
	Content->AddChildToVerticalBox(Feedback);
	auto* Layout = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Rebuild_MainLayoutV1"));
	Layout->AddChildToOverlay(Surface);
	if (MainActionOrnament)
	{
		auto* Ornament = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Rebuild_MainActionOrnament"));
		Ornament->SetBrushFromTexture(MainActionOrnament, true);
		Ornament->SetVisibility(ESlateVisibility::HitTestInvisible);
		auto* OrnamentSize = Tree->ConstructWidget<USizeBox>();
		OrnamentSize->SetWidthOverride(88.f);
		OrnamentSize->SetHeightOverride(88.f);
		OrnamentSize->SetContent(Ornament);
		if (auto* Slot = Layout->AddChildToOverlay(OrnamentSize))
		{
			Slot->SetHorizontalAlignment(HAlign_Right);
			Slot->SetVerticalAlignment(VAlign_Top);
			Slot->SetPadding(FMargin(0.f, 4.f, 4.f, 0.f));
		}
	}
	return Layout;
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
		auto* Caption = MakeLabel(Tree, Label, 17, NAME_None, PaperLight);
		Caption->SetJustification(ETextJustify::Left);
		Body->AddChildToVerticalBox(Caption)->SetPadding(FMargin(2.f, 2.f, 2.f, 0.f));
		auto* Meter = MakeMeter(Tree, Name, Fill);
		Body->AddChildToVerticalBox(Meter)->SetPadding(FMargin(2.f, 0.f, 2.f, 4.f));
		return Meter;
	}
}

void InkPlayerVisibleEntryWidgets::BuildHudSurface(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, UCanvasPanel* HudPage, UInkAnchorStatusWidget* LiveStatus,
	const FText& ActivityStatus, UTexture2D* RouteGlyphAtlas)
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
	IdentitySlot->SetOffsets(FMargin(24.f, 24.f, 344.f, 0.f));
	IdentitySlot->SetAutoSize(true);
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
			Index == 0, true, 88.f, Index == 2 ? FName(TEXT("Rebuild_HudCultivationLabel")) : NAME_None,
			88.f, 26, RouteGlyphAtlas, Index == 4 ? 5 : Index, 3, 2));
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
	Menu->SetContent(MenuBody);
	const FName MenuActions[] = {TEXT("OpenSaves"), TEXT("OpenSettings"), TEXT("ReturnToMainMenu")};
	const FText MenuLabels[] = {
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
