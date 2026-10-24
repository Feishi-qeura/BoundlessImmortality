#include "InkRankWulingComponents.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Frontend/InkFrontendWidget.h"

namespace InkRankWulingComponents
{
    namespace
    {
        UCanvasPanelSlot* Place(UCanvasPanel* Parent, UWidget* Child, const FPlacement& P)
        {
            if (!Parent || !Child) return nullptr;
            auto* Slot = Parent->AddChildToCanvas(Child);
            Slot->SetPosition(FVector2D(P.X, P.Y));
            Slot->SetSize(FVector2D(P.Width, P.Height));
            Slot->SetZOrder(P.ZOrder);
            return Slot;
        }
    }

    UImage* AddPaperPanel(UWidgetTree* Tree, UCanvasPanel* Parent, FName Name,
        UTexture2D* Texture, const FPlacement& P, FVector4f SlicePixels)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
        if (Texture)
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(Texture);
            Brush.ImageSize = FVector2D(Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight());
            if (SlicePixels.X > 0.f || SlicePixels.Y > 0.f || SlicePixels.Z > 0.f || SlicePixels.W > 0.f)
            {
                Brush.DrawAs = ESlateBrushDrawType::Box;
                Brush.Margin = FMargin(
                    SlicePixels.X / Brush.ImageSize.X,
                    SlicePixels.Z / Brush.ImageSize.Y,
                    SlicePixels.Y / Brush.ImageSize.X,
                    SlicePixels.W / Brush.ImageSize.Y);
            }
            else Brush.DrawAs = ESlateBrushDrawType::Image;
            Image->SetBrush(Brush);
        }
        else
        {
            Image->SetBrush(InkPresentationStyle::Surface(
                InkPresentationStyle::ESurface::FeaturePaper, FVector2D(P.Width, P.Height)));
        }
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Parent, Image, P);
        return Image;
    }

    UTextBlock* AddDynamicText(UWidgetTree* Tree, UCanvasPanel* Parent, FName Name,
        const FText& Text, int32 FontSize, const FPlacement& P, FLinearColor Color,
        ETextJustify::Type Justification)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Label->SetText(Text);
        auto Font = Label->GetFont();
        Font.Size = FontSize;
        Label->SetFont(Font);
        Label->SetColorAndOpacity(Color);
        Label->SetJustification(Justification);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Parent, Label, P);
        return Label;
    }

    UInkFrontendActionButton* AddActionButton(UWidgetTree* Tree, UCanvasPanel* Parent,
        UInkFrontendWidget* Frontend, FName WidgetName, FName Action, const FText& Caption,
        bool bSelected, bool bPrimary, const FPlacement& P)
    {
        if (!Tree || !Parent || !Frontend) return nullptr;
        auto* Button = Tree->ConstructWidget<UInkFrontendActionButton>(
            UInkFrontendActionButton::StaticClass(), WidgetName);
        Button->Action = Action;
        Button->SetStyle(InkPresentationStyle::FeatureButton(bSelected, bPrimary, false));
        auto* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
            FName(*(WidgetName.ToString() + TEXT("Caption"))));
        Text->SetText(Caption);
        Text->SetJustification(ETextJustify::Center);
        Button->SetContent(Text);
        Button->BindFrontend(Frontend);
        Place(Parent, Button, P);
        return Button;
    }

    UImage* AddNativeSurface(UWidgetTree* Tree, UCanvasPanel* Parent, FName Name,
        InkPresentationStyle::ESurface Surface, const FPlacement& P)
    {
        if (!Tree || !Parent) return nullptr;
        auto* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
        Image->SetBrush(InkPresentationStyle::Surface(Surface, FVector2D(P.Width, P.Height)));
        Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Parent, Image, P);
        return Image;
    }
}
