#pragma once

#include "Frontend/InkFeaturePanel.h"
#include "InkGiftRulesPanel.generated.h"

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkGiftRulesPanel : public UInkFeaturePanel
{
	GENERATED_BODY()

protected:
	virtual FText PanelTitle() const override;
	virtual void BuildPanelContent() override;
};
