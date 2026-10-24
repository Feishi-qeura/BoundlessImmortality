#include "InkPlayerMaterialSlotWidget.h"

#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UInkPlayerMaterialSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildDefaultTree();
    if (SlotButton)
    {
        SlotButton->OnClicked.AddUniqueDynamic(this, &UInkPlayerMaterialSlotWidget::HandleSlotClicked);
    }
}

void UInkPlayerMaterialSlotWidget::BuildDefaultTree()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MaterialSlotButton"));

    auto* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MaterialSlotSize"));
    Size->SetWidthOverride(144.f);
    Size->SetHeightOverride(176.f);
    SlotButton->AddChild(Size);

    auto* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MaterialSlotOverlay"));
    Size->AddChild(Overlay);

    FrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MaterialSlotFrame"));
    Overlay->AddChildToOverlay(FrameImage);

    auto* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MaterialSlotContent"));
    auto* ContentSlot = Overlay->AddChildToOverlay(Content);
    ContentSlot->SetHorizontalAlignment(HAlign_Fill);
    ContentSlot->SetVerticalAlignment(VAlign_Fill);

    ItemImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MaterialItemIcon"));
    auto* IconSlot = Content->AddChildToVerticalBox(ItemImage);
    IconSlot->SetHorizontalAlignment(HAlign_Center);
    IconSlot->SetPadding(FMargin(18.f, 14.f, 18.f, 4.f));

    ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MaterialItemName"));
    ItemNameText->SetJustification(ETextJustify::Center);
    auto* NameSlot = Content->AddChildToVerticalBox(ItemNameText);
    NameSlot->SetHorizontalAlignment(HAlign_Fill);
    NameSlot->SetPadding(FMargin(4.f, 2.f));

    OwnedCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MaterialOwnedCount"));
    OwnedCountText->SetJustification(ETextJustify::Center);
    auto* OwnedSlot = Content->AddChildToVerticalBox(OwnedCountText);
    OwnedSlot->SetHorizontalAlignment(HAlign_Fill);
    OwnedSlot->SetPadding(FMargin(4.f, 2.f, 4.f, 0.f));

    RequiredCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MaterialRequiredCount"));
    RequiredCountText->SetJustification(ETextJustify::Center);
    auto* RequiredSlot = Content->AddChildToVerticalBox(RequiredCountText);
    RequiredSlot->SetHorizontalAlignment(HAlign_Fill);
    RequiredSlot->SetPadding(FMargin(4.f, 0.f, 4.f, 6.f));

    WidgetTree->RootWidget = SlotButton;
}

void UInkPlayerMaterialSlotWidget::ConfigureSlot(const FName InSlotId, const FName InItemId,
    const FText& InItemName, const FText& InOwnedCount, const FText& InRequiredCount,
    UTexture2D* InItemIcon, UTexture2D* InFrameTexture, const bool bInCanSelect)
{
    BuildDefaultTree();
    SlotId = InSlotId;
    ItemId = InItemId;
    bCanSelect = bInCanSelect;

    if (ItemNameText) ItemNameText->SetText(InItemName);
    if (OwnedCountText) OwnedCountText->SetText(InOwnedCount);
    if (RequiredCountText) RequiredCountText->SetText(InRequiredCount);
    if (ItemImage)
    {
        if (InItemIcon)
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(InItemIcon);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            ItemImage->SetBrush(Brush);
        }
        ItemImage->SetVisibility(InItemIcon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (FrameImage)
    {
        if (InFrameTexture)
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(InFrameTexture);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            FrameImage->SetBrush(Brush);
        }
        FrameImage->SetVisibility(InFrameTexture ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    SetSelectionEnabled(bCanSelect);
}

void UInkPlayerMaterialSlotWidget::SetSelectionEnabled(const bool bEnabled)
{
    bCanSelect = bEnabled;
    if (SlotButton) SlotButton->SetIsEnabled(bEnabled);
}

void UInkPlayerMaterialSlotWidget::HandleSlotClicked()
{
    if (bCanSelect && !SlotId.IsNone() && !ItemId.IsNone())
    {
        OnSlotActivated.Broadcast(SlotId, ItemId);
    }
}


