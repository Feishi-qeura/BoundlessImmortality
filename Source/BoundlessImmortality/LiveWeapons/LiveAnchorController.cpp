#include "LiveAnchorController.h"
#include "LiveGMTestSession.h"
#include "LiveGMOverlay.h"
#include "Misc/CommandLine.h"
#include "LiveAnchorAbilityComponent.h"
#include "LiveWeaponGameMode.h"
#include "LiveWeaponSwarm.h"
#include "LiveAudienceAdapterComponent.h"
#include "LiveWeaponHUD.h"
#include "LiveMonster.h"
#include "EngineUtils.h"
#include "CultivationGameplayComponent.h"
#include "QiCultivationComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
ALiveAnchorController::ALiveAnchorController(){bAutoManageActiveCameraTarget=false;bShowMouseCursor=true;bEnableClickEvents=true;}
void ALiveAnchorController::OnPossess(APawn* ControlledPawn)
{
    Super::OnPossess(ControlledPawn);
    if(auto* Old=ControlledPawn->FindComponentByClass<UCultivationGameplayComponent>()){Old->SetComponentTickEnabled(false);Old->Deactivate();}
    if(auto* Qi=ControlledPawn->FindComponentByClass<UQiCultivationComponent>()){Qi->bEnableKeyboard=false;Qi->StopCultivation();}
    Ability=ControlledPawn->FindComponentByClass<ULiveAnchorAbilityComponent>();if(!Ability){Ability=NewObject<ULiveAnchorAbilityComponent>(ControlledPawn);Ability->RegisterComponent();}
    LiveCamera=GetWorld()->SpawnActor<ACameraActor>();LiveCamera->GetCameraComponent()->ProjectionMode=ECameraProjectionMode::Orthographic;
    LiveCamera->GetCameraComponent()->OrthoWidth=2600;LiveCamera->GetCameraComponent()->bConstrainAspectRatio=false;SetViewTarget(LiveCamera);
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Mode);
}
void ALiveAnchorController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);APawn* ControlledPawn=GetPawn();if(!ControlledPawn)return;
    const float X=float(IsInputKeyDown(EKeys::W))-float(IsInputKeyDown(EKeys::S));
    const float Y=float(IsInputKeyDown(EKeys::D))-float(IsInputKeyDown(EKeys::A));
    const float Yaw=LiveCamera?LiveCamera->GetActorRotation().Yaw:0;const FRotationMatrix Basis(FRotator(0,Yaw,0));
    ControlledPawn->AddMovementInput((Basis.GetUnitAxis(EAxis::X)*X+Basis.GetUnitAxis(EAxis::Y)*Y).GetSafeNormal());
    if(WasInputKeyJustPressed(EKeys::SpaceBar)&&Ability)Ability->TryPush();
    if(LiveCamera){const FVector Location=ControlledPawn->GetActorLocation()+FVector(-1200,0,1800);LiveCamera->SetActorLocationAndRotation(Location,(ControlledPawn->GetActorLocation()-Location).Rotation());}
#if !UE_BUILD_SHIPPING
    if(WasInputKeyJustPressed(EKeys::F10)){bool Overlay=false;for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)Overlay=true;if(!Overlay)if(auto* HUD=GetHUD<ALiveWeaponHUD>())HUD->bGMPanelOpen=!HUD->bGMPanelOpen;}
    if(WasInputKeyJustPressed(EKeys::F1))LiveAudience(100);if(WasInputKeyJustPressed(EKeys::F2))LiveAudience(500);if(WasInputKeyJustPressed(EKeys::F3))LiveAudience(1000);
    if(WasInputKeyJustPressed(EKeys::F4))LiveSpawnMonsters(12);
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four};for(int I=0;I<4;++I)if(WasInputKeyJustPressed(Keys[I]))LiveComment(TEXT("viewer_0000"),FString::FromInt(I+1));
#endif
}
bool ALiveAnchorController::ExecuteGMAction(FName Action)
{
#if !UE_BUILD_SHIPPING
    auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();
    if(!Mode||!Mode->Audience||!Mode->Swarm)return false;
    if(Action==TEXT("EnableTest"))return Mode->Audience->EnableDevelopmentSimulation();
    if(Mode->Audience->GetStatus()!=TEXT("本地模拟"))return false;
    if(Action==TEXT("Hitboxes")){Mode->Swarm->bDrawAttackHitboxes=!Mode->Swarm->bDrawAttackHitboxes;return true;}
    if(Action==TEXT("Add1"))return Mode->Audience->AppendSimulatedAudience(1)>0;
    if(Action==TEXT("Add10"))return Mode->Audience->AppendSimulatedAudience(10)>0;
    if(Action==TEXT("Add100"))return Mode->Audience->AppendSimulatedAudience(100)>0;
    if(Action==TEXT("Fill1000"))return Mode->Audience->AppendSimulatedAudience(1000)>0;
    if(Action==TEXT("ClearAudience"))return Mode->Audience->ClearSimulatedAudience();
    const FName WeaponActions[]={TEXT("Sword"),TEXT("Axe"),TEXT("Spear"),TEXT("Bow")};
    for(int32 I=0;I<4;++I)if(Action==WeaponActions[I])
    {
        for(const auto& State:Mode->Swarm->GetRoster().GetStates())Mode->Swarm->SwitchParticipantWeapon(State.Profile.ParticipantId,ELiveWeaponType(I));
        return true;
    }
    if(Action==TEXT("SpawnMonsters")){Mode->SpawnMonsters(12);return true;}
    if(Action==TEXT("ClearMonsters")){for(TActorIterator<ALiveMonster> It(GetWorld());It;++It)It->Destroy();return true;}
#endif
    return false;
}
void ALiveAnchorController::LiveAudience(int32 Count)
{
#if !UE_BUILD_SHIPPING
    if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();Mode&&Mode->Audience)Mode->Audience->AddSimulatedAudience(Count);
#endif
}
void ALiveAnchorController::LiveComment(const FString& Id,const FString& Text)
{
#if !UE_BUILD_SHIPPING
    if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();Mode&&Mode->Audience)Mode->Audience->SimulateComment(Id,Id,Text);
#endif
}
void ALiveAnchorController::LiveSpawnMonsters(int32 Count)
{
#if !UE_BUILD_SHIPPING
    if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();Mode&&Mode->Audience&&Mode->Audience->GetStatus()==TEXT("本地模拟"))Mode->SpawnMonsters(Count);
#endif
}
void ALiveAnchorController::LiveSkill(const FString& Id,FName Skill)
{
#if !UE_BUILD_SHIPPING
    if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();Mode&&Mode->Audience&&Mode->Audience->GetStatus()==TEXT("本地模拟"))Mode->Swarm->GrantSkill(TEXT("local:")+Id,Skill);
#endif
}


bool ALiveAnchorController::IsUsingGameplayCamera()const
{return GetPawn()&&LiveCamera&&GetViewTarget()==LiveCamera;}
