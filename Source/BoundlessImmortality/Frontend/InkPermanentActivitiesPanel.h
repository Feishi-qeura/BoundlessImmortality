#pragma once

#include "CoreMinimal.h"
#include "InkFeaturePanel.h"
#include "InkPermanentActivitiesPanel.generated.h"

/** Read-only presentation of permanent activities supplied by the authority snapshot. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkPermanentActivitiesPanel : public UInkFeaturePanel
{
	GENERATED_BODY()

protected:
	virtual FText PanelTitle() const override { return FText::FromString(TEXT("常驻活动")); }
	virtual void BuildPanelContent() override;
};
