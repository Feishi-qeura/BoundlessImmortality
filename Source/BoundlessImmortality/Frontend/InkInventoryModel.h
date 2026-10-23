#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFrontendTypes.h"
#include "UObject/Object.h"
#include "InkInventoryModel.generated.h"

/** One occupied physical bag slot. EntryId survives sorting and quantity changes. */
USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") FGuid EntryId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") FInkInventoryItem Item;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") int32 SlotIndex=INDEX_NONE;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") bool bIsNew=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") bool bEquipped=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") FString AuthorityInstanceId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory") double ExpiresUtc=0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkInventoryChanged);

/** Inventory state independent of the visual grid, category filters and current selection. */
UCLASS(BlueprintType, Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkInventoryModel : public UObject
{
    GENERATED_BODY()

public:
    static constexpr int32 TotalSlots=200;

    UPROPERTY(BlueprintAssignable, Category="Inventory") FInkInventoryChanged OnChanged;

    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetTotalSlotCount() const { return bAuthoritative?FMath::Max(TotalSlots,Entries.Num()):TotalSlots; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetUnlockedSlotCount() const { return UnlockedSlotCount; }
    UFUNCTION(BlueprintPure, Category="Inventory") int32 GetOccupiedSlotCount() const { return Entries.Num(); }
    UFUNCTION(BlueprintPure, Category="Inventory") TArray<FInkInventoryEntry> GetEntries() const { return Entries; }
    const TArray<FInkInventoryEntry>& GetEntriesView() const { return Entries; }
    const FInkInventoryEntry* FindEntry(FGuid EntryId) const;
    const FInkInventoryEntry* FindEntryAtSlot(int32 SlotIndex) const;
    /** Read-only projection: no purchase/unlock or local consumption of remote assets. */
    void ApplyAuthoritativeEntries(const TArray<FInkInventoryEntry>& InEntries,const FString& Revision);
    bool IsAuthoritative()const{return bAuthoritative;}

    /** Returns the actual unlocked count; occupied slots are never hidden or discarded. */
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 SetUnlockedSlotCount(int32 Count);
    /** Replaces supplied inventory data, using each item's Count. Returns the quantity that could not fit. */
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 SetItems(const TArray<FInkInventoryItem>& Items, bool bMarkNew=false);
    /** Quantity overrides Item.Count. Returns accepted quantity, allowing the provider to retain overflow. */
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 AddItem(const FInkInventoryItem& Item, int32 Quantity=1);
    /** Returns removed quantity. Zero/negative quantities and unknown identities have no effect. */
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 RemoveItem(FGuid EntryId, int32 Quantity=1);
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 UseItem(FGuid EntryId, int32 Quantity=1);
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 SellItem(FGuid EntryId, int32 Quantity=1);
    UFUNCTION(BlueprintCallable, Category="Inventory") int32 DecomposeItem(FGuid EntryId, int32 Quantity=1);
    UFUNCTION(BlueprintPure, Category="Inventory") bool CanUseEntry(FGuid EntryId) const;
    UFUNCTION(BlueprintPure, Category="Inventory") bool CanEquipEntry(FGuid EntryId) const;
    /** Equipping replaces another item in the same category and equipment slot. */
    UFUNCTION(BlueprintCallable, Category="Inventory") bool EquipItem(FGuid EntryId);
    UFUNCTION(BlueprintCallable, Category="Inventory") bool UnequipItem(FGuid EntryId);
    /** Called when the cursor enters an occupied slot, clearing only that entry's new marker. */
    UFUNCTION(BlueprintCallable, Category="Inventory") bool MarkSeen(FGuid EntryId);
    /** Compacts slots by category/name while preserving every entry's identity and new/equipped state. */
    UFUNCTION(BlueprintCallable, Category="Inventory") void SortItems(bool bAscending=true);

private:
    UPROPERTY(Transient) TArray<FInkInventoryEntry> Entries;
    UPROPERTY(Transient) int32 UnlockedSlotCount=20;
    bool bAuthoritative=false;
    FString AuthorityRevision;

    FInkInventoryEntry* FindMutableEntry(FGuid EntryId);
    int32 AddItemInternal(const FInkInventoryItem& Item, int32 Quantity, bool bMarkNew);
    int32 FindFirstEmptySlot() const;
    static bool CanStackTogether(const FInkInventoryItem& Existing, const FInkInventoryItem& Incoming);
};
