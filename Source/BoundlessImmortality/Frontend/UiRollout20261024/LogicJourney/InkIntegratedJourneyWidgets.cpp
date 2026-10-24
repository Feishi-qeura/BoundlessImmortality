#include "InkIntegratedJourneyWidgets.h"

void UInkIntegratedMapBadge::ConfigureBadge(EJourneyGlyphFamily Family,FName State,const FText& Caption)
{
    RolloutCaption=Caption;SetGlyphState(Family,State);SetCaption(RolloutCaption);
}
void UInkIntegratedMapBadge::NativeConstruct()
{
    Super::NativeConstruct();SetCaption(RolloutCaption);
}
