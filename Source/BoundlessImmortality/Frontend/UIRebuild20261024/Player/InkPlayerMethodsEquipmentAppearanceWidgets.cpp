#include "InkPlayerMethodsEquipmentAppearanceWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Frontend/InkInventoryModel.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	using namespace InkPlayerMethodsEquipmentAppearanceWidgets;

	FSlateBrush SurfaceBrush(const FLinearColor Fill, const FStyleTokens& Tokens, const float Stroke = 1.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = Fill;
		Brush.OutlineSettings.Color = Tokens.Accent;
		Brush.OutlineSettings.Width = Stroke;
		return Brush;
	}

	UTextBlock* Text(UWidgetTree* Tree, const FText& Value, const FSlateFontInfo& Font,
		const FLinearColor Color, const FName Name = NAME_None,
		const ETextJustify::Type Justification = ETextJustify::Left)
	{
		auto* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Widget->SetText(Value);
		Widget->SetFont(Font);
		Widget->SetColorAndOpacity(Color);
		Widget->SetJustification(Justification);
		Widget->SetAutoWrapText(true);
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Widget;
	}

	UButton* ActionButton(UWidgetTree* Tree, const FText& Caption, const FStyleTokens& Tokens,
		FActionRequested OnAction, const FName ActionId, const FActionPayload& Payload = FActionPayload(),
		const bool bEnabled = true, const bool bPrimary = false, const FText& DisabledReason = FText::GetEmpty())
	{
		auto* Button = Tree->ConstructWidget<UInkPlayerCActionButton>(UInkPlayerCActionButton::StaticClass());
		Button->SetActionRequest(OnAction, ActionId, Payload);
		Button->OnClicked.AddDynamic(Button, &UInkPlayerCActionButton::HandleClicked);
		FButtonStyle ButtonStyle;
		const FLinearColor Normal = bPrimary && bEnabled ? Tokens.Primary : Tokens.Paper;
		ButtonStyle.SetNormal(SurfaceBrush(Normal, Tokens, 1.5f));
		ButtonStyle.SetHovered(SurfaceBrush(bPrimary ? Tokens.Jade : Tokens.Page, Tokens, 2.f));
		ButtonStyle.SetPressed(SurfaceBrush(Tokens.Secondary, Tokens, 2.f));
		ButtonStyle.SetDisabled(SurfaceBrush(Tokens.Paper, Tokens, 1.f));
		ButtonStyle.SetNormalPadding(FMargin(0.f));
		ButtonStyle.SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
		Button->SetStyle(ButtonStyle);
		Button->SetIsEnabled(bEnabled);
		auto* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetMinDesiredWidth(Tokens.MinimumHitTarget);
		Size->SetMinDesiredHeight(Tokens.MinimumHitTarget);
		const FText VisibleCaption = !bEnabled && !DisabledReason.IsEmpty()
			? FText::Format(NSLOCTEXT("InkPlayerC", "DisabledButtonReason", "{0}\n{1}"), Caption, DisabledReason)
			: Caption;
		const FLinearColor CaptionColor = !bEnabled ? Tokens.Disabled : bPrimary ? Tokens.Paper : Tokens.Ink;
		auto* CaptionBlock = Text(Tree, VisibleCaption, Tokens.ButtonFont, CaptionColor, NAME_None, ETextJustify::Center);
		if (!bEnabled && !DisabledReason.IsEmpty()) CaptionBlock->SetToolTipText(DisabledReason);
		Size->SetContent(CaptionBlock);
		Button->SetContent(Size);
		return Button;
	}

	UImage* AtlasCell(UWidgetTree* Tree, UTexture2D* Atlas, const int32 Cell,
		const int32 Columns, const int32 Rows, const FVector2D Size)
	{
		if (!Atlas || Columns <= 0 || Rows <= 0 || Cell < 0 || Cell >= Columns * Rows) return nullptr;
		const int32 Column = Cell % Columns;
		const int32 Row = Cell / Columns;
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Atlas);
		Brush.SetImageSize(Size);
		Brush.SetUVRegion(FBox2D(FVector2D(float(Column) / Columns, float(Row) / Rows),
			FVector2D(float(Column + 1) / Columns, float(Row + 1) / Rows)));
		auto* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	FText StateText(const FProviderSlice& Slice)
	{
		if (!Slice.Message.IsEmpty()) return Slice.Message;
		switch (Slice.State)
		{
		case EProviderState::Unknown: return NSLOCTEXT("InkPlayerC", "Unknown", "数据尚未提供");
		case EProviderState::Loading: return NSLOCTEXT("InkPlayerC", "Loading", "正在同步");
		case EProviderState::Empty: return NSLOCTEXT("InkPlayerC", "Empty", "服务已返回空记录");
		case EProviderState::Pending: return NSLOCTEXT("InkPlayerC", "Pending", "请求处理中，已保留当前选择");
		case EProviderState::Offline: return NSLOCTEXT("InkPlayerC", "Offline", "离线 · 当前快照可能过期");
		case EProviderState::Error: return NSLOCTEXT("InkPlayerC", "Error", "读取失败");
		case EProviderState::Disabled: return NSLOCTEXT("InkPlayerC", "Disabled", "当前状态下不可用");
		default: return FText::GetEmpty();
		}
	}

	FText MethodNames(const TArray<FName>& Ids, const TArray<FCatalogEntry>& Catalog)
	{
		TArray<FText> Names;
		for (const FName Id : Ids)
		{
			if (Id.IsNone()) continue;
			const FCatalogEntry* Entry = Catalog.FindByPredicate([Id](const FCatalogEntry& Candidate)
				{ return Candidate.ItemId == Id; });
			Names.Add(Entry && !Entry->Name.IsEmpty() ? Entry->Name : FText::FromName(Id));
		}
		return FText::Join(NSLOCTEXT("InkPlayerC", "Separator", "、"), Names);
	}

	void AddProviderState(UWidgetTree* Tree, UVerticalBox* Parent, const FProviderSlice& Slice,
		const FStyleTokens& Tokens, const FName Name)
	{
		if (Slice.State == EProviderState::Data) return;
		auto* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Panel->SetBrush(SurfaceBrush(Tokens.Paper, Tokens));
		Panel->SetPadding(FMargin(Tokens.ContentInset));
		const FLinearColor Color = Slice.State == EProviderState::Error ? Tokens.Warning : Tokens.Muted;
		Panel->SetContent(Text(Tree, StateText(Slice), Tokens.BodyFont, Color));
		Parent->AddChildToVerticalBox(Panel)->SetPadding(FMargin(0.f, 8.f));
	}

	UBorder* Card(UWidgetTree* Tree, const FStyleTokens& Tokens, UWidget* Content = nullptr, const FName Name = NAME_None)
	{
		auto* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Panel->SetBrush(SurfaceBrush(Tokens.Paper, Tokens));
		Panel->SetPadding(FMargin(Tokens.ContentInset));
		if (Content) Panel->SetContent(Content);
		return Panel;
	}

	void AddHeader(UWidgetTree* Tree, UVerticalBox* Body, const FText& Title,
		const FStyleTokens& Tokens, const FActionRequested& OnAction, const FName PageName)
	{
		FActionPayload Empty;
		Body->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "Back", "返回"),
			Tokens, OnAction, TEXT("ui.page.back"), Empty))->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		Body->AddChildToVerticalBox(Text(Tree, Title, Tokens.TitleFont, Tokens.Ink,
			FName(*FString::Printf(TEXT("PlayerC_%s_Title"), *PageName.ToString()))))->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	void AddSectionHeading(UWidgetTree* Tree, UVerticalBox* Parent, const FText& Caption, const FStyleTokens& Tokens)
	{
		Parent->AddChildToVerticalBox(Text(Tree, Caption, Tokens.SectionFont, Tokens.Primary))
			->SetPadding(FMargin(0.f, 12.f, 0.f, 8.f));
	}

	void AddPageColumn(UWidgetTree* Tree, UVerticalBox* Body, UWidget* PrimaryColumn,
		UWidget* DetailColumn, const ELayoutMode Layout)
	{
		if (Layout == ELayoutMode::Landscape)
		{
			auto* Split = Tree->ConstructWidget<UHorizontalBox>();
			if (auto* Slot = Split->AddChildToHorizontalBox(PrimaryColumn))
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Slot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			}
			if (auto* Slot = Split->AddChildToHorizontalBox(DetailColumn)) Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Body->AddChildToVerticalBox(Split);
		}
		else
		{
			Body->AddChildToVerticalBox(PrimaryColumn)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			Body->AddChildToVerticalBox(DetailColumn);
		}
	}

	bool ContainsId(const TArray<FName>& Ids, const FName Id)
	{
		return Ids.ContainsByPredicate([Id](const FName Candidate) { return Candidate == Id; });
	}

	UWidget* CatalogRow(UWidgetTree* Tree, const FCatalogEntry& Entry, const FName SelectedId,
		const FStyleTokens& Tokens, UTexture2D* Atlas, const int32 FallbackCell,
		FActionRequested OnAction, const FName SelectAction)
	{
		FActionPayload Payload;
		Payload.ItemId = Entry.ItemId;
		Payload.EntryId = Entry.EntryId;
		auto* Button = ActionButton(Tree, FText::GetEmpty(), Tokens, OnAction, SelectAction, Payload,
			!Entry.ItemId.IsNone(), Entry.ItemId == SelectedId);
		auto* Content = Tree->ConstructWidget<UHorizontalBox>();
		if (auto* HitSize = Cast<USizeBox>(Button->GetContent()))
		{
			HitSize->SetMinDesiredWidth(0.f);
			HitSize->SetMinDesiredHeight(Tokens.MinimumHitTarget);
			HitSize->SetContent(Content);
		}
		if (Entry.Icon)
		{
			auto* Icon = Tree->ConstructWidget<UImage>();
			Icon->SetBrushFromTexture(Entry.Icon, true);
			if (auto* Slot = Content->AddChildToHorizontalBox(Icon))
			{
				Slot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else if (auto* Icon = AtlasCell(Tree, Atlas, FallbackCell, 3, 2, FVector2D(48.f)))
			if (auto* Slot = Content->AddChildToHorizontalBox(Icon)) Slot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		auto* Texts = Tree->ConstructWidget<UVerticalBox>();
		if (auto* Slot = Content->AddChildToHorizontalBox(Texts)) Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		const FLinearColor EntryColor = Entry.ItemId == SelectedId ? Tokens.Paper : Tokens.Ink;
		Texts->AddChildToVerticalBox(Text(Tree, Entry.Name, Tokens.BodyFont, EntryColor));
		if (!Entry.Detail.IsEmpty()) Texts->AddChildToVerticalBox(Text(Tree, Entry.Detail, Tokens.SecondaryFont, Tokens.Muted));
		if (Entry.bOwnedKnown)
			Texts->AddChildToVerticalBox(Text(Tree, Entry.bOwned
				? NSLOCTEXT("InkPlayerC", "Owned", "已持有")
				: NSLOCTEXT("InkPlayerC", "NotOwned", "未持有"), Tokens.SecondaryFont,
				Entry.ItemId == SelectedId ? Tokens.Paper : Tokens.Muted));
		return Button;
	}

	UWidget* InventoryRow(UWidgetTree* Tree, const FInkInventoryEntry& Entry, const FGuid SelectedEntryId,
		const FStyleTokens& Tokens, FActionRequested OnAction)
	{
		FActionPayload Payload;
		Payload.EntryId = Entry.EntryId;
		auto* Button = ActionButton(Tree, FText::GetEmpty(), Tokens, OnAction,
			TEXT("ui.methods.select-inventory-entry"), Payload, Entry.EntryId.IsValid(),
			Entry.EntryId == SelectedEntryId);
		auto* Content = Tree->ConstructWidget<UHorizontalBox>();
		if (auto* HitSize = Cast<USizeBox>(Button->GetContent()))
		{
			HitSize->SetMinDesiredWidth(0.f);
			HitSize->SetMinDesiredHeight(Tokens.MinimumHitTarget);
			HitSize->SetContent(Content);
		}
		if (UTexture2D* ItemIcon = Entry.Item.Icon.Get())
		{
			auto* Icon = Tree->ConstructWidget<UImage>();
			Icon->SetBrushFromTexture(ItemIcon, true);
			Content->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		}
		auto* Values = Tree->ConstructWidget<UVerticalBox>();
		if (auto* Fill = Content->AddChildToHorizontalBox(Values)) Fill->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		const FLinearColor EntryColor = Entry.EntryId == SelectedEntryId ? Tokens.Paper : Tokens.Ink;
		Values->AddChildToVerticalBox(Text(Tree, Entry.Item.Name, Tokens.BodyFont, EntryColor));
		Values->AddChildToVerticalBox(Text(Tree, FText::Format(
			NSLOCTEXT("InkPlayerC", "InventoryCount", "数量：{0}"), FText::AsNumber(Entry.Item.Count)),
			Tokens.SecondaryFont, Entry.EntryId == SelectedEntryId ? Tokens.Paper : Tokens.Muted));
		return Button;
	}

	void AddField(UWidgetTree* Tree, UVerticalBox* Parent, const FText& Label, const bool bKnown,
		const FText& Value, const FStyleTokens& Tokens)
	{
		auto* Row = Tree->ConstructWidget<UHorizontalBox>();
		Row->AddChildToHorizontalBox(Text(Tree, Label, Tokens.SecondaryFont, Tokens.Muted));
		Row->AddChildToHorizontalBox(Text(Tree, bKnown ? Value : NSLOCTEXT("InkPlayerC", "UnknownValue", "未提供"),
			Tokens.BodyFont, Tokens.Ink));
		Parent->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 4.f));
	}

	int32 AttributeCell(const FName AttributeId)
	{
		if (AttributeId == TEXT("Health")) return 0;
		if (AttributeId == TEXT("Attack")) return 1;
		if (AttributeId == TEXT("Defense")) return 2;
		if (AttributeId == TEXT("SpiritDamage")) return 3;
		if (AttributeId == TEXT("Critical")) return 4;
		if (AttributeId == TEXT("Evasion")) return 5;
		if (AttributeId == TEXT("Luck")) return 6;
		if (AttributeId == TEXT("MoveSpeed")) return 7;
		if (AttributeId == TEXT("CultivationSpeed")) return 8;
		return INDEX_NONE;
	}


	FText EquipmentSlotItemLabel(const FEquipmentSlot& Slot)
	{
		if (!Slot.bEntryKnown)
			return NSLOCTEXT("InkPlayerC", "SlotUnknown", "槽位数据未提供");

		const bool bHasActualEntry = Slot.EntryId.IsValid() || !Slot.ItemId.IsNone();
		if (!bHasActualEntry)
			return NSLOCTEXT("InkPlayerC", "SlotEmpty", "当前没有已配置物品");
		if (!Slot.ItemName.IsEmpty()) return Slot.ItemName;

		const FText ActualIdentity = !Slot.ItemId.IsNone()
			? FText::FromName(Slot.ItemId)
			: FText::FromString(Slot.EntryId.ToString());
		return FText::Format(NSLOCTEXT("InkPlayerC", "SlotNameUnknown", "物品名称未提供 · {0}"), ActualIdentity);
	}

	void AddCharacterAttributeGroup(UWidgetTree* Tree, UVerticalBox* Parent,
		const FCharacterAttributePreview& Preview, const FStyleTokens& Tokens, UTexture2D* Atlas)
	{
		AddSectionHeading(Tree, Parent, NSLOCTEXT("InkPlayerC", "AppearanceAttributes", "角色属性概览"), Tokens);
		AddProviderState(Tree, Parent, Preview.State, Tokens, TEXT("PlayerC_Appearance_AttributesState"));
		if (Preview.State.State != EProviderState::Data) return;
		for (const FInkCharacterAttribute& Attribute : Preview.Attributes)
		{
			auto* Row = Tree->ConstructWidget<UHorizontalBox>();
			UTexture2D* ActualIcon = Attribute.Icon.Get();
			if (ActualIcon)
			{
				auto* Icon = Tree->ConstructWidget<UImage>();
				Icon->SetBrushFromTexture(ActualIcon, true);
				Row->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			}
			else if (const int32 Cell = AttributeCell(Attribute.AttributeId); Cell != INDEX_NONE)
				if (auto* Icon = AtlasCell(Tree, Atlas, Cell, 3, 3, FVector2D(40.f)))
					Row->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			auto* Values = Tree->ConstructWidget<UVerticalBox>();
			if (auto* Fill = Row->AddChildToHorizontalBox(Values)) Fill->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Values->AddChildToVerticalBox(Text(Tree, Attribute.Label, Tokens.SecondaryFont, Tokens.Muted));
			Values->AddChildToVerticalBox(Text(Tree, Attribute.Value.IsEmpty()
				? NSLOCTEXT("InkPlayerC", "AttributeUnknown", "属性值未提供") : Attribute.Value,
				Tokens.BodyFont, Tokens.Ink));
			Parent->AddChildToVerticalBox(Card(Tree, Tokens, Row))->SetPadding(FMargin(0.f, 3.f));
		}
		if (Preview.Attributes.IsEmpty())
			Parent->AddChildToVerticalBox(Text(Tree, NSLOCTEXT("InkPlayerC", "AttributeProjectionEmpty", "属性响应未提供条目"),
				Tokens.BodyFont, Tokens.Muted));
	}
}

void UInkPlayerCActionButton::SetActionRequest(
	InkPlayerMethodsEquipmentAppearanceWidgets::FActionRequested InAction, const FName InActionId,
	const InkPlayerMethodsEquipmentAppearanceWidgets::FActionPayload& InPayload)
{
	Action = MoveTemp(InAction);
	ActionId = InActionId;
	Payload = InPayload;
}

void UInkPlayerCActionButton::HandleClicked()
{
	Action.ExecuteIfBound(ActionId, Payload);
}

UWidget* InkPlayerMethodsEquipmentAppearanceWidgets::BuildMethodsPage(UWidgetTree* Tree,
	const FMethodsVm& ViewModel, const FStyleTokens& Style, const ELayoutMode Layout,
	const FPageAtlases& Atlases, FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("PlayerC_Methods_Scroll"));
	Scroll->SetAllowOverscroll(false);
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerC_Methods_Root"));
	Scroll->AddChild(Body);
	AddHeader(Tree, Body, NSLOCTEXT("InkPlayerC", "MethodsTitle", "功法构筑"), Style, OnAction, TEXT("Methods"));

	auto* MainSupport = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, MainSupport, NSLOCTEXT("InkPlayerC", "CurrentLoadout", "当前配置"), Style);
	AddProviderState(Tree, MainSupport, ViewModel.MainState, Style, TEXT("PlayerC_Methods_MainState"));
	AddProviderState(Tree, MainSupport, ViewModel.SupportsState, Style, TEXT("PlayerC_Methods_SupportsState"));
	auto* LoadoutCard = Card(Tree, Style);
	auto* LoadoutDetails = Tree->ConstructWidget<UVerticalBox>();
	LoadoutCard->SetContent(LoadoutDetails);
	AddField(Tree, LoadoutDetails, NSLOCTEXT("InkPlayerC", "MainMethod", "主功法"), ViewModel.bMainKnown,
		ViewModel.MainMethodId.IsNone() ? NSLOCTEXT("InkPlayerC", "NoMain", "未配置") :
			MethodNames(TArray<FName>{ViewModel.MainMethodId}, ViewModel.Catalog), Style);
	AddField(Tree, LoadoutDetails, NSLOCTEXT("InkPlayerC", "SupportMethods", "辅修"), ViewModel.bSupportsKnown,
		ViewModel.SupportMethodIds.IsEmpty() ? NSLOCTEXT("InkPlayerC", "NoSupport", "未配置") :
			MethodNames(ViewModel.SupportMethodIds, ViewModel.Catalog), Style);
	MainSupport->AddChildToVerticalBox(LoadoutCard)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	FActionPayload ConfigurePayload;
	ConfigurePayload.MainMethodId = ViewModel.DraftMainMethodId;
	ConfigurePayload.SupportMethodIds = ViewModel.DraftSupportMethodIds;
	MainSupport->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "ConfigureMethods", "应用功法配置"),
		Style, OnAction, TEXT("configure-methods"), ConfigurePayload, ViewModel.bConfigureActionAvailable, true,
		ViewModel.ConfigureDisabledReason))->SetPadding(FMargin(0.f, 8.f));

	auto* CatalogColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, CatalogColumn, NSLOCTEXT("InkPlayerC", "MethodCatalog", "功法目录"), Style);
	AddProviderState(Tree, CatalogColumn, ViewModel.CatalogState, Style, TEXT("PlayerC_Methods_CatalogState"));
	if (ViewModel.CatalogState.State == EProviderState::Data)
	{
		for (const FCatalogEntry& Entry : ViewModel.Catalog)
		{
			FCatalogEntry RowEntry = Entry;
			if (ViewModel.OwnedMethodsState.State == EProviderState::Data ||
				ViewModel.OwnedMethodsState.State == EProviderState::Empty)
			{
				RowEntry.bOwnedKnown = true;
				RowEntry.bOwned = ViewModel.OwnedMethodIds.Contains(Entry.ItemId);
			}
			CatalogColumn->AddChildToVerticalBox(CatalogRow(Tree, RowEntry, ViewModel.SelectedMethodId,
				Style, Atlases.MethodsGlyphs, 0, OnAction, TEXT("ui.methods.select")))->SetPadding(FMargin(0.f, 3.f));
		}
		if (ViewModel.Catalog.IsEmpty())
			CatalogColumn->AddChildToVerticalBox(Text(Tree, NSLOCTEXT("InkPlayerC", "NoMethodRows", "目录响应未提供条目"),
				Style.BodyFont, Style.Muted));
	}

	auto* DetailColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, DetailColumn, NSLOCTEXT("InkPlayerC", "SelectedMethod", "所选功法"), Style);
	const FCatalogEntry* Selected = ViewModel.Catalog.FindByPredicate([&ViewModel](const FCatalogEntry& Entry)
		{ return Entry.ItemId == ViewModel.SelectedMethodId; });
	if (!ViewModel.SelectedMethodId.IsNone())
	{
		if (Selected && !Selected->Name.IsEmpty())
			DetailColumn->AddChildToVerticalBox(Card(Tree, Style, CatalogRow(Tree, *Selected, ViewModel.SelectedMethodId,
				Style, Atlases.MethodsGlyphs, 0, OnAction, TEXT("ui.methods.select"))))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		else
		{
			auto* UnknownDetails = Tree->ConstructWidget<UVerticalBox>();
			UnknownDetails->AddChildToVerticalBox(Text(Tree,
				FText::Format(NSLOCTEXT("InkPlayerC", "SelectedMethodNameUnknown", "功法名称未提供 · {0}"),
					FText::FromName(ViewModel.SelectedMethodId)), Style.SectionFont, Style.Ink));
			UnknownDetails->AddChildToVerticalBox(Text(Tree,
				NSLOCTEXT("InkPlayerC", "SelectedMethodDetailUnknown", "功法说明未提供"),
				Style.SecondaryFont, Style.Muted));
			DetailColumn->AddChildToVerticalBox(Card(Tree, Style, UnknownDetails))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		FActionPayload LearnPayload;
		LearnPayload.ItemId = ViewModel.SelectedMethodId;
        LearnPayload.EntryId = ViewModel.SelectedInventoryEntryId;
		DetailColumn->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "LearnMethod", "研习此功法"),
			Style, OnAction, TEXT("learn-method"), LearnPayload,
			ViewModel.bLearnActionAvailable && !ViewModel.SelectedMethodId.IsNone(), true,
			ViewModel.LearnDisabledReason))->SetPadding(FMargin(0.f, 4.f));

		if (Selected)
		{
			FActionPayload MainDraft; MainDraft.ItemId = ViewModel.SelectedMethodId;
			DetailColumn->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "SelectMainDraft", "设为主功法草案"),
				Style, OnAction, TEXT("ui.methods.set-main-draft"), MainDraft))->SetPadding(FMargin(0.f, 4.f));
			DetailColumn->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "ToggleSupportDraft", "切换辅修草案"),
				Style, OnAction, TEXT("ui.methods.toggle-support-draft"), MainDraft))->SetPadding(FMargin(0.f, 4.f));
		}
	}
	else
		DetailColumn->AddChildToVerticalBox(Text(Tree, NSLOCTEXT("InkPlayerC", "NoMethodSelected", "请选择已提供的目录项"),
			Style.BodyFont, Style.Muted));

	AddPageColumn(Tree, Body, CatalogColumn, DetailColumn, Layout);
	Body->AddChildToVerticalBox(MainSupport)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	auto* InventoryColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, InventoryColumn, NSLOCTEXT("InkPlayerC", "MethodInventory", "背包物品"), Style);
	AddProviderState(Tree, InventoryColumn, ViewModel.InventoryState, Style, TEXT("PlayerC_Methods_InventoryState"));
	if (ViewModel.InventoryState.State == EProviderState::Data && ViewModel.Inventory)
	{
		for (const FInkInventoryEntry& Entry : ViewModel.Inventory->GetEntriesView())
			InventoryColumn->AddChildToVerticalBox(InventoryRow(Tree, Entry, ViewModel.SelectedInventoryEntryId,
				Style, OnAction))->SetPadding(FMargin(0.f, 3.f));
		if (ViewModel.Inventory->GetEntriesView().IsEmpty())
			InventoryColumn->AddChildToVerticalBox(Text(Tree,
				NSLOCTEXT("InkPlayerC", "InventoryProjectionEmpty", "当前背包投影没有可展示条目"),
				Style.BodyFont, Style.Muted));
		if (const FInkInventoryEntry* SelectedEntry = ViewModel.Inventory->FindEntry(ViewModel.SelectedInventoryEntryId))
		{
			auto* SelectedDetails = Tree->ConstructWidget<UVerticalBox>();
			SelectedDetails->AddChildToVerticalBox(Text(Tree, SelectedEntry->Item.Name, Style.SectionFont, Style.Ink));
			if (!SelectedEntry->Item.Description.IsEmpty())
				SelectedDetails->AddChildToVerticalBox(Text(Tree, SelectedEntry->Item.Description, Style.BodyFont, Style.Muted));
			SelectedDetails->AddChildToVerticalBox(Text(Tree, FText::Format(
				NSLOCTEXT("InkPlayerC", "SelectedInventoryCount", "当前数量：{0}"),
				FText::AsNumber(SelectedEntry->Item.Count)), Style.BodyFont, Style.Ink));
			InventoryColumn->AddChildToVerticalBox(Card(Tree, Style, SelectedDetails,
				TEXT("PlayerC_Methods_InventoryDetail")))->SetPadding(FMargin(0.f, 8.f));
		}
	}
	else if (ViewModel.InventoryState.State == EProviderState::Data)
		AddProviderState(Tree, InventoryColumn, FProviderSlice{EProviderState::Unknown,
			NSLOCTEXT("InkPlayerC", "InventoryModelMissing", "背包模型尚未绑定")},
			Style, TEXT("PlayerC_Methods_InventoryModelState"));
	Body->AddChildToVerticalBox(InventoryColumn)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	return Scroll;
}

UWidget* InkPlayerMethodsEquipmentAppearanceWidgets::BuildEquipmentPage(UWidgetTree* Tree,
	const FEquipmentVm& ViewModel, const FStyleTokens& Style, const ELayoutMode Layout,
	const FPageAtlases& Atlases, FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("PlayerC_Equipment_Scroll"));
	Scroll->SetAllowOverscroll(false);
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerC_Equipment_Root"));
	Scroll->AddChild(Body);
	AddHeader(Tree, Body, NSLOCTEXT("InkPlayerC", "EquipmentTitle", "装备与法宝"), Style, OnAction, TEXT("Equipment"));

	auto* SlotColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, SlotColumn, NSLOCTEXT("InkPlayerC", "EquipmentSlots", "已提供装备位"), Style);
	AddProviderState(Tree, SlotColumn, ViewModel.EquipmentState, Style, TEXT("PlayerC_Equipment_SlotsState"));
	if (ViewModel.EquipmentState.State == EProviderState::Data)
	{
		for (const FEquipmentSlot& SlotValue : ViewModel.Slots)
		{
			FActionPayload Payload;
			Payload.SlotId = SlotValue.SlotId;
			Payload.EntryId = SlotValue.EntryId;
			Payload.ItemId = SlotValue.ItemId;
			auto* Row = Tree->ConstructWidget<UHorizontalBox>();
			if (SlotValue.Icon)
			{
				auto* Icon = Tree->ConstructWidget<UImage>();
				Icon->SetBrushFromTexture(SlotValue.Icon, true);
				Row->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			}
		else if (auto* Icon = AtlasCell(Tree, Atlases.EquipmentGlyphs, 0, 3, 2, FVector2D(48.f)))
				Row->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			auto* Values = Tree->ConstructWidget<UVerticalBox>();
			if (auto* Fill = Row->AddChildToHorizontalBox(Values)) Fill->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Values->AddChildToVerticalBox(Text(Tree, SlotValue.Label, Style.SecondaryFont, Style.Muted));
			Values->AddChildToVerticalBox(Text(Tree, EquipmentSlotItemLabel(SlotValue), Style.BodyFont, Style.Ink));
			if (SlotValue.bEntryKnown && !SlotValue.ItemDetail.IsEmpty())
				Values->AddChildToVerticalBox(Text(Tree, SlotValue.ItemDetail, Style.SecondaryFont, Style.Muted));
			auto* Select = ActionButton(Tree, NSLOCTEXT("InkPlayerC", "SelectSlot", "查看槽位"), Style,
				OnAction, TEXT("ui.equipment.select-slot"), Payload, !SlotValue.SlotId.IsNone());
			Row->AddChildToHorizontalBox(Select);
			SlotColumn->AddChildToVerticalBox(Card(Tree, Style, Row))->SetPadding(FMargin(0.f, 3.f));
		}
		if (ViewModel.Slots.IsEmpty())
			SlotColumn->AddChildToVerticalBox(Text(Tree, NSLOCTEXT("InkPlayerC", "NoSlots", "槽位响应未提供条目"),
				Style.BodyFont, Style.Muted));
	}

	auto* DetailColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, DetailColumn, NSLOCTEXT("InkPlayerC", "EquipmentDetail", "物品与法宝"), Style);
	AddProviderState(Tree, DetailColumn, ViewModel.InventoryState, Style, TEXT("PlayerC_Equipment_InventoryState"));
	if (ViewModel.bFabaoKnown)
	{
		auto* FabaoBody = Tree->ConstructWidget<UVerticalBox>();
		if (ViewModel.FabaoIcon)
		{
			auto* Icon = Tree->ConstructWidget<UImage>();
			Icon->SetBrushFromTexture(ViewModel.FabaoIcon, true);
			FabaoBody->AddChildToVerticalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
		else if (auto* Icon = AtlasCell(Tree, Atlases.EquipmentGlyphs, 1, 3, 2, FVector2D(48.f)))
			FabaoBody->AddChildToVerticalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		FabaoBody->AddChildToVerticalBox(Text(Tree, ViewModel.FabaoName.IsEmpty()
			? NSLOCTEXT("InkPlayerC", "FabaoCurrentUnknown", "法宝名称未提供") : ViewModel.FabaoName,
			Style.BodyFont, Style.Ink));
		DetailColumn->AddChildToVerticalBox(Card(Tree, Style, FabaoBody))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	else AddProviderState(Tree, DetailColumn, FProviderSlice{EProviderState::Unknown,
		NSLOCTEXT("InkPlayerC", "FabaoUnknown", "当前法宝状态未提供")}, Style, TEXT("PlayerC_Equipment_FabaoUnknown"));

	if (ViewModel.InventoryState.State == EProviderState::Data)
	{
		for (const FCatalogEntry& Entry : ViewModel.OwnedItems)
		{
			DetailColumn->AddChildToVerticalBox(CatalogRow(Tree, Entry, NAME_None, Style,
				Atlases.EquipmentGlyphs, 0, OnAction, TEXT("ui.equipment.select-entry")))->SetPadding(FMargin(0.f, 3.f));
		}
		if (ViewModel.OwnedItems.IsEmpty())
			DetailColumn->AddChildToVerticalBox(Text(Tree, NSLOCTEXT("InkPlayerC", "NoOwnedItems", "物品响应未提供条目"),
				Style.BodyFont, Style.Muted));
	}

	FActionPayload ItemPayload; ItemPayload.EntryId = ViewModel.SelectedEntryId; ItemPayload.Quantity = ViewModel.Quantity;
	DetailColumn->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "EquipSelected", "装备所选物品"),
		Style, OnAction, TEXT("inventory-equip"), ItemPayload,
		ViewModel.bEquipActionAvailable && ViewModel.SelectedEntryId.IsValid(), true,
		ViewModel.EquipDisabledReason))->SetPadding(FMargin(0.f, 4.f));
	DetailColumn->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "UnequipSelected", "卸下所选物品"),
		Style, OnAction, TEXT("inventory-unequip"), ItemPayload,
		ViewModel.bUnequipActionAvailable && ViewModel.SelectedEntryId.IsValid(), false,
		ViewModel.UnequipDisabledReason))->SetPadding(FMargin(0.f, 4.f));
	DetailColumn->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "ActivateFabao", "启用当前法宝"),
		Style, OnAction, TEXT("activate-fabao"), FActionPayload(),
		ViewModel.bActivateFabaoAvailable, true, ViewModel.ActivateFabaoDisabledReason))->SetPadding(FMargin(0.f, 4.f));
	AddProviderState(Tree, DetailColumn, ViewModel.CatalogState, Style, TEXT("PlayerC_Equipment_CatalogState"));

	AddPageColumn(Tree, Body, SlotColumn, DetailColumn, Layout);
	return Scroll;
}

UWidget* InkPlayerMethodsEquipmentAppearanceWidgets::BuildAppearancePage(UWidgetTree* Tree,
	const FAppearanceVm& ViewModel, const FCharacterAttributePreview& CharacterPreview,
	const FStyleTokens& Style, const ELayoutMode Layout,
	const FPageAtlases& Atlases, FActionRequested OnAction)
{
	if (!Tree) return nullptr;
	auto* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("PlayerC_Appearance_Scroll"));
	Scroll->SetAllowOverscroll(false);
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PlayerC_Appearance_Root"));
	Scroll->AddChild(Body);
	AddHeader(Tree, Body, NSLOCTEXT("InkPlayerC", "AppearanceTitle", "名片外观"), Style, OnAction, TEXT("Appearance"));
	AddProviderState(Tree, Body, ViewModel.AppearanceState, Style, TEXT("PlayerC_Appearance_State"));

	auto* PreviewColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, PreviewColumn, NSLOCTEXT("InkPlayerC", "LocalPreview", "本地试戴预览"), Style);
	auto* IdentityContent = Tree->ConstructWidget<UHorizontalBox>();
	auto* IdentityFields = Tree->ConstructWidget<UVerticalBox>();
	if (ViewModel.bPortraitKnown && ViewModel.Portrait)
	{
		auto* PortraitImage = Tree->ConstructWidget<UImage>();
		PortraitImage->SetBrushFromTexture(ViewModel.Portrait, true);
		if (auto* PortraitSlot = IdentityContent->AddChildToHorizontalBox(PortraitImage))
		{
			PortraitSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
			PortraitSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	if (auto* IdentitySlot = IdentityContent->AddChildToHorizontalBox(IdentityFields))
		IdentitySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	if (auto* Ornament = AtlasCell(Tree, Atlases.AppearanceOrnaments, 5, 3, 2, FVector2D(56.f)))
		IdentityFields->AddChildToVerticalBox(Ornament)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	IdentityFields->AddChildToVerticalBox(Text(Tree, ViewModel.bIdentityKnown
		? ViewModel.Identity : NSLOCTEXT("InkPlayerC", "IdentityUnknown", "角色身份未提供"), Style.TitleFont, Style.Ink));
	AddField(Tree, IdentityFields, NSLOCTEXT("InkPlayerC", "CurrentFramePreview", "当前头像框预览"),
		ViewModel.bPreviewFrameKnown, ViewModel.bPreviewFrameKnown ? FText::FromName(ViewModel.PreviewFrame) : FText::GetEmpty(), Style);
	AddField(Tree, IdentityFields, NSLOCTEXT("InkPlayerC", "CurrentBarPreview", "当前状态条预览"),
		ViewModel.bPreviewBarKnown, ViewModel.bPreviewBarKnown ? FText::FromName(ViewModel.PreviewBar) : FText::GetEmpty(), Style);
	IdentityFields->AddChildToVerticalBox(Text(Tree,
		NSLOCTEXT("InkPlayerC", "PreviewOnlyNotice", "仅本地预览 · 不表示已解锁、已装备或已保存"),
		Style.SecondaryFont, Style.Muted));
	auto* IdentityPreview=Card(Tree,Style,IdentityContent);FStyleTokens PreviewTokens=Style;
    if(ViewModel.bPreviewFrameKnown)PreviewTokens.Accent=ViewModel.PreviewFrame==TEXT("青金头像框")?FLinearColor(.2f,.44f,.52f,1):ViewModel.PreviewFrame==TEXT("山水头像框")?FLinearColor(.35f,.42f,.32f,1):Style.Accent;
    FSlateBrush PreviewBrush=SurfaceBrush(Style.Paper,PreviewTokens,ViewModel.bPreviewFrameKnown?3.f:1.f);PreviewBrush.DrawAs=ESlateBrushDrawType::RoundedBox;PreviewBrush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;PreviewBrush.OutlineSettings.CornerRadii=FVector4(4,4,4,4);IdentityPreview->SetBrush(PreviewBrush);PreviewColumn->AddChildToVerticalBox(IdentityPreview)->SetPadding(FMargin(0.f,0.f,0.f,12.f));
    if(ViewModel.bPreviewBarKnown){auto* Swatch=Tree->ConstructWidget<UBorder>();Swatch->SetBrushColor(ViewModel.PreviewBar==TEXT("护盾")?FLinearColor(.25f,.48f,.8f,1):ViewModel.PreviewBar==TEXT("华丽")?FLinearColor(.68f,.46f,.15f,1):ViewModel.PreviewBar==TEXT("渐变")?FLinearColor(.55f,.3f,.6f,1):FLinearColor(.25f,.46f,.45f,1));auto* Bounds=Tree->ConstructWidget<USizeBox>();Bounds->SetHeightOverride(16);Bounds->SetContent(Swatch);PreviewColumn->AddChild(Bounds);PreviewColumn->AddChild(Text(Tree,NSLOCTEXT("InkPlayerC","BarColorOnly","状态条配色预览，不表示气血或进度"),Style.SecondaryFont,Style.Muted));}
	AddCharacterAttributeGroup(Tree, PreviewColumn, CharacterPreview, Style, Atlases.CharacterAttributeGlyphs);

	auto* OptionsColumn = Tree->ConstructWidget<UVerticalBox>();
	AddSectionHeading(Tree, OptionsColumn, NSLOCTEXT("InkPlayerC", "AppearanceChoices", "试戴样式"), Style);
	const bool bFrameTab = ViewModel.ActiveTab != TEXT("Bar");
	auto* Tabs = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PlayerC_Appearance_Tabs"));
	for (int32 TabIndex = 0; TabIndex < 2; ++TabIndex)
	{
		const bool bTabIsFrame = TabIndex == 0;
		FActionPayload TabPayload; TabPayload.SlotId = bTabIsFrame ? TEXT("Frame") : TEXT("Bar");
		auto* Tab = ActionButton(Tree, bTabIsFrame
			? NSLOCTEXT("InkPlayerC", "FrameTab", "头像框")
			: NSLOCTEXT("InkPlayerC", "BarTab", "状态条"), Style, OnAction,
			TEXT("ui.appearance.select-tab"), TabPayload, true, bTabIsFrame == bFrameTab);
		if (auto* Slot = Tabs->AddChildToHorizontalBox(Tab))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(3.f));
		}
	}
	OptionsColumn->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 4.f, 0.f, 12.f));
	if (bFrameTab)
	{
		for (const FPreviewOption& Option : ViewModel.FrameOptions)
		{
			FActionPayload Payload; Payload.PreviewStyleId = Option.StyleId;
			OptionsColumn->AddChildToVerticalBox(ActionButton(Tree, Option.Label.IsEmpty() ? FText::FromName(Option.StyleId) : Option.Label, Style, OnAction,
				TEXT("ui.appearance.select-frame"), Payload, !Option.StyleId.IsNone(),
				Option.StyleId == ViewModel.DraftPreviewFrame))->SetPadding(FMargin(0.f, 3.f));
		}
		if (ViewModel.FrameOptions.IsEmpty())
			OptionsColumn->AddChildToVerticalBox(Text(Tree, ViewModel.bFrameOptionsKnown
				? NSLOCTEXT("InkPlayerC", "EmptyFrameChoices", "当前没有可预览样式")
				: NSLOCTEXT("InkPlayerC", "UnknownFrameChoices", "试戴选项尚未提供"), Style.BodyFont, Style.Muted));
	}
	else
	{
		for (const FPreviewOption& Option : ViewModel.BarOptions)
		{
			FActionPayload Payload; Payload.PreviewStyleId = Option.StyleId;
			OptionsColumn->AddChildToVerticalBox(ActionButton(Tree, Option.Label.IsEmpty() ? FText::FromName(Option.StyleId) : Option.Label, Style, OnAction,
				TEXT("ui.appearance.select-bar"), Payload, !Option.StyleId.IsNone(),
				Option.StyleId == ViewModel.DraftPreviewBar))->SetPadding(FMargin(0.f, 3.f));
		}
		if (ViewModel.BarOptions.IsEmpty())
			OptionsColumn->AddChildToVerticalBox(Text(Tree, ViewModel.bBarOptionsKnown
				? NSLOCTEXT("InkPlayerC", "EmptyBarChoices", "当前没有可预览样式")
				: NSLOCTEXT("InkPlayerC", "UnknownBarChoices", "试戴选项尚未提供"), Style.BodyFont, Style.Muted));
	}

	AddPageColumn(Tree, Body, PreviewColumn, OptionsColumn, Layout);
	FActionPayload PreviewPayload; PreviewPayload.PreviewStyleId = ViewModel.SelectedPreviewStyleId;
	Body->AddChildToVerticalBox(ActionButton(Tree, NSLOCTEXT("InkPlayerC", "PreviewAppearance", "预览所选样式"),
		Style, OnAction, TEXT("preview-appearance"), PreviewPayload,
		ViewModel.bPreviewActionAvailable && !ViewModel.SelectedPreviewStyleId.IsNone(), true,
		ViewModel.PreviewDisabledReason))->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	return Scroll;
}
