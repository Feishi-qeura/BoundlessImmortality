#pragma once
#include "Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.h"
#include "InkIntegratedJourneyWidgets.generated.h"

/** Retains runtime captions across Slate construction/reconstruction of the delivered badge. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkIntegratedMapBadge : public UJourneyStateBadgeWidget
{
    GENERATED_BODY()
public:
    void ConfigureBadge(EJourneyGlyphFamily Family,FName State,const FText& Caption);
protected:
    virtual void NativeConstruct() override;
private:
    FText RolloutCaption;
};
