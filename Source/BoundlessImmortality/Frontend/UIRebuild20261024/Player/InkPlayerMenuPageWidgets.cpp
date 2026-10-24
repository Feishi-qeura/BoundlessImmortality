#include "InkPlayerMenuPageWidgets.h"
#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"

#include "Frontend/InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
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

	FSlateBrush BoxBrush(const FLinearColor Fill, const FLinearColor Outline = Gold)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(4.f);
		Brush.TintColor = Fill;
		Brush.OutlineSettings.Color = Outline;
		Brush.OutlineSettings.Width = 1.5f;
		return Brush;
	}

	FButtonStyle ActionStyle(const bool bSelected = false)
	{
		FButtonStyle Style;
		Style.SetNormal(BoxBrush(bSelected ? Ink : PaperLight, Gold));
		Style.SetHovered(BoxBrush(bSelected ? Jade : Paper, Gold));
		Style.SetPressed(BoxBrush(bSelected ? Jade : Paper, Gold));
		Style.SetDisabled(BoxBrush(Paper, Gold));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		return Style;
	}

	FSlateBrush CardBrush()
	{
		return BoxBrush(Paper.CopyWithNewOpacity(.96f), Gold);
	}

	UTextBlock* MakeLabel(UWidgetTree* Tree, const FText& Text, int32 FontSize = 24, FName Name = NAME_None)
	{
		auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Text);
		const FSlateFontInfo Font = InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, FontSize);
		Label->SetFont(Font);
		Label->SetColorAndOpacity(Ink);
		Label->SetAutoWrapText(true);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Label;
	}

	UInkFrontendActionButton* MakeAction(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		const FName Action, const FText& Caption, const bool bEnabled = true)
	{
		const FName WidgetName(*FString::Printf(TEXT("Batch2_%s"), *Action.ToString()));
		auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(), WidgetName);
		Button->Action = Action;
		Button->BindFrontend(Frontend);
		Button->SetIsEnabled(bEnabled);
		Button->SetStyle(ActionStyle());
		auto* Sizing = Tree->ConstructWidget<USizeBox>();
		Sizing->SetMinDesiredHeight(88.f);
		auto* Label = MakeLabel(Tree, Caption, 24);
		Label->SetJustification(ETextJustify::Center);
		Sizing->SetContent(Label);
		if(auto* Slot=Cast<USizeBoxSlot>(Label->Slot))Slot->SetVerticalAlignment(VAlign_Center);
		Button->SetContent(Sizing);
		if(auto* Slot=Cast<UButtonSlot>(Sizing->Slot))
		{Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(12.f,4.f));}
		return Button;
	}

	void AddAction(UHorizontalBox* Row, UInkFrontendActionButton* Button)
	{
		if (auto* Slot = Row->AddChildToHorizontalBox(Button))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(4.f, 6.f));
		}
	}

	FText SlotSummary(const FInkSaveSlotInfo& Info)
	{
		if (!Info.bExists)
		{
			return NSLOCTEXT("InkPlayerMenuPages", "EmptySlot", "此处尚无存档");
		}
		if (!Info.bValid)
		{
			return Info.ValidationError.IsEmpty()
				? NSLOCTEXT("InkPlayerMenuPages", "InvalidSlot", "此存档无法读取")
				: FText::FromString(Info.ValidationError);
		}

		const int64 TotalSeconds = FMath::Max<int64>(0, FMath::RoundToInt64(Info.PlayedSeconds));
		const int64 Hours = TotalSeconds / 3600;
		const int64 Minutes = (TotalSeconds / 60) % 60;
		const FText Name = Info.DisplayName.IsEmpty()
			? FText::Format(NSLOCTEXT("InkPlayerMenuPages", "SlotNumber", "行程 {0}"), FText::AsNumber(Info.SlotIndex + 1))
			: FText::FromString(Info.DisplayName);
		return FText::Format(
			NSLOCTEXT("InkPlayerMenuPages", "ValidSlotSummary", "{0}\n{1} · {2} · 游历 {3} 小时 {4} 分"),
			Name,
			FText::FromString(Info.MapName),
			FText::AsDateTime(Info.SavedAt, EDateTimeStyle::Short, EDateTimeStyle::Short),
			FText::AsNumber(Hours), FText::AsNumber(Minutes));
	}
}

UWidget* InkPlayerMenuPageWidgets::BuildSavesBody(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
	const TArray<FInkSaveSlotInfo>& Slots, const bool bCanSave, const FText& SaveDisabledReason)
{
	if (!Tree || !Frontend) return nullptr;
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Batch2SavesBody"));

	for (int32 Index = 0; Index < UInkSaveSubsystem::ManualSlotCount; ++Index)
	{
		const FInkSaveSlotInfo* Info = Slots.FindByPredicate([Index](const FInkSaveSlotInfo& Item)
		{
			return Item.SlotIndex == Index;
		});
		const FInkSaveSlotInfo EmptyInfo{};
		const FInkSaveSlotInfo& Actual = Info ? *Info : EmptyInfo;

		auto* Card = Tree->ConstructWidget<UBorder>();
		Card->SetBrush(CardBrush());
		Card->SetPadding(FMargin(16.f));
		auto* CardBody = Tree->ConstructWidget<UVerticalBox>();
		Card->SetContent(CardBody);
		CardBody->AddChildToVerticalBox(MakeLabel(Tree,
			FText::Format(NSLOCTEXT("InkPlayerMenuPages", "SlotTitle", "行程 {0}"), FText::AsNumber(Index + 1)), 28));
		CardBody->AddChildToVerticalBox(MakeLabel(Tree, SlotSummary(Actual), 22));

		auto* Actions = Tree->ConstructWidget<UHorizontalBox>();
		const FName SaveAction(*FString::Printf(TEXT("SaveSlot_%d"), Index));
		const FName LoadAction(*FString::Printf(TEXT("LoadSlot_%d"), Index));
		const FName DeleteAction(*FString::Printf(TEXT("DeleteSlot_%d"), Index));
		AddAction(Actions, MakeAction(Tree, Frontend, SaveAction,
			Actual.bExists ? NSLOCTEXT("InkPlayerMenuPages", "OverwriteSlot", "覆盖此存档")
				: NSLOCTEXT("InkPlayerMenuPages", "SaveSlot", "保存行程"), bCanSave));
		AddAction(Actions, MakeAction(Tree, Frontend, LoadAction,
			NSLOCTEXT("InkPlayerMenuPages", "LoadSlot", "载入"), Actual.bValid));
		AddAction(Actions, MakeAction(Tree, Frontend, DeleteAction,
			NSLOCTEXT("InkPlayerMenuPages", "DeleteSlot", "删除"), Actual.bExists));
		CardBody->AddChildToVerticalBox(Actions);
		if (auto* Slot = Body->AddChildToVerticalBox(Card)) Slot->SetPadding(FMargin(0.f, 8.f));
	}
	if (!bCanSave && !SaveDisabledReason.IsEmpty())
	{
		Body->AddChildToVerticalBox(MakeLabel(Tree, SaveDisabledReason, 22));
	}
	return Body;
}

UWidget* InkPlayerMenuPageWidgets::BuildSettingsBody(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
	const int32 OverallQuality, const float EffectiveFrameRateLimit, const bool bReducedMotion)
{
	if (!Tree || !Frontend) return nullptr;
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Batch2SettingsBody"));

	const auto AddGroup = [Tree, Body, Frontend](const FText& Heading, const TArray<TPair<FName, FText>>& Options,
		const FName SelectedAction)
	{
		auto* Card = Tree->ConstructWidget<UBorder>();
		Card->SetBrush(CardBrush());
		Card->SetPadding(FMargin(16.f));
		auto* Content = Tree->ConstructWidget<UVerticalBox>();
		Card->SetContent(Content);
		Content->AddChildToVerticalBox(MakeLabel(Tree, Heading, 30));
		auto* Row = Tree->ConstructWidget<UHorizontalBox>();
		for (const TPair<FName, FText>& Option : Options)
		{
			auto* Button = MakeAction(Tree, Frontend, Option.Key, Option.Value);
			Button->SetStyle(ActionStyle(Option.Key == SelectedAction));
			if (auto* Slot = Row->AddChildToHorizontalBox(Button))
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Slot->SetPadding(FMargin(4.f, 6.f));
			}
		}
		Content->AddChildToVerticalBox(Row);
		if (auto* Slot = Body->AddChildToVerticalBox(Card)) Slot->SetPadding(FMargin(0.f, 8.f));
	};

	const FName QualityAction = OverallQuality == 0 ? FName(TEXT("QualityLow")) : OverallQuality == 1 ? FName(TEXT("QualityMedium")) : OverallQuality == 2 ? FName(TEXT("QualityHigh")) : FName(NAME_None);
	AddGroup(NSLOCTEXT("InkPlayerMenuPages", "Quality", "画面品质"), {
		{TEXT("QualityLow"), NSLOCTEXT("InkPlayerMenuPages", "Low", "流畅")},
		{TEXT("QualityMedium"), NSLOCTEXT("InkPlayerMenuPages", "Medium", "均衡")},
		{TEXT("QualityHigh"), NSLOCTEXT("InkPlayerMenuPages", "High", "精致")}}, QualityAction);
	const FText QualityValue = OverallQuality == 0 ? NSLOCTEXT("InkPlayerMenuPages", "CurrentLow", "当前：流畅")
		: OverallQuality == 1 ? NSLOCTEXT("InkPlayerMenuPages", "CurrentMedium", "当前：均衡")
		: OverallQuality == 2 ? NSLOCTEXT("InkPlayerMenuPages", "CurrentHigh", "当前：精致")
		: NSLOCTEXT("InkPlayerMenuPages", "CurrentCustomQuality", "当前：自定义");
	Body->AddChildToVerticalBox(MakeLabel(Tree, QualityValue, 22, TEXT("Batch2_QualityValue")))->SetPadding(FMargin(12.f, 0.f, 12.f, 8.f));

	const FName FrameAction = FMath::IsNearlyEqual(EffectiveFrameRateLimit, 30.f) ? FName(TEXT("Frame30"))
		: FMath::IsNearlyEqual(EffectiveFrameRateLimit, 60.f) ? FName(TEXT("Frame60")) : FName(NAME_None);
	AddGroup(NSLOCTEXT("InkPlayerMenuPages", "FrameRate", "帧率上限"), {
		{TEXT("Frame30"), NSLOCTEXT("InkPlayerMenuPages", "FPS30", "30 帧")},
		{TEXT("Frame60"), NSLOCTEXT("InkPlayerMenuPages", "FPS60", "60 帧")}}, FrameAction);
	const FText FrameValue = EffectiveFrameRateLimit > 0.f
		? FText::Format(NSLOCTEXT("InkPlayerMenuPages", "CurrentFrameLimit", "当前上限：{0} 帧"), FText::AsNumber(FMath::RoundToInt(EffectiveFrameRateLimit)))
		: NSLOCTEXT("InkPlayerMenuPages", "UnlimitedFrames", "当前：不限制");
	Body->AddChildToVerticalBox(MakeLabel(Tree, FrameValue, 22, TEXT("Batch2_FrameValue")))->SetPadding(FMargin(12.f, 0.f, 12.f, 8.f));

	auto* MotionCard = Tree->ConstructWidget<UBorder>();
	MotionCard->SetBrush(CardBrush());
	MotionCard->SetPadding(FMargin(16.f));
	auto* MotionRow = Tree->ConstructWidget<UHorizontalBox>();
	MotionCard->SetContent(MotionRow);
	MotionRow->AddChildToHorizontalBox(MakeLabel(Tree, NSLOCTEXT("InkPlayerMenuPages", "ReducedMotion", "减少动态效果"), 26));
	auto* MotionButton = MakeAction(Tree, Frontend, TEXT("ToggleReducedMotion"),
		NSLOCTEXT("InkPlayerMenuPages", "Toggle", "切换"));
	MotionButton->SetStyle(ActionStyle(bReducedMotion));
	AddAction(MotionRow, MotionButton);
	if (auto* Slot = Body->AddChildToVerticalBox(MotionCard)) Slot->SetPadding(FMargin(0.f, 8.f));

	auto* Footer = Tree->ConstructWidget<UHorizontalBox>();
	AddAction(Footer, MakeAction(Tree, Frontend, TEXT("ApplySettings"),
		NSLOCTEXT("InkPlayerMenuPages", "ApplySettings", "应用并保存")));
	Body->AddChildToVerticalBox(Footer);
	return Body;
}
