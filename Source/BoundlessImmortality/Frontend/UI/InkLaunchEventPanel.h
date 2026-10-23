#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFeaturePanel.h"
#include "InkLaunchEventPanel.generated.h"

/** Qingyun launch activity. Progress and grants are read exclusively from authority. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkLaunchEventPanel : public UInkFeaturePanel
{
    GENERATED_BODY()

public:
    virtual void RefreshLiveData() override;

protected:
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;

private:
    FString SelectedTab=TEXT("overview");
    FString RenderKey;
    bool bStartRequestPending=false;
    TMap<FString,float> TabScrollOffsets;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ConnectionLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TimeLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HonorExpiryLabel;
    void UpdateLiveLabels();
};
