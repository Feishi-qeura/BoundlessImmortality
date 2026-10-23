#pragma once

#include "CoreMinimal.h"
#include "Components/ScrollBar.h"
#include "InkInventoryScrollBar.generated.h"

class UScrollBox;

/** Independently skinnable drag handle; the painted rail is a sibling image. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API UInkInventoryScrollBar : public UScrollBar
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Scrolling")
    TObjectPtr<UScrollBox> TargetScrollBox;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Scrolling",meta=(ClampMin="0.05",ClampMax="0.95"))
    float ThumbFraction=.42f;

    UFUNCTION(BlueprintCallable, Category="Ink UI|Scrolling")
    void SynchronizeScrollPosition();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void HandleUserScrolled(float OffsetFraction);
};
