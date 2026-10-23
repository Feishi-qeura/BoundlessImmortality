#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Cultivation/CultivationTypes.h"
#include "InkAttributeRating.generated.h"

UENUM(BlueprintType)
enum class EInkAttributeRating : uint8 { D, C, B, A, S, SS, SSS, Unrated };

USTRUCT(BlueprintType)
struct FInkAttributeRatingContext
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 RealmIndex=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bV14Rules=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FCultivationAttributes BaseAttributes;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double LegacyRealmGrowth=.25;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double LegacyMeditationPerSecond=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) double BaseMoveSpeed=400;
};

/** Read-only display grading, with one fixed set of thresholds per major realm. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkAttributeRatingLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure,Category="Ink UI|Attribute Rating") static int32 MajorIndex(int32 RealmIndex);
    UFUNCTION(BlueprintPure,Category="Ink UI|Attribute Rating") static FString MajorName(int32 RealmIndex);
    UFUNCTION(BlueprintPure,Category="Ink UI|Attribute Rating") static TArray<double> Thresholds(FName Attribute,const FInkAttributeRatingContext& Context);
    UFUNCTION(BlueprintPure,Category="Ink UI|Attribute Rating") static EInkAttributeRating Evaluate(FName Attribute,double Value,const FInkAttributeRatingContext& Context);
    UFUNCTION(BlueprintPure,Category="Ink UI|Attribute Rating") static FString GradeName(EInkAttributeRating Grade);
};
