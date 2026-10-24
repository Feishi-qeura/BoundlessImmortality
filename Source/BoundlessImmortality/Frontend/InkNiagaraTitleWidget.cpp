#include "InkNiagaraTitleWidget.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkPresentationStyle.h"
#include "InkFrontendWidget.h"
#include "UI/InkUiTheme.h"
#include "Cultivation/CultivationNameplate.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraSystemWidget.h"
#include "NiagaraUIComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstanceController.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraEmitterInstance.h"

namespace InkNiagaraTitlePresentation
{
FSlateBrush HonorBrush(FName Id)
{
    FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(130,30);
    if(Id.IsNone()||Id==TEXT("Cyberspace"))return FSlateBrush();
    const FString Name=TEXT("MI_Rank_Title_")+Id.ToString();
    B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*(InkUiAssets::ResolveNamed(Name)),nullptr,LOAD_NoWarn|LOAD_Quiet));
    if(!B.GetResourceObject())return FSlateBrush();
    const auto UV=CultivationNameplate::WeaponUV(Id);if(UV.GetArea()>0)B.ImageSize=UV.GetSize()*FVector2D(1254);return B;
}
FSlateBrush RealmBrush(int32 Index)
{
    FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(130,26);
    B.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Shared/T_RealmBanners.T_RealmBanners"),nullptr,LOAD_NoWarn|LOAD_Quiet));
    const auto UV=CultivationNameplate::BannerUV(Index);B.SetUVRegion(FBox2f(FVector2f(UV.Min),FVector2f(UV.Max)));B.ImageSize=UV.GetSize()*FVector2D(1122,1402);return B;
}
FSlateBrush RealmIconBrush(int32 Index)
{
    FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(34);
    const FString Name=FString::Printf(TEXT("MI_RealmIconStage_%02d"),FMath::Clamp(Index,0,33));
    B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*(InkUiAssets::ResolveNamed(Name)),nullptr,LOAD_NoWarn|LOAD_Quiet));
    const auto UV=CultivationNameplate::BadgeUV(Index);B.ImageSize=UV.GetSize()*FVector2D(1536,1024);return B;
}
FSlateBrush AvatarFrameBrush(){return InkPresentationStyle::NavigationIcon(0,FVector2D(64));}
int32 QualityForTitleId(FName Id)
{
    if(Id==TEXT("LeftGuardian")||Id==TEXT("RightGuardian"))return 1;
    if(Id==TEXT("SevenStar"))return 2;if(Id==TEXT("SoulEater"))return 3;
    if(Id==TEXT("Godslayer"))return 4;if(Id==TEXT("DivineBlade"))return 5;if(Id==TEXT("Cyberspace"))return 6;return 0;
}
int32 QualityForRealmIndex(int32 Index){return FMath::Clamp(Index,0,33)*6/33;}
float ParticleRateForQuality(int32 Q){return 7.f+FMath::Clamp(Q,0,6)*5.f;}
bool ShouldAnimate(bool Visible,bool Reduced,bool Player,bool Painted){return Visible&&!Reduced&&Player&&Painted;}
}

UInkNiagaraTitleWidget::UInkNiagaraTitleWidget(const FObjectInitializer& Init):Super(Init){bIsVolatile=true;}
void UInkNiagaraTitleWidget::Configure(EInkNiagaraTitleKind NewKind,const FText& Text,int32 NewQuality,const FSlateBrush& NewPlaque)
{
    NewQuality=FMath::Clamp(NewQuality,0,6);
    if(Kind==NewKind&&Label.EqualTo(Text)&&Quality==NewQuality&&Plaque==NewPlaque)return;
    const bool KindChanged=Kind!=NewKind;Kind=NewKind;Label=Text;Quality=NewQuality;Plaque=NewPlaque;
    if(KindChanged)StopEffect();ApplyConfiguration();
}
void UInkNiagaraTitleWidget::SetReducedMotion(bool Reduced){bReducedMotion=Reduced;if(Reduced){StopEffect();if(Artwork)Artwork->SetRenderTransformAngle(0);}}
void UInkNiagaraTitleWidget::SetPresentationVisible(bool Visible)
{bPresentationVisible=Visible;SetVisibility(Visible?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);if(!Visible)StopEffect();}
void UInkNiagaraTitleWidget::SetEffectStyle(FLinearColor Tint,float Intensity)
{
    EffectTint=Tint;EffectIntensity=FMath::IsFinite(Intensity)?FMath::Clamp(Intensity,0.f,12.f):0;
    if(EffectIntensity<=0||EffectTint.A<=0)StopEffect();
}
TSharedRef<SWidget> UInkNiagaraTitleWidget::RebuildWidget()
{
    bActivationRequested=false;LastPaintTime=-1;
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!Effect)
    {
        auto* Root=WidgetTree->ConstructWidget<UOverlay>();WidgetTree->RootWidget=Root;
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);
        Artwork=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("TitleArtwork"));Fit->SetContent(Artwork);auto* FitSlot=Root->AddChildToOverlay(Fit);FitSlot->SetHorizontalAlignment(HAlign_Fill);FitSlot->SetVerticalAlignment(VAlign_Fill);
        Effect=WidgetTree->ConstructWidget<UNiagaraSystemWidget>(UNiagaraSystemWidget::StaticClass(),TEXT("TitleParticles"));
        Effect->AutoActivate=false;Effect->TickWhenPaused=true;Effect->SetDesiredWidgetSize(FVector2D(130,30));
        auto* EffectSlot=Root->AddChildToOverlay(Effect);EffectSlot->SetHorizontalAlignment(HAlign_Fill);EffectSlot->SetVerticalAlignment(VAlign_Fill);
        Caption=WidgetTree->ConstructWidget<UTextBlock>();Caption->SetJustification(ETextJustify::Center);Caption->SetAutoWrapText(false);
        auto Font=InkUiTypography::MakeFont(EInkUiTextRole::Body,15);Font.OutlineSettings.OutlineSize=1;Font.OutlineSettings.OutlineColor=FLinearColor(.035f,.045f,.04f,.95f);Caption->SetFont(Font);
        Caption->SetColorAndOpacity(FLinearColor(1,.86f,.54f,1));
        auto* CaptionSlot=Root->AddChildToOverlay(Caption);CaptionSlot->SetHorizontalAlignment(HAlign_Fill);CaptionSlot->SetVerticalAlignment(VAlign_Center);
    }
    ApplyConfiguration();return Super::RebuildWidget();
}
void UInkNiagaraTitleWidget::ApplyConfiguration()
{
    if(Artwork){Artwork->SetBrush(Plaque);Artwork->SetVisibility(Plaque.GetResourceObject()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);}
    if(Caption)
    {
        const bool HomePlaque=Plaque.GetResourceName().ToString().Contains(TEXT("T_HomeRealmPlaque"));
        if(HomePlaque){auto Font=InkUiTypography::MakeFont(EInkUiTextRole::Display,15);Font.Size=Caption->GetFont().Size;Caption->SetFont(Font);}
        Caption->SetText(Label);Caption->SetVisibility((!Plaque.GetResourceObject()||HomePlaque)&&!Label.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
    }
    if(!Effect)return;
    const TCHAR* Names[]={TEXT("Honor"),TEXT("Realm"),TEXT("RealmIcon"),TEXT("AvatarFrame")};
    const FString Name=FString(TEXT("NS_"))+Names[uint8(Kind)]+TEXT("Particles");
    const FString Path=InkUiAssets::TitleParticlesPath(Name);
    auto* System=InkUiAssets::Load<UNiagaraSystem>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(Effect->NiagaraSystemReference!=System){StopEffect();Effect->UpdateNiagaraSystemReference(System);}
}
bool UInkNiagaraTitleWidget::IsTickable()const{return bSlateReady&&!IsTemplate()&&!IsUnreachable()&&!HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&GetWorld()&&GetWorld()->IsGameWorld()&&!GetWorld()->bIsTearingDown;}
TStatId UInkNiagaraTitleWidget::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UInkNiagaraTitleWidget,STATGROUP_Tickables);}
bool UInkNiagaraTitleWidget::IsEffectActive()const{return IsValid(Effect)&&Effect->GetNiagaraComponent()&&Effect->GetNiagaraComponent()->IsActive();}
int32 UInkNiagaraTitleWidget::GetParticleCount()const
{
    int32 Count=0;auto* C=IsValid(Effect)?Effect->GetNiagaraComponent():nullptr;
    if(C&&C->GetSystemInstanceController())
    {
        auto Controller=C->GetSystemInstanceController();Controller->WaitForConcurrentTickAndFinalize();
        if(auto* Instance=Controller->GetSystemInstance_Unsafe())for(const auto& Emitter:Instance->GetEmitters())Count+=Emitter->GetNumParticles();
    }
    return Count;
}
void UInkNiagaraTitleWidget::StopEffect()
{
    bActivationRequested=false;
    if(IsValid(Effect))if(auto* C=Effect->GetNiagaraComponent()){C->SetAutoActivateParticle(false);if(C->IsRegistered())C->DeactivateImmediate();}
}
void UInkNiagaraTitleWidget::Tick(float)
{
    if(!IsTickable())return;
    bool Visible=bPresentationVisible&&IsVisible()&&GetRenderOpacity()>.001f&&EffectIntensity>0&&EffectTint.A>0;
    for(auto* Parent=GetParent();Parent;Parent=Parent->GetParent())Visible=Visible&&Parent->IsVisible()&&Parent->GetRenderOpacity()>.001f;
    bool Reduced=bReducedMotion;if(auto* F=GetTypedOuter<UInkFrontendWidget>())Reduced|=F->bReducedMotion;
    if(Caption&&!Label.IsEmpty())
    {
        const int32 Size=FMath::Clamp(FMath::FloorToInt(FMath::Min(float(LastPaintSize.Y*.62),float(LastPaintSize.X*.93/FMath::Max(1,Label.ToString().Len())))),8,24);
        if(Caption->GetFont().Size!=Size){auto Font=Caption->GetFont();Font.Size=Size;Caption->SetFont(Font);}
    }
    const bool Player=GetWorld()&&!GetWorld()->bIsTearingDown&&GetWorld()->GetFirstPlayerController()&&GetWorld()->GetFirstPlayerController()->GetPawn();
    const bool Painted=LastPaintTime>0&&FPlatformTime::Seconds()-LastPaintTime<.18;
    if(!InkNiagaraTitlePresentation::ShouldAnimate(Visible,Reduced,Player,Painted)){StopEffect();return;}
    if(Artwork&&Kind==EInkNiagaraTitleKind::RealmIcon)Artwork->SetRenderTransformAngle(FMath::Sin(float(GetWorld()->GetTimeSeconds())*.8f)*2.5f);
    if(!Effect||!Effect->NiagaraSystemReference)return;
    if(auto* C=Effect->GetNiagaraComponent())
    {
        C->SetVariableVec2(TEXT("User.Extent"),LastPaintSize*.5);
        C->SetVariableFloat(TEXT("User.Quality"),float(Quality));
        C->SetVariableFloat(TEXT("User.Rate"),InkNiagaraTitlePresentation::ParticleRateForQuality(Quality)*FMath::Clamp(EffectIntensity,.1f,3.f));
        C->SetVariableFloat(TEXT("User.Speed"),.55f+Quality*.13f);
        C->SetVariableFloat(TEXT("User.ParticleSize"),2.2f+Quality*.45f);
        static const FLinearColor Colors[]={FLinearColor(.5f,.76f,.67f,1),FLinearColor(.37f,.77f,.96f,1),FLinearColor(.78f,.48f,1,1),FLinearColor(1,.67f,.23f,1),FLinearColor(1,.43f,.19f,1),FLinearColor(1,.85f,.32f,1),FLinearColor(1,.94f,.69f,1)};
        FLinearColor Color=(Kind==EInkNiagaraTitleKind::AvatarFrame?FLinearColor(1,.76f,.25f,1):Colors[Quality])*EffectTint;Color.A*=EffectIntensity;
        C->SetVariableLinearColor(TEXT("User.Color"),Color);
        if(!bActivationRequested){Effect->ActivateSystem(true);bActivationRequested=true;}
    }
}
int32 UInkNiagaraTitleWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& C,FSlateWindowElementList& E,int32 L,const FWidgetStyle& S,bool B)const
{
    LastPaintTime=FPlatformTime::Seconds();LastPaintSize=G.GetLocalSize();return Super::NativePaint(A,G,C,E,L,S,B);
}
void UInkNiagaraTitleWidget::NativeConstruct(){Super::NativeConstruct();bSlateReady=true;}
void UInkNiagaraTitleWidget::NativeDestruct(){bSlateReady=false;LastPaintTime=-1;StopEffect();Super::NativeDestruct();}
void UInkNiagaraTitleWidget::ReleaseSlateResources(bool bReleaseChildren){bSlateReady=false;LastPaintTime=-1;StopEffect();Super::ReleaseSlateResources(bReleaseChildren);}

TSharedRef<SWidget> UInkNiagaraNameplateWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!Honor)
    {
        auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
        const auto Layout=CultivationNameplate::Layout();
        auto Add=[&](TObjectPtr<UInkNiagaraTitleWidget>& Target,const TCHAR* Name,const FBox2D& Box)
        {Target=WidgetTree->ConstructWidget<UInkNiagaraTitleWidget>(UInkNiagaraTitleWidget::StaticClass(),Name);auto* S=Root->AddChildToCanvas(Target);S->SetPosition(Box.Min);S->SetSize(Box.GetSize());};
        Add(Honor,TEXT("SwordHonor"),Layout.Weapon);Add(Realm,TEXT("SwordRealm"),Layout.Realm);Add(Icon,TEXT("SwordRealmIcon"),Layout.Badge);
        auto Place=[&](UWidget* Widget,const FBox2D& Box)
        {auto* S=Root->AddChildToCanvas(Widget);S->SetPosition(Box.Min);S->SetSize(Box.GetSize());};
        Avatar=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("SwordAvatar"));Place(Avatar,Layout.Avatar);
        AvatarBorder=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("SwordAvatarBorder"));Place(AvatarBorder,Layout.Avatar);
        // The avatar's flowing accent uses the avatar rectangle, independently of the realm badge above it.
        Add(Frame,TEXT("SwordAvatarEffect"),Layout.Avatar);
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetStretchDirection(EStretchDirection::DownOnly);Place(Fit,Layout.Name);
        Identity=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("SwordIdentity"));
        auto Font=InkUiTypography::MakeFont(EInkUiTextRole::Body,13);Font.OutlineSettings.OutlineSize=1;Font.OutlineSettings.OutlineColor=FLinearColor(0,0,0,.9f);
        Identity->SetFont(Font);Identity->SetJustification(ETextJustify::Center);Identity->SetAutoWrapText(false);Identity->SetColorAndOpacity(FLinearColor(.94f,.97f,.93f,1));Fit->SetContent(Identity);
    }
    bIdentityApplied=false;
    Configure(CachedTitle,CachedHonor,CachedRealmIndex,CachedRealm,bCachedReduced);SetIdentity(ParticipantId,CachedDisplayName,CachedAvatar,CachedAvatarFrame);SetEffectStyle(CachedTint,CachedIntensity,CachedPulse);return Super::RebuildWidget();
}
void UInkNiagaraNameplateWidget::SetIdentity(const FString& Id,const FString& DisplayName,UTexture2D* Texture,int32 AvatarFrame)
{
    if(bIdentityApplied&&ParticipantId==Id&&CachedDisplayName==DisplayName&&CachedAvatar==Texture&&CachedAvatarFrame==FMath::Clamp(AvatarFrame,0,15))return;
    ParticipantId=Id;CachedDisplayName=DisplayName;CachedAvatar=Texture;CachedAvatarFrame=FMath::Clamp(AvatarFrame,0,15);
    if(!Avatar)return;
    bIdentityApplied=true;
    auto* Chart=InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Shared/T_CohesionIcons.T_CohesionIcons"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    auto ChartBrush=[&](int32 Tile)
    {
        FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Chart);const auto UV=CultivationNameplate::OrnamentUV(Tile);
        Brush.SetUVRegion(FBox2f(FVector2f(UV.Min),FVector2f(UV.Max)));Brush.ImageSize=FVector2D(32);return Brush;
    };
    if(Avatar)
    {
        if(Texture)
        {
            FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::RoundedBox;Brush.ImageType=ESlateBrushImageType::FullColor;Brush.ImageSize=FVector2D(32);
            Brush.OutlineSettings=FSlateBrushOutlineSettings(16.f);Brush.SetResourceObject(Texture);Avatar->SetBrush(Brush);Avatar->SetColorAndOpacity(FLinearColor::White);
        }
        else{Avatar->SetBrush(ChartBrush(4));Avatar->SetColorAndOpacity(FLinearColor(.75f,.81f,.79f,1));}
    }
    if(AvatarBorder)AvatarBorder->SetBrush(ChartBrush(CachedAvatarFrame));
    if(Identity)Identity->SetText(FText::FromString(CachedDisplayName));
}
void UInkNiagaraNameplateWidget::Configure(FName Id,const FText& Text,int32 Index,const FText& RealmText,bool Reduced)
{
    CachedTitle=Id;CachedHonor=Text;CachedRealmIndex=Index;CachedRealm=RealmText;bCachedReduced=Reduced;
    using namespace InkNiagaraTitlePresentation;
    if(Honor){Honor->Configure(EInkNiagaraTitleKind::Honor,Text,QualityForTitleId(Id),HonorBrush(Id));Honor->SetReducedMotion(Reduced);}
    if(Realm){Realm->Configure(EInkNiagaraTitleKind::Realm,RealmText,QualityForRealmIndex(Index),RealmBrush(Index));Realm->SetReducedMotion(Reduced);}
    if(Icon){Icon->Configure(EInkNiagaraTitleKind::RealmIcon,FText::GetEmpty(),QualityForRealmIndex(Index),RealmIconBrush(Index));Icon->SetReducedMotion(Reduced);}
    // Static framing stays in AvatarBorder; this modest accent contains only local flowing particles.
    if(Frame){Frame->Configure(EInkNiagaraTitleKind::AvatarFrame,FText::GetEmpty(),FMath::Max(QualityForTitleId(Id),QualityForRealmIndex(Index)));Frame->SetReducedMotion(Reduced);}
}
void UInkNiagaraNameplateWidget::StopEffects(){for(auto* W:{Honor.Get(),Realm.Get(),Icon.Get(),Frame.Get()})if(W)W->SetPresentationVisible(false);}
void UInkNiagaraNameplateWidget::SetEffectStyle(FLinearColor Tint,float Intensity,float Pulse)
{
    CachedTint=Tint;CachedIntensity=Intensity;CachedPulse=FMath::Clamp(Pulse,0.f,1.f);
    const float Strength=FMath::Clamp(Intensity,0.f,4.f)*(1.f+CachedPulse*2.f);
    for(auto* W:{Honor.Get(),Realm.Get(),Icon.Get()})if(W)W->SetEffectStyle(Tint,Strength);
    if(Frame)Frame->SetEffectStyle(Tint,Strength*.55f);
}
TArray<UNiagaraSystemWidget*> UInkNiagaraNameplateWidget::GetEffects()const
{TArray<UNiagaraSystemWidget*> Out;for(auto* W:{Honor.Get(),Realm.Get(),Icon.Get(),Frame.Get()})if(W&&W->GetEffectWidget())Out.Add(W->GetEffectWidget());return Out;}
void UInkNiagaraNameplateWidget::NativeDestruct(){StopEffects();Super::NativeDestruct();}
