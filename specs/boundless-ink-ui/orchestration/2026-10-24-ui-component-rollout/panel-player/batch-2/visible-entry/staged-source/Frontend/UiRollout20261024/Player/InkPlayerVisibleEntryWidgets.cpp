#include "InkPlayerVisibleEntryWidgets.h"

#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkPresentationStyle.h"
#include "Frontend/UI/InkUiTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	UTextBlock* MakeLabel(UWidgetTree* Tree, const FText& Text, const int32 Size,
		const FName Name = NAME_None)
	{
		auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		Label->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Body, Size));
		Label->SetColorAndOpacity(InkPresentationStyle::Ink());
		Label->SetAutoWrapText(true);
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	}

	UInkFrontendActionButton* MakeButton(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		const FName WidgetName, const FName Action, const FText& Label,
		const bool bPrimary, const bool bEnabled = true)
	{
		auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(
			UInkFrontendActionButton::StaticClass(), WidgetName);
		Button->Action = Action;
		Button->BindFrontend(Frontend);
		Button->SetIsEnabled(bEnabled);
		Button->SetStyle(InkPresentationStyle::FeatureButton(false, bPrimary));
		auto* Sizing = Tree->ConstructWidget<USizeBox>();
		Sizing->SetMinDesiredHeight(88.f);
		Sizing->SetMinDesiredWidth(88.f);
		auto* Text = MakeLabel(Tree, Label, bPrimary ? 30 : 24);
		Text->SetColorAndOpacity(bPrimary ? InkPresentationStyle::Paper() : InkPresentationStyle::Ink());
		Sizing->SetContent(Text);
		Button->SetContent(Sizing);
		return Button;
	}

	void AddFillAction(UHorizontalBox* Row, UInkFrontendActionButton* Button)
	{
		if (auto* Slot = Row->AddChildToHorizontalBox(Button))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(4.f));
		}
	}
}

UWidget* InkPlayerVisibleEntryWidgets::BuildMainMenuSurface(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, const bool bCanContinue, const FText& ContinueStatus)
{
	if (!Tree || !Frontend) return nullptr;

	// This compact surface lives at the existing bottom-anchored MainMenuScroll.
	// It deliberately leaves the title and live camera viewport untouched.
	auto* Surface = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VisibleEntry_MainMenuV3Surface"));
	FSlateBrush Paper = InkPresentationStyle::Surface(InkPresentationStyle::ESurface::FeaturePaper, FVector2D(680.f, 420.f));
	FLinearColor Tint = Paper.TintColor.GetSpecifiedColor();
	Tint.A = .90f;
	Paper.TintColor = Tint;
	Surface->SetBrush(Paper);
	Surface->SetBrushColor(FLinearColor::White);
	Surface->SetPadding(FMargin(12.f));

	auto* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VisibleEntry_MainMenuActions"));
	Surface->SetContent(Content);
	Content->AddChildToVerticalBox(MakeButton(Tree, Frontend, TEXT("Main_V3_StartGame"), TEXT("StartGame"),
		NSLOCTEXT("InkVisibleEntry", "StartGame", "开始游戏"), true));
	Content->AddChildToVerticalBox(MakeButton(Tree, Frontend, TEXT("Main_V3_ContinueGame"), TEXT("ContinueGame"),
		NSLOCTEXT("InkVisibleEntry", "ContinueGame", "继续游戏"), true, bCanContinue))->SetPadding(FMargin(0.f, 4.f));

	auto* Status = MakeLabel(Tree, ContinueStatus, 22, TEXT("Main_V3_ContinueState"));
	Status->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(61, 88, 97)));
	Content->AddChildToVerticalBox(Status)->SetPadding(FMargin(4.f, 2.f, 4.f, 6.f));

	auto* Utilities = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VisibleEntry_MainMenuUtilities"));
	AddFillAction(Utilities, MakeButton(Tree, Frontend, TEXT("Main_V3_OpenSaves"), TEXT("OpenSaves"),
		NSLOCTEXT("InkVisibleEntry", "Saves", "存档"), false));
	AddFillAction(Utilities, MakeButton(Tree, Frontend, TEXT("Main_V3_OpenSettings"), TEXT("OpenSettings"),
		NSLOCTEXT("InkVisibleEntry", "Settings", "设置"), false));
	AddFillAction(Utilities, MakeButton(Tree, Frontend, TEXT("Main_V3_RequestQuit"), TEXT("RequestQuit"),
		NSLOCTEXT("InkVisibleEntry", "Quit", "退出"), false));
	Content->AddChildToVerticalBox(Utilities)->SetPadding(FMargin(0.f, 4.f));

	auto* Feedback = MakeLabel(Tree, FText::GetEmpty(), 20, TEXT("Main_V3_Feedback"));
	Feedback->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(170, 73, 55)));
	Feedback->SetVisibility(ESlateVisibility::Collapsed);
	Content->AddChildToVerticalBox(Feedback);
	return Surface;
}

UWidget* InkPlayerVisibleEntryWidgets::BuildHudNavigation(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, UCanvasPanel* HudPage)
{
	if (!Tree || !Frontend || !HudPage) return nullptr;

	// A bottom-only rail protects the center playfield and live character.
	auto* Surface = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VisibleEntry_HudRailV3"));
	FSlateBrush Paper = InkPresentationStyle::Surface(InkPresentationStyle::ESurface::FeaturePaper, FVector2D(672.f, 112.f));
	FLinearColor Tint = Paper.TintColor.GetSpecifiedColor();
	Tint.A = .86f;
	Paper.TintColor = Tint;
	Surface->SetBrush(Paper);
	Surface->SetBrushColor(FLinearColor::White);
	Surface->SetPadding(FMargin(6.f));

	auto* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VisibleEntry_HudActions"));
	Surface->SetContent(Row);
	const FName Actions[] = {TEXT("OpenCharacter"), TEXT("OpenInventory"), TEXT("OpenLeaderboard"), TEXT("OpenSaves"), TEXT("OpenSettings"), TEXT("ReturnToMainMenu")};
	const FText Labels[] = {
		NSLOCTEXT("InkVisibleEntry", "Character", "角色"),
		NSLOCTEXT("InkVisibleEntry", "Inventory", "背包"),
		NSLOCTEXT("InkVisibleEntry", "Leaderboard", "排行"),
		NSLOCTEXT("InkVisibleEntry", "HudSaves", "存档"),
		NSLOCTEXT("InkVisibleEntry", "HudSettings", "设置"),
		NSLOCTEXT("InkVisibleEntry", "ReturnMain", "返回主菜单")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
	{
		const FName WidgetName(*FString::Printf(TEXT("HUD_V3_%s"), *Actions[Index].ToString()));
		AddFillAction(Row, MakeButton(Tree, Frontend, WidgetName, Actions[Index], Labels[Index], Index == 0));
	}

	auto* Slot = HudPage->AddChildToCanvas(Surface);
	Slot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	Slot->SetAlignment(FVector2D(0.f, 1.f));
	Slot->SetOffsets(FMargin(24.f, 0.f, 24.f, -24.f));
	Slot->SetAutoSize(true);
	Slot->SetZOrder(2);
	return Surface;
}
