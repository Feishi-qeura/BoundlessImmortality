#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkLaunchHudCountdownWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UInkFrontendWidget;

/** Compact HUD launch status entry; visibility and countdown use the authoritative launch snapshot. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkLaunchHudCountdownWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UInkLaunchHudCountdownWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Activities")
    void Configure(UInkFrontendWidget* InFrontend);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

private:
    UPROPERTY(EditDefaultsOnly, Category="Ink UI|Activities") TObjectPtr<UTexture2D> FrameTexture;
    UPROPERTY(Transient) TObjectPtr<UImage> FrameImage;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CountdownText;
    UPROPERTY(Transient) TObjectPtr<UButton> HitTarget;
    UPROPERTY(Transient) TWeakObjectPtr<UInkFrontendWidget> Frontend;
    double NextRefreshAt = 0.0;
    double LastAuthorityUtc = 0.0;
    double LastAuthorityMonotonic = 0.0;

    UFUNCTION() void HandleOpenLaunchEvent();
    void RefreshFromAuthority(double MonotonicNow);
};
