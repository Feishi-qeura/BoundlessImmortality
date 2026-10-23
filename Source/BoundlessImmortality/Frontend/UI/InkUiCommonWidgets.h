#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InkUiViewModels.h"
#include "InkUiCommonWidgets.generated.h"
class UTextBlock;

/** Stateless status view: owns neither requests nor timers. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkUiRequestStateWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(EInkUiPageState State, const FText& Reason);
protected:
    virtual TSharedRef<SWidget> RebuildWidget()override;
private:
    EInkUiPageState State=EInkUiPageState::Loading;
    FText Reason;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
    void Refresh();
};
