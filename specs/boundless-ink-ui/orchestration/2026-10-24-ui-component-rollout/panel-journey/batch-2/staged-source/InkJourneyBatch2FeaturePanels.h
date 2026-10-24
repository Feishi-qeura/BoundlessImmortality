#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFeaturePanel.h"
#include "Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h"
#include "InkJourneyBatch2Widgets.h"
#include "InkJourneyBatch2FeaturePanels.generated.h"

/** Concrete feature route wrapper for the live resource-detail child widget. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyResourceDetailFeaturePanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FText PanelTitle() const override { return FText::FromString(TEXT("灵脉 · 资源详情")); }
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    UFUNCTION() void HandleTravelRequested(const FString& EntryId);
    UFUNCTION() void HandleChildBackRequested();
    UPROPERTY(Transient) TObjectPtr<UInkResourceDetailBatch2Widget> DetailWidget;
    FString AuthorityContentKey;
    float RefreshElapsed = 0.f;
};

/** FeaturePanel bridge keeps result children alive and uses the original feature back stack. */
UCLASS(Abstract, Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkJourneyOutcomeFeaturePanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FText PanelTitle() const override { return FText::FromString(FeaturePanelTitle()); }
    virtual void BuildPanelContent() override;
    virtual const TCHAR* FeaturePanelTitle() const PURE_VIRTUAL(UInkJourneyOutcomeFeaturePanel::FeaturePanelTitle, return TEXT("历练结果"););
    virtual TSubclassOf<UInkJourneyOutcomeBatch2Widget> OutcomeWidgetClass() const PURE_VIRTUAL(UInkJourneyOutcomeFeaturePanel::OutcomeWidgetClass, return nullptr;);
    virtual void PopulateOutcome(const InkJourneyAdapter::FContext& Context, UInkJourneyOutcomeBatch2Widget* Widget) = 0;
    UFUNCTION() void HandleChildBackRequested();
    UPROPERTY(Transient) TObjectPtr<UInkJourneyOutcomeBatch2Widget> OutcomeWidget;
private:
    FString AuthorityContentKey;
    float RefreshElapsed = 0.f;
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkWorldResultFeaturePanel : public UInkJourneyOutcomeFeaturePanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* FeaturePanelTitle() const override { return TEXT("世界历练结果"); }
    virtual TSubclassOf<UInkJourneyOutcomeBatch2Widget> OutcomeWidgetClass() const override { return UInkWorldResultBatch2Widget::StaticClass(); }
    virtual void PopulateOutcome(const InkJourneyAdapter::FContext& Context, UInkJourneyOutcomeBatch2Widget* Widget) override;
};

UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkRaceResultFeaturePanel : public UInkJourneyOutcomeFeaturePanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* FeaturePanelTitle() const override { return TEXT("灵脉竞采结果"); }
    virtual TSubclassOf<UInkJourneyOutcomeBatch2Widget> OutcomeWidgetClass() const override { return UInkRaceResultBatch2Widget::StaticClass(); }
    virtual void PopulateOutcome(const InkJourneyAdapter::FContext& Context, UInkJourneyOutcomeBatch2Widget* Widget) override;
};
