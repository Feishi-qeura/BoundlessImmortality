#pragma once
#include "Frontend/UiRollout20261024/Activities/InkActivityRolloutPanels.h"
#include "Frontend/UiRollout20261024/Activities/InkLaunchHudCountdownWidget.h"
#include "InkIntegratedActivityPanels.generated.h"

/** Retains the delivered four-category shell and restores reachable detail navigation. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkIntegratedActivitiesPanel : public UInkActivitiesRolloutPanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    FString RolloutActivityKey;
    float RolloutPollElapsed=0;
};

/** Compose the delivered cycle seam with all existing authority display fields. */
UCLASS(Abstract)
class BOUNDLESSIMMORTALITY_API UInkIntegratedCyclePanel : public UInkActivityCycleRolloutPanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual void BuildPanelContent() override;
private:
    FString RolloutActivityKey;
    float RolloutPollElapsed=0;
};
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkIntegratedDailyPanel : public UInkIntegratedCyclePanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const override { return TEXT("daily"); }
};
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkIntegratedLimitedPanel : public UInkIntegratedCyclePanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const override { return TEXT("limited"); }
};
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkIntegratedPermanentPanel : public UInkIntegratedCyclePanel
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ActivityCycle() const override { return TEXT("permanent"); }
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkIntegratedLaunchCountdown : public UInkLaunchHudCountdownWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeConstruct() override;
};
