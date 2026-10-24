#include "InkPlayerCraftSourcesWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"

namespace
{
	using namespace InkPlayerCraftSourcesWidgets;

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
		auto* Block = Tree->ConstructWidget<UTextBlock>();
		Block->SetText(Value);
		Block->SetFont(Font);
		Block->SetColorAndOpacity(Color);
		Block->SetJustification(Justification);
		Block->SetAutoWrapText(true);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FText& Caption, const FStyleTokens& Tokens,
		FActionRequested OnAction, const FName ActionId, const FActionPayload& Payload = FActionPayload(),
		const bool bEnabled = true, const bool bPrimary = false, const FText& DisabledReason = FText::GetEmpty())
	{
		auto* Button = Tree->ConstructWidget<UInkPlayerDActionButton>();
		Button->SetActionRequest(OnAction, ActionId, Payload);
		Button->OnClicked.AddDynamic(Button, &UInkPlayerDActionButton::HandleClicked);
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
		const FText VisibleCaption = !bEnabled && !DisabledReason.IsEmpty()
			? FText::Format(NSLOCTEXT("InkPlayerD", "DisabledCaption", "{0}\n{1}"), Caption, DisabledReason)
			: Caption;
		const FLinearColor CaptionColor = !bEnabled ? Tokens.Disabled : bPrimary ? Tokens.Paper : Tokens.Ink;
		auto* Label = MakeText(Tree, VisibleCaption, Tokens.ButtonFont, CaptionColor, ETextJustify::Center);
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

	FText ProviderText(const FProviderSlice& Slice)
	{
		if (!Slice.Message.IsEmpty()) return Slice.Message;
		switch (Slice.State)
		{
		case EProviderState::Unknown: return NSLOCTEXT("InkPlayerD", "Unknown", "数据尚未提供");
		case EProviderState::Loading: return NSLOCTEXT("InkPlayerD", "Loading", "正在同步");
		case EProviderState::Empty: return NSLOCTEXT("InkPlayerD", "Empty", "服务已返回空记录");
		case EProviderState::Pending: return NSLOCTEXT("InkPlayerD", "Pending", "请求处理中，保留当前选择");
		case EProviderState::Offline: return NSLOCTEXT("InkPlayerD", "Offline", "离线 · 快照可能过期");
		case EProviderState::Error: return NSLOCTEXT("InkPlayerD", "Error", "读取失败");
		case EProviderState::Disabled: return NSLOCTEXT("InkPlayerD", "Disabled", "当前状态下不可用");
		default: return FText::GetEmpty();
		}
	}

	void AddProvider(UWidgetTree* Tree, UVerticalBox* Box, const FProviderSlice& Slice,
		const FStyleTokens& Tokens)
	{
		if (Slice.State == EProviderState::Data) return;
		auto* Message = MakeText(Tree, ProviderText(Slice), Tokens.BodyFont,
			Slice.State == EProviderState::Error ? Tokens.Warning : Tokens.Muted);
		Box->AddChild(MakeCard(Tree, Tokens, Message));
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

	FText CountText(const bool bKnown, const int64 Count)
	{
		return bKnown ? FText::AsNumber(Count) : NSLOCTEXT("InkPlayerD", "UnknownCount", "数量未提供");
	}

	FText DisplayId(const FName Id)
	{
		return Id.IsNone() ? NSLOCTEXT("InkPlayerD", "ValueUnknown", "尚未提供") : FText::FromName(Id);
	}

	void AddHeader(UWidgetTree* Tree, UVerticalBox* Page, const FText& Title,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		auto* Header = Tree->ConstructWidget<UHorizontalBox>();
		FActionPayload Empty;
		Header->AddChildToHorizontalBox(MakeButton(Tree,
			NSLOCTEXT("InkPlayerD", "Back", "返回"), Tokens, OnAction, TEXT("ui.page.back"), Empty));
		Header->AddChildToHorizontalBox(MakeText(Tree, Title, Tokens.TitleFont, Tokens.Primary));
		Page->AddChild(Header);
		AddGap(Tree, Page, 12.f);
	}

	void AddRecipeCatalog(UWidgetTree* Tree, UVerticalBox* Column, const FRecipeVm& Vm,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		AddProvider(Tree, Column, Vm.CatalogState, Tokens);
		for (const FRecipeRow& Recipe : Vm.Recipes)
		{
			auto* Content = Tree->ConstructWidget<UVerticalBox>();
			Content->AddChildToVerticalBox(MakeText(Tree, Recipe.Name.IsEmpty()
				? DisplayId(Recipe.RecipeId) : Recipe.Name, Tokens.SectionFont, Tokens.Ink));
			if (!Recipe.Summary.IsEmpty())
				Content->AddChildToVerticalBox(MakeText(Tree, Recipe.Summary, Tokens.SecondaryFont, Tokens.Muted));
			FActionPayload Select; Select.RecipeId = Recipe.RecipeId;
			Content->AddChildToVerticalBox(MakeButton(Tree,
				NSLOCTEXT("InkPlayerD", "SelectRecipe", "查看配方"), Tokens, OnAction,
				TEXT("ui.craft.select-recipe"), Select, true, Recipe.RecipeId == Vm.SelectedRecipeId));
			Column->AddChild(MakeCard(Tree, Tokens, Content));
			AddGap(Tree, Column);
		}
	}

	void AddInventoryCrossCheck(UWidgetTree* Tree, UVerticalBox* Column, const FInventoryVm& Vm,
		const FStyleTokens& Tokens)
	{
		auto* Section = Tree->ConstructWidget<UVerticalBox>();
		Section->AddChildToVerticalBox(MakeText(Tree,
			NSLOCTEXT("InkPlayerD", "InventoryTitle", "背包只读记录"), Tokens.SectionFont, Tokens.Primary));
		AddProvider(Tree, Section, Vm.State, Tokens);
		for (const FInventoryEntry& Entry : Vm.Entries)
		{
			const FText ItemName = Entry.Name.IsEmpty() ? DisplayId(Entry.ItemId) : Entry.Name;
			const FText Value = FText::Format(NSLOCTEXT("InkPlayerD", "InventoryCount", "{0} × {1}"),
				ItemName, FText::AsNumber(Entry.Count));
			Section->AddChildToVerticalBox(MakeText(Tree, Value, Tokens.SecondaryFont, Tokens.Muted));
		}
		Column->AddChild(MakeCard(Tree, Tokens, Section));
	}

	void AddCraftDetail(UWidgetTree* Tree, UVerticalBox* Column, const FRecipeVm& Vm,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		AddProvider(Tree, Column, Vm.SelectionState, Tokens);
		if (Vm.SelectedRecipeId.IsNone())
		{
			Column->AddChild(MakeCard(Tree, Tokens, MakeText(Tree,
				NSLOCTEXT("InkPlayerD", "NoRecipeSelected", "选择一条实际配方以查看详情"), Tokens.BodyFont, Tokens.Muted)));
			return;
		}

		Column->AddChild(MakeText(Tree,
			FText::Format(NSLOCTEXT("InkPlayerD", "RecipeId", "配方 · {0}"), DisplayId(Vm.SelectedRecipeId)),
			Tokens.SectionFont, Tokens.Primary));
		AddProvider(Tree, Column, Vm.IngredientsState, Tokens);
		for (const FRecipeIngredient& Ingredient : Vm.Ingredients)
		{
			const FText Name = Ingredient.Name.IsEmpty() ? DisplayId(Ingredient.ItemId) : Ingredient.Name;
			AddField(Tree, Column, Name, CountText(Ingredient.bCountKnown, Ingredient.Count), Tokens);
		}
		AddProvider(Tree, Column, Vm.OutputState, Tokens);
		const FText OutputName = Vm.Output.Name.IsEmpty() ? DisplayId(Vm.Output.ItemId) : Vm.Output.Name;
		AddField(Tree, Column, NSLOCTEXT("InkPlayerD", "Output", "实际产出"),
			OutputName.IsEmpty() ? CountText(false, 0) : FText::Format(
				NSLOCTEXT("InkPlayerD", "OutputCount", "{0} × {1}"), OutputName,
				CountText(Vm.Output.bCountKnown, Vm.Output.Count)), Tokens);

	}

	void AddCraftFooter(UWidgetTree* Tree, UVerticalBox* Footer, const FRecipeVm& Vm,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		FActionPayload Request; Request.RecipeId = Vm.SelectedRecipeId;
		const bool bSelectedRecipe = !Vm.SelectedRecipeId.IsNone();
		Footer->AddChildToVerticalBox(MakeButton(Tree,
			NSLOCTEXT("InkPlayerD", "RequestCraft", "申请炼制"), Tokens, OnAction,
			TEXT("ui.craft.request-confirmation"), Request,
			Vm.bCraftAvailable && bSelectedRecipe, true, Vm.CraftDisabledReason));
		if (Vm.bCraftConfirmationOpen && Vm.ConfirmationRecipeId == Vm.SelectedRecipeId && bSelectedRecipe)
		{
			auto* Confirmation = Tree->ConstructWidget<UVerticalBox>();
			Confirmation->AddChildToVerticalBox(MakeText(Tree,
				NSLOCTEXT("InkPlayerD", "ConfirmDanger", "确认后将提交给服务校验并执行炼制"),
				Tokens.SectionFont, Tokens.Warning));
			if (!Vm.ConfirmationSummary.IsEmpty())
				Confirmation->AddChildToVerticalBox(MakeText(Tree, Vm.ConfirmationSummary, Tokens.BodyFont, Tokens.Ink));
			FActionPayload Confirm; Confirm.RecipeId = Vm.ConfirmationRecipeId;
			Confirmation->AddChildToVerticalBox(MakeButton(Tree,
				NSLOCTEXT("InkPlayerD", "ConfirmCraft", "确认提交"), Tokens, OnAction,
				TEXT("craft"), Confirm, Vm.bCraftAvailable, true, Vm.CraftDisabledReason));
			Confirmation->AddChildToVerticalBox(MakeButton(Tree,
				NSLOCTEXT("InkPlayerD", "CancelCraft", "取消"), Tokens, OnAction,
				TEXT("ui.craft.cancel-confirmation")));
			Footer->AddChildToVerticalBox(MakeCard(Tree, Tokens, Confirmation));
		}
	}
}

void UInkPlayerDActionButton::SetActionRequest(FActionRequested InAction, const FName InActionId,
	const InkPlayerCraftSourcesWidgets::FActionPayload& InPayload)
{
	Action = InAction;
	ActionId = InActionId;
	Payload = InPayload;
}

void UInkPlayerDActionButton::HandleClicked()
{
	if (Action.IsBound()) Action.Execute(ActionId, Payload);
}

UWidget* InkPlayerCraftSourcesWidgets::BuildCraftPage(UWidgetTree* Tree, const FRecipeVm& Recipe,
	const FInventoryVm& Inventory, const FStyleTokens& Style, const ELayoutMode Layout,
	FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftPage"));
	AddHeader(Tree, Page, NSLOCTEXT("InkPlayerD", "CraftHeading", "物品炼制 · 配方与实际产出"), Style, OnAction);

	FActionPayload Query; Query.Offset = Recipe.QueryOffset; Query.bOwnedOnly = Recipe.bOwnedOnly;
	auto* QueryRow = Tree->ConstructWidget<UHorizontalBox>();
	QueryRow->AddChildToHorizontalBox(MakeButton(Tree,
		NSLOCTEXT("InkPlayerD", "QueryRecipes", "刷新配方"), Style, OnAction,
		TEXT("query-recipes"), Query, Recipe.bQueryAvailable, false, Recipe.CatalogState.Message));
	FActionPayload Toggle; Toggle.Offset = Recipe.QueryOffset; Toggle.bOwnedOnly = !Recipe.bOwnedOnly;
	QueryRow->AddChildToHorizontalBox(MakeButton(Tree,
		Recipe.bOwnedOnly ? NSLOCTEXT("InkPlayerD", "OwnedOnly", "仅看已拥有 · 开")
			: NSLOCTEXT("InkPlayerD", "AllRecipes", "仅看已拥有 · 关"), Style, OnAction,
		TEXT("ui.craft.toggle-owned-only"), Toggle));
	Page->AddChild(QueryRow);

	auto* ContentScroll = Tree->ConstructWidget<UScrollBox>();
	ContentScroll->SetOrientation(Orient_Vertical);
	auto* Footer = Tree->ConstructWidget<UVerticalBox>();
	auto* Left = Tree->ConstructWidget<UVerticalBox>();
	AddRecipeCatalog(Tree, Left, Recipe, Style, OnAction);
	AddInventoryCrossCheck(Tree, Left, Inventory, Style);
	auto* Right = Tree->ConstructWidget<UVerticalBox>();
	AddCraftDetail(Tree, Right, Recipe, Style, OnAction);
	if (Layout == ELayoutMode::Landscape)
	{
		auto* Columns = Tree->ConstructWidget<UHorizontalBox>();
		Columns->AddChildToHorizontalBox(Left);
		Columns->AddChildToHorizontalBox(Right);
		ContentScroll->AddChild(Columns);
	}
	else
	{
		ContentScroll->AddChild(Left);
		ContentScroll->AddChild(Right);
	}
	Page->AddChild(ContentScroll);
	AddCraftFooter(Tree, Footer, Recipe, Style, OnAction);
	Page->AddChild(Footer);
	auto* Frame = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CraftPageFrame"));
	Frame->SetBrush(SurfaceBrush(Style.Page, Style));
	Frame->SetPadding(FMargin(Style.ContentInset));
	Frame->SetContent(Page);
	return Frame;
}

UWidget* InkPlayerCraftSourcesWidgets::BuildItemSourcesPage(UWidgetTree* Tree, const FItemSourceVm& ViewModel,
	const FStyleTokens& Style, const ELayoutMode Layout, FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemSourcesPage"));
	AddHeader(Tree, Page, NSLOCTEXT("InkPlayerD", "SourcesTitle", "物品来源"), Style, OnAction);
	Page->AddChild(MakeCard(Tree, Style, MakeText(Tree,
		ViewModel.ItemId.IsNone() ? NSLOCTEXT("InkPlayerD", "ItemUnknown", "物品尚未提供")
			: FText::Format(NSLOCTEXT("InkPlayerD", "SourcesForItem", "物品 · {0}"), DisplayId(ViewModel.ItemId)),
		Style.SectionFont, Style.Primary)));
	AddGap(Tree, Page, 12.f);

	FActionPayload Query; Query.Offset = ViewModel.QueryOffset; Query.bOwnedOnly = ViewModel.bOwnedOnly;
	auto* QueryRow = Tree->ConstructWidget<UHorizontalBox>();
	QueryRow->AddChildToHorizontalBox(MakeButton(Tree,
		NSLOCTEXT("InkPlayerD", "QuerySources", "刷新来源"), Style, OnAction,
		TEXT("query-sources"), Query, ViewModel.bQueryAvailable, false, ViewModel.QueryDisabledReason));
	FActionPayload Toggle; Toggle.Offset = ViewModel.QueryOffset; Toggle.bOwnedOnly = !ViewModel.bOwnedOnly;
	QueryRow->AddChildToHorizontalBox(MakeButton(Tree,
		ViewModel.bOwnedOnly ? NSLOCTEXT("InkPlayerD", "OwnedOnlyOn", "仅看已拥有 · 开")
			: NSLOCTEXT("InkPlayerD", "OwnedOnlyOff", "仅看已拥有 · 关"), Style, OnAction,
		TEXT("ui.sources.toggle-owned-only"), Toggle));
	Page->AddChild(QueryRow);

	auto* Content = Tree->ConstructWidget<UScrollBox>();
	Content->SetOrientation(Orient_Vertical);
	const FItemSourceRow* SelectedSource = ViewModel.Sources.FindByPredicate([&ViewModel](const FItemSourceRow& Row)
		{ return Row.SourceId == ViewModel.SelectedSourceId && Row.ItemId == ViewModel.ItemId; });
	auto* List = Tree->ConstructWidget<UVerticalBox>();
	AddProvider(Tree, List, ViewModel.State, Style);
	for (const FItemSourceRow& Row : ViewModel.Sources)
	{
		auto* Entry = Tree->ConstructWidget<UVerticalBox>();
		const FText SourceName = Row.Source.IsEmpty()
			? NSLOCTEXT("InkPlayerD", "SourceUnknown", "来源说明未提供") : Row.Source;
		Entry->AddChildToVerticalBox(MakeText(Tree, SourceName, Style.SectionFont, Style.Ink));
		Entry->AddChildToVerticalBox(MakeText(Tree,
			FText::Format(NSLOCTEXT("InkPlayerD", "SourceMethod", "方式 · {0}"), DisplayId(Row.Method)),
			Style.SecondaryFont, Style.Muted));
		if (!Row.Activity.IsEmpty())
			Entry->AddChildToVerticalBox(MakeText(Tree, Row.Activity, Style.BodyFont, Style.Ink));
		FActionPayload Select; Select.SourceId = Row.SourceId; Select.ItemId = Row.ItemId;
		Entry->AddChildToVerticalBox(MakeButton(Tree,
			Row.SourceId == ViewModel.SelectedSourceId
				? NSLOCTEXT("InkPlayerD", "SelectedSource", "已选来源")
				: NSLOCTEXT("InkPlayerD", "SelectSource", "查看此来源"), Style, OnAction,
			TEXT("ui.sources.select-entry"), Select, true, Row.SourceId == ViewModel.SelectedSourceId));
		List->AddChild(MakeCard(Tree, Style, Entry));
		AddGap(Tree, List);
	}
	if (Layout == ELayoutMode::Landscape)
	{
		auto* Columns = Tree->ConstructWidget<UHorizontalBox>();
		Columns->AddChildToHorizontalBox(List);
		auto* Detail = Tree->ConstructWidget<UVerticalBox>();
		AddProvider(Tree, Detail, ViewModel.State, Style);
		if (SelectedSource)
		{
			Detail->AddChildToVerticalBox(MakeText(Tree, SelectedSource->Source, Style.SectionFont, Style.Primary));
			if (!SelectedSource->Activity.IsEmpty()) Detail->AddChildToVerticalBox(MakeText(Tree, SelectedSource->Activity, Style.BodyFont, Style.Ink));
		}
		Columns->AddChildToHorizontalBox(Detail);
		Content->AddChild(Columns);
	}
	else Content->AddChild(List);
	Page->AddChild(Content);
	if (SelectedSource && SelectedSource->Method == TEXT("fixed_selected_reward")
		&& SelectedSource->ItemId == ViewModel.ItemId)
	{
		FActionPayload Focus; Focus.SourceId = SelectedSource->SourceId; Focus.ItemId = SelectedSource->ItemId;
		Page->AddChild(MakeButton(Tree, NSLOCTEXT("InkPlayerD", "ChooseFocus", "设为定向选择"),
			Style, OnAction, TEXT("choose-focus"), Focus, ViewModel.bChooseFocusAvailable,
			true, ViewModel.ChooseFocusDisabledReason));
	}
	auto* Frame = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemSourcesPageFrame"));
	Frame->SetBrush(SurfaceBrush(Style.Page, Style));
	Frame->SetPadding(FMargin(Style.ContentInset));
	Frame->SetContent(Page);
	return Frame;
}
