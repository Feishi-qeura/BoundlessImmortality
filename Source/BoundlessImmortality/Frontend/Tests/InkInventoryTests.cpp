#if WITH_DEV_AUTOMATION_TESTS

#include "Frontend/InkInventoryModel.h"
#include "Misc/AutomationTest.h"

namespace InkInventoryTests
{
FInkInventoryItem MakeItem(const TCHAR* Id, const TCHAR* Category = TEXT("Material"), bool bStackable = true)
{
    FInkInventoryItem Item;
    Item.ItemId = Id;
    Item.Name = FText::FromString(Id);
    Item.Category = Category;
    Item.Count = 1;
    Item.bStackable = bStackable;
    Item.MaxStackSize = 100;
    return Item;
}
}

// A capacity bug would accept a twenty-first initial slot, lose an occupied slot on relock,
// or overwrite an entry when the two-hundredth physical slot is full.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkInventoryCapacityTest, "BoundlessImmortality.Frontend.Inventory.Capacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkInventoryCapacityTest::RunTest(const FString&)
{
    auto* Model = NewObject<UInkInventoryModel>();
    auto Sword = InkInventoryTests::MakeItem(TEXT("Sword"), TEXT("Equipment"), false);
    TestEqual(TEXT("Only twenty initial slots accept items"), Model->AddItem(Sword, 100), 20);
    TestEqual(TEXT("Full initial bag rejects further items"), Model->AddItem(Sword, 1), 0);
    TestEqual(TEXT("Relocking never hides occupied entries"), Model->SetUnlockedSlotCount(1), 20);
    TestEqual(TEXT("Unlock request cannot exceed two hundred"), Model->SetUnlockedSlotCount(300), 200);
    TestEqual(TEXT("Remaining one hundred eighty slots can be filled"), Model->AddItem(Sword, 200), 180);
    TestEqual(TEXT("All two hundred physical slots occupied"), Model->GetOccupiedSlotCount(), 200);
    TestEqual(TEXT("Two hundred is a hard capacity"), Model->AddItem(Sword, 1), 0);
    TestNotNull(TEXT("Last physical slot contains an item"), Model->FindEntryAtSlot(199));
    TestNull(TEXT("No item is allocated beyond the bag"), Model->FindEntryAtSlot(200));
    const FGuid RemovedId = Model->FindEntryAtSlot(4)->EntryId;
    TestEqual(TEXT("Remove creates exactly one empty slot"), Model->RemoveItem(RemovedId), 1);
    TestEqual(TEXT("Acquisition reuses that empty slot"), Model->AddItem(Sword, 1), 1);
    TestNotNull(TEXT("Reused slot remains visible"), Model->FindEntryAtSlot(4));
    TestNull(TEXT("Removed stable identity is no longer usable"), Model->FindEntry(RemovedId));
    return true;
}

// Removing MarkSeen or setting bIsNew whenever a quantity increases breaks these transitions.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkInventoryNewMarkerTest, "BoundlessImmortality.Frontend.Inventory.NewMarker", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkInventoryNewMarkerTest::RunTest(const FString&)
{
    auto* Model = NewObject<UInkInventoryModel>();
    auto Herb = InkInventoryTests::MakeItem(TEXT("Herb"));
    Herb.MaxStackSize = 3;
    TestEqual(TEXT("First herb acquired"), Model->AddItem(Herb, 2), 2);
    const FGuid HerbId = Model->GetEntries()[0].EntryId;
    TestTrue(TEXT("New type has new marker"), Model->FindEntry(HerbId)->bIsNew);
    TestTrue(TEXT("Hover clears the marker"), Model->MarkSeen(HerbId));
    TestFalse(TEXT("Seen item no longer marked"), Model->FindEntry(HerbId)->bIsNew);
    TestEqual(TEXT("Owned stack fills and overflows"), Model->AddItem(Herb, 4), 4);
    TestEqual(TEXT("Overflow uses one additional slot"), Model->GetOccupiedSlotCount(), 2);
    TestEqual(TEXT("Original stack filled"), Model->FindEntry(HerbId)->Item.Count, 3);
    TestFalse(TEXT("Increasing seen stack does not mark it new"), Model->FindEntry(HerbId)->bIsNew);
    TestFalse(TEXT("Overflow of an owned stackable type is not new"), Model->FindEntryAtSlot(1)->bIsNew);
    TestEqual(TEXT("Overflow preserves accepted quantity"), Model->FindEntryAtSlot(1)->Item.Count, 3);
    TestEqual(TEXT("Exhaust first stack"), Model->RemoveItem(HerbId, 100), 3);
    const FGuid RemainingId = Model->FindEntryAtSlot(1)->EntryId;
    TestEqual(TEXT("Exhaust remaining type"), Model->RemoveItem(RemainingId, 100), 3);
    Model->AddItem(Herb, 1);
    TestTrue(TEXT("Reacquisition after ownership ends is new"), Model->GetEntries()[0].bIsNew);

    auto Sword = InkInventoryTests::MakeItem(TEXT("Sword"), TEXT("Equipment"), false);
    Model->AddItem(Sword, 2);
    TestTrue(TEXT("First nonstackable copy is new"), Model->FindEntryAtSlot(1)->bIsNew);
    TestTrue(TEXT("Second nonstackable copy is independently new"), Model->FindEntryAtSlot(2)->bIsNew);
    Model->MarkSeen(Model->FindEntryAtSlot(1)->EntryId);
    TestTrue(TEXT("Hover does not clear a different copy"), Model->FindEntryAtSlot(2)->bIsNew);
    TestFalse(TEXT("Unknown hover has no effect"), Model->MarkSeen(FGuid::NewGuid()));
    return true;
}

// Incorrect category gating would allow a weapon to be consumed, a pill to be equipped,
// or a forbidden material to be used. Equipped items must remain protected from removal.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkInventoryActionsTest, "BoundlessImmortality.Frontend.Inventory.Actions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkInventoryActionsTest::RunTest(const FString&)
{
    auto* Model = NewObject<UInkInventoryModel>();
    auto Sword = InkInventoryTests::MakeItem(TEXT("Sword"), TEXT("Equipment"), false);
    Sword.bUsable = true; // Category restriction takes precedence over the author flag.
    auto OtherSword = Sword;
    OtherSword.ItemId = TEXT("OtherSword");
    auto Treasure = InkInventoryTests::MakeItem(TEXT("Treasure"), TEXT("Treasure"), false);
    auto Pill = InkInventoryTests::MakeItem(TEXT("Pill"), TEXT("Consumable"));
    auto Herb = InkInventoryTests::MakeItem(TEXT("Herb"));
    auto UsableMaterial = InkInventoryTests::MakeItem(TEXT("SpiritStone"));
    UsableMaterial.bUsable = true;
    const TArray<FInkInventoryItem> Items = {Sword, OtherSword, Treasure, Pill, Herb, UsableMaterial};
    for (const auto& Item : Items) Model->AddItem(Item, 2);
    auto FindId = [Model](FName Id)
    {
        for (const auto& Entry : Model->GetEntries()) if (Entry.Item.ItemId == Id) return Entry.EntryId;
        return FGuid();
    };
    const FGuid SwordId = FindId(TEXT("Sword"));
    const FGuid OtherSwordId = FindId(TEXT("OtherSword"));
    const FGuid TreasureId = FindId(TEXT("Treasure"));
    const FGuid PillId = FindId(TEXT("Pill"));
    const FGuid HerbId = FindId(TEXT("Herb"));
    const FGuid MaterialId = FindId(TEXT("SpiritStone"));
    TestFalse(TEXT("Equipment cannot Use despite author flag"), Model->CanUseEntry(SwordId));
    TestFalse(TEXT("Treasure cannot Use"), Model->CanUseEntry(TreasureId));
    TestEqual(TEXT("Forbidden use leaves equipment intact"), Model->UseItem(SwordId), 0);
    TestTrue(TEXT("Equipment can Equip"), Model->CanEquipEntry(SwordId));
    TestTrue(TEXT("Treasure can Equip"), Model->CanEquipEntry(TreasureId));
    TestFalse(TEXT("Pills never Equip"), Model->CanEquipEntry(PillId));
    TestFalse(TEXT("Materials never Equip"), Model->CanEquipEntry(MaterialId));
    TestFalse(TEXT("Ordinary material cannot Use"), Model->CanUseEntry(HerbId));
    TestEqual(TEXT("Pill use consumes requested quantity"), Model->UseItem(PillId, 1), 1);
    TestEqual(TEXT("Pill stack retains remaining quantity"), Model->FindEntry(PillId)->Item.Count, 1);
    TestEqual(TEXT("Usable material consumes requested quantity"), Model->UseItem(MaterialId, 2), 2);
    TestNull(TEXT("Exhausted usable material removes its slot"), Model->FindEntry(MaterialId));
    TestTrue(TEXT("Equip sword"), Model->EquipItem(SwordId));
    TestTrue(TEXT("Equipped state recorded"), Model->FindEntry(SwordId)->bEquipped);
    TestEqual(TEXT("Equipped item cannot be sold"), Model->SellItem(SwordId), 0);
    TestEqual(TEXT("Equipped item cannot be decomposed"), Model->DecomposeItem(SwordId), 0);
    TestTrue(TEXT("Replacement sword equips"), Model->EquipItem(OtherSwordId));
    TestFalse(TEXT("Replacement unequips previous same-slot sword"), Model->FindEntry(SwordId)->bEquipped);
    TestTrue(TEXT("Treasure equips independently"), Model->EquipItem(TreasureId));
    TestTrue(TEXT("Treasure does not replace weapon"), Model->FindEntry(OtherSwordId)->bEquipped);
    TestTrue(TEXT("Unequip reverses state"), Model->UnequipItem(OtherSwordId));
    TestEqual(TEXT("Unequipped sword can be sold"), Model->SellItem(OtherSwordId), 1);
    TestEqual(TEXT("Normal material can be decomposed"), Model->DecomposeItem(HerbId, 2), 2);

    auto StackSword = InkInventoryTests::MakeItem(TEXT("StackSword"), TEXT("Equipment"));
    Model->AddItem(StackSword, 3);
    const FGuid StackSwordId = FindId(TEXT("StackSword"));
    Model->MarkSeen(StackSwordId);
    TestTrue(TEXT("Stackable equipment remains equippable"), Model->EquipItem(StackSwordId));
    TestEqual(TEXT("New copies can join an equipped stack"), Model->AddItem(StackSword, 2), 2);
    TestEqual(TEXT("Equipped stack retains total quantity"), Model->FindEntry(StackSwordId)->Item.Count, 5);
    TestTrue(TEXT("Adding copies preserves equipped state"), Model->FindEntry(StackSwordId)->bEquipped);
    TestFalse(TEXT("Adding copies does not recreate new marker"), Model->FindEntry(StackSwordId)->bIsNew);
    TestFalse(TEXT("Stackable equipment still cannot Use"), Model->CanUseEntry(StackSwordId));
    TestEqual(TEXT("Equipped stack remains protected from sale"), Model->SellItem(StackSwordId, 5), 0);
    Model->UnequipItem(StackSwordId);
    TestEqual(TEXT("After unequip, stack can be sold in quantity"), Model->SellItem(StackSwordId, 4), 4);
    TestEqual(TEXT("Quantity sale leaves one copy"), Model->FindEntry(StackSwordId)->Item.Count, 1);
    return true;
}

// Sorting by array index instead of stable identity would act on a different item here.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkInventoryStableSelectionTest, "BoundlessImmortality.Frontend.Inventory.StableIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkInventoryStableSelectionTest::RunTest(const FString&)
{
    auto* Model = NewObject<UInkInventoryModel>();
    auto Pill = InkInventoryTests::MakeItem(TEXT("ZPill"), TEXT("Consumable"));
    auto Sword = InkInventoryTests::MakeItem(TEXT("ASword"), TEXT("Equipment"), false);
    Model->AddItem(Pill, 5);
    Model->AddItem(Sword, 1);
    const FGuid SelectedPill = Model->FindEntryAtSlot(0)->EntryId;
    Model->MarkSeen(SelectedPill);
    Model->SortItems();
    if (!TestNotNull(TEXT("Selected identity survives sorting"), Model->FindEntry(SelectedPill))) return false;
    TestEqual(TEXT("Selection still refers to pill"), Model->FindEntry(SelectedPill)->Item.ItemId, FName(TEXT("ZPill")));
    TestEqual(TEXT("Equipment sorts ahead of pill"), Model->FindEntryAtSlot(0)->Item.ItemId, FName(TEXT("ASword")));
    TestFalse(TEXT("Sorting preserves seen state"), Model->FindEntry(SelectedPill)->bIsNew);
    TestEqual(TEXT("Action after sort consumes the selected pill"), Model->UseItem(SelectedPill, 2), 2);
    TestEqual(TEXT("Selected pill now has three"), Model->FindEntry(SelectedPill)->Item.Count, 3);
    TestEqual(TEXT("Sword remains unchanged"), Model->FindEntryAtSlot(0)->Item.Count, 1);
    Model->SortItems(false);
    TestEqual(TEXT("Descending order puts pill before equipment"), Model->FindEntryAtSlot(0)->Item.ItemId, FName(TEXT("ZPill")));
    TestEqual(TEXT("Descending sort retains selected identity"), Model->FindEntry(SelectedPill)->SlotIndex, 0);
    TestEqual(TEXT("Descending sort retains selected quantity"), Model->FindEntry(SelectedPill)->Item.Count, 3);
    TestFalse(TEXT("Descending sort does not restore new marker"), Model->FindEntry(SelectedPill)->bIsNew);
    Model->SortItems();
    TestEqual(TEXT("Default sort restores ascending order"), Model->FindEntryAtSlot(0)->Item.ItemId, FName(TEXT("ASword")));
    return true;
}

// Quantity conversion, bad input, or accidental automatic stacking of nonstackable items
// would lose quantities in this range or change the expected occupied-slot count.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkInventoryQuantityRangeTest, "BoundlessImmortality.Frontend.Inventory.Quantity1To100", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkInventoryQuantityRangeTest::RunTest(const FString&)
{
    for (const TCHAR* Category : {TEXT("Equipment"), TEXT("Material"), TEXT("Consumable"), TEXT("Treasure")})
    {
        for (int32 Quantity = 1; Quantity <= 100; ++Quantity)
        {
            auto* StackModel = NewObject<UInkInventoryModel>();
            auto StackItem = InkInventoryTests::MakeItem(TEXT("RangeStack"), Category);
            TestEqual(TEXT("Stack accepts requested range quantity"), StackModel->AddItem(StackItem, Quantity), Quantity);
            TestEqual(TEXT("Stack occupies one slot"), StackModel->GetOccupiedSlotCount(), 1);
            TestEqual(TEXT("Stack retains requested quantity"), StackModel->FindEntryAtSlot(0)->Item.Count, Quantity);

            auto* SingleModel = NewObject<UInkInventoryModel>();
            SingleModel->SetUnlockedSlotCount(100);
            auto SingleItem = InkInventoryTests::MakeItem(TEXT("RangeSingle"), Category, false);
            TestEqual(TEXT("Nonstackable accepts requested range quantity"), SingleModel->AddItem(SingleItem, Quantity), Quantity);
            TestEqual(TEXT("Each nonstackable copy uses a slot"), SingleModel->GetOccupiedSlotCount(), Quantity);
            TestEqual(TEXT("Every individual entry has quantity one"), SingleModel->FindEntryAtSlot(Quantity - 1)->Item.Count, 1);
        }
    }
    auto* Model = NewObject<UInkInventoryModel>();
    auto Invalid = InkInventoryTests::MakeItem(TEXT(""));
    TestEqual(TEXT("Missing item identity rejected"), Model->AddItem(Invalid, 5), 0);
    auto Valid = InkInventoryTests::MakeItem(TEXT("Valid"));
    TestEqual(TEXT("Zero acquisition rejected"), Model->AddItem(Valid, 0), 0);
    TestEqual(TEXT("Negative acquisition rejected"), Model->AddItem(Valid, -2), 0);
    Valid.Count = 7;
    Model->SetItems({Valid});
    TestEqual(TEXT("Import uses Count from input"), Model->FindEntryAtSlot(0)->Item.Count, 7);
    TestFalse(TEXT("Initial data import is already seen"), Model->FindEntryAtSlot(0)->bIsNew);
    TestEqual(TEXT("Negative removal rejected"), Model->RemoveItem(Model->FindEntryAtSlot(0)->EntryId, -1), 0);
    TestEqual(TEXT("Invalid removal leaves quantity intact"), Model->FindEntryAtSlot(0)->Item.Count, 7);
    return true;
}

#endif
