#include "InkUiCommonWidgets.h"
#include "InkUiTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

void UInkUiRequestStateWidget::Configure(EInkUiPageState InState,const FText& InReason){State=InState;Reason=InReason;Refresh();}
TSharedRef<SWidget> UInkUiRequestStateWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!Label){Label=WidgetTree->ConstructWidget<UTextBlock>();WidgetTree->RootWidget=Label;}
    Refresh();return Super::RebuildWidget();
}
void UInkUiRequestStateWidget::Refresh()
{
    if(!Label)return;
    Label->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Secondary));
    Label->SetColorAndOpacity(InkUiTypography::Theme()->InkTint);
    Label->SetAutoWrapText(true);Label->SetText(Reason);
    SetVisibility(Reason.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
}
