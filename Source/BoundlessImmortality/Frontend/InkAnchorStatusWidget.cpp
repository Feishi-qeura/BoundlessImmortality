#include "InkAnchorStatusWidget.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFrontendWidget.h"
#include "InkPresentationStyle.h"
#include "InkHomeArt.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "InkNiagaraTitleWidget.h"
#include "UI/InkUiTheme.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "CultivationGameplayComponent.h"
#include "LiveInteractionSubsystem.h"
#include "Animation/AnimSequence.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/FontCache.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

UInkAnchorStatusWidget::UInkAnchorStatusWidget(const FObjectInitializer& Initializer):Super(Initializer)
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    SetIsFocusable(false);
}

float UInkAnchorStatusWidget::SafeFraction(double Current,double Max)
{
    return FMath::IsFinite(Current)&&FMath::IsFinite(Max)&&Max>0?float(FMath::Clamp(Current/Max,0.,1.)):0.f;
}

FVector2D UInkAnchorStatusWidget::ResponsiveSize(FVector2D Viewport)
{
    return FVector2D(FMath::Min(440.,FMath::Max(272.,Viewport.X-48)),128);
}

FSlateRect UInkAnchorStatusWidget::MeterInterior(FSlateRect Track)
{
    return FSlateRect(Track.Left+2,Track.Top+2,Track.Right-2,Track.Bottom-2);
}

FInkAnchorActionPresentation UInkAnchorStatusWidget::ResolveAction(ECultivationActionState State,float Elapsed,float Duration)
{
    FInkAnchorActionPresentation P;P.bActive=State!=ECultivationActionState::Idle;
    switch(State)
    {
    case ECultivationActionState::MeditationStart:P.Label=TEXT("凝神入定");break;
    case ECultivationActionState::MeditationLoop:P.Label=TEXT("静心修炼 · 持续中");P.bIndeterminate=true;break;
    case ECultivationActionState::MeditationEnd:P.Label=TEXT("收功回神");break;
    case ECultivationActionState::ChargeStart:P.Label=TEXT("聚气蓄势");break;
    case ECultivationActionState::ChargeLoop:P.Label=TEXT("持续蓄力 · 松开发招");P.bIndeterminate=true;break;
    case ECultivationActionState::ChargeRelease:P.Label=TEXT("运气出招");break;
    default:P.Label=TEXT("自由行动");break;
    }
    if(P.bActive&&!P.bIndeterminate)
    {
        if(FMath::IsFinite(Duration)&&Duration>0)P.Fraction=SafeFraction(Elapsed,Duration);
        else P.bIndeterminate=true;
    }
    return P;
}

TSharedRef<SWidget> UInkAnchorStatusWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!DesiredSizeBox)
    {
        DesiredSizeBox=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("AnchorStatusSize"));
        DesiredSizeBox->SetWidthOverride(440);DesiredSizeBox->SetHeightOverride(128);
        DesiredSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);WidgetTree->RootWidget=DesiredSizeBox;
        EffectsCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();DesiredSizeBox->SetContent(EffectsCanvas);
        auto* Art=WidgetTree->ConstructWidget<UInkAnchorStatusArt>();Art->Owner=this;Art->ForceVolatile(true);Art->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* ArtSlot=EffectsCanvas->AddChildToCanvas(Art);ArtSlot->SetAnchors(FAnchors(0,0,1,1));ArtSlot->SetOffsets(FMargin(0));ArtSlot->SetZOrder(0);
        RealmTitle=WidgetTree->ConstructWidget<UInkNiagaraTitleWidget>(UInkNiagaraTitleWidget::StaticClass(),TEXT("AnchorRealmTitle"));
        AvatarFrame=WidgetTree->ConstructWidget<UInkNiagaraTitleWidget>(UInkNiagaraTitleWidget::StaticClass(),TEXT("AnchorAvatarFrame"));
        for(auto* Effect:{RealmTitle.Get(),AvatarFrame.Get()}){Effect->SetOwningPlayer(GetOwningPlayer());EffectsCanvas->AddChildToCanvas(Effect)->SetZOrder(1);}
    }
    return Super::RebuildWidget();
}

void UInkAnchorStatusWidget::Configure(UInkFrontendWidget* InFrontend)
{
    Frontend=InFrontend;
    TextFont=InFrontend?InFrontend->TextFont.LoadSynchronous():InkUiAssets::Load<UFont>(nullptr,TEXT("/Game/UI/Ink/Fonts/Shared/F_InkBody.F_InkBody"));
    RefreshFromWorld();
}

void UInkAnchorStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();SetVisibility(ESlateVisibility::SelfHitTestInvisible);SetIsFocusable(false);
    PortraitFallback=PlaceholderPortrait.LoadSynchronous();
    CachePresentationArt();
    if(!TextFont)TextFont=InkUiAssets::Load<UFont>(nullptr,TEXT("/Game/UI/Ink/Fonts/Shared/F_InkBody.F_InkBody"));
    AvatarHost=GetGameInstance()?GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>():nullptr;
    if(auto* Host=AvatarHost.Get())Host->OnAvatarReady.AddUniqueDynamic(this,&ThisClass::HandleAvatarReady);
    if(DesiredSizeBox)
    {
        const double Scale=UWidgetLayoutLibrary::GetViewportScale(this);
        const FVector2D Size=ResponsiveSize(UWidgetLayoutLibrary::GetViewportSize(this)/FMath::Max(.01,Scale));
        DesiredSizeBox->SetWidthOverride(Size.X);
        // Idle has no action row; release its height instead of reserving blank paper.
        DesiredSizeBox->SetHeightOverride(Size.Y);
    }
    bInitializedValues=false;RefreshFromWorld();
}

void UInkAnchorStatusWidget::CachePresentationArt()
{
    using namespace InkPresentationStyle;
    PresentationSurfaces.Reset();
    PresentationSurfaces.Add(InkHomeArt::Brush(TEXT("home-identity-ink")));
    for(int32 Kind=0;Kind<3;++Kind)
    {
        PresentationSurfaces.Add(Kind<2?InkHomeArt::Brush(TEXT("home-bar-track")):Meter(Kind,false));
        PresentationSurfaces.Add(Kind<2?InkHomeArt::Brush(Kind==0?TEXT("home-health-fill"):TEXT("home-qi-fill")):Meter(Kind,true));
    }
    PresentationSurfaces.Add(InkWash());
    PresentationIcons.Reset();
    for(const auto Part:{EIcon::AvatarFrame,EIcon::AnchorPortrait,EIcon::Health,EIcon::Shield,EIcon::Experience,EIcon::Action})PresentationIcons.Add(Icon(Part));
}

void UInkAnchorStatusWidget::NativeDestruct()
{
    for(auto* Effect:{RealmTitle.Get(),AvatarFrame.Get()})if(Effect)Effect->SetPresentationVisible(false);
    if(auto* Host=AvatarHost.Get())Host->OnAvatarReady.RemoveDynamic(this,&ThisClass::HandleAvatarReady);
    ObserveDamage(nullptr);AvatarHost.Reset();RequestedAvatarId.Empty();RequestedAvatarUrl.Empty();Super::NativeDestruct();
}

void UInkAnchorStatusWidget::SetShieldPresentation(double Current,double Max)
{
    MaxShield=FMath::IsFinite(Max)?FMath::Max(0.,Max):0;
    Shield=FMath::IsFinite(Current)?FMath::Clamp(Current,0.,MaxShield):0;
}

void UInkAnchorStatusWidget::SetAvatarPresentation(UTexture2D* Texture,const FString& IdentityUrl,bool bPlaceholder)
{
    bAvatarOverride=Texture!=nullptr||!IdentityUrl.IsEmpty();AvatarTexture=Texture;AvatarUrl=IdentityUrl;bAvatarPlaceholder=bPlaceholder||!Texture;
    RequestedAvatarId.Empty();RequestedAvatarUrl.Empty();NextAvatarAttempt=0;
}

void UInkAnchorStatusWidget::HandleAvatarReady(const FString& UserId,UTexture2D* Texture)
{
    if(UserId!=RequestedAvatarId||RequestedAvatarUrl!=AvatarUrl||!Texture)return;
    AvatarTexture=Texture;bAvatarPlaceholder=false;
}

void UInkAnchorStatusWidget::RefreshFromWorld()
{
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    if(!State){bHasAnchor=false;ObserveDamage(nullptr);return;}
    const auto Snapshot=State->GetParticipantSnapshot(State->GetAnchorId());
    auto* AnchorHealth=State->GetAnchorHealth();ObserveDamage(AnchorHealth);
    bHasAnchor=!Snapshot.ParticipantId.IsEmpty()&&AnchorHealth;
    if(!bHasAnchor){bInitializedValues=false;return;}
    const bool IdentityChanged=AnchorId!=Snapshot.ParticipantId;
    if(IdentityChanged)
    {
        AnchorId=Snapshot.ParticipantId;bInitializedValues=false;bAvatarOverride=false;AvatarTexture=nullptr;
        RequestedAvatarId.Empty();RequestedAvatarUrl.Empty();NextAvatarAttempt=0;DamageFlash=0;
    }
    AnchorName=Snapshot.DisplayName;RealmName=Snapshot.RealmName;
    Health=AnchorHealth->Health;MaxHealth=AnchorHealth->MaxHealth;bDead=AnchorHealth->bDead;
    Cultivation=Snapshot.Cultivation;RequiredCultivation=Snapshot.RequiredCultivation;
    if(!bInitializedValues||LastRealmIndex!=Snapshot.RealmIndex)
    {
        // A realm advancement starts a new threshold; do not interpolate backwards through the old realm.
        DisplayedHealth=Health;DisplayedHealthFraction=SafeFraction(Health,MaxHealth);
        DisplayedCultivation=Cultivation;DisplayedCultivationFraction=SafeFraction(Cultivation,RequiredCultivation);
        HealthProgress.Reset(SafeFraction(Health,MaxHealth));CultivationProgress.Reset(SafeFraction(Cultivation,RequiredCultivation));ShieldProgress.Reset(SafeFraction(Shield,MaxShield));ActionProgress.Reset(ActionFraction);
        bInitializedValues=true;LastRealmIndex=Snapshot.RealmIndex;
    }
    if(!bAvatarOverride)
    {
        if(AvatarUrl!=Snapshot.AvatarUrl){AvatarUrl=Snapshot.AvatarUrl;AvatarTexture=nullptr;RequestedAvatarId.Empty();RequestedAvatarUrl.Empty();NextAvatarAttempt=0;}
        bAvatarPlaceholder=AvatarTexture==nullptr;
        // The live provider also receives the profile before a cultivation snapshot refresh.
        if(auto* Host=AvatarHost.Get())if(!Host->GetAnchorAvatarUrl().IsEmpty()&&Host->GetScopedUserId(Host->GetCurrentSession().AnchorUserId)==AnchorId)
        {
            if(AvatarUrl!=Host->GetAnchorAvatarUrl()){AvatarUrl=Host->GetAnchorAvatarUrl();AvatarTexture=nullptr;RequestedAvatarId.Empty();NextAvatarAttempt=0;}
            if(auto* Cached=Host->GetCachedAvatar(AvatarUrl)){AvatarTexture=Cached;bAvatarPlaceholder=false;}
        }
    }
    if(RealmTitle)RealmTitle->Configure(EInkNiagaraTitleKind::Realm,RealmName,InkNiagaraTitlePresentation::QualityForRealmIndex(Snapshot.RealmIndex),InkHomeArt::Brush(TEXT("home-realm-plaque")));
    if(AvatarFrame)AvatarFrame->Configure(EInkNiagaraTitleKind::AvatarFrame,FText::GetEmpty(),FMath::Max(3,InkNiagaraTitlePresentation::QualityForRealmIndex(Snapshot.RealmIndex)),InkHomeArt::Brush(TEXT("home-avatar-frame")));
    if(!AvatarTexture&&AvatarUrl.StartsWith(TEXT("https://"))&&AvatarHost.IsValid()&&FPlatformTime::Seconds()>=NextAvatarAttempt)
    {
        NextAvatarAttempt=FPlatformTime::Seconds()+15.;RequestedAvatarUrl=AvatarUrl;
        // A URL-specific request key prevents an older URL reply for the same identity replacing its new avatar.
        RequestedAvatarId=TEXT("anchorhud:")+AnchorId+TEXT(":")+FString::Printf(TEXT("%08x"),GetTypeHash(AvatarUrl));
        if(auto* Host=AvatarHost.Get())Host->RequestAvatar(RequestedAvatarId,AvatarUrl);
    }
    const auto* Pawn=Cast<APawn>(AnchorHealth->GetOwner());
    const auto* Action=Pawn?Pawn->FindComponentByClass<UCultivationGameplayComponent>():nullptr;
    FInkAnchorActionPresentation P;
    if(Action&&!Action->IsMenuPresentation()&&!bDead)
    {
        float Duration=0;
        if(Action->State==ECultivationActionState::MeditationStart||Action->State==ECultivationActionState::MeditationEnd)Duration=FMath::Clamp(Action->InkTransitionSeconds,.6f,2.f);
        else if(const auto* Clip=Action->GetCurrentAnimation())Duration=Clip->GetPlayLength();
        P=ResolveAction(Action->State,Action->StateElapsed,Duration);
    }
    else P.Label=bDead?TEXT("已倒下 · 返回后重新入世"):TEXT("自由行动");
    ActionLabel=P.Label;ActionFraction=P.Fraction;bActionIndeterminate=P.bIndeterminate;bActionActive=P.bActive;
}

int32 UInkAnchorStatusWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const
{
    return Super::NativePaint(Args,Geometry,CullingRect,Elements,Layer,Style,bParentEnabled);
}

int32 UInkAnchorStatusArt::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& R,FSlateWindowElementList& E,int32 L,const FWidgetStyle& S,bool B)const
{
    return Owner.IsValid()?Owner->PaintStatus(A,G,R,E,L,S,B):L;
}

int32 UInkAnchorStatusWidget::PaintStatus(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const
{
    if(PresentationSurfaces.Num()!=8)return Layer;
    if(!bHasAnchor)
    {
        const FSlateRoundedBoxBrush PendingInk(FLinearColor(.025f,.035f,.032f,.8f),8.f);
        FSlateDrawElement::MakeBox(Elements,Layer,Geometry.ToPaintGeometry(FVector2f(Geometry.GetLocalSize().X,52),FSlateLayoutTransform()),&PendingInk,ESlateDrawEffect::None,PendingInk.GetTint(Style)*Style.GetColorAndOpacityTint());
        FSlateDrawElement::MakeText(Elements,Layer+1,Geometry.ToPaintGeometry(FVector2f(Geometry.GetLocalSize().X-24,44),FSlateLayoutTransform(FVector2f(12,4))),FText::FromString(TEXT("主播状态 · 待同步")),InkUiTypography::MakeFont(EInkUiTextRole::Body,22),ESlateDrawEffect::None,InkPresentationStyle::Paper()*Style.GetColorAndOpacityTint());
        return Layer+1;
    }
    const FVector2D Scale(Geometry.GetLocalSize().X/440.,Geometry.GetLocalSize().Y/128.);
    if(Scale.X<=0||Scale.Y<=0)return Layer;
    const FLinearColor Light=InkPresentationStyle::Paper(),Ink=InkPresentationStyle::Ink();
    const FSlateRoundedBoxBrush PortraitPaper(FLinearColor::FromSRGBColor(FColor(214,228,212)),44.f);
    auto Art=[&](const FSlateBrush& Brush,int Z,FVector2D P,FVector2D S,FLinearColor Tint=FLinearColor::White)
    {
        FSlateDrawElement::MakeBox(Elements,Layer+Z,Geometry.ToPaintGeometry(FVector2f(S*Scale),FSlateLayoutTransform(FVector2f(P*Scale))),&Brush,ESlateDrawEffect::None,Tint*Brush.GetTint(Style)*Style.GetColorAndOpacityTint());
    };
    auto Text=[&](FString Value,FVector2D P,float Width,int Size,FLinearColor Color,bool Center=false,bool Outline=false,float Height=30)
    {
        auto Font=InkUiTypography::MakeFont(EInkUiTextRole::Data,FMath::Max(12,FMath::RoundToInt(Size*FMath::Min(Scale.X,Scale.Y))));
        if(TextFont)Font.FontObject=TextFont;
        Font.OutlineSettings.OutlineSize=Outline?1:0;
        Font.OutlineSettings.OutlineColor=FLinearColor(.012f,.026f,.024f,1);
        const auto Cache=FSlateApplication::Get().GetRenderer()->GetFontCache();
        const float FontScale=FMath::Max(.01f,Geometry.GetAccumulatedLayoutTransform().GetScale());
        auto Shape=[&](const FString& String){return Cache->ShapeBidirectionalText(String,Font,FontScale,TextBiDi::ETextDirection::LeftToRight,ETextShapingMethod::FullShaping);};
        auto Glyphs=Shape(Value);const float Available=Width*Scale.X;
        while(Value.Len()>0&&Glyphs->GetMeasuredWidth()/FontScale>Available)
        {
            const int32 Last=Value.Len()-1;
            const bool Pair=Last>0&&Value[Last]>=0xDC00&&Value[Last]<=0xDFFF&&Value[Last-1]>=0xD800&&Value[Last-1]<=0xDBFF;
            Value.LeftChopInline(Pair?2:1);Glyphs=Shape(Value+TEXT("…"));
        }
        FVector2D Offset=P*Scale;
        if(Center)Offset.X+=FMath::Max(0.f,(Available-Glyphs->GetMeasuredWidth()/FontScale)*.5f);
        Offset.Y+=FMath::Max(0.f,(Height*Scale.Y-Glyphs->GetMaxTextHeight()/FontScale)*.5f);
        FSlateDrawElement::MakeShapedText(Elements,Layer+7,Geometry.ToPaintGeometry(FVector2f(Available,Height*Scale.Y),FSlateLayoutTransform(FVector2f(Offset))),Glyphs,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),Outline?FLinearColor(.012f,.026f,.024f,1):FLinearColor::Transparent);
    };
    auto Number=[](double V)->FString
    {
        if(FMath::Abs(V)>=1.e8)return FString::Printf(TEXT("%.2f亿"),V/1.e8);
        if(FMath::Abs(V)>=1.e4)return FString::Printf(TEXT("%.1f万"),V/1.e4);
        return FString::Printf(TEXT("%.0f"),V);
    };
    auto Bar=[&](int32 Kind,float Fraction,float Trail)
    {
        const auto Bounds=MeterBounds(Kind);
        const FSlateRoundedBoxBrush TrackInk(FLinearColor(.01f,.02f,.02f,.95f),4.f);
        const FSlateRoundedBoxBrush FillInk(Kind==0?FLinearColor(.7f,.09f,.06f,1):FLinearColor(.16f,.55f,.53f,1),3.f);
        Art(TrackInk,2,Bounds.GetTopLeft(),Bounds.GetSize());
        const auto Inside=MeterInterior(Bounds);
        auto FillTo=[&](float FractionToDraw,FLinearColor Tint)
        {
            const float Value=FMath::IsFinite(FractionToDraw)?FMath::Clamp(FractionToDraw,0.f,1.f):0.f;
            if(Value>0)Art(FillInk,3,Inside.GetTopLeft(),FVector2D(Inside.GetSize().X*Value,Inside.GetSize().Y),Tint);
        };
        if(Trail>Fraction)FillTo(Trail,FLinearColor(.5f,.5f,.5f,1));
        FillTo(Fraction,FLinearColor::White);
    };
    Elements.PushClip(FSlateClippingZone(Geometry));
    // A single fitted ink stroke supports the compact band; numbers sit beside thin meters.
    InkHomeArt::Paint(TEXT("home-identity-ink"),PresentationSurfaces[0],FSlateRect(0,0,440*Scale.X,128*Scale.Y),Geometry,Elements,Layer,Style.GetColorAndOpacityTint());
    const auto Portrait=AvatarPortraitBounds();const FVector2D PortraitP=Portrait.GetTopLeft(),PortraitS=Portrait.GetSize();
    if(AvatarTexture&&!bAvatarPlaceholder)
    {
        FSlateBrush Image;Image.DrawAs=ESlateBrushDrawType::Image;Image.SetResourceObject(PortraitMask?static_cast<UObject*>(PortraitMask.Get()):static_cast<UObject*>(AvatarTexture.Get()));
        Image.ImageType=ESlateBrushImageType::FullColor;Image.OutlineSettings=FSlateBrushOutlineSettings(26.f);Art(Image,4,PortraitP,PortraitS);
    }
    else
    {
        Art(PortraitPaper,4,PortraitP,PortraitS);
        Text(TEXT("主"),PortraitP,PortraitS.X,28,Ink,true,false,PortraitS.Y);
    }
    Text(AnchorName.IsEmpty()?TEXT("主播"):AnchorName,FVector2D(104,4),324,24,Light,false,true,30);
    Bar(0,DisplayedHealthFraction,HealthTrailFraction);
    Text(TEXT("血量"),FVector2D(104,34),48,20,Light,false,true,30);
    Text(Number(DisplayedHealth)+TEXT("/")+Number(MaxHealth),FVector2D(300,34),132,20,Light,false,true,30);
    Bar(1,DisplayedCultivationFraction,CultivationTrailFraction);
    Text(TEXT("修为"),FVector2D(104,66),48,20,Light,false,true,30);
    Text(RequiredCultivation>0?Number(DisplayedCultivation)+TEXT("/")+Number(RequiredCultivation):TEXT("最高境界"),FVector2D(300,66),132,20,Light,false,true,30);
    Text(MaxShield>0?TEXT("护盾 ")+Number(Shield)+TEXT("/")+Number(MaxShield):TEXT("护盾 · 未激活"),FVector2D(168,98),264,18,Light,false,true,26);
    Elements.PopClip();
    return Layer+8;
}

void UInkAnchorStatusWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);RefreshFromWorld();
    if(AvatarTexture&&!bAvatarPlaceholder&&MaskedPortrait!=AvatarTexture.Get())
    {
        if(!PortraitMask)if(auto* Material=InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/HUD/M_HomeAvatarCircle.M_HomeAvatarCircle")))PortraitMask=UMaterialInstanceDynamic::Create(Material,this);
        if(PortraitMask){PortraitMask->SetTextureParameterValue(TEXT("Avatar"),AvatarTexture);MaskedPortrait=AvatarTexture;}
    }
    const float Delta=FMath::IsFinite(DeltaSeconds)?FMath::Clamp(DeltaSeconds,0.f,.25f):0;
    AnimationTime+=Delta;DamageFlash=FMath::Max(0.f,DamageFlash-Delta/ .24f);
    const bool Reduced=Frontend.IsValid()&&Frontend->bReducedMotion;
    const bool Shown=bHasAnchor&&Frontend.IsValid()&&Frontend->CurrentPage==TEXT("HUD");
    const FVector2D EffectScale(Geometry.GetLocalSize().X/440.,Geometry.GetLocalSize().Y/128.);
    auto MoveEffect=[&](UInkNiagaraTitleWidget* Effect,FVector2D P,FVector2D S)
    {
        if(!Effect)return;
        Effect->SetReducedMotion(Reduced);Effect->SetPresentationVisible(Shown);
        if(auto* Slot=Cast<UCanvasPanelSlot>(Effect->Slot)){Slot->SetPosition(P*EffectScale);Slot->SetSize(S*EffectScale);}
    };
    MoveEffect(AvatarFrame,AvatarFrameBounds().GetTopLeft(),AvatarFrameBounds().GetSize());
    MoveEffect(RealmTitle,RealmTitleBounds().GetTopLeft(),RealmTitleBounds().GetSize());
    if(Reduced)DamageFlash=0;
    auto Update=[&](FInkProgressState& State,float Value,double Maximum,double& LastMaximum){if(Reduced||LastMaximum!=Maximum)State.Reset(Value);LastMaximum=Maximum;State.SetTarget(Value,ProgressSettings);State.Tick(Delta,ProgressSettings);};
    Update(HealthProgress,SafeFraction(Health,MaxHealth),MaxHealth,LastMaxHealth);Update(ShieldProgress,SafeFraction(Shield,MaxShield),MaxShield,LastMaxShield);Update(CultivationProgress,SafeFraction(Cultivation,RequiredCultivation),RequiredCultivation,LastRequiredCultivation);
    ActionProgress.SetTarget(ActionFraction,ProgressSettings);ActionProgress.Tick(Delta,ProgressSettings);
    DisplayedHealth=Health;DisplayedCultivation=Cultivation;DisplayedShield=Shield;
    DisplayedHealthFraction=HealthProgress.Value;HealthTrailFraction=HealthProgress.Trail;
    DisplayedCultivationFraction=CultivationProgress.Value;CultivationTrailFraction=CultivationProgress.Trail;
    DisplayedShieldFraction=ShieldProgress.Value;ShieldTrailFraction=ShieldProgress.Trail;
    if(DesiredSizeBox)
    {
        const double Scale=UWidgetLayoutLibrary::GetViewportScale(this);
        const FVector2D Size=ResponsiveSize(UWidgetLayoutLibrary::GetViewportSize(this)/FMath::Max(.01,Scale));
        DesiredSizeBox->SetWidthOverride(Size.X);
        // Idle has no action row; release its height instead of reserving blank paper.
        DesiredSizeBox->SetHeightOverride(Size.Y);
    }
}

void UInkAnchorStatusWidget::ObserveDamage(UCultivationHealthComponent* Component)
{
    if(ObservedHealth.Get()==Component)return;
    if(ObservedHealth.IsValid())ObservedHealth->OnDamageApplied.RemoveDynamic(this,&ThisClass::HandleDamage);
    ObservedHealth=Component;DamageFlash=0;
    if(Component)Component->OnDamageApplied.AddUniqueDynamic(this,&ThisClass::HandleDamage);
}
void UInkAnchorStatusWidget::HandleDamage(double Applied,double Maximum)
{if(Applied>0&&Maximum>0)DamageFlash=1;}
