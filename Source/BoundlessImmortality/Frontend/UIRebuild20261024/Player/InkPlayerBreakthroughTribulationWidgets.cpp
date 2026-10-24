#include "InkPlayerBreakthroughTribulationWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"

namespace
{
	using namespace InkPlayerBreakthroughTribulationWidgets;

	FSlateBrush SurfaceBrush(const FLinearColor Fill, const FStyleTokens& Tokens, const float Stroke = 1.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = Fill;
		Brush.OutlineSettings.Color = Tokens.Accent;
		Brush.OutlineSettings.Width = Stroke;
		return Brush;
	}

	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Value, const FSlateFontInfo& Font,
		const FLinearColor Color, const ETextJustify::Type Justification = ETextJustify::Left)
	{
		auto* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(Value);
		Text->SetFont(Font);
		Text->SetColorAndOpacity(Color);
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(true);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Text;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FText& Caption, const FStyleTokens& Tokens,
		FActionRequested OnAction, const FName ActionId, const FActionPayload& Payload = FActionPayload(),
		const bool bEnabled = true, const bool bPrimary = false, const FText& DisabledReason = FText::GetEmpty())
	{
		auto* Button = Tree->ConstructWidget<UInkPlayerEActionButton>();
		Button->SetActionRequest(OnAction, ActionId, Payload);
		Button->OnClicked.AddDynamic(Button, &UInkPlayerEActionButton::HandleClicked);
		FButtonStyle ButtonStyle;
		ButtonStyle.SetNormal(SurfaceBrush(bPrimary && bEnabled ? Tokens.Primary : Tokens.Paper, Tokens, 1.5f));
		ButtonStyle.SetHovered(SurfaceBrush(bPrimary ? Tokens.Jade : Tokens.Page, Tokens, 2.f));
		ButtonStyle.SetPressed(SurfaceBrush(Tokens.Secondary, Tokens, 2.f));
		ButtonStyle.SetDisabled(SurfaceBrush(Tokens.Paper, Tokens));
		ButtonStyle.SetNormalPadding(FMargin(0.f));
		Button->SetStyle(ButtonStyle);
		Button->SetIsEnabled(bEnabled);
		auto* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetMinDesiredWidth(Tokens.MinimumHitTarget);
		Size->SetMinDesiredHeight(Tokens.MinimumHitTarget);
		const FText LabelText = !bEnabled && !DisabledReason.IsEmpty()
			? FText::Format(NSLOCTEXT("InkPlayerE", "DisabledButton", "{0}\n{1}"), Caption, DisabledReason)
			: Caption;
		auto* Label = MakeText(Tree, LabelText, Tokens.ButtonFont,
			!bEnabled ? Tokens.Disabled : bPrimary ? Tokens.Paper : Tokens.Ink, ETextJustify::Center);
		if (!bEnabled && !DisabledReason.IsEmpty()) Label->SetToolTipText(DisabledReason);
		Size->SetContent(Label);
		Button->SetContent(Size);
		return Button;
	}

	UBorder* MakeCard(UWidgetTree* Tree, const FStyleTokens& Tokens, UWidget* Content)
	{
		auto* Card = Tree->ConstructWidget<UBorder>();
		Card->SetBrush(SurfaceBrush(Tokens.Paper, Tokens));
		Card->SetPadding(FMargin(Tokens.ContentInset));
		Card->SetContent(Content);
		return Card;
	}

	void AddGap(UWidgetTree* Tree, UVerticalBox* Box, const float Height = 8.f)
	{
		auto* Gap = Tree->ConstructWidget<USizeBox>();
		Gap->SetHeightOverride(Height);
		Box->AddChild(Gap);
	}

	FText ProviderText(const FProviderSlice& Provider)
	{
		if (!Provider.Message.IsEmpty()) return Provider.Message;
		switch (Provider.State)
		{
		case EProviderState::Unknown: return NSLOCTEXT("InkPlayerE", "Unknown", "数据尚未提供");
		case EProviderState::Loading: return NSLOCTEXT("InkPlayerE", "Loading", "正在同步");
		case EProviderState::Empty: return NSLOCTEXT("InkPlayerE", "Empty", "服务已返回空记录");
		case EProviderState::Pending: return NSLOCTEXT("InkPlayerE", "Pending", "请求处理中，保留当前选择");
		case EProviderState::Offline: return NSLOCTEXT("InkPlayerE", "Offline", "离线 · 快照可能过期");
		case EProviderState::Error: return NSLOCTEXT("InkPlayerE", "Error", "读取失败");
		case EProviderState::Disabled: return NSLOCTEXT("InkPlayerE", "Disabled", "当前状态下不可用");
		default: return FText::GetEmpty();
		}
	}

	void AddProvider(UWidgetTree* Tree, UVerticalBox* Box, const FProviderSlice& Provider,
		const FStyleTokens& Tokens)
	{
		if (Provider.State == EProviderState::Data) return;
		auto* Message = MakeText(Tree, ProviderText(Provider), Tokens.BodyFont,
			Provider.State == EProviderState::Error ? Tokens.Warning : Tokens.Muted);
		Box->AddChild(MakeCard(Tree, Tokens, Message));
	}

	FText KnownText(const bool bKnown, const FText& Value)
	{
		return bKnown ? Value : NSLOCTEXT("InkPlayerE", "FieldUnknown", "尚未提供");
	}

	FText ItemLabel(const FName ItemId, const FText& Name)
	{
		if (!Name.IsEmpty()) return Name;
		return ItemId.IsNone() ? NSLOCTEXT("InkPlayerE", "ItemUnknown", "物品尚未提供") : FText::FromName(ItemId);
	}

	void AddField(UWidgetTree* Tree, UVerticalBox* Box, const FText& Label, const FText& Value,
		const FStyleTokens& Tokens)
	{
		auto* Row = Tree->ConstructWidget<UHorizontalBox>();
		Row->AddChildToHorizontalBox(MakeText(Tree, Label, Tokens.SecondaryFont, Tokens.Muted));
		Row->AddChildToHorizontalBox(MakeText(Tree, Value, Tokens.BodyFont, Tokens.Ink));
		Box->AddChild(MakeCard(Tree, Tokens, Row));
		AddGap(Tree, Box, 4.f);
	}

	void AddHeader(UWidgetTree* Tree, UVerticalBox* Page, const FText& Title,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		auto* Header = Tree->ConstructWidget<UHorizontalBox>();
		Header->AddChildToHorizontalBox(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "Back", "返回"),
			Tokens, OnAction, TEXT("ui.page.back")));
		Header->AddChildToHorizontalBox(MakeText(Tree, Title, Tokens.TitleFont, Tokens.Primary));
		Page->AddChild(Header);
	}

	void AddInventory(UWidgetTree* Tree, UVerticalBox* Box, const FInventoryVm& Inventory,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		auto* InventoryContent = Tree->ConstructWidget<UVerticalBox>();
		InventoryContent->AddChildToVerticalBox(MakeText(Tree,
			NSLOCTEXT("InkPlayerE", "InventoryHeading", "背包 · 只读记录"), Tokens.SectionFont, Tokens.Primary));
		AddProvider(Tree, InventoryContent, Inventory.State, Tokens);
		for (const FInventoryRow& Entry : Inventory.Entries)
		{
			auto* Row = Tree->ConstructWidget<UHorizontalBox>();
			const FText Count = FText::AsNumber(Entry.Count);
			Row->AddChildToHorizontalBox(MakeText(Tree,
				FText::Format(NSLOCTEXT("InkPlayerE", "InventoryEntry", "{0} × {1}"),
					ItemLabel(Entry.ItemId, Entry.Name), Count), Tokens.SecondaryFont, Tokens.Muted));
			FActionPayload AddMaterial; AddMaterial.ItemId = Entry.ItemId; AddMaterial.CountDelta = 1;
			Row->AddChildToHorizontalBox(MakeButton(Tree,
				NSLOCTEXT("InkPlayerE", "AddMaterial", "加入材料选择"), Tokens, OnAction,
				TEXT("ui.breakthrough.add-material"), AddMaterial,
				Entry.Count > 0 && !Entry.ItemId.IsNone()));
			InventoryContent->AddChildToVerticalBox(MakeCard(Tree, Tokens, Row));
		}
		Box->AddChild(MakeCard(Tree, Tokens, InventoryContent));
	}

	void AddBreakthroughSelection(UWidgetTree* Tree, UVerticalBox* Box, const FBreakthroughVm& Vm,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		AddProvider(Tree, Box, Vm.State, Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Stage", "当前阶段"),
			Vm.Stage.bKnown ? ItemLabel(Vm.Stage.StageId, FText::GetEmpty())
				: NSLOCTEXT("InkPlayerE", "StageUnknown", "尚未提供"), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "FromRealm", "当前境界序号"),
			Vm.Stage.bFromRealmKnown ? FText::AsNumber(Vm.Stage.FromRealmIndex)
				: NSLOCTEXT("InkPlayerE", "RealmUnknown", "尚未提供"), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "ToRealm", "目标境界序号"),
			Vm.Stage.bToRealmKnown ? FText::AsNumber(Vm.Stage.ToRealmIndex)
				: NSLOCTEXT("InkPlayerE", "RealmUnknown2", "尚未提供"), Tokens);

		Box->AddChild(MakeText(Tree, NSLOCTEXT("InkPlayerE", "Materials", "材料选择"),
			Tokens.SectionFont, Tokens.Primary));
		AddProvider(Tree, Box, Vm.SelectionState, Tokens);
		if (Vm.Selection.IsEmpty() && Vm.SelectionState.State == EProviderState::Empty)
			Box->AddChild(MakeCard(Tree, Tokens, MakeText(Tree,
				NSLOCTEXT("InkPlayerE", "NoMaterials", "当前没有已选材料"), Tokens.BodyFont, Tokens.Muted)));
		for (const FBreakthroughMaterial& Material : Vm.Selection)
		{
			auto* Row = Tree->ConstructWidget<UHorizontalBox>();
			Row->AddChildToHorizontalBox(MakeText(Tree,
				ItemLabel(Material.ItemId, Material.Name), Tokens.BodyFont, Tokens.Ink));
			const FText Count = Material.bCountKnown ? FText::AsNumber(Material.Count)
				: NSLOCTEXT("InkPlayerE", "CountUnknown", "数量未提供");
			Row->AddChildToHorizontalBox(MakeText(Tree, Count, Tokens.NumberFont, Tokens.Primary));
			FActionPayload Select; Select.ItemId = Material.ItemId;
			Select.SelectionRevision = Vm.SelectionRevision;
			Row->AddChildToHorizontalBox(MakeButton(Tree,
				Material.ItemId == Vm.SelectedItemId ? NSLOCTEXT("InkPlayerE", "Selected", "已选")
					: NSLOCTEXT("InkPlayerE", "Select", "选择"), Tokens, OnAction,
				TEXT("ui.breakthrough.select-material"), Select, !Material.ItemId.IsNone(),
				Material.ItemId == Vm.SelectedItemId));
			FActionPayload Decrease = Select; Decrease.CountDelta = -1;
			Row->AddChildToHorizontalBox(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "Decrease", "减少"),
				Tokens, OnAction, TEXT("ui.breakthrough.adjust-count"), Decrease, Material.bCountKnown && Material.Count > 0));
			FActionPayload Increase = Select; Increase.CountDelta = 1;
			Row->AddChildToHorizontalBox(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "Increase", "增加"),
				Tokens, OnAction, TEXT("ui.breakthrough.adjust-count"), Increase, Material.bCountKnown));
			Box->AddChild(MakeCard(Tree, Tokens, Row));
			AddGap(Tree, Box);
		}
	}

	void AddQuote(UWidgetTree* Tree, UVerticalBox* Box, const FBreakthroughQuote& Quote,
		const FStyleTokens& Tokens)
	{
		Box->AddChild(MakeText(Tree, NSLOCTEXT("InkPlayerE", "QuoteHeading", "实际报价"),
			Tokens.SectionFont, Tokens.Primary));
		AddProvider(Tree, Box, Quote.State, Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Q", "Q"), KnownText(Quote.bQKnown, Quote.Q), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Band", "档位"), KnownText(Quote.bBandKnown, Quote.Band), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Kind", "类型"), KnownText(Quote.bKindKnown, Quote.Kind), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Quality", "品质"), KnownText(Quote.bQualityKnown, Quote.Quality), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "NormalizedQ", "归一化 Q"),
			KnownText(Quote.bNormalizedQKnown, Quote.NormalizedQ), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Readiness", "准备状态"),
			KnownText(Quote.bReadinessKnown, Quote.Readiness), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Slots", "材料槽"),
			KnownText(Quote.bSlotsKnown, Quote.Slots), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Sources", "材料来源"),
			KnownText(Quote.bSourcesKnown, Quote.Sources), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Growth", "成长范围"),
			KnownText(Quote.bGrowthRangeKnown, Quote.GrowthRange), Tokens);
		if (Quote.bForecastKnown && !Quote.OptionalForecast.IsEmpty())
		{
			for (const FText& Forecast : Quote.OptionalForecast)
				Box->AddChild(MakeText(Tree, Forecast, Tokens.SecondaryFont, Tokens.Muted));
		}
		else AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Forecast", "可选预览"),
			Quote.bForecastKnown ? NSLOCTEXT("InkPlayerE", "ForecastEmpty", "服务已返回空预览")
				: NSLOCTEXT("InkPlayerE", "ForecastUnknown", "尚未提供"), Tokens);
	}

	FText PhaseName(const FTribulationVm& Vm)
	{
		if (!Vm.PhaseLabel.IsEmpty()) return Vm.PhaseLabel;
		switch (Vm.Phase)
		{
		case ETribulationPhase::Prepare: return NSLOCTEXT("InkPlayerE", "PhasePrepare", "准备");
		case ETribulationPhase::Trial: return NSLOCTEXT("InkPlayerE", "PhaseTrial", "天劫进行中");
		case ETribulationPhase::Reconcile: return NSLOCTEXT("InkPlayerE", "PhaseReconcile", "结果对账");
		case ETribulationPhase::Succeeded: return NSLOCTEXT("InkPlayerE", "PhaseSucceeded", "已成功");
		case ETribulationPhase::Failed: return NSLOCTEXT("InkPlayerE", "PhaseFailed", "已失败");
		case ETribulationPhase::Cancelled: return NSLOCTEXT("InkPlayerE", "PhaseCancelled", "已取消");
		default: return NSLOCTEXT("InkPlayerE", "PhaseUnknown", "阶段尚未提供");
		}
	}

	void AddTrialPhase(UWidgetTree* Tree, UVerticalBox* Box, const FTribulationVm& Vm,
		const FStyleTokens& Tokens)
	{
		AddProvider(Tree, Box, Vm.State, Tokens);
		auto* PhaseCard = MakeCard(Tree, Tokens, MakeText(Tree, PhaseName(Vm), Tokens.TitleFont,
			Vm.Phase == ETribulationPhase::Failed ? Tokens.Warning : Tokens.Primary));
		Box->AddChild(PhaseCard);
	}

	void AddTrialDetails(UWidgetTree* Tree, UVerticalBox* Box, const FTribulationVm& Vm,
		const FStyleTokens& Tokens)
	{
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Attempt", "本次尝试"),
			Vm.bAttemptKnown && !Vm.AttemptId.IsNone() ? FText::FromName(Vm.AttemptId)
				: NSLOCTEXT("InkPlayerE", "AttemptUnknown", "尚未提供"), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "TrialStart", "开始时间"),
			KnownText(Vm.bTrialStartKnown, Vm.TrialStart), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Deadline", "截止时间"),
			KnownText(Vm.bDeadlineKnown, Vm.Deadline), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Cleanup", "清理时间"),
			KnownText(Vm.bCleanupAtKnown, Vm.CleanupAt), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Helpers", "助力人数"),
			Vm.bHelperCountKnown ? FText::AsNumber(Vm.HelperCount)
				: NSLOCTEXT("InkPlayerE", "HelpersUnknown", "尚未提供"), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "Probability", "概率基点"),
			Vm.bProbabilityKnown ? FText::AsNumber(Vm.ProbabilityBasisPoints)
				: NSLOCTEXT("InkPlayerE", "ProbabilityUnknown", "尚未提供"), Tokens);
		AddField(Tree, Box, NSLOCTEXT("InkPlayerE", "ServerUtc", "服务端时间 UTC"),
			KnownText(Vm.bServerUtcKnown, Vm.ServerUtc), Tokens);
		auto* Settlement = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
			TEXT("TribulationSettlementCard"));
		Settlement->AddChildToVerticalBox(MakeText(Tree,
			NSLOCTEXT("InkPlayerE", "SettlementHeading", "对账结果"), Tokens.SectionFont, Tokens.Primary));
		Settlement->AddChildToVerticalBox(MakeText(Tree,
			FText::Format(NSLOCTEXT("InkPlayerE", "OutcomeRow", "唯一结算结果 · {0}"),
				KnownText(Vm.bOutcomeKnown, Vm.ExclusiveOutcome)), Tokens.BodyFont, Tokens.Ink));
		Settlement->AddChildToVerticalBox(MakeText(Tree,
			FText::Format(NSLOCTEXT("InkPlayerE", "ReceiptRow", "回执 · {0}"),
				Vm.bReceiptKnown && !Vm.ReceiptId.IsNone() ? FText::FromName(Vm.ReceiptId)
					: NSLOCTEXT("InkPlayerE", "ReceiptUnknown", "尚未提供")), Tokens.SecondaryFont, Tokens.Muted));
		Box->AddChild(MakeCard(Tree, Tokens, Settlement));
	}

	void AddTribulationActions(UWidgetTree* Tree, UVerticalBox* Box, const FTribulationVm& Vm,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		FActionPayload Reconcile; Reconcile.AttemptId = Vm.AttemptId;
		Box->AddChild(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "Reconcile", "刷新对账"),
			Tokens, OnAction, TEXT("reconcile-breakthrough"), Reconcile,
			Vm.bReconcileAvailable && Vm.Phase == ETribulationPhase::Reconcile
				&& Vm.bAttemptKnown && !Vm.AttemptId.IsNone(),
			true, Vm.ReconcileDisabledReason));

		FActionPayload Cancel; Cancel.AttemptId = Vm.AttemptId;
		Box->AddChild(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "CancelAttempt", "申请取消准备"),
			Tokens, OnAction, TEXT("ui.tribulation.request-cancel-confirmation"), Cancel,
			Vm.bCancelAvailable && Vm.Phase == ETribulationPhase::Prepare
				&& Vm.bAttemptKnown && !Vm.AttemptId.IsNone(), false,
			Vm.CancelDisabledReason));

		if (Vm.bCancelConfirmationOpen && Vm.ConfirmationAttemptId == Vm.AttemptId
			&& Vm.bAttemptKnown && Vm.Phase == ETribulationPhase::Prepare)
		{
			auto* Confirmation = Tree->ConstructWidget<UVerticalBox>();
			Confirmation->AddChildToVerticalBox(MakeText(Tree,
				NSLOCTEXT("InkPlayerE", "CancelConfirmText", "仅在服务尚未开始天劫时取消此准备记录"),
				Tokens.SectionFont, Tokens.Warning));
			FActionPayload Confirm; Confirm.AttemptId = Vm.AttemptId;
			Confirmation->AddChildToVerticalBox(MakeButton(Tree,
				NSLOCTEXT("InkPlayerE", "ConfirmCancel", "确认取消"), Tokens, OnAction,
				TEXT("cancel-breakthrough"), Confirm, Vm.bCancelAvailable, true,
				Vm.CancelDisabledReason));
			Confirmation->AddChildToVerticalBox(MakeButton(Tree,
				NSLOCTEXT("InkPlayerE", "DismissCancel", "保留准备"), Tokens, OnAction,
				TEXT("ui.tribulation.cancel-confirmation")));
			Box->AddChild(MakeCard(Tree, Tokens, Confirmation));
		}
	}
}

void UInkPlayerEActionButton::SetActionRequest(FActionRequested InAction, const FName InActionId,
	const InkPlayerBreakthroughTribulationWidgets::FActionPayload& InPayload)
{
	Action = InAction;
	ActionId = InActionId;
	Payload = InPayload;
}

void UInkPlayerEActionButton::HandleClicked()
{
	if (Action.IsBound()) Action.Execute(ActionId, Payload);
}

UWidget* InkPlayerBreakthroughTribulationWidgets::BuildBreakthroughPage(UWidgetTree* Tree,
	const FBreakthroughVm& ViewModel, const FInventoryVm& Inventory, const FStyleTokens& Style,
	const ELayoutMode Layout, FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BreakthroughPage"));
	AddHeader(Tree, Page, NSLOCTEXT("InkPlayerE", "BreakthroughTitle", "突破准备"), Style, OnAction);

	auto* Scroll = Tree->ConstructWidget<UScrollBox>();
	Scroll->SetOrientation(Orient_Vertical);
	auto* Footer = Tree->ConstructWidget<UVerticalBox>();
	auto* Left = Tree->ConstructWidget<UVerticalBox>();
	AddBreakthroughSelection(Tree, Left, ViewModel, Style, OnAction);
	AddInventory(Tree, Left, Inventory, Style, OnAction);
	auto* Right = Tree->ConstructWidget<UVerticalBox>();
	AddQuote(Tree, Right, ViewModel.Quote, Style);

	FActionPayload QuotePayload; QuotePayload.SelectionRevision = ViewModel.SelectionRevision;
	QuotePayload.bReworkMode = ViewModel.bReworkMode;
	for (const FBreakthroughMaterial& Material : ViewModel.Selection)
	{
		FMaterialSelection& Entry = QuotePayload.Selection.AddDefaulted_GetRef();
		Entry.ItemId = Material.ItemId; Entry.Count = Material.Count; Entry.bCountKnown = Material.bCountKnown;
	}
	Footer->AddChildToVerticalBox(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "Quote", "获取实际报价"),
		Style, OnAction, TEXT("quote-breakthrough"), QuotePayload, ViewModel.bQuoteAvailable,
		false, ViewModel.QuoteDisabledReason));

	FActionPayload ToggleRework; ToggleRework.bReworkMode = !ViewModel.bReworkMode;
	ToggleRework.SelectionRevision = ViewModel.SelectionRevision;
	Footer->AddChildToVerticalBox(MakeButton(Tree,
		ViewModel.bReworkMode ? NSLOCTEXT("InkPlayerE", "ReworkOn", "重整品质 · 开")
			: NSLOCTEXT("InkPlayerE", "ReworkOff", "重整品质 · 关"), Style, OnAction,
		TEXT("ui.breakthrough.set-rework-mode"), ToggleRework));

	FActionPayload Request; Request.bReworkMode = ViewModel.bReworkMode;
	Request.SelectionRevision = ViewModel.SelectionRevision;
	Request.StageSubjectRevision = ViewModel.StageSubjectRevision;
	for (const FBreakthroughMaterial& Material : ViewModel.Selection)
	{
		FMaterialSelection& Entry = Request.Selection.AddDefaulted_GetRef();
		Entry.ItemId = Material.ItemId; Entry.Count = Material.Count; Entry.bCountKnown = Material.bCountKnown;
	}
	Footer->AddChildToVerticalBox(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "RequestBreakthrough", "申请突破"),
		Style, OnAction, TEXT("ui.breakthrough.request-confirmation"), Request,
		ViewModel.bReworkMode ? ViewModel.bReworkAvailable : ViewModel.bPrepareAvailable, true,
		ViewModel.bReworkMode ? ViewModel.ReworkDisabledReason : ViewModel.PrepareDisabledReason));

	if (ViewModel.bConfirmationOpen
		&& ViewModel.ConfirmationStageSubjectRevision == ViewModel.StageSubjectRevision
		&& ViewModel.ConfirmationSelectionRevision == ViewModel.SelectionRevision
		&& ViewModel.bConfirmationReworkMode == ViewModel.bReworkMode)
	{
		auto* Confirm = Tree->ConstructWidget<UVerticalBox>();
		Confirm->AddChildToVerticalBox(MakeText(Tree,
			NSLOCTEXT("InkPlayerE", "ConfirmWarning", "确认后将提交当前材料选择，由服务重新报价并校验"),
			Style.SectionFont, Style.Warning));
		FActionPayload Submit = Request;
		Submit.StageSubjectRevision = ViewModel.ConfirmationStageSubjectRevision;
		Submit.SelectionRevision = ViewModel.ConfirmationSelectionRevision;
		Submit.bReworkMode = ViewModel.bConfirmationReworkMode;
		Confirm->AddChildToVerticalBox(MakeButton(Tree,
			ViewModel.bReworkMode ? NSLOCTEXT("InkPlayerE", "ConfirmRework", "确认重整")
				: NSLOCTEXT("InkPlayerE", "ConfirmPrepare", "确认申请"), Style, OnAction,
			ViewModel.bReworkMode ? TEXT("rework-quality") : TEXT("prepare-breakthrough"), Submit,
			ViewModel.bReworkMode ? ViewModel.bReworkAvailable : ViewModel.bPrepareAvailable, true,
			ViewModel.bReworkMode ? ViewModel.ReworkDisabledReason : ViewModel.PrepareDisabledReason));
		Confirm->AddChildToVerticalBox(MakeButton(Tree, NSLOCTEXT("InkPlayerE", "Cancel", "返回修改"),
			Style, OnAction, TEXT("ui.breakthrough.cancel-confirmation")));
		Footer->AddChildToVerticalBox(MakeCard(Tree, Style, Confirm));
	}

	if (Layout == ELayoutMode::Landscape)
	{
		auto* Columns = Tree->ConstructWidget<UHorizontalBox>();
		if(auto* ColumnLayout=Columns->AddChildToHorizontalBox(Left)){ColumnLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnLayout->SetPadding(FMargin(6,0));}
		if(auto* ColumnLayout=Columns->AddChildToHorizontalBox(Right)){ColumnLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnLayout->SetPadding(FMargin(6,0));}
		Scroll->AddChild(Columns);
	}
	else
	{
		Scroll->AddChild(Left);
		Scroll->AddChild(Right);
	}
	Page->AddChild(Scroll);
	Page->AddChild(Footer);
	auto* Frame = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BreakthroughPageFrame"));
	Frame->SetBrush(SurfaceBrush(Style.Page, Style));
	Frame->SetPadding(FMargin(Style.ContentInset));
	Frame->SetContent(Page);
	return Frame;
}

UWidget* InkPlayerBreakthroughTribulationWidgets::BuildTribulationPage(UWidgetTree* Tree,
	const FTribulationVm& ViewModel, const FStyleTokens& Style, const ELayoutMode Layout,
	FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TribulationPage"));
	AddHeader(Tree, Page, NSLOCTEXT("InkPlayerE", "TribulationTitle", "天劫助力"), Style, OnAction);
	auto* Scroll = Tree->ConstructWidget<UScrollBox>();
	Scroll->SetOrientation(Orient_Vertical);

	if (Layout == ELayoutMode::Landscape)
	{
		auto* Columns = Tree->ConstructWidget<UHorizontalBox>();
		auto* StatusColumn = Tree->ConstructWidget<UVerticalBox>();
		AddTrialPhase(Tree, StatusColumn, ViewModel, Style);
		auto* SettlementColumn = Tree->ConstructWidget<UVerticalBox>();
		AddTrialDetails(Tree, SettlementColumn, ViewModel, Style);
		if(auto* ColumnLayout=Columns->AddChildToHorizontalBox(StatusColumn)){ColumnLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnLayout->SetPadding(FMargin(6,0));}
		if(auto* ColumnLayout=Columns->AddChildToHorizontalBox(SettlementColumn)){ColumnLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnLayout->SetPadding(FMargin(6,0));}
		Scroll->AddChild(Columns);
	}
	else
	{
		auto* Main = Tree->ConstructWidget<UVerticalBox>();
		AddTrialPhase(Tree, Main, ViewModel, Style);
		AddTrialDetails(Tree, Main, ViewModel, Style);
		Scroll->AddChild(Main);
	}
	Page->AddChild(Scroll);
	auto* Footer = Tree->ConstructWidget<UVerticalBox>();
	AddTribulationActions(Tree, Footer, ViewModel, Style, OnAction);
	Page->AddChild(Footer);
	auto* Frame = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TribulationPageFrame"));
	Frame->SetBrush(SurfaceBrush(Style.Page, Style));
	Frame->SetPadding(FMargin(Style.ContentInset));
	Frame->SetContent(Page);
	return Frame;
}
