#include "CultivationGameplayComponent.h"
#include "QiCultivationComponent.h"
#include "NiagaraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Font.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

UCultivationGameplayComponent::UCultivationGameplayComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PrePhysics;
    InkTransitionMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/1/QiCultivation/M_MeditationInkTransition.M_MeditationInkTransition")));
}

void UCultivationGameplayComponent::BeginPlay()
{
    Super::BeginPlay();
    Character=Cast<ACharacter>(GetOwner());
    Driver=Character ? Character->GetMesh() : nullptr;
    bActiveMap=Character && Driver && GetWorld() &&
        (EnabledMapName.IsNone() || UWorld::RemovePIEPrefix(GetWorld()->GetMapName())==EnabledMapName.ToString());
    if(!bActiveMap){SetComponentTickEnabled(false);return;}
    TInlineComponentArray<UNiagaraComponent*> Effects(Character);
    for(auto* Effect:Effects)
        if(Effect->GetFName()==InkEffectComponentName){InkEffect=Effect;break;}
    if(InkEffect)
    {
        ResetInkEffect();
    }
    Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
}

void UCultivationGameplayComponent::SetInkEffectPlaying(bool bPlaying)
{
    if(!IsValid(InkEffect))return;
    if(bPlaying)
    {
        // Restart even if the previous activation is still draining its remaining particles.
        InkEffect->Activate(true);
    }
    else
    {
        InkEffect->Deactivate();
    }
}

void UCultivationGameplayComponent::ResetInkEffect()
{
    if(!IsValid(InkEffect))return;
    // Deactivate cancels pending activation; immediate deactivation alone does not.
    InkEffect->Deactivate();
    InkEffect->DeactivateImmediate();
}

void UCultivationGameplayComponent::BindController(APlayerController* Controller)
{
    BoundController=Controller;
    AddTickPrerequisiteActor(Controller);
    InputBlocker=NewObject<UInputComponent>(GetOwner(),TEXT("CultivationInput"));
    InputBlocker->Priority=1000;
    InputBlocker->bBlockInput=true;
    InputBlocker->RegisterComponent();
    Controller->PushInputComponent(InputBlocker);
    Controller->StopMovement();
    Qi=GetOwner()->FindComponentByClass<UQiCultivationComponent>();
    if(Qi){bPreviousQiKeyboard=Qi->bEnableKeyboard;Qi->bEnableKeyboard=false;Qi->CancelCultivation();}
    bPreviousMouseDown=Controller->IsInputKeyDown(EKeys::LeftMouseButton);
    bToggleLatched=Controller->IsInputKeyDown(EKeys::C);
    ToggleHeldSeconds=0;
}

void UCultivationGameplayComponent::UnbindController()
{
    FinishAction();
    ResetInkEffect();
    if(auto* PC=BoundController.Get())
    {
        if(InputBlocker)PC->PopInputComponent(InputBlocker);
        RemoveTickPrerequisiteActor(PC);
    }
    if(InputBlocker){InputBlocker->DestroyComponent();InputBlocker=nullptr;}
    if(Qi){Qi->CancelCultivation();Qi->bEnableKeyboard=bPreviousQiKeyboard;Qi=nullptr;}
    BoundController.Reset();
    bAttackHeld=false;bPreviousMouseDown=false;bToggleLatched=false;ToggleHeldSeconds=0;
}

bool UCultivationGameplayComponent::CanStart(bool Meditation) const
{
    if(!bActiveMap || !Driver || !Character || !BoundController.IsValid() ||
        !Character->GetCharacterMovement()->IsMovingOnGround())return false;
    auto* Mesh=Driver->GetSkeletalMeshAsset();
    if(!Mesh)return false;
    const UAnimSequence* Clips[]={Meditation ? MeditationLoop.Get() : ChargeStart.Get(),
        Meditation ? MeditationLoop.Get() : ChargeLoop.Get(),Meditation ? MeditationLoop.Get() : ChargeRelease.Get()};
    for(const auto* Clip:Clips)
        if(!Clip || Clip->GetPlayLength()<=0 || Clip->GetSkeleton()!=Mesh->GetSkeleton())return false;
    return true;
}

void UCultivationGameplayComponent::BeginAction()
{
    PreviousAnimationMode=Driver->GetAnimationMode();
    PreviousAnimClass=Driver->GetAnimClass();
    PreviousSingleAnimation=Driver->GetSingleNodeInstance() ? Driver->GetSingleNodeInstance()->GetCurrentAsset() : nullptr;
    auto* Movement=Character->GetCharacterMovement();
    PreviousMovementMode=Movement->MovementMode;PreviousCustomMovementMode=Movement->CustomMovementMode;
    Character->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();Movement->DisableMovement();
    if(auto* PC=BoundController.Get()){PC->StopMovement();PC->SetIgnoreMoveInput(true);}
    bMovementLease=true;
    Driver->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    LastMoveDirection=FVector::ZeroVector;
}

UAnimSequence* UCultivationGameplayComponent::GetCurrentAnimation() const
{
    switch(State)
    {
    case ECultivationActionState::MeditationStart:return bMeditationPoseSwitched ? MeditationLoop.Get() : nullptr;
    case ECultivationActionState::MeditationLoop:return MeditationLoop;
    case ECultivationActionState::MeditationEnd:return bMeditationPoseSwitched ? nullptr : MeditationLoop.Get();
    case ECultivationActionState::ChargeStart:return ChargeStart;
    case ECultivationActionState::ChargeLoop:return ChargeLoop;
    case ECultivationActionState::ChargeRelease:return ChargeRelease;
    default:return nullptr;
    }
}

void UCultivationGameplayComponent::EnterState(ECultivationActionState NewState)
{
    State=NewState;StateElapsed=0;
    if(State==ECultivationActionState::MeditationStart || State==ECultivationActionState::MeditationEnd)
    {
        bMeditationPoseSwitched=false;
        if(!EnsureInkCover()){FinishAction();return;}
        SetInkEffectPlaying(State==ECultivationActionState::MeditationStart);
        if(State==ECultivationActionState::MeditationEnd && Qi)Qi->StopCultivation();
        UpdateInkCover();return;
    }
    ClearInkCover();
    UAnimSequence* Clip=GetCurrentAnimation();
    if(!Clip){FinishAction();return;}
    const bool Loop=State==ECultivationActionState::MeditationLoop || State==ECultivationActionState::ChargeLoop;
    Driver->PlayAnimation(Clip,Loop);
    if(Qi)
    {
        if(State==ECultivationActionState::MeditationLoop)Qi->StartCultivation();
        else Qi->StopCultivation();
    }
}

bool UCultivationGameplayComponent::RequestCultivationToggle()
{
    if(bMenuPresentation||bFrontendInputBlocked)return false;
    if(State==ECultivationActionState::Idle)
    {
        if(!CanStart(true)||!EnsureInkCover())return false;
        bExitMeditationRequested=false;BeginAction();EnterState(ECultivationActionState::MeditationStart);return true;
    }
    if(State==ECultivationActionState::MeditationStart){bExitMeditationRequested=!bExitMeditationRequested;return true;}
    if(State==ECultivationActionState::MeditationLoop){EnterState(ECultivationActionState::MeditationEnd);return true;}
    return false;
}

bool UCultivationGameplayComponent::SetAttackHeld(bool Held)
{
    if(bMenuPresentation||(bFrontendInputBlocked&&Held))return false;
    bAttackHeld=Held;
    if(Held && State==ECultivationActionState::Idle)
    {
        if(!CanStart(false))return false;
        BeginAction();EnterState(ECultivationActionState::ChargeStart);return true;
    }
    if(!Held && State==ECultivationActionState::ChargeLoop){EnterState(ECultivationActionState::ChargeRelease);return true;}
    return State==ECultivationActionState::ChargeStart;
}
void UCultivationGameplayComponent::InterruptAction()
{
    if(bMenuPresentation)return;
    bAttackHeld=false;bPreviousMouseDown=false;bToggleLatched=false;ToggleHeldSeconds=0;
    FinishAction();ResetInkEffect();if(Qi)Qi->CancelCultivation();
}

void UCultivationGameplayComponent::SetFrontendInputBlocked(bool bBlocked)
{
    if(bFrontendInputBlocked==bBlocked)return;
    // Release a held attack so opening a frontend page cannot leave ChargeLoop latched.
    if(bBlocked&&!bMenuPresentation)SetAttackHeld(false);
    bFrontendInputBlocked=bBlocked;
    bAttackHeld=false;bPreviousMouseDown=false;bToggleLatched=false;ToggleHeldSeconds=0;
    LastMoveDirection=FVector::ZeroVector;
    if(Character)Character->ConsumeMovementInputVector();
    if(!bBlocked)if(auto* PC=BoundController.Get())
    {
        bPreviousMouseDown=PC->IsInputKeyDown(EKeys::LeftMouseButton);
        bToggleLatched=PC->IsInputKeyDown(EKeys::C);
    }
}

bool UCultivationGameplayComponent::SetMenuPresentation(bool bEnabled)
{
    if(bMenuPresentation==bEnabled)return true;
    if(!bEnabled)
    {
        bMenuPresentation=false;
        UnbindController();
        if(Driver)Driver->VisibilityBasedAnimTickOption=AnimationTickBeforeMenuPresentation;
        SetComponentTickEnabled(bTickBeforeMenuPresentation);
        return true;
    }
    // Validate before touching the existing actor. Menu playback needs no local controller.
    if(!HasBegunPlay()||!bActiveMap||!Character||!Driver||!Character->GetCharacterMovement())return false;
    const auto* Mesh=Driver->GetSkeletalMeshAsset();
    if(!Mesh||!MeditationLoop||MeditationLoop->GetPlayLength()<=0||MeditationLoop->GetSkeleton()!=Mesh->GetSkeleton())return false;
    UnbindController();
    bTickBeforeMenuPresentation=IsComponentTickEnabled();
    AnimationTickBeforeMenuPresentation=Driver->VisibilityBasedAnimTickOption;
    Qi=GetOwner()->FindComponentByClass<UQiCultivationComponent>();
    if(Qi){bPreviousQiKeyboard=Qi->bEnableKeyboard;Qi->bEnableKeyboard=false;Qi->CancelCultivation();}
    bMenuPresentation=true;
    SetComponentTickEnabled(true);
    BeginAction();
    EnterState(ECultivationActionState::MeditationLoop);
    SetInkEffectPlaying(true);
    return State==ECultivationActionState::MeditationLoop;
}

void UCultivationGameplayComponent::FinishAction()
{
    ClearInkCover();
    SetInkEffectPlaying(false);
    if(Qi && bMovementLease)Qi->StopCultivation();
    if(bMovementLease)
    {
        // Exit pose is already restored under the opaque cover; do not restart its AnimBP.
        if(!(State==ECultivationActionState::MeditationEnd && bMeditationPoseSwitched))RestoreAnimation();
        if(Character)
        {
            Character->ConsumeMovementInputVector();
            Character->GetCharacterMovement()->SetMovementMode(PreviousMovementMode,PreviousCustomMovementMode);
        }
        if(auto* PC=BoundController.Get())PC->SetIgnoreMoveInput(false);
    }
    bMovementLease=false;bExitMeditationRequested=false;State=ECultivationActionState::Idle;StateElapsed=0;
}

void UCultivationGameplayComponent::AdvanceAction(float DeltaTime)
{
    if(State==ECultivationActionState::Idle)return;
    StateElapsed+=DeltaTime;
    if(State==ECultivationActionState::MeditationStart || State==ECultivationActionState::MeditationEnd)
    {
        UpdateInkCover();
        const float Duration=FMath::Clamp(InkTransitionSeconds,.6f,2.f);
        if(!bMeditationPoseSwitched && StateElapsed>=Duration*.4f)
        {
            // At this point the material has a fully opaque center covering the whole character.
            bMeditationPoseSwitched=true;
            if(State==ECultivationActionState::MeditationStart)Driver->PlayAnimation(MeditationLoop,true);
            else RestoreAnimation();
        }
        if(StateElapsed>=Duration)
        {
            if(State==ECultivationActionState::MeditationStart)
                EnterState(bExitMeditationRequested ? ECultivationActionState::MeditationEnd : ECultivationActionState::MeditationLoop);
            else FinishAction();
        }
        return;
    }
    auto* Clip=GetCurrentAnimation();
    if(!Clip){FinishAction();return;}
    if(State==ECultivationActionState::MeditationLoop || State==ECultivationActionState::ChargeLoop)return;
    if(StateElapsed<Clip->GetPlayLength())return;
    switch(State)
    {
    case ECultivationActionState::MeditationStart:
        EnterState(bExitMeditationRequested ? ECultivationActionState::MeditationEnd : ECultivationActionState::MeditationLoop);break;
    case ECultivationActionState::ChargeStart:
        EnterState(bAttackHeld ? ECultivationActionState::ChargeLoop : ECultivationActionState::ChargeRelease);break;
    default:FinishAction();break;
    }
}

void UCultivationGameplayComponent::PollInput(APlayerController* Controller,float DeltaTime)
{
    if(Controller->IsInputKeyDown(EKeys::C))
    {
        ToggleHeldSeconds+=DeltaTime;
        if(!bToggleLatched && ToggleHeldSeconds>=FMath::Clamp(HoldToggleSeconds,.05f,2.f))
        {bToggleLatched=true;RequestCultivationToggle();}
    }
    else{ToggleHeldSeconds=0;bToggleLatched=false;}
    const bool MouseDown=Controller->IsInputKeyDown(EKeys::LeftMouseButton);
    if(MouseDown!=bPreviousMouseDown){bPreviousMouseDown=MouseDown;SetAttackHeld(MouseDown);}
    LastMoveDirection=FVector::ZeroVector;
    if(State!=ECultivationActionState::Idle)return;
    const float Forward=float(Controller->IsInputKeyDown(EKeys::W))-float(Controller->IsInputKeyDown(EKeys::S));
    const float Right=float(Controller->IsInputKeyDown(EKeys::D))-float(Controller->IsInputKeyDown(EKeys::A));
    if(Forward==0 && Right==0)return;
    const float Yaw=Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetCameraRotation().Yaw : Controller->GetControlRotation().Yaw;
    const FRotationMatrix Basis(FRotator(0,Yaw,0));
    LastMoveDirection=(Basis.GetUnitAxis(EAxis::X)*Forward+Basis.GetUnitAxis(EAxis::Y)*Right).GetSafeNormal2D();
    Character->AddMovementInput(LastMoveDirection,1.f);
}

void UCultivationGameplayComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
    if(!bActiveMap || !Character)return;
    if(bMenuPresentation){AdvanceAction(DeltaTime);return;}
    auto* Controller=Cast<APlayerController>(Character->GetController());
    if(!Controller || !Character->IsLocallyControlled())
    {if(BoundController.IsValid() || InputBlocker)UnbindController();return;}
    if(BoundController.Get()!=Controller){UnbindController();BindController(Controller);}
    if(!bFrontendInputBlocked)PollInput(Controller,DeltaTime);
    AdvanceAction(DeltaTime);
}

void UCultivationGameplayComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bMenuPresentation)SetMenuPresentation(false);
    UnbindController();
    if(InkCover){InkCover->DestroyComponent();InkCover=nullptr;InkMaterial=nullptr;}
    if(InkCaption){InkCaption->RemoveFromParent();InkCaption=nullptr;}
    if(Character)Character->GetCharacterMovement()->RemoveTickPrerequisiteComponent(this);
    Super::EndPlay(Reason);
}

void UCultivationGameplayComponent::RestoreAnimation()
{
    if(!Driver)return;
    Driver->SetAnimationMode(PreviousAnimationMode);
    if(PreviousAnimationMode==EAnimationMode::AnimationBlueprint)Driver->SetAnimInstanceClass(PreviousAnimClass);
    else if(PreviousSingleAnimation)Driver->PlayAnimation(PreviousSingleAnimation,true);
}

bool UCultivationGameplayComponent::EnsureInkCover()
{
    if(InkCover&&InkMaterial)return true;
    auto* Material=InkTransitionMaterial.LoadSynchronous();
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    if(!Character||!Material||!Plane)return false;
    InkCover=NewObject<UStaticMeshComponent>(Character,NAME_None,RF_Transient);
    InkCover->SetStaticMesh(Plane);InkCover->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InkCover->SetCanEverAffectNavigation(false);InkCover->SetCastShadow(false);
    InkCover->SetTranslucentSortPriority(100);InkCover->SetHiddenInGame(true);
    InkCover->RegisterComponent();
    InkMaterial=UMaterialInstanceDynamic::Create(Material,this);InkCover->SetMaterial(0,InkMaterial);
    if(auto* PC=BoundController.Get())
    {
        InkCaption=CreateWidget<UCultivationInkCaptionWidget>(PC);
        if(InkCaption){InkCaption->Configure(this);InkCaption->AddToPlayerScreen(120);}
    }
    return true;
}

void UCultivationGameplayComponent::UpdateInkCover()
{
    if(!InkCover||!InkMaterial||!Character)return;
    const auto* PC=BoundController.Get();const APlayerCameraManager* Camera=PC?PC->PlayerCameraManager.Get():nullptr;
    if(!Camera)return;
    const float T=StateElapsed/FMath::Clamp(InkTransitionSeconds,.6f,2.f);
    InkCoverage=FMath::SmoothStep(0.f,.28f,T)*(1-FMath::SmoothStep(.65f,1.f,T));
    InkMaterial->SetScalarParameterValue(TEXT("Coverage"),InkCoverage);
    InkMaterial->SetScalarParameterValue(TEXT("Progress"),T);
    const FRotationMatrix Basis(Camera->GetCameraRotation());
    const FVector Forward=Basis.GetUnitAxis(EAxis::X),Right=Basis.GetUnitAxis(EAxis::Y);
    InkCover->SetWorldLocation(Character->GetActorLocation()-Forward*150);
    InkCover->SetWorldRotation(FRotationMatrix::MakeFromXZ(Right,-Forward).Rotator());
    const float Size=FMath::Max(320.f,Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()*3.8f);
    InkCover->SetWorldScale3D(FVector(Size/100,Size/100,1));
    InkCover->SetHiddenInGame(InkCoverage<=0);
    if(InkCaption)InkCaption->SetVisibility(InkCoverage>0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
}

void UCultivationGameplayComponent::ClearInkCover()
{
    InkCoverage=0;if(InkCover)InkCover->SetHiddenInGame(true);
    if(InkMaterial)InkMaterial->SetScalarParameterValue(TEXT("Coverage"),0);
    if(InkCaption)InkCaption->SetVisibility(ESlateVisibility::Collapsed);
}

FText UCultivationGameplayComponent::GetInkCaptionText()const
{
    if(State==ECultivationActionState::MeditationStart)return MeditationEntryCaption;
    if(State==ECultivationActionState::MeditationEnd)return MeditationExitCaption;
    return FText::GetEmpty();
}

void UCultivationInkCaptionWidget::Configure(UCultivationGameplayComponent* Component)
{
    Source=Component;SetIsFocusable(false);SetVisibility(ESlateVisibility::Collapsed);
    CaptionFont=LoadObject<UFont>(nullptr,TEXT("/Game/LiveWeapons/Fonts/F_LiveNames.F_LiveNames"));
}

int32 UCultivationInkCaptionWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const
{
    const auto* Component=Source.Get();auto* PC=GetOwningPlayer();
    if(!Component||!PC||!Component->GetOwner()||Component->InkCoverage<=0)return Layer;
    const FText Caption=Component->GetInkCaptionText();if(Caption.IsEmpty())return Layer;
    FVector2D Position;if(!PC->ProjectWorldLocationToScreen(Component->GetOwner()->GetActorLocation(),Position,true))return Layer;
    Position/=FMath::Max(.01f,UWidgetLayoutLibrary::GetViewportScale(this));
    FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Regular",30);if(CaptionFont)Font.FontObject=CaptionFont;
    Font.OutlineSettings.OutlineSize=2;Font.OutlineSettings.OutlineColor=FLinearColor(.025f,.02f,.015f,1);
    const FString Text=Caption.ToString();const FVector2D Size=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text,Font);
    const float T=Component->StateElapsed/FMath::Clamp(Component->InkTransitionSeconds,.6f,2.f);
    Position-=Size*.5f+FVector2D(0,10+12*T);
    FSlateDrawElement::MakeText(Elements,Layer+1,Geometry.ToPaintGeometry(FVector2f(Size),FSlateLayoutTransform(FVector2f(Position))),Text,Font,ESlateDrawEffect::None,FLinearColor(.95f,.87f,.64f,Component->InkCoverage));
    return Layer+1;
}
