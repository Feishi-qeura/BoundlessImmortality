#include "InkSignInChestWidget.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "NiagaraSystemWidget.h"
#include "NiagaraUIComponent.h"
#include "NiagaraSystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

void UInkSignInChestWidget::Configure(UInkFrontendWidget* Frontend,FName Route,bool Open,bool Eligible)
{OwnerFrontend=Frontend;OwnerRoute=Route;bOpen=Open;bEligible=Eligible;if(!Eligible||Open)StopGlow();InvalidateLayoutAndVolatility();}
TSharedRef<SWidget> UInkSignInChestWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!Glow)
    {
        auto* Root=WidgetTree->ConstructWidget<UOverlay>();WidgetTree->RootWidget=Root;
        Glow=WidgetTree->ConstructWidget<UNiagaraSystemWidget>(UNiagaraSystemWidget::StaticClass(),TEXT("ClaimableChestNiagara"));
        Glow->NiagaraSystemReference=InkUiAssets::Load<UNiagaraSystem>(nullptr,TEXT("/Game/UI/Ink/Niagara/Rewards/NS_ClaimableChest.NS_ClaimableChest"));
        Glow->AutoActivate=false;Glow->TickWhenPaused=true;Glow->SetDesiredWidgetSize(FVector2D(96));
        Glow->SetVisibility(ESlateVisibility::HitTestInvisible);Root->AddChildToOverlay(Glow);
    }
    return Super::RebuildWidget();
}
bool UInkSignInChestWidget::IsTickable()const{return bSlateReady&&!IsTemplate()&&!IsUnreachable()&&!HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&GetWorld()&&GetWorld()->IsGameWorld()&&!GetWorld()->bIsTearingDown;}
TStatId UInkSignInChestWidget::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UInkSignInChestWidget,STATGROUP_Tickables);}
bool UInkSignInChestWidget::IsGlowActive()const{return IsValid(Glow)&&Glow->GetNiagaraComponent()&&Glow->GetNiagaraComponent()->IsActive();}
void UInkSignInChestWidget::StopGlow()
{if(IsValid(Glow))if(auto* C=Glow->GetNiagaraComponent()){C->SetAutoActivateParticle(false);if(C->IsRegistered())C->DeactivateImmediate();}}
void UInkSignInChestWidget::Tick(float)
{
    if(!IsTickable())return;
    const auto* F=OwnerFrontend.Get();
    const bool Visible=F&&((F->CurrentPage==TEXT("SignIn")&&F->SignInSelectedRoute==OwnerRoute)||F->CurrentPage==OwnerRoute&&OwnerRoute==TEXT("MonthlySignIn"));
    if(!bEligible||bOpen||!Visible||!GetParent()||(F&&F->bReducedMotion)){StopGlow();return;}
    if(IsValid(Glow)&&!IsGlowActive())Glow->ActivateSystem(false);
}
void UInkSignInChestWidget::NativeConstruct(){Super::NativeConstruct();bSlateReady=true;}
void UInkSignInChestWidget::NativeDestruct(){bSlateReady=false;StopGlow();Super::NativeDestruct();}
void UInkSignInChestWidget::ReleaseSlateResources(bool bReleaseChildren){bSlateReady=false;StopGlow();Super::ReleaseSlateResources(bReleaseChildren);}
int32 UInkSignInChestWidget::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled)const
{
    Layer=Super::NativePaint(Args,G,Clip,Out,Layer,Style,Enabled)+1;
    const auto Size=G.GetLocalSize();const FVector2D Scale=Size/96.;
    if(auto* Atlas=InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Rewards/T_ChestStates.T_ChestStates"),nullptr,LOAD_NoWarn|LOAD_Quiet))
    {
        FSlateBrush Art;Art.DrawAs=ESlateBrushDrawType::Image;Art.SetResourceObject(Atlas);
        Art.SetUVRegion(FBox2f(FVector2f(bOpen?.5f:0,0),FVector2f(bOpen?1.f:.5f,1)));
        FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(),&Art,ESlateDrawEffect::None,Style.GetColorAndOpacityTint());return Layer;
    }
    const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
    auto Box=[&](float X,float Y,float W,float H,FLinearColor C)
    {FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(FVector2f(FVector2D(W,H)*Scale),FSlateLayoutTransform(FVector2f(FVector2D(X,Y)*Scale))),Brush,ESlateDrawEffect::None,C*Style.GetColorAndOpacityTint());};
    const FLinearColor Ink(.07f,.085f,.075f,1),Wood(.26f,.19f,.10f,1),Gold(.67f,.46f,.17f,1);
    // Lid, dark empty cavity, front plank, brass edges; no coins or rewards after claim.
    if(bOpen){Box(16,14,64,24,Ink);Box(20,17,56,17,Wood);Box(14,40,68,18,Ink);Box(20,44,56,10,FLinearColor(.018f,.024f,.021f,1));}
    else {Box(14,30,68,30,Ink);Box(18,33,60,23,Wood);Box(19,35,58,3,Gold);}
    Box(14,56,68,25,Ink);Box(18,59,60,18,Wood);Box(18,61,60,2,Gold);Box(20,59,5,18,Gold);Box(71,59,5,18,Gold);Box(18,77,60,3,Gold);
    if(!bOpen){Box(42,51,12,18,Gold);Box(46,56,4,7,Ink);}
    return Layer;
}
