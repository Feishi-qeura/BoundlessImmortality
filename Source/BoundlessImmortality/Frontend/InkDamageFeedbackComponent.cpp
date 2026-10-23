#include "InkDamageFeedbackComponent.h"
#include "InkProgressState.h"
#include "LiveWeapons/LiveAnchorController.h"
#include "LiveWeapons/LiveTitleEffectsComponent.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UInkDamageCameraModifier::Pulse(const FInkHitShakeSettings& Settings,float Scale)
{
    // A weaker rapid hit cannot replace an already stronger impulse.
    if(Remaining>0&&Settings.RotationAmplitude*Scale<Hit.RotationAmplitude*Strength)return;
    Hit=Settings;Strength=FMath::Clamp(Scale,0.f,2.f);Age=0;Remaining=FMath::Clamp(Hit.Duration,.01f,2.f);
}
bool UInkDamageCameraModifier::ModifyCamera(float Delta,FMinimalViewInfo& POV)
{
    if(Remaining<=0)return false;
    Age+=FMath::Max(0.f,Delta);Remaining=FMath::Max(0.f,Remaining-Delta);
    const float Envelope=FMath::Square(Remaining/FMath::Max(.01f,Hit.Duration))*Strength;
    const float Phase=Age*FMath::Clamp(Hit.Frequency,1.f,60.f)*2*PI;
    const FRotator Base=POV.Rotation;
    POV.Location+=Base.RotateVector(FVector(0,FMath::Sin(Phase*1.17f),FMath::Sin(Phase*.83f)))*FMath::Clamp(Hit.LocationAmplitude,0.f,12.f)*Envelope;
    POV.Rotation+=FRotator(FMath::Sin(Phase),FMath::Sin(Phase*1.31f),FMath::Sin(Phase*.73f)*.3f)*FMath::Clamp(Hit.RotationAmplitude,0.f,4.f)*Envelope;
    return false;
}
UInkDamageFeedbackComponent::UInkDamageFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
    for(int32 I=0;I<4;++I){FInkHitShakeSettings S;S.RotationAmplitude=I==0?.22f:I==1?.5f:I==2?1.f:1.8f;S.LocationAmplitude=I==0?.4f:I==1?.9f:I==2?1.8f:3.2f;S.Duration=.2f+I*.08f;Tiers.Add(S);}
}
void UInkDamageFeedbackComponent::Observe(UCultivationHealthComponent* Health)
{
    if(Observed.Get()==Health)return;
    if(Observed.IsValid())Observed->OnDamageApplied.RemoveDynamic(this,&ThisClass::OnDamage);
    Observed=Health;if(Health)Health->OnDamageApplied.AddUniqueDynamic(this,&ThisClass::OnDamage);
}
void UInkDamageFeedbackComponent::OnDamage(double Applied,double Maximum)
{
    ++DamageEventCount;LastDamageRatio=Maximum>0?Applied/Maximum:0;LastDamageTier=InkFeedback::DamageTier(LastDamageRatio);
    const auto* Pawn=Cast<APawn>(GetOwner());auto* PC=Pawn?Cast<APlayerController>(Pawn->GetController()):nullptr;
    const auto* Live=Cast<ALiveAnchorController>(PC);
    const bool GameplayView=PC&&(PC->GetViewTarget()==Pawn||(Live&&Live->IsUsingGameplayCamera()));
    if(!bCameraFeedback||!PC||!PC->IsLocalController()||!GameplayView||ULiveTitleEffectsComponent::IsPresentationBlocked(GetWorld(),false)||LastDamageTier==0||!Tiers.IsValidIndex(LastDamageTier-1))return;
    if(!Modifier&&PC->PlayerCameraManager)Modifier=Cast<UInkDamageCameraModifier>(PC->PlayerCameraManager->AddNewCameraModifier(UInkDamageCameraModifier::StaticClass()));
    if(Modifier)Modifier->Pulse(Tiers[LastDamageTier-1],Strength);
}
void UInkDamageFeedbackComponent::PreviewDamageRatio(double Ratio)
{
#if !UE_BUILD_SHIPPING
    FString Suffix;
    if(FParse::Value(FCommandLine::Get(),TEXT("saveddirsuffix="),Suffix)&&!Suffix.IsEmpty()&&FMath::IsFinite(Ratio))OnDamage(FMath::Clamp(Ratio,0.,1.),1.);
#endif
}
void UInkDamageFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Observe(nullptr);if(Modifier){Modifier->Stop();if(auto* Camera=Cast<APlayerCameraManager>(Modifier->GetOuter()))Camera->RemoveCameraModifier(Modifier);Modifier=nullptr;}Super::EndPlay(Reason);
}
