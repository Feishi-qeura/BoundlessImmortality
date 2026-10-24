#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkPlayerMaterialSlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInkPlayerMaterialSlotActivated, FName, SlotId, FName, ItemId);

/**
 * Presentation-only material slot for Player panels.
 * All names, counts, icons, selection permission, and frame art are supplied by the owning
 * authority-backed adapter. This widget never invents quantities or executes gameplay actions.
 */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API UInkPlayerMaterialSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Player")
    void ConfigureSlot(FName InSlotId, FName InItemId, const FText& InItemName,
        const FText& InOwnedCount, const FText& InRequiredCount,
        UTexture2D* InItemIcon, UTexture2D* InFrameTexture, bool bInCanSelect);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Player")
    void SetSelectionEnabled(bool bEnabled);

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Player")
    FInkPlayerMaterialSlotActivated OnSlotActivated;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Player")
    FName SlotId;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Player")
    FName ItemId;

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(Transient) TObjectPtr<UButton> SlotButton;
    UPROPERTY(Transient) TObjectPtr<UImage> FrameImage;
    UPROPERTY(Transient) TObjectPtr<UImage> ItemImage;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ItemNameText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> OwnedCountText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RequiredCountText;

    bool bCanSelect = false;

    void BuildDefaultTree();
    UFUNCTION() void HandleSlotClicked();
};
