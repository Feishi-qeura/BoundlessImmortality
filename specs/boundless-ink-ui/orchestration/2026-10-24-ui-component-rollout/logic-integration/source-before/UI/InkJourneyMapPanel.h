#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFeaturePanel.h"
#include "InkJourneyMapPanel.generated.h"

/** World atlas and local resource basin share navigation, never gameplay authority. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyMapPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Atlas") TSoftObjectPtr<UTexture2D> WorldMapTexture;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Atlas") TSoftObjectPtr<UTexture2D> ResourceMapTexture;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Atlas") bool bResourceMap=false;
protected:
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    FString AtlasKey;
    bool bTabChosen=false;
};

/** Destination sheet owns confirmation; map selection survives cancellation/back. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyDestinationPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Atlas") bool bConfirmTravel=false;
protected:
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    bool IsResource() const;
    bool CanTravel() const;
    bool bTravelPending=false;
    FString DetailKey;
    FString PreviousRound;
    FString PreviousQueue;
};
