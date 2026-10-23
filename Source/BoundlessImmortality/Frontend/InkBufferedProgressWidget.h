#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkProgressState.h"
#include "Styling/SlateBrush.h"
#include "InkBufferedProgressWidget.generated.h"

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkBufferedProgressWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UInkBufferedProgressWidget(const FObjectInitializer& Initializer);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Progress") FInkProgressSettings Settings;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Progress") FLinearColor FillColor=FLinearColor(.24f,.49f,.47f,1);
    UPROPERTY(Transient,BlueprintReadOnly,Category="Progress") float Fraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Progress") float TrailFraction=0;
    UFUNCTION(BlueprintCallable,Category="Progress") void SetFraction(float Value,bool bReset=false);
protected:
    virtual void NativeTick(const FGeometry& Geometry,float Delta)override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
private:
    FInkProgressState State;
    UPROPERTY(Transient) FSlateBrush Track;
    UPROPERTY(Transient) FSlateBrush Fill;
};
