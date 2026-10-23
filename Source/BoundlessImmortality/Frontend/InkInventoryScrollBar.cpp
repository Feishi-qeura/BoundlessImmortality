#include "InkInventoryScrollBar.h"
#include "Components/ScrollBox.h"

TSharedRef<SWidget> UInkInventoryScrollBar::RebuildWidget()
{
    auto Result = Super::RebuildWidget();
    MyScrollBar->SetOnUserScrolled(FOnUserScrolled::CreateUObject(this, &UInkInventoryScrollBar::HandleUserScrolled));
    return Result;
}

void UInkInventoryScrollBar::SynchronizeScrollPosition()
{
    if (!TargetScrollBox || !MyScrollBar.IsValid()) return;
    const float End = TargetScrollBox->GetScrollOffsetOfEnd();
    const float Progress = End > KINDA_SMALL_NUMBER
        ? FMath::Clamp(TargetScrollBox->GetScrollOffset() / End, 0.f, 1.f) : 0.f;
    SetState(Progress * (1.f - ThumbFraction), ThumbFraction);
    SetIsEnabled(End > KINDA_SMALL_NUMBER);
}

void UInkInventoryScrollBar::HandleUserScrolled(float OffsetFraction)
{
    if (!TargetScrollBox) return;
    const float Progress = FMath::Clamp(OffsetFraction / (1.f - ThumbFraction), 0.f, 1.f);
    TargetScrollBox->SetScrollOffset(Progress * TargetScrollBox->GetScrollOffsetOfEnd());
    SynchronizeScrollPosition();
}
