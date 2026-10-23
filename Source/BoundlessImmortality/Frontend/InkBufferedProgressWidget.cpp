#include "InkBufferedProgressWidget.h"
#include "InkPresentationStyle.h"
#include "Rendering/DrawElements.h"

UInkBufferedProgressWidget::UInkBufferedProgressWidget(const FObjectInitializer& I):Super(I)
{SetVisibility(ESlateVisibility::HitTestInvisible);SetIsFocusable(false);Track=InkPresentationStyle::Surface(InkPresentationStyle::ESurface::BarTrack);Fill=InkPresentationStyle::Surface(InkPresentationStyle::ESurface::BarFill);}
void UInkBufferedProgressWidget::SetFraction(float Value,bool bReset)
{if(bReset)State.Reset(Value);else State.SetTarget(Value,Settings);Fraction=State.Value;TrailFraction=State.Trail;Invalidate(EInvalidateWidgetReason::Paint);}
void UInkBufferedProgressWidget::NativeTick(const FGeometry& G,float Delta)
{Super::NativeTick(G,Delta);State.Tick(Delta,Settings);TrailFraction=State.Trail;}
int32 UInkBufferedProgressWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& R,FSlateWindowElementList& E,int32 L,const FWidgetStyle& S,bool B)const
{
    L=Super::NativePaint(A,G,R,E,L,S,B);const auto Size=G.GetLocalSize();
    auto Paint=[&](float Start,float End,const FSlateBrush& Brush,FLinearColor C,int32 Z)
    {if(End<=Start)return;E.PushClip(FSlateClippingZone(G.MakeChild(FVector2f(Size.X*(End-Start),Size.Y),FSlateLayoutTransform(FVector2f(Size.X*Start,0)))));FSlateDrawElement::MakeBox(E,L+Z,G.ToPaintGeometry(),&Brush,ESlateDrawEffect::None,C*S.GetColorAndOpacityTint());E.PopClip();};
    Paint(0,1,Track,FLinearColor::White,0);
    Paint(0,FMath::Max(Fraction,TrailFraction),Fill,FLinearColor(FillColor.R*.35f,FillColor.G*.35f,FillColor.B*.35f,1),1);
    Paint(0,Fraction,Fill,FillColor,2);
    if(TrailFraction<Fraction)Paint(TrailFraction,Fraction,Fill,FLinearColor(.03f,.05f,.05f,.35f),3);
    return L+3;
}
