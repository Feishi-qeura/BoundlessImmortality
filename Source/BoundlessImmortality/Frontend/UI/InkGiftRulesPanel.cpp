#include "InkGiftRulesPanel.h"

FText UInkGiftRulesPanel::PanelTitle() const
{
	return FText::FromString(TEXT("礼物效果规则"));
}

void UInkGiftRulesPanel::BuildPanelContent()
{
	const float MessageHeight = 128.f;
	const float MessageY = FMath::Max(0.f, (BodyHeight - MessageHeight) * .5f);
	Label(BodyCanvas, TEXT("GiftRulesUnavailable"), TEXT("礼物效果规则待同步\n同步后可在此查看详细说明"),
		0, MessageY, BodyWidth, MessageHeight, 32, true);
}
