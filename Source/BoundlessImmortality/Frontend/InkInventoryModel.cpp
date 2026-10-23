#include "Frontend/InkInventoryModel.h"

namespace InkInventory
{
int32 CategoryOrder(FName Category)
{
    if (Category == TEXT("Equipment")) return 0;
    if (Category == TEXT("Material")) return 1;
    if (Category == TEXT("Consumable")) return 2;
    if (Category == TEXT("Treasure")) return 3;
    return 4;
}
}

const FInkInventoryEntry* UInkInventoryModel::FindEntry(FGuid EntryId) const
{
    return Entries.FindByPredicate([EntryId](const FInkInventoryEntry& Entry) { return Entry.EntryId == EntryId; });
}

FInkInventoryEntry* UInkInventoryModel::FindMutableEntry(FGuid EntryId)
{
    return Entries.FindByPredicate([EntryId](const FInkInventoryEntry& Entry) { return Entry.EntryId == EntryId; });
}

const FInkInventoryEntry* UInkInventoryModel::FindEntryAtSlot(int32 SlotIndex) const
{
    return Entries.FindByPredicate([SlotIndex](const FInkInventoryEntry& Entry) { return Entry.SlotIndex == SlotIndex; });
}

void UInkInventoryModel::ApplyAuthoritativeEntries(const TArray<FInkInventoryEntry>& InEntries,const FString& Revision)
{
    if(bAuthoritative&&AuthorityRevision==Revision)return;
    TSet<FGuid> Seen;for(const auto& E:Entries)if(!E.bIsNew)Seen.Add(E.EntryId);
    Entries=InEntries;bAuthoritative=true;AuthorityRevision=Revision;
    for(int32 I=0;I<Entries.Num();++I){Entries[I].SlotIndex=I;Entries[I].bIsNew=!Seen.Contains(Entries[I].EntryId);}
    UnlockedSlotCount=GetTotalSlotCount();OnChanged.Broadcast();
}

int32 UInkInventoryModel::SetUnlockedSlotCount(int32 Count)
{
    if(bAuthoritative)return UnlockedSlotCount;
    int32 MinimumOccupiedCount = 0;
    for (const auto& Entry : Entries) MinimumOccupiedCount = FMath::Max(MinimumOccupiedCount, Entry.SlotIndex + 1);
    const int32 NewCount = FMath::Max(MinimumOccupiedCount, FMath::Clamp(Count, 0, TotalSlots));
    if (NewCount != UnlockedSlotCount)
    {
        UnlockedSlotCount = NewCount;
        OnChanged.Broadcast();
    }
    return UnlockedSlotCount;
}

int32 UInkInventoryModel::SetItems(const TArray<FInkInventoryItem>& Items, bool bMarkNew)
{
    if(bAuthoritative)return 0;
    Entries.Reset();
    int64 RejectedQuantity = 0;
    for (const auto& Item : Items)
    {
        const int32 Requested = FMath::Max(0, Item.Count);
        RejectedQuantity += Requested - AddItemInternal(Item, Requested, bMarkNew);
    }
    OnChanged.Broadcast();
    return static_cast<int32>(FMath::Min<int64>(RejectedQuantity, MAX_int32));
}

bool UInkInventoryModel::CanStackTogether(const FInkInventoryItem& Existing, const FInkInventoryItem& Incoming)
{
    return Existing.bStackable && Incoming.bStackable && Existing.ItemId == Incoming.ItemId
        && Existing.Category == Incoming.Category && Existing.EquipmentSlot == Incoming.EquipmentSlot
        && Existing.bUsable == Incoming.bUsable && Existing.bSellable == Incoming.bSellable
        && Existing.bDecomposable == Incoming.bDecomposable
        && FMath::Max(1, Existing.MaxStackSize) == FMath::Max(1, Incoming.MaxStackSize);
}

int32 UInkInventoryModel::FindFirstEmptySlot() const
{
    for (int32 Slot = 0; Slot < UnlockedSlotCount; ++Slot) if (!FindEntryAtSlot(Slot)) return Slot;
    return INDEX_NONE;
}

int32 UInkInventoryModel::AddItemInternal(const FInkInventoryItem& Item, int32 Quantity, bool bMarkNew)
{
    if (Item.ItemId.IsNone() || Quantity <= 0) return 0;
    const bool bOwnedStackableType = Item.bStackable && Entries.ContainsByPredicate([&Item](const FInkInventoryEntry& Entry)
    {
        return Entry.Item.ItemId == Item.ItemId && Entry.Item.bStackable;
    });
    int32 Remaining = Quantity;
    if (Item.bStackable)
    {
        for (auto& Entry : Entries)
        {
            if (!CanStackTogether(Entry.Item, Item)) continue;
            const int32 Available = FMath::Max(0, FMath::Max(1, Entry.Item.MaxStackSize) - Entry.Item.Count);
            const int32 Added = FMath::Min(Available, Remaining);
            Entry.Item.Count += Added;
            Remaining -= Added;
            if (Remaining == 0) break;
        }
    }
    while (Remaining > 0)
    {
        const int32 SlotIndex = FindFirstEmptySlot();
        if (SlotIndex == INDEX_NONE) break;
        FInkInventoryEntry& Entry = Entries.AddDefaulted_GetRef();
        Entry.EntryId = FGuid::NewGuid();
        Entry.Item = Item;
        Entry.Item.MaxStackSize = FMath::Max(1, Item.MaxStackSize);
        Entry.Item.Count = Item.bStackable ? FMath::Min(Remaining, Entry.Item.MaxStackSize) : 1;
        Entry.SlotIndex = SlotIndex;
        Entry.bIsNew = bMarkNew && !bOwnedStackableType;
        Remaining -= Entry.Item.Count;
    }
    return Quantity - Remaining;
}

int32 UInkInventoryModel::AddItem(const FInkInventoryItem& Item, int32 Quantity)
{
    if(bAuthoritative)return 0;
    // Native callers may supply an existing entry's Item by reference; allocations below
    // can relocate Entries, so keep the definition independent of its source storage.
    const FInkInventoryItem ItemDefinition = Item;
    const int32 Accepted = AddItemInternal(ItemDefinition, Quantity, true);
    if (Accepted > 0) OnChanged.Broadcast();
    return Accepted;
}

int32 UInkInventoryModel::RemoveItem(FGuid EntryId, int32 Quantity)
{
    if(bAuthoritative)return 0;
    FInkInventoryEntry* Entry = FindMutableEntry(EntryId);
    if (!Entry || Entry->bEquipped || Quantity <= 0) return 0;
    const int32 Removed = FMath::Min(Entry->Item.Count, Quantity);
    Entry->Item.Count -= Removed;
    if (Entry->Item.Count == 0) Entries.RemoveAll([EntryId](const FInkInventoryEntry& Candidate) { return Candidate.EntryId == EntryId; });
    if (Removed > 0) OnChanged.Broadcast();
    return Removed;
}

bool UInkInventoryModel::CanUseEntry(FGuid EntryId) const
{
    const FInkInventoryEntry* Entry = FindEntry(EntryId);
    if (!Entry || Entry->bEquipped || Entry->Item.Count <= 0) return false;
    return Entry->Item.Category == TEXT("Consumable") || (Entry->Item.Category == TEXT("Material") && Entry->Item.bUsable);
}

bool UInkInventoryModel::CanEquipEntry(FGuid EntryId) const
{
    const FInkInventoryEntry* Entry = FindEntry(EntryId);
    return Entry && Entry->Item.Count > 0 && (Entry->Item.Category == TEXT("Equipment") || Entry->Item.Category == TEXT("Treasure"));
}

int32 UInkInventoryModel::UseItem(FGuid EntryId, int32 Quantity)
{
    return CanUseEntry(EntryId) ? RemoveItem(EntryId, Quantity) : 0;
}

int32 UInkInventoryModel::SellItem(FGuid EntryId, int32 Quantity)
{
    const FInkInventoryEntry* Entry = FindEntry(EntryId);
    return Entry && Entry->Item.bSellable ? RemoveItem(EntryId, Quantity) : 0;
}

int32 UInkInventoryModel::DecomposeItem(FGuid EntryId, int32 Quantity)
{
    const FInkInventoryEntry* Entry = FindEntry(EntryId);
    return Entry && Entry->Item.bDecomposable ? RemoveItem(EntryId, Quantity) : 0;
}

bool UInkInventoryModel::EquipItem(FGuid EntryId)
{
    if(bAuthoritative)return false;
    if (!CanEquipEntry(EntryId)) return false;
    FInkInventoryEntry* Selected = FindMutableEntry(EntryId);
    if (Selected->bEquipped) return true;
    for (auto& Entry : Entries)
    {
        if (Entry.Item.Category == Selected->Item.Category && Entry.Item.EquipmentSlot == Selected->Item.EquipmentSlot)
            Entry.bEquipped = Entry.EntryId == EntryId;
    }
    OnChanged.Broadcast();
    return true;
}

bool UInkInventoryModel::UnequipItem(FGuid EntryId)
{
    if(bAuthoritative)return false;
    FInkInventoryEntry* Entry = FindMutableEntry(EntryId);
    if (!Entry || !Entry->bEquipped) return false;
    Entry->bEquipped = false;
    OnChanged.Broadcast();
    return true;
}

bool UInkInventoryModel::MarkSeen(FGuid EntryId)
{
    FInkInventoryEntry* Entry = FindMutableEntry(EntryId);
    if (!Entry || !Entry->bIsNew) return false;
    Entry->bIsNew = false;
    OnChanged.Broadcast();
    return true;
}

void UInkInventoryModel::SortItems(bool bAscending)
{
    Entries.StableSort([bAscending](const FInkInventoryEntry& A, const FInkInventoryEntry& B)
    {
        const int32 ACategory = InkInventory::CategoryOrder(A.Item.Category);
        const int32 BCategory = InkInventory::CategoryOrder(B.Item.Category);
        if (ACategory != BCategory) return bAscending ? ACategory < BCategory : ACategory > BCategory;
        const int32 NameOrder = A.Item.Name.ToString().Compare(B.Item.Name.ToString(), ESearchCase::IgnoreCase);
        if (NameOrder != 0) return bAscending ? NameOrder < 0 : NameOrder > 0;
        return bAscending ? A.Item.ItemId.LexicalLess(B.Item.ItemId) : B.Item.ItemId.LexicalLess(A.Item.ItemId);
    });
    for (int32 SlotIndex = 0; SlotIndex < Entries.Num(); ++SlotIndex) Entries[SlotIndex].SlotIndex = SlotIndex;
    OnChanged.Broadcast();
}
