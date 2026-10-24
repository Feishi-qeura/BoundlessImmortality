#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFeaturePanel.h"
#include "InkActivityRolloutPanels.generated.h"

/** Activity overview shell; each category presents only rows explicitly returned by the authority snapshot. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkActivitiesRolloutPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
protected:
    virtual FText PanelTitle() const override { return FText::FromString(TEXT("仙途活动")); }
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    FString SelectedCycle = TEXT("daily");
};

/** Shared read-only renderer for one explicitly classified activity cycle. */
UCLASS(Abstract)
class BOUNDLESSIMMORTALITY_API UInkActivityCycleRolloutPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const PURE_VIRTUAL(UInkActivityCycleRolloutPanel::ActivityCycle, return TEXT(""));
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkDailyActivitiesRolloutPanel : public UInkActivityCycleRolloutPanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const override { return TEXT("daily"); }
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkLimitedActivitiesRolloutPanel : public UInkActivityCycleRolloutPanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const override { return TEXT("limited"); }
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkPermanentActivitiesRolloutPanel : public UInkActivityCycleRolloutPanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const override { return TEXT("permanent"); }
};
