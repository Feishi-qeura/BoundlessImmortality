#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFeaturePanel.h"
#include "InkActivitiesBatch2Panels.generated.h"

/** Staged launch overview/tasks page; authority stays in UWulingPrototypeSubsystem. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkLaunchEventBatch2Panel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual FText PanelTitle() const override { return FText::FromString(TEXT("青云开服活动")); }
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    FString SelectedTab = TEXT("overview");
    bool bConfirmStart = false;
};

/** Separate contribution ranking surface; only server-returned rank/score is shown. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkLaunchRankBatch2Panel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual FText PanelTitle() const override { return FText::FromString(TEXT("青云贡献榜")); }
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
};

/** Shared read-only daily/limited activity detail shell. */
UCLASS(Abstract)
class BOUNDLESSIMMORTALITY_API UInkActivityDetailBatch2PanelBase : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual const TCHAR* ExpectedCycle() const PURE_VIRTUAL(UInkActivityDetailBatch2PanelBase::ExpectedCycle, return TEXT("daily"););
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
    FString CapturedServiceDay;
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkDailyActivityDetailBatch2Panel : public UInkActivityDetailBatch2PanelBase
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ExpectedCycle() const override { return TEXT("daily"); }
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkTimedActivityDetailBatch2Panel : public UInkActivityDetailBatch2PanelBase
{
    GENERATED_BODY()
protected:
    virtual const TCHAR* ExpectedCycle() const override { return TEXT("limited"); }
};
