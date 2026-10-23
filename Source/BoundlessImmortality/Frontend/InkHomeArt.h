#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "InkHomeArt.generated.h"

namespace InkHomeArt
{
    struct FSprite { const TCHAR* Id; const TCHAR* Name; FVector2D Size; FVector4 MarginsLRTB; };
    BOUNDLESSIMMORTALITY_API const FSprite* Find(FName Id);
    BOUNDLESSIMMORTALITY_API FSlateBrush Brush(FName Id);
    BOUNDLESSIMMORTALITY_API FSlateRect Fit(FVector2D Source,FSlateRect Bounds);
    BOUNDLESSIMMORTALITY_API bool HorizontalSlice(FName Id,FVector2D Destination);
    BOUNDLESSIMMORTALITY_API FSlateRect ArtBounds(FName Id,FSlateRect Container);
    BOUNDLESSIMMORTALITY_API FMargin NormalizedMargin(FName Id);
    BOUNDLESSIMMORTALITY_API void Paint(FName Id,const FSlateBrush& Brush,FSlateRect Bounds,const FGeometry& Geometry,FSlateWindowElementList& Elements,int32 Layer,FLinearColor Tint=FLinearColor::White,bool ForceFit=false);
}

/** Approved home art only; owns no business state or input. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkHomeArtWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Ink UI|Home") void Configure(FName Id);
    UFUNCTION(BlueprintPure,Category="Ink UI|Home") FString DescribeArt()const;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Home") FName Component;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Home") FVector2D PolicyContainerSize=FVector2D::ZeroVector;
protected:
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
private:
    UPROPERTY(Transient) FSlateBrush Sprite;
};
