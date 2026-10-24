#pragma once
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateTypes.h"

class UWidgetTree;
class UWidget;
class UCanvasPanel;
class UCanvasPanelSlot;
class UTextBlock;
class UBorder;
class UTexture2D;
class UInkFrontendWidget;
class UInkFrontendActionButton;

/** newui-r1 tokens; presentation only. No rewards, eligibility or gameplay state. */
namespace InkRebuildUI
{
enum class ETextRole:uint8 {Display,Title,Section,Body,Number,Button,Secondary,Hud};
constexpr float SafePadding=24.f;
constexpr float TouchSize=88.f;
BOUNDLESSIMMORTALITY_API FLinearColor Ink();
BOUNDLESSIMMORTALITY_API FLinearColor Muted();
BOUNDLESSIMMORTALITY_API FLinearColor Paper();
BOUNDLESSIMMORTALITY_API FLinearColor Background();
BOUNDLESSIMMORTALITY_API FLinearColor Primary();
BOUNDLESSIMMORTALITY_API FLinearColor Jade();
BOUNDLESSIMMORTALITY_API FLinearColor Gold();
BOUNDLESSIMMORTALITY_API FLinearColor Warning();
BOUNDLESSIMMORTALITY_API FSlateFontInfo Font(ETextRole Role,int32 LogicalSize=0);
BOUNDLESSIMMORTALITY_API FSlateBrush Surface(FLinearColor Fill,float Radius=8.f,bool bOutline=true);
BOUNDLESSIMMORTALITY_API FButtonStyle ButtonStyle(bool bPrimary=false,bool bSelected=false);
BOUNDLESSIMMORTALITY_API UCanvasPanelSlot* Place(UCanvasPanel* Parent,UWidget* Child,FVector4 Bounds,int32 Z=1);
BOUNDLESSIMMORTALITY_API UTextBlock* Text(UWidgetTree* Tree,UCanvasPanel* Parent,FName Name,const FText& Value,ETextRole Role,FVector4 Bounds,int32 LogicalSize=0,bool bCentered=false);
BOUNDLESSIMMORTALITY_API UBorder* Card(UWidgetTree* Tree,UCanvasPanel* Parent,FName Name,FVector4 Bounds,bool bEmphasized=false);
BOUNDLESSIMMORTALITY_API UInkFrontendActionButton* Action(UWidgetTree* Tree,UCanvasPanel* Parent,UInkFrontendWidget* Owner,FName Name,FName ActionId,const FText& Caption,FVector4 Bounds,bool bPrimary=false,bool bSelected=false);
}
