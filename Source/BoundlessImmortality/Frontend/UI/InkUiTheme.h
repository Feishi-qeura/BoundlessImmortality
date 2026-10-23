#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "InkUiTheme.generated.h"

class UFont;
UENUM(BlueprintType)
enum class EInkUiTextRole : uint8 { Display, Section, Body, Button, Data, Secondary, HudLabel };

UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API UInkUiThemeData : public UDataAsset
{
    GENERATED_BODY()
public:
    UInkUiThemeData();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") TObjectPtr<UFont> DisplayFont;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") TObjectPtr<UFont> BodyFont;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") TObjectPtr<UFont> DataFont;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") TObjectPtr<UFont> FallbackFont;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 DisplaySize=44;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 SectionSize=32;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 BodySize=28;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 ButtonSize=28;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 DataSize=26;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 SecondarySize=24;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography") int32 HudLabelSize=26;
    /** Tokens are logical pixel sizes; Slate measures points at 96 DPI (project design DPI is 72). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Typography",meta=(ClampMin="0.5",ClampMax="1.5")) float SlatePointScale=.75f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Layout",meta=(ClampMin="0",ClampMax="64")) float OuterPadding=24;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Layout",meta=(ClampMin="88",ClampMax="128")) float MinHitSize=88;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface") FLinearColor PaperTint;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface") FLinearColor InkTint;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface") FLinearColor GoldTint;
    /** One page shell owns this paper; embedded content never paints another full paper. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface",meta=(ClampMin="0.7",ClampMax="1")) float PaperOpacity=1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface",meta=(ClampMin="0",ClampMax="0.8")) float BackdropOpacity=.35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface",meta=(ClampMin="0",ClampMax="0.3")) float DecorationOpacity=.08f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Layout",meta=(ClampMin="88",ClampMax="128")) float HeaderHeight=88.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Layout",meta=(ClampMin="104",ClampMax="160")) float FooterHeight=104.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ink UI|Surface", meta=(ClampMin="24",ClampMax="48")) float FeatherWidth=36;
};

namespace InkUiTypography
{
    BOUNDLESSIMMORTALITY_API const UInkUiThemeData* Theme();
    BOUNDLESSIMMORTALITY_API FSlateFontInfo MakeFont(EInkUiTextRole Role, int32 SizeOverride=0);
}
