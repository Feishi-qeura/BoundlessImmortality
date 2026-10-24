#include "InkPlayerCharacterInventoryWidgets.h"
#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"

#include "Frontend/InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	const FLinearColor Primary = FLinearColor::FromSRGBColor(FColor(23, 62, 55));
	const FLinearColor Secondary = FLinearColor::FromSRGBColor(FColor(66, 95, 103));
	const FLinearColor Accent = FLinearColor::FromSRGBColor(FColor(154, 115, 52));
	const FLinearColor Page = FLinearColor::FromSRGBColor(FColor(244, 236, 221));
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(251, 247, 238));
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(27, 41, 37));
	const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(82, 89, 79));
	const FLinearColor Jade = FLinearColor::FromSRGBColor(FColor(50, 111, 99));
	const FLinearColor Warning = FLinearColor::FromSRGBColor(FColor(156, 67, 54));
	const FLinearColor Disabled = FLinearColor::FromSRGBColor(FColor(112, 110, 102));

	FSlateBrush PanelBrush(const FLinearColor Fill, const float Stroke = 1.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(4.f);
		Brush.TintColor = Fill;
		Brush.OutlineSettings.Color = Accent;
		Brush.OutlineSettings.Width = Stroke;
		return Brush;
	}

	FButtonStyle ActionStyle(bool bPrimary, bool bEnabled);

	UTextBlock* Label(UWidgetTree* Tree, const FText& Text, const int32 Size,
		const FName Name = NAME_None, const FLinearColor Color = Ink,
		const ETextJustify::Type Align = ETextJustify::Left)
	{
		auto* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		const FSlateFontInfo Font = InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, Size);
		Block->SetFont(Font);
		Block->SetColorAndOpacity(Color);
		Block->SetJustification(Align);
		Block->SetAutoWrapText(true);
		Block->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Block;
	}

	UInkFrontendActionButton* ActionButton(UWidgetTree* Tree, UInkFrontendWidget* Frontend,
		const FName Name, const FName Action, const FText& Caption, const bool bPrimary = false,
		const bool bEnabled = true, const int32 LabelSize = 26)
	{
		auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(), Name);
		Button->Action = Action;
		Button->BindFrontend(Frontend);
		Button->SetIsEnabled(bEnabled);
		Button->SetStyle(ActionStyle(bPrimary, bEnabled));
		auto* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetMinDesiredWidth(88.f);
		Size->SetMinDesiredHeight(88.f);
		Size->SetContent(Label(Tree, Caption, LabelSize, NAME_None, !bEnabled || bPrimary ? Paper : Ink,
			ETextJustify::Center));
		if(auto* Slot=Cast<USizeBoxSlot>(Size->GetContent()->Slot))Slot->SetVerticalAlignment(VAlign_Center);
		Button->SetContent(Size);
		if(auto* Slot=Cast<UButtonSlot>(Size->Slot))
		{Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Center);Slot->SetPadding(FMargin(12.f,4.f));}
		return Button;
	}

	FButtonStyle ActionStyle(const bool bPrimary, const bool bEnabled)
	{
		FButtonStyle Style;
		const FLinearColor Normal = !bEnabled ? Disabled : bPrimary ? Primary : Paper;
		Style.SetNormal(PanelBrush(Normal, 1.5f));
		Style.SetHovered(PanelBrush(bPrimary ? Jade : Page, 2.f));
		Style.SetPressed(PanelBrush(Secondary, 2.f));
		Style.SetDisabled(PanelBrush(Disabled, 1.f));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
		return Style;
	}

	UImage* AtlasIcon(UWidgetTree* Tree, UTexture2D* Atlas, const FName Name,
		const int32 Cell, const int32 Columns, const int32 Rows, const FVector2D& Size)
	{
		if (!Atlas || Columns <= 0 || Rows <= 0 || Cell < 0 || Cell >= Columns * Rows) return nullptr;
		const int32 Column = Cell % Columns;
		const int32 Row = Cell / Columns;
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Atlas);
		Brush.SetImageSize(Size);
		Brush.SetUVRegion(FBox2D(
			FVector2D(float(Column) / Columns, float(Row) / Rows),
			FVector2D(float(Column + 1) / Columns, float(Row + 1) / Rows)));
		auto* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	int32 AttributeGlyph(const FName Id)
	{
		if (Id == TEXT("Health")) return 0;
		if (Id == TEXT("Attack")) return 1;
		if (Id == TEXT("Defense")) return 2;
		if (Id == TEXT("SpiritDamage")) return 3;
		if (Id == TEXT("Critical")) return 4;
		if (Id == TEXT("Evasion")) return 5;
		if (Id == TEXT("Luck")) return 6;
		if (Id == TEXT("MoveSpeed")) return 7;
		if (Id == TEXT("CultivationSpeed")) return 8;
		return INDEX_NONE;
	}

	int32 CategoryGlyph(const FName Category)
	{
		if (Category == TEXT("Equipment")) return 0;
		if (Category == TEXT("Material")) return 1;
		if (Category == TEXT("Consumable")) return 2;
		if (Category == TEXT("Treasure")) return 3;
		return INDEX_NONE;
	}

	FText StateLabel(const InkPlayerCharacterInventoryWidgets::EProviderState State, const FText& ProvidedMessage)
	{
		if (!ProvidedMessage.IsEmpty()) return ProvidedMessage;
		using E = InkPlayerCharacterInventoryWidgets::EProviderState;
		switch (State)
		{
		case E::Unknown: return NSLOCTEXT("InkPlayerRebuild", "DataUnknown", "数据尚未提供");
		case E::Loading: return NSLOCTEXT("InkPlayerRebuild", "DataLoading", "正在同步");
		case E::Empty: return NSLOCTEXT("InkPlayerRebuild", "DataEmpty", "当前没有已提供记录");
		case E::Pending: return NSLOCTEXT("InkPlayerRebuild", "DataPending", "请求处理中");
		case E::Offline: return NSLOCTEXT("InkPlayerRebuild", "DataOffline", "离线 · 当前数据可能过期");
		case E::Error: return NSLOCTEXT("InkPlayerRebuild", "DataError", "数据读取失败");
		case E::Disabled: return NSLOCTEXT("InkPlayerRebuild", "DataDisabled", "当前状态下不可用");
		default: return FText::GetEmpty();
		}
	}

	void AddState(UWidgetTree* Tree, UVerticalBox* Body,
		const InkPlayerCharacterInventoryWidgets::EProviderState State, const FText& Message)
	{
		using E = InkPlayerCharacterInventoryWidgets::EProviderState;
		if (State == E::Data && Message.IsEmpty()) return;
		auto* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Panel->SetBrush(PanelBrush(Paper));
		Panel->SetPadding(FMargin(24.f));
		Panel->SetContent(Label(Tree, StateLabel(State, Message), 28, NAME_None, State == E::Error ? Warning : Muted));
		Body->AddChildToVerticalBox(Panel)->SetPadding(FMargin(0.f, 12.f));
	}

	void AddAttributeRow(UWidgetTree* Tree, UVerticalBox* Body, const FInkCharacterAttribute& Attribute,
		UTexture2D* GlyphAtlas, const int32 Index)
	{
		auto* Row = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("Rebuild_CharacterAttribute_%d"), Index)));
		Row->SetBrush(PanelBrush(Paper));
		Row->SetPadding(FMargin(16.f, 8.f));
		auto* Content = Tree->ConstructWidget<UHorizontalBox>();
		Row->SetContent(Content);
		if (const int32 Glyph = AttributeGlyph(Attribute.AttributeId); Glyph != INDEX_NONE)
		{
			if (auto* Icon = AtlasIcon(Tree, GlyphAtlas, NAME_None, Glyph, 3, 3, FVector2D(40.f)))
				if (auto* Slot = Content->AddChildToHorizontalBox(Icon))
				{
					Slot->SetVerticalAlignment(VAlign_Center);
					Slot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
				}
		}
		auto* Values = Tree->ConstructWidget<UVerticalBox>();
		if (auto* Slot = Content->AddChildToHorizontalBox(Values)) Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Values->AddChildToVerticalBox(Label(Tree, Attribute.Label, 24, NAME_None, Muted));
		Values->AddChildToVerticalBox(Label(Tree, Attribute.Value, 28, NAME_None, Ink));
		Body->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 4.f));
	}
}

UWidget* InkPlayerCharacterInventoryWidgets::BuildCharacterPage(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, const FText& CharacterName, const FText& RealmName,
	const TArray<FInkCharacterAttribute>& Attributes, const TArray<FInkDisciplineEntry>& Disciplines,
	UTexture2D* AttributeGlyphAtlas, const EProviderState State, const FText& StateMessage)
{
	if (!Tree || !Frontend) return nullptr;
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rebuild_CharacterBodyV1"));

	auto* Identity = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rebuild_CharacterIdentityV1"));
	Identity->SetBrush(PanelBrush(Primary, 2.f));
	Identity->SetPadding(FMargin(24.f));
	auto* IdentityBody = Tree->ConstructWidget<UVerticalBox>();
	Identity->SetContent(IdentityBody);
	IdentityBody->AddChildToVerticalBox(Label(Tree, CharacterName, 40, TEXT("Rebuild_CharacterName"), Paper));
	IdentityBody->AddChildToVerticalBox(Label(Tree, RealmName, 28, TEXT("Rebuild_CharacterRealm"),
		FLinearColor::FromSRGBColor(FColor(219, 193, 142))));
	Body->AddChildToVerticalBox(Identity)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	AddState(Tree, Body, State, StateMessage);

	if (State == EProviderState::Data)
	{
		auto* AttributesHeading = Label(Tree, NSLOCTEXT("InkPlayerRebuild", "CharacterAttributes", "角色属性"), 32);
		Body->AddChildToVerticalBox(AttributesHeading)->SetPadding(FMargin(0.f, 12.f, 0.f, 4.f));
		for (int32 Index = 0; Index < Attributes.Num(); ++Index)
			AddAttributeRow(Tree, Body, Attributes[Index], AttributeGlyphAtlas, Index);

		auto* DisciplinesHeading = Label(Tree, NSLOCTEXT("InkPlayerRebuild", "CharacterDisciplines", "修行配置"), 32);
		Body->AddChildToVerticalBox(DisciplinesHeading)->SetPadding(FMargin(0.f, 24.f, 0.f, 4.f));
		for (const FInkDisciplineEntry& Discipline : Disciplines)
		{
			auto* Card = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Card->SetBrush(PanelBrush(Paper));
			Card->SetPadding(FMargin(16.f));
			auto* CardBody = Tree->ConstructWidget<UVerticalBox>();
			Card->SetContent(CardBody);
			const FText CategoryLabel=FText::FromString(Discipline.Category==TEXT("Inner")?TEXT("内功"):Discipline.Category==TEXT("Movement")?TEXT("身法"):Discipline.Category==TEXT("Martial")?TEXT("武技"):Discipline.Category==TEXT("Manual")?TEXT("典籍"):TEXT("修行类别待同步"));
			CardBody->AddChildToVerticalBox(Label(Tree, CategoryLabel, 24, NAME_None, Muted));
			CardBody->AddChildToVerticalBox(Label(Tree, Discipline.Name, 28));
			if (!Discipline.Detail.IsEmpty()) CardBody->AddChildToVerticalBox(Label(Tree, Discipline.Detail, 24, NAME_None, Muted));
			Body->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.f, 4.f));
		}
	}

	const FName RouteActions[] = {
		TEXT("OpenEquipmentFabao"), TEXT("OpenCultivationMethods"),
		TEXT("OpenBreakthroughPrepare"), TEXT("OpenCharacterAppearance")};
	const FText RouteLabels[] = {
		NSLOCTEXT("InkPlayerRebuild", "Equipment", "装备"),
		NSLOCTEXT("InkPlayerRebuild", "Methods", "功法"),
		NSLOCTEXT("InkPlayerRebuild", "Breakthrough", "突破"),
		NSLOCTEXT("InkPlayerRebuild", "Appearance", "外观")};
	for (int32 RowIndex = 0; RowIndex < 2; ++RowIndex)
	{
		auto* Row = Tree->ConstructWidget<UHorizontalBox>();
		for (int32 Column = 0; Column < 2; ++Column)
		{
			const int32 Index = RowIndex * 2 + Column;
			auto* Button = ActionButton(Tree, Frontend, FName(*FString::Printf(TEXT("Rebuild_CharacterRoute_%d"), Index)),
				RouteActions[Index], RouteLabels[Index]);
			if (auto* Slot = Row->AddChildToHorizontalBox(Button))
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Slot->SetPadding(FMargin(4.f));
			}
		}
		Body->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 4.f));
	}
	return Body;
}

UWidget* InkPlayerCharacterInventoryWidgets::BuildInventoryPage(UWidgetTree* Tree,
	UInkFrontendWidget* Frontend, UInkInventoryModel* Inventory, const FGuid SelectedEntryId,
	const FName ActiveFilter, const FString& SearchQuery, const int32 PageIndex,
	const int32 ActionQuantity, const bool bCapacityKnown,
	const int32 UnlockedSlotCount, UTexture2D* CategoryGlyphAtlas, const EProviderState State,
	const FText& StateMessage, const float AvailableWidth, const bool bMutationsBusy)
{
	if (!Tree || !Frontend) return nullptr;
	auto* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rebuild_InventoryBodyV1"));


	if (bCapacityKnown && Inventory)
		Body->AddChildToVerticalBox(Label(Tree, FText::Format(
			NSLOCTEXT("InkPlayerRebuild", "InventoryCapacity", "{0} / {1} 格已占用"),
			FText::AsNumber(Inventory->GetOccupiedSlotCount()), FText::AsNumber(UnlockedSlotCount)),
			24, TEXT("Rebuild_InventoryCapacity"), Muted));
	AddState(Tree, Body, State, StateMessage);

	const FName FilterActions[] = {TEXT("FilterAll"), TEXT("FilterEquipment"), TEXT("FilterMaterial"), TEXT("FilterConsumable"), TEXT("FilterTreasure")};
	const FName FilterIds[] = {TEXT("All"), TEXT("Equipment"), TEXT("Material"), TEXT("Consumable"), TEXT("Treasure")};
	const FText FilterLabels[] = {
		NSLOCTEXT("InkPlayerRebuild", "AllItems", "全部"),
		NSLOCTEXT("InkPlayerRebuild", "EquipmentItems", "装备"),
		NSLOCTEXT("InkPlayerRebuild", "MaterialItems", "材料"),
		NSLOCTEXT("InkPlayerRebuild", "ConsumableItems", "消耗品"),
		NSLOCTEXT("InkPlayerRebuild", "TreasureItems", "法宝")};
	auto* Filters = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Rebuild_InventoryFiltersV1"));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(FilterActions); ++Index)
	{
		auto* Button = ActionButton(Tree, Frontend, FName(*FString::Printf(TEXT("Rebuild_InventoryFilter_%s"), *FilterIds[Index].ToString())),
			FilterActions[Index], FilterLabels[Index], false, true, 24);
		Button->SetStyle(ActionStyle(ActiveFilter == FilterIds[Index], true));
		if (ActiveFilter == FilterIds[Index])
			if (auto* Size = Cast<USizeBox>(Button->GetContent()))
				if (auto* Caption = Cast<UTextBlock>(Size->GetContent())) Caption->SetColorAndOpacity(Paper);
		if (auto* Slot = Filters->AddChildToHorizontalBox(Button))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(2.f));
		}
	}
	Body->AddChildToVerticalBox(Filters)->SetPadding(FMargin(0.f, 12.f, 0.f, 8.f));
	if (!SearchQuery.IsEmpty())
		Body->AddChildToVerticalBox(Label(Tree, FText::Format(
			NSLOCTEXT("InkPlayerRebuild", "InventorySearchQuery", "搜索：{0}"), FText::FromString(SearchQuery)), 24, NAME_None, Muted));

	if (State == EProviderState::Data && Inventory)
	{
		auto* Grid = Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Rebuild_InventoryItemsV1"));
		Grid->SetMinDesiredSlotWidth(104.f);
		Grid->SetMinDesiredSlotHeight(120.f);
		Grid->SetSlotPadding(FMargin(4.f));
		TArray<int32> VisibleSlots;
		const bool bShowPhysicalSlots = ActiveFilter == TEXT("All") && SearchQuery.IsEmpty() && bCapacityKnown;
		if (bShowPhysicalSlots)
		{
			for (int32 Slot = 0; Slot < Inventory->GetTotalSlotCount(); ++Slot) VisibleSlots.Add(Slot);
		}
		else
		{
			for (const FInkInventoryEntry& Entry : Inventory->GetEntriesView())
			{
				if (ActiveFilter != TEXT("All") && Entry.Item.Category != ActiveFilter) continue;
				if (!SearchQuery.IsEmpty() && !Entry.Item.Name.ToString().Contains(SearchQuery, ESearchCase::IgnoreCase) &&
					!Entry.Item.ItemId.ToString().Contains(SearchQuery, ESearchCase::IgnoreCase)) continue;
				VisibleSlots.Add(Entry.SlotIndex);
			}
			VisibleSlots.Sort();
		}
		constexpr int32 SlotsPerPage = 40;
		const int32 PageCount = FMath::Max(1, FMath::DivideAndRoundUp(VisibleSlots.Num(), SlotsPerPage));
		const int32 SafePageIndex = FMath::Clamp(PageIndex, 0, PageCount - 1);
		const int32 PageStart = SafePageIndex * SlotsPerPage;
		const int32 PageEnd = FMath::Min(PageStart + SlotsPerPage, VisibleSlots.Num());
		const int32 Columns=FMath::Clamp(FMath::FloorToInt(AvailableWidth/112.f),1,8);
		int32 VisibleIndex = 0;
		for (int32 SlotOffset = PageStart; SlotOffset < PageEnd; ++SlotOffset)
		{
			const int32 SlotIndex = VisibleSlots[SlotOffset];
			const FInkInventoryEntry* Entry = Inventory->FindEntryAtSlot(SlotIndex);
			const bool bLocked = bCapacityKnown && SlotIndex >= UnlockedSlotCount;
			const bool bSelected = Entry && Entry->EntryId == SelectedEntryId;
			const FName WidgetName = Entry
				? FName(*FString::Printf(TEXT("Rebuild_InventoryEntry_%s"), *Entry->EntryId.ToString(EGuidFormats::Digits)))
				: FName(*FString::Printf(TEXT("Rebuild_InventorySlot_%d"), SlotIndex));
			auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(), WidgetName);
			Button->Action = Entry?FName(*(TEXT("InventorySelectId_")+Entry->EntryId.ToString(EGuidFormats::Digits)))
				:FName(*FString::Printf(TEXT("InventorySelectEmpty_%d"),SlotIndex));
			Button->BindFrontend(Frontend);
			Button->SetStyle(ActionStyle(bSelected, !bLocked));
			Button->SetIsEnabled(!bLocked);
			auto* Content = Tree->ConstructWidget<UVerticalBox>();
			Button->SetContent(Content);
			if (Entry)
			{
				if (UTexture2D* ItemIcon = Entry->Item.Icon.LoadSynchronous())
				{
					auto* Image = Tree->ConstructWidget<UImage>();
					Image->SetBrushFromTexture(ItemIcon, true);
					Image->SetVisibility(ESlateVisibility::HitTestInvisible);
					Content->AddChildToVerticalBox(Image)->SetPadding(FMargin(0.f, 4.f));
				}
				else if (auto* Icon = AtlasIcon(Tree, CategoryGlyphAtlas, NAME_None,
					CategoryGlyph(Entry->Item.Category), 3, 2, FVector2D(48.f)))
					Content->AddChildToVerticalBox(Icon)->SetPadding(FMargin(0.f, 4.f));
				Content->AddChildToVerticalBox(Label(Tree, Entry->Item.Name, 20, NAME_None,
					bSelected ? Paper : Ink, ETextJustify::Center));
				if (Entry->Item.Count > 1)
					Content->AddChildToVerticalBox(Label(Tree, FText::AsNumber(Entry->Item.Count), 20, NAME_None,
						bSelected ? Paper : Muted, ETextJustify::Center));
			}
			else if (auto* Icon = AtlasIcon(Tree, CategoryGlyphAtlas, NAME_None, bLocked ? 5 : 4, 3, 2, FVector2D(40.f)))
				Content->AddChildToVerticalBox(Icon)->SetPadding(FMargin(0.f, 6.f));
			else
				Content->AddChildToVerticalBox(Label(Tree, bLocked ? NSLOCTEXT("InkPlayerRebuild", "LockedSlot", "未解锁")
					: NSLOCTEXT("InkPlayerRebuild", "EmptySlot", "空槽"), 20, NAME_None,
					bLocked ? Paper : Muted, ETextJustify::Center));
			auto* SlotSize = Tree->ConstructWidget<USizeBox>();
			SlotSize->SetMinDesiredWidth(104.f);
			SlotSize->SetMinDesiredHeight(120.f);
			SlotSize->SetContent(Button);
			Grid->AddChildToUniformGrid(SlotSize, VisibleIndex / Columns, VisibleIndex % Columns);
			++VisibleIndex;
		}
		if (VisibleSlots.IsEmpty())
			Body->AddChildToVerticalBox(Label(Tree, SearchQuery.IsEmpty()
				? NSLOCTEXT("InkPlayerRebuild", "NoFilterMatches", "此分类暂无已提供物品")
				: NSLOCTEXT("InkPlayerRebuild", "NoSearchMatches", "没有符合条件的已提供物品"), 26, NAME_None, Muted));
		Body->AddChildToVerticalBox(Grid)->SetPadding(FMargin(0.f, 8.f));
		auto* Paging = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Rebuild_InventoryPagingV1"));
		auto* Previous = ActionButton(Tree, Frontend, TEXT("Rebuild_InventoryPrevious"), TEXT("InventoryPrevious"),
			NSLOCTEXT("InkPlayerRebuild", "Previous", "上一页"), false, SafePageIndex > 0, 24);
		auto* Next = ActionButton(Tree, Frontend, TEXT("Rebuild_InventoryNext"), TEXT("InventoryNext"),
			NSLOCTEXT("InkPlayerRebuild", "Next", "下一页"), false, SafePageIndex + 1 < PageCount, 24);
		Paging->AddChildToHorizontalBox(Previous);
		Paging->AddChildToHorizontalBox(Label(Tree, FText::Format(NSLOCTEXT("InkPlayerRebuild", "PageCount", "{0} / {1}"),
			FText::AsNumber(SafePageIndex + 1), FText::AsNumber(PageCount)), 24, TEXT("Rebuild_InventoryPage"), Muted, ETextJustify::Center));
		Paging->AddChildToHorizontalBox(Next);
		Body->AddChildToVerticalBox(Paging)->SetPadding(FMargin(0.f, 8.f));

		const FInkInventoryEntry* Selected = SelectedEntryId.IsValid() ? Inventory->FindEntry(SelectedEntryId) : nullptr;
		auto* Detail = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Rebuild_InventoryDetailV1"));
		Detail->SetBrush(PanelBrush(Paper));
		Detail->SetPadding(FMargin(24.f));
		auto* DetailBody = Tree->ConstructWidget<UVerticalBox>();
		Detail->SetContent(DetailBody);
		if (Selected)
		{
			DetailBody->AddChildToVerticalBox(Label(Tree, Selected->Item.Name, 32));
			DetailBody->AddChildToVerticalBox(Label(Tree, Selected->Item.Description, 24, NAME_None, Muted));
			DetailBody->AddChildToVerticalBox(Label(Tree, FText::Format(
				NSLOCTEXT("InkPlayerRebuild", "InventorySelectedCount", "持有数量：{0}"), FText::AsNumber(Selected->Item.Count)), 24));
			auto* QuantityRow = Tree->ConstructWidget<UHorizontalBox>();
			for (const TPair<FName, FText>& Step : {
				TPair<FName, FText>(TEXT("InventoryQuantityMinus"), NSLOCTEXT("InkPlayerRebuild", "QuantityMinus", "−")),
				TPair<FName, FText>(TEXT("InventoryQuantityPlus"), NSLOCTEXT("InkPlayerRebuild", "QuantityPlus", "+"))})
			{
				auto* Button = ActionButton(Tree, Frontend, FName(*FString::Printf(TEXT("Rebuild_%s"), *Step.Key.ToString())), Step.Key, Step.Value);
				if (auto* Slot = QuantityRow->AddChildToHorizontalBox(Button)) Slot->SetPadding(FMargin(4.f));
			}
			QuantityRow->AddChildToHorizontalBox(Label(Tree, FText::AsNumber(ActionQuantity), 28,
				TEXT("Rebuild_InventoryActionQuantity"), Ink, ETextJustify::Center));
			DetailBody->AddChildToVerticalBox(QuantityRow)->SetPadding(FMargin(0.f, 8.f));

			auto* Actions = Tree->ConstructWidget<UHorizontalBox>();
			const FName ActionIds[] = {TEXT("InventoryUse"), TEXT("InventoryEquip"), TEXT("InventorySell")};
			const bool bCanUse = Inventory->CanUseEntry(SelectedEntryId);
			const bool bCanEquip = Selected->bEquipped || Inventory->CanEquipEntry(SelectedEntryId);
			const bool bCanRecycle = Selected->Item.bSellable && !Selected->bEquipped;
			const FText ActionLabels[] = {
				NSLOCTEXT("InkPlayerRebuild", "UseItem", "使用"),
				Selected->bEquipped ? NSLOCTEXT("InkPlayerRebuild", "UnequipItem", "卸下") : NSLOCTEXT("InkPlayerRebuild", "EquipItem", "装备"),
				NSLOCTEXT("InkPlayerRebuild", "RecycleItem", "回收")};
		const bool Enabled[] = {bCanUse, bCanEquip, bCanRecycle};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ActionIds); ++Index)
		{
			auto* Button = ActionButton(Tree, Frontend, FName(*FString::Printf(TEXT("Rebuild_InventoryAction_%s"), *ActionIds[Index].ToString())),
				ActionIds[Index], ActionLabels[Index], Index == 0, Enabled[Index] && !bMutationsBusy);
			if (auto* Slot = Actions->AddChildToHorizontalBox(Button))
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				Slot->SetPadding(FMargin(4.f));
			}
		}
		DetailBody->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0.f, 8.f));
		Body->AddChildToVerticalBox(Detail)->SetPadding(FMargin(0.f, 16.f));
	}
	else if (State == EProviderState::Data)
	{
		Body->AddChildToVerticalBox(Label(Tree,
			NSLOCTEXT("InkPlayerRebuild", "SelectInventoryItem", "选择物品查看详情"), 26, NAME_None, Muted))
			->SetPadding(FMargin(0.f, 16.f));
	}
	}
	return Body;
}
