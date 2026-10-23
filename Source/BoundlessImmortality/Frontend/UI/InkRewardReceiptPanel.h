#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkRewardReceiptPanel.generated.h"

class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UButton;
class FJsonObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInkRewardReceiptClosed);

/** A receipt is a view of an already committed grant. Dismissing never grants again. */
UCLASS(BlueprintType,Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkRewardReceiptPanel : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Ink UI|Rewards") void SetReceiptJson(const FString& Json);
    UFUNCTION(BlueprintCallable,Category="Ink UI|Rewards") void CloseReceipt();
    UPROPERTY(BlueprintAssignable,Category="Ink UI|Rewards") FInkRewardReceiptClosed OnClosed;
    UPROPERTY(BlueprintReadOnly,Category="Ink UI|Rewards") FString ReceiptId;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Root;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Card;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Source;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Note;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Items;
    UPROPERTY(Transient) TObjectPtr<UButton> Close;
    UPROPERTY(Transient) TObjectPtr<UWidget> Rule;
    TSharedPtr<FJsonObject> Receipt;
    FVector2D LastSize=FVector2D::ZeroVector;
    bool bClosed=false;
    void Populate();
    void Layout(const FVector2D& Size);
};
