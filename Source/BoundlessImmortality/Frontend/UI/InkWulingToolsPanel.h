#pragma once
#include "Frontend/InkFeaturePanel.h"
#include "InkWulingToolsPanel.generated.h"
class FJsonObject;
class UScrollBox;
class UInkCoreMaterialBinding;

/** Functional child pages reuse the current shell and styles. Authority owns every write. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkWulingToolsPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData()override;
protected:
    virtual FText PanelTitle()const override;
    virtual void BuildPanelContent()override;
    virtual void HandlePanelAction(const FString& Action)override;
private:
    UPROPERTY(Transient) TObjectPtr<UInkCoreMaterialBinding> RolloutMaterialBinding;
    UFUNCTION() void HandleRolloutMaterialFeedback(const FString& Message);
    int32 Offset=0;
    bool bOwnedOnly=false,bBuilding=false;
    bool bRework=false;
    bool bShowNavigation=false;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> CatalogScroll;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> DetailScroll;
    FString RenderKey;
    FString ActivityTab=TEXT("overview");
    TArray<TSharedPtr<FJsonObject>> DisplayRows;
    TMap<FString,int32> MaterialSelection;
    TSharedPtr<FJsonObject> SelectedData;
    FString LastMaterialId;
    int32 ListingQuantity=1,ListingPrice=4;
    int32 RankOffset=0;
    FString CatalogKind()const;
    FString SelectionJson()const;
    void ExecuteSelected();
};
