#include "InkCharacterCultivationWidget.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemWidget.h"
#include "NiagaraUIComponent.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"

namespace InkCharacterCultivation
{
EInkDantianStage StageForRealm(int32 Realm)
{return Realm<9?EInkDantianStage::Qi:Realm<13?EInkDantianStage::Foundation:Realm<16?EInkDantianStage::Crystal:Realm<22?EInkDantianStage::GoldenCore:EInkDantianStage::NascentSoul;}
FLinearColor StageColor(EInkDantianStage Stage)
{
    const FColor Colors[]={FColor(58,152,255),FColor(244,218,66),FColor(170,91,236),FColor(255,171,29),FColor(255,169,195)};
    return FLinearColor::FromSRGBColor(Colors[FMath::Clamp(int32(Stage),0,4)]);
}
FVector2D BorderPoint(FVector2D Center,FVector2D Size,float Angle)
{return Center+FVector2D(FMath::Clamp(FMath::Cos(Angle)*1.414214f,-1.f,1.f)*Size.X*.5,FMath::Clamp(FMath::Sin(Angle)*1.414214f,-1.f,1.f)*Size.Y*.5);}
}

void UInkCharacterCultivationWidget::Configure(UInkFrontendWidget* Owner,FVector2D Size,const TArray<FInkCharacterSlotEffect>& Nodes,
    FVector2D Target,int32 Realm,FLinearColor Color,float Speed,float ParticleSize,float Radius)
{
    Frontend=Owner;Extent=Size;Sources=Nodes;Dantian=Target;RealmIndex=Realm;CoreColor=Color;
    BorderSpeed=FMath::Clamp(Speed,.05f,3.f);MoteSize=FMath::Clamp(ParticleSize,1.f,8.f);CoreRadius=FMath::Clamp(Radius,8.f,48.f);
    UpdateGeometry();
}
TSharedRef<SWidget> UInkCharacterCultivationWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!Canvas)
    {
        Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
        auto Add=[&](const TCHAR* Name,const TCHAR* Asset)
        {
            auto* Effect=WidgetTree->ConstructWidget<UNiagaraSystemWidget>(UNiagaraSystemWidget::StaticClass(),Name);
            Effect->AutoActivate=false;Effect->TickWhenPaused=true;
            Effect->NiagaraSystemReference=InkUiAssets::Load<UNiagaraSystem>(nullptr,Asset);
            Effect->SetVisibility(ESlateVisibility::HitTestInvisible);Canvas->AddChildToCanvas(Effect);return Effect;
        };
        auto* FrameBase=InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/Shared/M_QualityFrameUI.M_QualityFrameUI"));
        for(int32 I=0;I<8;++I)
        {
            auto* Frame=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),*FString::Printf(TEXT("ProfileSlotGlow_%d"),I));
            auto* Material=FrameBase?UMaterialInstanceDynamic::Create(FrameBase,this):nullptr;
            if(Material)Frame->SetBrushFromMaterial(Material);
            Canvas->AddChildToCanvas(Frame)->SetZOrder(0);Frames.Add(Frame);FrameMaterials.Add(Material);
            auto* Border=Add(*FString::Printf(TEXT("ProfileBorder_%d"),I),TEXT("/Game/UI/Ink/Niagara/Shared/NS_NodeBorder.NS_NodeBorder"));
            CastChecked<UCanvasPanelSlot>(Border->Slot)->SetZOrder(1);Borders.Add(Border);
        }
        Glow=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("ProfileDantianGlow"));
        if(auto* M=InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/Character/M_DantianUI.M_DantianUI")))
        {GlowMaterial=UMaterialInstanceDynamic::Create(M,this);Glow->SetBrushFromMaterial(GlowMaterial);}
        Glow->SetVisibility(ESlateVisibility::HitTestInvisible);Canvas->AddChildToCanvas(Glow)->SetZOrder(2);
        Core=Add(TEXT("ProfileDantianNiagara"),TEXT("/Game/UI/Ink/Niagara/Character/NS_Dantian.NS_Dantian"));
        CastChecked<UCanvasPanelSlot>(Core->Slot)->SetZOrder(3);
    }
    UpdateGeometry();return Super::RebuildWidget();
}
void UInkCharacterCultivationWidget::UpdateGeometry()
{
    if(!Canvas)return;
    for(int32 I=0;I<Frames.Num();++I)
    {
        const bool Equipped=Sources.IsValidIndex(I)&&Sources[I].bUnlocked&&Sources[I].bEquipped&&Sources[I].QualityTier>0;
        Frames[I]->SetVisibility(Equipped?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
        if(!Equipped)continue;
        const auto& Node=Sources[I];const float Scale=Node.FrameSize.Y/104,Quality=FMath::Clamp(Node.QualityTier,1,6);
        const float Width=(.9f+.42f*Quality)*Scale,Radius=(2+.8f*Quality)*Scale,Pad=Radius*3;
        const auto Size=Node.FrameSize+FVector2D(Pad*2);auto* S=CastChecked<UCanvasPanelSlot>(Frames[I]->Slot);
        S->SetPosition(Node.Position-Size*.5);S->SetSize(Size);
        if(auto* M=FrameMaterials[I].Get())
        {
            M->SetVectorParameterValue(TEXT("GlowColor"),Node.Color);
            M->SetVectorParameterValue(TEXT("CanvasSize"),FLinearColor(Size.X,Size.Y,0,0));
            M->SetVectorParameterValue(TEXT("FrameHalfSize"),FLinearColor(Node.FrameSize.X*.5,Node.FrameSize.Y*.5,0,0));
            M->SetScalarParameterValue(TEXT("BorderWidth"),Width);M->SetScalarParameterValue(TEXT("GlowRadius"),Radius);
            M->SetScalarParameterValue(TEXT("Intensity"),.75f+.1f*Quality);M->SetScalarParameterValue(TEXT("CornerRadius"),5*Scale);
        }
    }
    for(auto* Effect:GetEffects())
    {auto* Layout=CastChecked<UCanvasPanelSlot>(Effect->Slot);Layout->SetPosition(FVector2D::ZeroVector);Layout->SetSize(Extent);Effect->SetDesiredWidgetSize(Extent);}
    auto* Layout=CastChecked<UCanvasPanelSlot>(Glow->Slot);Layout->SetPosition(Dantian-FVector2D(CoreRadius*2));Layout->SetSize(FVector2D(CoreRadius*4));
    if(GlowMaterial)
    {GlowMaterial->SetVectorParameterValue(TEXT("CoreColor"),CoreColor);GlowMaterial->SetScalarParameterValue(TEXT("Halo"),GetDantianStage()==EInkDantianStage::GoldenCore?1:0);}
}
TArray<UNiagaraSystemWidget*> UInkCharacterCultivationWidget::GetEffects()const
{TArray<UNiagaraSystemWidget*> Result;for(const auto& Border:Borders)if(Border)Result.Add(Border.Get());if(Core)Result.Add(Core.Get());return Result;}
bool UInkCharacterCultivationWidget::IsTickable()const{return !IsTemplate()&&GetWorld()&&GetWorld()->IsGameWorld();}
TStatId UInkCharacterCultivationWidget::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UInkCharacterCultivationWidget,STATGROUP_Tickables);}
void UInkCharacterCultivationWidget::StopEffects()
{
    if(!bRequested)return;
    for(auto* E:GetEffects()){E->DeactivateSystem();if(auto* C=E->GetNiagaraComponent()){C->Deactivate();C->DeactivateImmediate();}}
    bRequested=false;
}
void UInkCharacterCultivationWidget::Tick(float)
{
    const auto* F=Frontend.Get();bool Visible=F&&F->CurrentPage==TEXT("CharacterProfile")&&GetParent()&&IsVisible();
    for(auto* P=GetParent();P;P=P->GetParent())Visible&=P->IsVisible()&&P->GetRenderOpacity()>.001f;
    if(!Visible||F->bReducedMotion||LastPaint<0||FPlatformTime::Seconds()-LastPaint>.2||GetWorld()->bIsTearingDown){StopEffects();return;}
    auto Convert=[&](FVector2D P){return FVector2D(P.X-Extent.X*.5,Extent.Y*.5-P.Y);};
    for(int32 I=0;I<Borders.Num();++I)if(auto* C=Borders[I]->GetNiagaraComponent())
    {
        if(!Sources.IsValidIndex(I)||!Sources[I].bUnlocked||!Sources[I].bEquipped||Sources[I].QualityTier<=0)
        {if(C->IsActive()){Borders[I]->DeactivateSystem();C->Deactivate();C->DeactivateImmediate();}continue;}
        const auto& Node=Sources[I];C->SetVariableVec2(TEXT("User.Source"),Convert(Node.Position));
        C->SetVariableVec2(TEXT("User.Extent"),Node.FrameSize*.5);
        C->SetVariableFloat(TEXT("User.Speed"),BorderSpeed);C->SetVariableFloat(TEXT("User.Rate"),22);
        C->SetVariableFloat(TEXT("User.ParticleSize"),MoteSize*(.7f+.2f*FMath::Clamp(Node.QualityTier,1,6)));
        C->SetVariableLinearColor(TEXT("User.Color"),Node.Color);
        if(!C->IsActive())Borders[I]->ActivateSystem(true);
    }
    if(Core)if(auto* C=Core->GetNiagaraComponent())
    {
        C->SetVariableVec2(TEXT("User.Target"),Convert(Dantian));C->SetVariableFloat(TEXT("User.Speed"),BorderSpeed);
        C->SetVariableFloat(TEXT("User.Rate"),28);C->SetVariableFloat(TEXT("User.ParticleSize"),2.1f);
        C->SetVariableFloat(TEXT("User.Radius"),CoreRadius);C->SetVariableFloat(TEXT("User.Halo"),GetDantianStage()==EInkDantianStage::GoldenCore?1:0);
        C->SetVariableLinearColor(TEXT("User.Color"),CoreColor);if(!bRequested)Core->ActivateSystem(true);
    }
    if(GlowMaterial)GlowMaterial->SetScalarParameterValue(TEXT("Pulse"),.94f+.06f*FMath::Sin(GetWorld()->GetTimeSeconds()*2));
    bRequested=true;
}
void UInkCharacterCultivationWidget::NativeDestruct(){StopEffects();Super::NativeDestruct();}
int32 UInkCharacterCultivationWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& Clip,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled)const
{
    LastPaint=FPlatformTime::Seconds();
    return Super::NativePaint(A,G,Clip,Out,Layer,Style,Enabled);
}
