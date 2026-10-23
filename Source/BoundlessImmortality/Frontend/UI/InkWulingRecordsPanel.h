#pragma once
#include "Frontend/InkFeaturePanel.h"
#include "InkWulingRecordsPanel.generated.h"
class FJsonObject;
class UScrollBox;
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkWulingRecordsPanel:public UInkFeaturePanel
{
    GENERATED_BODY()
public:virtual void RefreshLiveData()override;
protected:
    virtual FText PanelTitle()const override;
    virtual void BuildPanelContent()override;
    virtual void HandlePanelAction(const FString& Action)override;
private:
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> NameInput;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> RecordScroll;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> SummaryScroll;
    FString DraftName,SelectedMember,SelectedListing,StateKey;
    int32 Offset=0;
    double ListingVersion=0;
    bool bConfirmDissolve=false;
    TArray<TSharedPtr<FJsonObject>> Rows;
};
