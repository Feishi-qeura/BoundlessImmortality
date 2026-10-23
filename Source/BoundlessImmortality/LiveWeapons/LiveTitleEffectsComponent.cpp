#include "LiveTitleEffectsComponent.h"
#include "LiveWeaponSwarm.h"
#include "LiveWeaponHUD.h"
#include "LiveGMOverlay.h"
#include "LiveWeaponGameMode.h"
#include "LiveAudienceAdapterComponent.h"
#include "Cultivation/CultivationNameplate.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkNiagaraTitleWidget.h"
#include "NiagaraSystemWidget.h"
#include "NiagaraUIComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

ULiveTitleEffectsComponent::ULiveTitleEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick=true;PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
    TitleSystem=TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/UI/Ink/Niagara/Shared/NS_InkTitleAura.NS_InkTitleAura")));
}
bool ULiveTitleEffectsComponent::IsPresentationBlocked(UWorld* World,bool IncludeGM)
{
    if(!World)return true;
    for(TActorIterator<AInkMenuDirector> It(World);It;++It)
        if(It->bMenuActive||(It->FrontendWidget&&It->FrontendWidget->IsInViewport()&&It->FrontendWidget->IsVisible()&&It->FrontendWidget->CurrentPage!=TEXT("HUD")))return true;
    bool NewGM=false;
    if(IncludeGM)
    {
        for(TActorIterator<ALiveGMOverlay> It(World);It;++It)if(It->Panel){NewGM=true;if(It->Panel->bPanelOpen)return true;}
        if(!NewGM)if(auto* PC=World->GetFirstPlayerController())if(auto* HUD=Cast<ALiveWeaponHUD>(PC->GetHUD()))if(HUD->bGMPanelOpen)return true;
    }
    return false;
}
bool ULiveTitleEffectsComponent::IsAnchorVisible(APlayerController* PC,const FVector& Position,bool Occlude)
{
    if(!PC)return false;FVector2D P;int32 W,H;PC->GetViewportSize(W,H);
    if(!PC->ProjectWorldLocationToScreen(Position,P,true)||P.X<0||P.Y<0||P.X>W||P.Y>H)return false;
    if(Occlude&&PC->PlayerCameraManager)
    {
        FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(InkTitleOcclusion),true);Params.AddIgnoredActor(PC->GetPawn());
        if(PC->GetWorld()->LineTraceSingleByChannel(Hit,PC->PlayerCameraManager->GetCameraLocation(),Position,ECC_Visibility,Params))return false;
    }
    return true;
}
void ULiveTitleEffectsComponent::Retire(const FString& Id)
{
    if(auto* Effect=Effects.FindRef(Id).Get()){Effect->StopEffects();Effect->RemoveFromParent();++ReclaimedEffectCount;}
    Effects.Remove(Id);
}
TArray<UNiagaraComponent*> ULiveTitleEffectsComponent::GetActiveEffects()const
{TArray<UNiagaraComponent*> Out;for(const auto& Pair:Effects)if(Pair.Value)for(auto* Widget:Pair.Value->GetEffects())if(auto* C=Widget->GetNiagaraComponent())Out.Add(C);return Out;}
TArray<UInkNiagaraNameplateWidget*> ULiveTitleEffectsComponent::GetActiveNameplates()const
{TArray<UInkNiagaraNameplateWidget*> Out;for(const auto& Pair:Effects)if(Pair.Value)Out.Add(Pair.Value);return Out;}
void ULiveTitleEffectsComponent::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);auto* Swarm=Cast<ALiveWeaponSwarm>(GetOwner());auto* PC=GetWorld()->GetFirstPlayerController();
    bool Visible=bEnabled&&Swarm&&!Swarm->IsHidden()&&Swarm->IsActorTickEnabled()&&Swarm->GetPresentationMode()==ELiveWeaponPresentationMode::Gameplay&&PC&&PC->GetPawn()&&PC->PlayerCameraManager;
    Visible=Visible&&!IsPresentationBlocked(GetWorld());
    TArray<FLiveWeaponLabelSample> Samples;if(Visible)Swarm->GetPresentationSamples(Samples);
    if(PC&&PC->GetPawn())Samples.Sort([&](const FLiveWeaponLabelSample& A,const FLiveWeaponLabelSample& B){const bool AP=A.ParticipantId==Swarm->GetPersonalSwordId(),BP=B.ParticipantId==Swarm->GetPersonalSwordId();if(AP!=BP)return AP;const int64 AT=FMath::FloorToInt64(A.LastInteractionTime/2),BT=FMath::FloorToInt64(B.LastInteractionTime/2);if(AT!=BT)return AT>BT;return FVector::DistSquared(A.Position,PC->GetPawn()->GetActorLocation())<FVector::DistSquared(B.Position,PC->GetPawn()->GetActorLocation());});
    TSet<FString> Keep;
    bool Reduced=false;for(TActorIterator<AInkMenuDirector> It(GetWorld());It;++It)if(It->FrontendWidget)Reduced|=It->FrontendWidget->bReducedMotion;
    bool UsesSlate=false;for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)if(It->Panel&&It->Panel->bDrawCultivationPresentation)UsesSlate=true;
    ULiveAudienceAdapterComponent* Audience=nullptr;
    if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();Mode&&Mode->Swarm==Swarm)Audience=Mode->Audience;
    if(!Audience)for(TActorIterator<AInkMenuDirector> It(GetWorld());It;++It)if(It->GameplaySwarm==Swarm){Audience=It->GameplayAudience;break;}
    if(!Audience)for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)if(It->Swarm==Swarm){Audience=It->Audience;break;}
    const float DPI=FMath::Max(.01f,UWidgetLayoutLibrary::GetViewportScale(this));
    for(const auto& Sample:Samples)
    {
        if(Keep.Num()>=FMath::Clamp(MaximumEffects,0,128))break;
        if(!IsAnchorVisible(PC,Sample.Position,bOccludeWithScene))continue;
        FVector2D Projected;PC->ProjectWorldLocationToScreen(Sample.Position,Projected,true);
        float Scale=CultivationNameplate::DistanceScale(FVector::Distance(Swarm->GetWeaponPosition(Sample.ParticipantId),PC->GetPawn()->GetActorLocation()),Swarm->NameplateDistanceSettings);
        if(!UsesSlate){if(const auto* HUD=Cast<ALiveWeaponHUD>(PC->GetHUD()))Scale*=FMath::Clamp(HUD->NameplateScale,.05f,1.f);Scale/=DPI;}
        auto* Effect=Effects.FindRef(Sample.ParticipantId).Get();
        if(!Effect){Effect=CreateWidget<UInkNiagaraNameplateWidget>(PC);if(!Effect)continue;Effect->SetVisibility(ESlateVisibility::HitTestInvisible);Effect->AddToViewport(12);Effects.Add(Sample.ParticipantId,Effect);}
        const FVector2D Size=CultivationNameplate::Layout().Size*Scale;
        Effect->SetDesiredSizeInViewport(CultivationNameplate::Layout().Size);
        Effect->SetRenderTransformPivot(FVector2D::ZeroVector);Effect->SetRenderScale(FVector2D(Scale));
        Effect->SetPositionInViewport(Projected/DPI-FVector2D(Size.X*.5,Size.Y),false);
        auto* State=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();auto Data=CultivationNameplate::Resolve(State,Swarm->GetProgressionOwnerId(Sample.ParticipantId),Sample.DisplayName);
        if(Audience){Data.AvatarTexture=Audience->GetAvatar(Sample.ParticipantId);Audience->RequestVisibleAvatar(Sample.ParticipantId);}
        Effect->SetIdentity(Sample.ParticipantId,Data.DisplayName,Data.AvatarTexture,Data.AvatarFrame);
        Effect->Configure(Data.TitleId,FText::FromString(Data.WeaponTitle),Data.RealmIndex,FText::FromString(Data.RealmTitle),Reduced);
        Effect->SetEffectStyle(TitleColor,Intensity,float(Data.RealmAdvancePulse));Keep.Add(Sample.ParticipantId);
    }
    TArray<FString> Keys;Effects.GetKeys(Keys);for(const auto& Id:Keys)if(!Keep.Contains(Id))Retire(Id);
    ActiveEffectCount=Effects.Num();ActiveSystemCount=GetActiveEffects().Num();ActiveParticipantIds=Keep.Array();
}
void ULiveTitleEffectsComponent::EndPlay(const EEndPlayReason::Type Reason)
{TArray<FString> Keys;Effects.GetKeys(Keys);for(const auto& Id:Keys)Retire(Id);ActiveEffectCount=0;ActiveSystemCount=0;ActiveParticipantIds.Reset();Super::EndPlay(Reason);}
