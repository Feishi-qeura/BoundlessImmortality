#include "InkRolloutMaterialSlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

UTexture2D* UInkRolloutMaterialSlotWidget::MaterialFrame()
{
    return LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/ComponentRollout20261024/Player/T_BreakthroughMaterialSlotFrame_Default_v1.T_BreakthroughMaterialSlotFrame_Default_v1"),nullptr,LOAD_NoWarn|LOAD_Quiet);
}
void UInkRolloutMaterialSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    auto* Frame=Cast<UImage>(GetWidgetFromName(TEXT("MaterialSlotFrame")));
    auto* Overlay=Frame?Cast<UOverlay>(Frame->GetParent()):nullptr;
    if(!Overlay)return;
    Frame->RemoveFromParent();
    auto* FrameSize=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("RolloutSquareMaterialFrame"));
    FrameSize->SetWidthOverride(144);FrameSize->SetHeightOverride(144);FrameSize->SetContent(Frame);
    auto* Layout=CastChecked<UOverlaySlot>(Overlay->InsertChildAt(0,FrameSize));
    Layout->SetHorizontalAlignment(HAlign_Center);Layout->SetVerticalAlignment(VAlign_Top);
}
