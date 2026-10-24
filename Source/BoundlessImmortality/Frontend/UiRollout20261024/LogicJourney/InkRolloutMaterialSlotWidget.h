#pragma once
#include "Frontend/UiRollout20261024/Player/InkPlayerMaterialSlotWidget.h"
#include "InkRolloutMaterialSlotWidget.generated.h"

/** Keeps the additive full-image square frame at its authored aspect ratio. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkRolloutMaterialSlotWidget : public UInkPlayerMaterialSlotWidget
{
    GENERATED_BODY()
public:
    static UTexture2D* MaterialFrame();
protected:
    virtual void NativeOnInitialized() override;
};
