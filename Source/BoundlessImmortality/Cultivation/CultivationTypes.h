#pragma once
#include "CoreMinimal.h"
#include "CultivationTypes.generated.h"

UENUM(BlueprintType)
enum class ECultivationGift : uint8 { FairyWand, LoveExplosion, RareChest, SuperJet, MysteryAirdrop, Strawberry, SuperAirdrop, LifePotion };

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationAttributes
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double MaxHealth=100;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double HealthRegeneration=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double Attack=10;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double Defense=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double SpiritDamage=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double CultivationSpeed=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double CriticalChance=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double EvasionChance=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double Luck=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double MoveSpeed=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double Swordsmanship=1;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationRuleConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") TArray<FText> RealmNames;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") TArray<double> ExperienceThresholds;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") TArray<int32> BreakthroughMaterialCosts;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") FName BreakthroughMaterialId=TEXT("RealmStone");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") double DefaultThreshold=100;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") double ThresholdGrowth=1.65;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") double AttributeGrowthPerRealm=.25;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="境界") double AnchorBonusPerRealm=.18;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="属性") FCultivationAttributes BaseAttributes;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修为") double MeditationPerSecond=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修为") double ExperiencePerHit=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修为") double ExperiencePerKill=10;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="修为") double OfflinePerSecond=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="掉落") int32 MaterialPerKill=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double WandBase=1.15;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double WandStackCoefficient=.5;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double WandSeconds=30;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double LoveExperience=19999;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double SoulPersonalMultiplier=2.8;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double SoulAudienceShare=.2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double SoulAnchorShare=.5;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double SoulDuration=90;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double SoulCooldown=180;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double JetFlatBonus=200000;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double JetDuration=300;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double JetCooldown=180;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double EnemyReduction=.2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double ImmunityChance=.2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") int32 StarCharges=7;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double StarCooldown=180;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double StarHealthThreshold=.5;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double StarHealFraction=.2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double PotionBase=3.25;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double PotionStackCoefficient=.65;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double PotionSeconds=21600;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double PotionAudienceMultiplier=1.28;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="礼物") double PotionAnchorMultiplier=2;
    FCultivationRuleConfig();
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationGiftEvent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString MessageId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) ECultivationGift GiftId=ECultivationGift::FairyWand;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 Count=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double EventUtcSeconds=0;
    /** Original room of this receipt, preserved when a pending gift follows its owner. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString SourceRoomId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bDevelopment=true;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationPotionSource
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString RoomId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double ExpiresUtcSeconds=0;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationParticipantRecord
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString DisplayName;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString AvatarUrl;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bIsAnchor=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bPresent=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 RealmIndex=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double Cultivation=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TMap<FName,int32> Materials;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<ECultivationGift> UnlockedGifts;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FName TitleId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int64 WandCount=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double WandExpiresUtc=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int64 PotionCount=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double PotionExpiresUtc=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FString PotionRoomId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double SoulActiveUntil=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double SoulReadyAt=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double JetActiveUntil=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double JetReadyAt=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<double> StarReadyAt;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int64 Version=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double LastSeenUtc=0;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationParticipantSnapshot
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString ParticipantId;
    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(BlueprintReadOnly) FString AvatarUrl;
    UPROPERTY(BlueprintReadOnly) bool bIsAnchor=false;
    UPROPERTY(BlueprintReadOnly) bool bPresent=false;
    UPROPERTY(BlueprintReadOnly) int32 RealmIndex=0;
    UPROPERTY(BlueprintReadOnly) FText RealmName;
    UPROPERTY(BlueprintReadOnly) FName TitleId;
    UPROPERTY(BlueprintReadOnly) FText TitleName;
    UPROPERTY(BlueprintReadOnly) double Cultivation=0;
    UPROPERTY(BlueprintReadOnly) double RequiredCultivation=0;
    UPROPERTY(BlueprintReadOnly) double EffectiveCultivationPerSecond=0;
    UPROPERTY(BlueprintReadOnly) double ActualDPS=0;
    UPROPERTY(BlueprintReadOnly) FCultivationAttributes PersonalAttributes;
    UPROPERTY(BlueprintReadOnly) FCultivationAttributes Attributes;
    UPROPERTY(BlueprintReadOnly) double PotionExpiresUtc=0;
    UPROPERTY(BlueprintReadOnly) int64 WandCount=0;
    UPROPERTY(BlueprintReadOnly) int64 PotionCount=0;
    UPROPERTY(BlueprintReadOnly) int32 ReadyStarCharges=0;
    UPROPERTY(BlueprintReadOnly) int32 BreakthroughMaterialCount=0;
    UPROPERTY(BlueprintReadOnly) int32 BreakthroughMaterialCost=0;
    /** Two-second local presentation pulse; never persisted as progression. */
    UPROPERTY(BlueprintReadOnly) double RealmAdvancePulse=0;
};
