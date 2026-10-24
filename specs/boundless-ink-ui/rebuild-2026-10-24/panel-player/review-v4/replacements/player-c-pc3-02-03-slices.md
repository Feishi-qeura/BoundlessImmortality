# Minimal replacement slices: PC3-02 and PC3-03

Target is the frozen Player-c `.cpp` named in `../diff/player-c-pc3-02-03.diff`. The unified diff is the applyable source of truth. These slices do not alter Player-c on disk.

## PC3-02: known selected ID, catalog row optional

```cpp
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
```

## PC3-03: equipment name unknown versus known empty slot

```cpp
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
```

Replace the existing slot display expression with:

```cpp
Values->AddChildToVerticalBox(Text(Tree, EquipmentSlotItemLabel(SlotValue), Style.BodyFont, Style.Ink));
```
