#pragma once

#include "CoreMinimal.h"
#include "InkFrontendTypes.generated.h"

class UTexture2D;

/** Display data supplied by the game. The frontend does not invent progression or currency. */
USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkCharacterAttribute
{
    GENERATED_BODY()
    /** Health / Attack / Defense / SpiritDamage / Critical / Evasion / Luck / MoveSpeed / Swordsmanship / CultivationSpeed / Realm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName AttributeId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Value;
    /** Typed score input. Health uses MaxHealth; probabilities use 0..1, never display strings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bHasNumericValue=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double NumericValue=0;
    /** Unmodified walking speed for movement grading, in cm/s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double RatingBaseValue=0;
    /** Optional override; otherwise AttributeId chooses the matching ink icon. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkDisciplineEntry
{
    GENERATED_BODY()
    /** Inner / Movement / Martial / Manual. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Category=TEXT("Inner");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Detail;
    /** Equipped item's quality, 1..6. Zero means unknown/unconfigured and emits no glow. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,meta=(ClampMin="0",ClampMax="6")) int32 QualityTier=0;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkInventoryItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Description;
    /** Equipment / Material / Consumable (pills) / Treasure. Other values appear under All. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Category=TEXT("Material");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 Count=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> Icon;
    /** Nonstackable acquisitions allocate one slot per copy. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Stacking") bool bStackable=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Stacking", meta=(ClampMin="1")) int32 MaxStackSize=100;
    /** Materials may opt into Use. Consumables always support Use; equipment and treasures never do. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Actions") bool bUsable=false;
    /** Weapon / Armor / Ring / Necklace; treasures occupy their own category of equipment slots. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Actions") FName EquipmentSlot=TEXT("Weapon");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Actions") bool bSellable=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Actions") bool bDecomposable=true;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkLeaderboardEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    /** Anchor / CultivationSpeed / Damage. The provider owns ordering and ranking. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Category=TEXT("Anchor");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1")) int32 Rank=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Metric;
    /** Optional earned title: LeftGuardian / RightGuardian / Novice / Godslayer / SoulEater / SevenStar / DivineBlade / Cyberspace. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TitleId;
    /** Optional realm shown after the metric on compact boards. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Realm;
    /** Authoritative stage index for the existing 34 realm banner images. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RealmIndex=INDEX_NONE;
    /** Optional actual provider metric, in damage/s, sword count or cultivation/s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) double NumericMetric=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UTexture2D> AvatarTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString AvatarUrl;
};
