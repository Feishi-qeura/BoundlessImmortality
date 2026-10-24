#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InkCoreMaterialBinding.generated.h"

class UInkFrontendWidget;
class UInkPlayerMaterialSlotWidget;
class UTexture2D;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkCoreBindingFeedback,const FString&,Message);

/** Keeps the delivered Player material atom on the existing authority action path. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkCoreMaterialBinding : public UObject
{
    GENERATED_BODY()
public:
    // Host owns this object with a UPROPERTY and calls Refresh after service changes.
    void Configure(UInkFrontendWidget* Owner,FName Route,bool Rework=false);
    void BindSlot(UInkPlayerMaterialSlotWidget* Slot,FName SlotId,FName ItemId,UTexture2D* Icon=nullptr,UTexture2D* Frame=nullptr);
    void Refresh();
    void ClearSelection();
    void Submit();
    bool CanSubmit()const;
    const TMap<FString,int32>& GetSelection()const{return Selection;}
    UPROPERTY(BlueprintAssignable) FInkCoreBindingFeedback OnFeedback;
private:
    UPROPERTY(Transient) TWeakObjectPtr<UInkFrontendWidget> Frontend;
    UPROPERTY(Transient) TArray<TObjectPtr<UInkPlayerMaterialSlotWidget>> Slots;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UTexture2D>> Icons;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UTexture2D>> Frames;
    TMap<FString,int32> Selection;
    FName Page;
    bool bRework=false;
    bool IsActive()const;
    UFUNCTION() void HandleSlotActivated(FName SlotId,FName ItemId);
};
