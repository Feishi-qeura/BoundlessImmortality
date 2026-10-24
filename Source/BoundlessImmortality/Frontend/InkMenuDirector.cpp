#include "Frontend/InkMenuDirector.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h"
#include "Frontend/InkSaveSubsystem.h"
#include "CultivationGameplayComponent.h"
#include "QiCultivationComponent.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveWeaponDefinition.h"
#include "LiveWeapons/LiveGMOverlay.h"
#include "LiveWeapons/LiveWeaponGameMode.h"
#include "LiveWeapons/LiveAudienceAdapterComponent.h"
#include "LiveWeapons/LiveMonster.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Cultivation/CultivationPrototypeClient.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "ComponentInstanceDataCache.h"
#include "HAL/PlatformTime.h"
#include "UObject/ConstructorHelpers.h"

AInkMenuDirector::AInkMenuDirector()
{
    PrimaryActorTick.bCanEverTick=true;
    SwarmClass=ALiveWeaponSwarm::StaticClass();
    WidgetClass=UInkRebuildFrontendWidget::StaticClass();
    DirectorAudience=CreateDefaultSubobject<ULiveAudienceAdapterComponent>(TEXT("GameplayAudience"));
    EncounterMonsterClass=ALiveMonster::StaticClass();
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sword(TEXT("/Game/Isometric_World/Sky_Temple/Meshes/Sculptures/SM_Scultpture_Sword_01.SM_Scultpture_Sword_01"));
    PresentationSwordMesh=Sword.Object;
}

AInkMenuDirector::~AInkMenuDirector()=default;
void AInkMenuDirector::AddReferencedObjects(UObject* Object,FReferenceCollector& Collector)
{
    auto* Self=CastChecked<AInkMenuDirector>(Object);
    if(Self->PresentationInstanceData)Self->PresentationInstanceData->AddReferencedObjects(Collector);
    Super::AddReferencedObjects(Object,Collector);
}

bool AInkMenuDirector::EnsurePresentationPawn()
{
    if(IsValid(GetPresentationPawn()))return true;
    if(!PresentationPawnClass)return false;
    auto* Pawn=GetWorld()->SpawnActorDeferred<APawn>(PresentationPawnClass,PresentationSpawnTransform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Pawn)return false;
    Pawn->AutoPossessPlayer=EAutoReceiveInput::Disabled;
    Pawn->FinishSpawning(PresentationSpawnTransform,false,PresentationInstanceData.Get());
    PresentationActor=Pawn;return true;
}

void AInkMenuDirector::BeginPlay()
{
    Super::BeginPlay();
    PresentationStatus=TEXT("等待本地玩家与菜单场景");
}

APawn* AInkMenuDirector::GetPresentationPawn()const{return Cast<APawn>(PresentationActor);}
int32 AInkMenuDirector::GetPresentationSwordCount()const{return IsValid(PresentationSwarm)?PresentationSwarm->GetParticipantCount():0;}
int32 AInkMenuDirector::GetPresentationLabelCount()const
{
    TArray<FLiveWeaponLabelSample> Samples;if(IsValid(PresentationSwarm))PresentationSwarm->GetPresentationSamples(Samples);return Samples.Num();
}
int32 AInkMenuDirector::GetAlertSwordCount()const
{
    if(!IsValid(PresentationSwarm))return 0;
    int32 Count=0;
    for(const auto& Entry:PresentationSwarm->GetRoster().GetStates())
        if(PresentationSwarm->GetWeaponPhase(Entry.Profile.ParticipantId)==ELiveWeaponFlightPhase::Alert)++Count;
    return Count;
}
bool AInkMenuDirector::IsPresentationAlert()const{return IsValid(PresentationSwarm)&&PresentationSwarm->GetPresentationMode()==ELiveWeaponPresentationMode::Alert;}

bool AInkMenuDirector::IsGameplayPawnCandidate(const APawn* Pawn)const
{
    if(!IsValid(Pawn)||Pawn==GetPresentationPawn())return false;
    const UClass* MenuClass=PresentationPawnClass?PresentationPawnClass.Get():(IsValid(PresentationActor)?PresentationActor->GetClass():nullptr);
    return !MenuClass||!Pawn->IsA(MenuClass);
}

bool AInkMenuDirector::EnsureFrontend()
{
    if(IsValid(FrontendWidget))return true;
    auto* PC=LocalController.Get();
    if(!PC)return false;
    // Existing map instances may serialize the old WidgetClass. The rebuilt
    // runtime always starts from the independent tree, including native fallback.
    UClass* RuntimeClass=LoadClass<UInkFrontendWidget>(nullptr,
        TEXT("/Game/newUI/Runtime/WBP_NewUIFrontend.WBP_NewUIFrontend_C"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(!RuntimeClass||!RuntimeClass->IsChildOf(UInkRebuildFrontendWidget::StaticClass()))
        RuntimeClass=UInkRebuildFrontendWidget::StaticClass();
    WidgetClass=RuntimeClass;
    FrontendWidget=CreateWidget<UInkFrontendWidget>(PC,RuntimeClass);
    if(!FrontendWidget)return false;
    FrontendWidget->Configure(this);
    FrontendWidget->AddToViewport(200);
    UE_LOG(LogTemp,Display,TEXT("InkFrontend.Created widget=%s class=%s director=%s"),
        *FrontendWidget->GetPathName(),*FrontendWidget->GetClass()->GetPathName(),*GetPathName());
    return true;
}

void AInkMenuDirector::ConfigurePresentationSwarm()
{
    if(!PresentationSwarm)return;
    auto* Swarm=PresentationSwarm.Get();
    Swarm->IdleRadiusMin=PresentationIdleRadius.X;Swarm->IdleRadiusMax=PresentationIdleRadius.Y;
    Swarm->IdleHeightMin=PresentationIdleHeight.X;Swarm->IdleHeightMax=PresentationIdleHeight.Y;
    Swarm->AlertRadiusMin=PresentationAlertRadius.X;Swarm->AlertRadiusMax=PresentationAlertRadius.Y;
    Swarm->AlertHeightMin=PresentationAlertHeight.X;Swarm->AlertHeightMax=PresentationAlertHeight.Y;
    // Menu-only motion stays beside the robe, bounded by feet and head.
    Swarm->CharacterAvoidanceExtent=FVector(24,24,40);
    Swarm->bAvoidCharacterOnScreen=false;
    Swarm->PresentationPathScale=FMath::Clamp(PresentationPathScale,0.f,1.f);
    if(auto* Character=Cast<ACharacter>(GetPresentationPawn()))
    {
        const float HalfHeight=Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        Swarm->bConstrainPresentationHeight=true;
        Swarm->PresentationHeightRange=FVector2D(-HalfHeight+2,HalfHeight-5);
    }
    ULiveWeaponDefinition* Source=nullptr;
    for(ULiveWeaponDefinition* Definition:Swarm->Definitions)if(Definition&&Definition->WeaponType==ELiveWeaponType::Sword){Source=Definition;break;}
    auto* Definition=Source?DuplicateObject<ULiveWeaponDefinition>(Source,Swarm):NewObject<ULiveWeaponDefinition>(Swarm);
    Definition->SetFlags(RF_Transient);Definition->ClearFlags(RF_Public|RF_Standalone);
    Definition->WeaponType=ELiveWeaponType::Sword;
    if(PresentationSwordMesh)
    {
        Definition->Mesh=PresentationSwordMesh;
        Definition->Material=PresentationSwordMesh->GetMaterial(0);
        Swarm->PresentationMeshRotation=PresentationSwordRotation;
    }
    if(Definition->Mesh)
    {
        const FBoxSphereBounds Bounds=Definition->Mesh->GetBounds();
        const double LongestAxis=(Bounds.BoxExtent*2).GetMax();
        Definition->MeshScale=FVector(FMath::Max(1.f,PresentationSwordLength)/FMath::Max(1.,LongestAxis));
        Swarm->PresentationMeshCenter=Bounds.Origin;
    }
    Swarm->Definitions.Reset();Swarm->Definitions.Add(Definition);
}

bool AInkMenuDirector::TryEnterMenu()
{
    if(bMenuActive)return true;
    auto* PC=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
    if(!PC||!PC->IsLocalController())return false;
    LocalController=PC;
    if(!EnsurePresentationPawn()){PresentationStatus=TEXT("尚未指定菜单演示角色");return false;}
    auto* Pawn=GetPresentationPawn();
    if(!IsValid(Pawn)){PresentationStatus=TEXT("尚未指定菜单演示角色");return false;}
    if(!PresentationPawnClass)
    {
        PresentationPawnClass=Pawn->GetClass();PresentationSpawnTransform=Pawn->GetActorTransform();
        PresentationInstanceData=MakeUnique<FComponentInstanceDataCache>(Pawn);
    }
    auto* Cultivation=Pawn->FindComponentByClass<UCultivationGameplayComponent>();
    TInlineComponentArray<UCameraComponent*> Cameras(Pawn);
    UCameraComponent* Camera=nullptr;
    for(auto* Candidate:Cameras)if(Candidate->IsActive()){Camera=Candidate;break;}
    if(!Camera&&!Cameras.IsEmpty())Camera=Cameras[0];
    if(!Camera||!Cultivation||!SwarmClass){PresentationStatus=TEXT("演示角色缺少相机、修炼组件或剑灵配置");return false;}
    // Transfer an existing settings-page keyboard lease to the presentation component.
    // Otherwise returning via a submenu could overwrite its original Qi setting on exit.
    TArray<FQiInputState> TransferredQi;
    for(int32 I=BlockedQi.Num()-1;I>=0;--I)
    {
        if(auto* Qi=BlockedQi[I].Component.Get();Qi&&Qi->GetOwner()==Pawn)
        {
            Qi->bEnableKeyboard=BlockedQi[I].bKeyboardEnabled;
            TransferredQi.Add(BlockedQi[I]);BlockedQi.RemoveAt(I);
        }
    }
    if(!Cultivation->SetMenuPresentation(true))
    {
        for(const auto& Entry:TransferredQi){if(auto* Qi=Entry.Component.Get())Qi->bEnableKeyboard=false;BlockedQi.Add(Entry);}
        PresentationStatus=TEXT("演示修炼尚未就绪，请检查循环动画与骨架");return false;
    }
    PresentationCultivation=Cultivation;
    PreviousViewTarget=PC->GetViewTarget();
    bPreviousMouseCursor=PC->bShowMouseCursor;
    bPreviousAutoCamera=PC->bAutoManageActiveCameraTarget;
    bPreviousAspectConstraint=Camera->bConstrainAspectRatio;
    bCameraWasActive=Camera->IsActive();
    PresentationCamera=Camera;
    Camera->bConstrainAspectRatio=false;
    Camera->SetActive(true);
    PC->bAutoManageActiveCameraTarget=false;
    PC->SetViewTarget(Pawn);
    if(APawn* CurrentPlayer=PC->GetPawn();CurrentPlayer&&CurrentPlayer!=Pawn)
    {
        HiddenGameplayPawn=CurrentPlayer;bGameplayPawnWasHidden=CurrentPlayer->IsHidden();
        CurrentPlayer->SetActorHiddenInGame(true);
        if(auto* Character=Cast<ACharacter>(CurrentPlayer))
        {
            auto* Move=Character->GetCharacterMovement();FrozenMenuMovement=Move;
            MenuPreviousMovementMode=uint8(Move->MovementMode);MenuPreviousCustomMovementMode=Move->CustomMovementMode;
            Move->StopMovementImmediately();Move->DisableMovement();
        }
    }
    bMenuActive=true;
    SetFrontendInputBlocked(true);
    SuspendOverlays();
    const FTransform Transform(Pawn->GetActorRotation(),Pawn->GetActorLocation());
    PresentationSwarm=GetWorld()->SpawnActorDeferred<ALiveWeaponSwarm>(SwarmClass,Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(PresentationSwarm)
    {
        ConfigurePresentationSwarm();
        PresentationSwarm->SetPresentationMode(ELiveWeaponPresentationMode::Idle);
        PresentationSwarm->SetAnchor(Pawn);
        PresentationSwarm->bAutomaticAttacks=false;
        PresentationSwarm->bDrawAttackHitboxes=false;
        PresentationSwarm->SetPresentationMode(ELiveWeaponPresentationMode::Idle);
        PresentationSwarm->FinishSpawning(Transform);
        for(int32 I=0;I<10;++I)
            PresentationSwarm->AddParticipant({FString::Printf(TEXT("MenuPresentation_%02d"),I),TEXT(""),TEXT("")});
    }
    if(!PresentationSwarm||GetPresentationSwordCount()!=10||!EnsureFrontend())
    {
        LeaveMenu();PresentationStatus=TEXT("菜单演示初始化失败，将再次尝试");return false;
    }
    PresentationElapsedSeconds=0;PresentationStartedAt=FPlatformTime::Seconds();
    bPresentationReady=true;bEnterMenuRequested=false;
    PresentationStatus=TEXT("修炼演示就绪");
    if(auto* GameInstance=GetGameInstance())if(auto* Saves=GameInstance->GetSubsystem<UInkSaveSubsystem>())Saves->EndGameplay();
    FrontendWidget->ShowPage(FName(TEXT("Main")));
    return true;
}

void AInkMenuDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(bEnterMenuRequested&&!bMenuActive){TryEnterMenu();return;}
    if(!bMenuActive)
    {
        if(GameplayPawn.IsValid())
        {
            RememberSafeGameplayTransform();
            if(!bGameplaySystemsReady)EnsureGameplaySystems();
            TickGameplayEncounters(DeltaSeconds);
            GameplayPublishElapsed+=FMath::Max(0.f,DeltaSeconds);
            if(bPublishGameplayData&&GameplayPublishElapsed>=FMath::Max(.1f,GameplayDataPublishInterval)){GameplayPublishElapsed=0;PublishGameplayData();}
        }
        if(bFrontendInputBlocked)SuspendOverlays();return;
    }
    auto* PC=LocalController.Get();auto* Pawn=GetPresentationPawn();
    if(!PC||!IsValid(Pawn)||!IsValid(PresentationSwarm))
    {
        LeaveMenu();bEnterMenuRequested=true;return;
    }
    // Keep the exact component camera; do not author an offset or create another camera actor.
    if(PC->GetViewTarget()!=Pawn)PC->SetViewTarget(Pawn);
    SuspendOverlays();
    // Visible head can differ from the capsule; keep the complete sword below it.
    TInlineComponentArray<USkeletalMeshComponent*> Meshes(Pawn);
    for(auto* Body:Meshes)if(Body->GetName()==TEXT("Body")&&Body->DoesSocketExist(TEXT("head")))
    {
        PresentationSwarm->PresentationHeightRange.Y=Body->GetSocketLocation(TEXT("head")).Z-Pawn->GetActorLocation().Z+8;
        break;
    }
    // Presentation cadence is wall time, including on devices that miss their target frame rate.
    PresentationElapsedSeconds=FMath::Max(0.0,FPlatformTime::Seconds()-PresentationStartedAt);
    const double Interval=FMath::Max(.1,double(PresentationInterval));
    const bool bAlert=FMath::Fmod(double(PresentationElapsedSeconds),Interval*2)>=Interval;
    PresentationSwarm->SetPresentationMode(bAlert?ELiveWeaponPresentationMode::Alert:ELiveWeaponPresentationMode::Idle);
}

bool AInkMenuDirector::StartGame()
{
    auto* PC=LocalController.Get();
    if(!PC||!EnsureFrontend())return false;
    auto* Demo=GetPresentationPawn();
    APawn* Player=GameplayPawn.Get();
    // BP_TopDownCharacter2 (/Game/1/BP_TopDown) belongs only to the menu.
    // Keep the map's independent gameplay pawn; never adopt a presentation-class copy.
    if(!IsGameplayPawnCandidate(Player))Player=nullptr;
    if(!Player&&IsGameplayPawnCandidate(PC->GetPawn()))Player=PC->GetPawn();
    if(!Player)
    {
        auto* Mode=GetWorld()->GetAuthGameMode();if(!Mode||!PC->HasAuthority())return false;
        UClass* PawnClass=GameplayPawnClass?GameplayPawnClass.Get():Mode->GetDefaultPawnClassForController(PC);
        if(!PawnClass||(PresentationPawnClass&&PawnClass->IsChildOf(PresentationPawnClass.Get())))
        {PresentationStatus=TEXT("游戏角色配置不能使用菜单展示角色类。");return false;}
        FTransform Spawn;
        if(!GetSafeReentryTransform(Spawn))
        {PresentationStatus=TEXT("尚未找到游戏出生点，请检查PlayerStart。");return false;}
        Player=GetWorld()->SpawnActorDeferred<APawn>(PawnClass,Spawn,this,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
        if(!Player)return false;
        Player->AutoPossessPlayer=EAutoReceiveInput::Disabled;Player->FinishSpawning(Spawn);
    }
    bEnterMenuRequested=false;
    LeaveMenu();
    if(PC->GetPawn()!=Player)PC->Possess(Player);
    if(PC->GetPawn()!=Player)return false;
    GameplayPawn=Player;
    if(!RespawnGameplayPawn(Player))
    {
        const FString Failure=PresentationStatus;
        ReturnToMainMenu();PresentationStatus=Failure;
        return false;
    }
    PC->bAutoManageActiveCameraTarget=true;PC->SetViewTarget(Player);
    if(IsValid(Demo))Demo->Destroy();PresentationActor=nullptr;
    SetFrontendInputBlocked(false);
    FrontendWidget->ShowPage(FName(TEXT("HUD")));
    EnsureGameplaySystems();GameplayPublishElapsed=0;if(bPublishGameplayData)PublishGameplayData();
    PresentationStatus=TEXT("已进入游戏");
    if(auto* GameInstance=GetGameInstance())if(auto* Saves=GameInstance->GetSubsystem<UInkSaveSubsystem>())
    {
        if(!Saves->BeginGameplay(PC->GetPawn()))
        {
            PresentationStatus=Saves->LastError;
            FrontendWidget->SetFeedback(FText::FromString(Saves->LastError));
        }
    }
    return true;
}

void AInkMenuDirector::ReturnToMainMenu()
{
    SuspendGameplaySystems();
    bEnterMenuRequested=true;
    if(bMenuActive)
    {
        PresentationElapsedSeconds=0;PresentationStartedAt=FPlatformTime::Seconds();
        if(PresentationSwarm)PresentationSwarm->SetPresentationMode(ELiveWeaponPresentationMode::Idle);
        SetFrontendInputBlocked(true);
        if(EnsureFrontend())FrontendWidget->ShowPage(FName(TEXT("Main")));
        bEnterMenuRequested=false;return;
    }
    TryEnterMenu();
}

void AInkMenuDirector::LeaveMenu()
{
    if(IsValid(PresentationSwarm))PresentationSwarm->Destroy();PresentationSwarm=nullptr;
    if(auto* Cultivation=PresentationCultivation.Get())Cultivation->SetMenuPresentation(false);
    PresentationCultivation.Reset();
    if(auto* Camera=PresentationCamera.Get())
    {
        Camera->bConstrainAspectRatio=bPreviousAspectConstraint;
        Camera->SetActive(bCameraWasActive);
    }
    PresentationCamera.Reset();
    if(auto* Pawn=HiddenGameplayPawn.Get())Pawn->SetActorHiddenInGame(bGameplayPawnWasHidden);
    HiddenGameplayPawn.Reset();
    if(auto* Move=FrozenMenuMovement.Get())Move->SetMovementMode(EMovementMode(MenuPreviousMovementMode),MenuPreviousCustomMovementMode);
    FrozenMenuMovement.Reset();
    if(bMenuActive)
    {
        if(auto* PC=LocalController.Get())
        {
            PC->bAutoManageActiveCameraTarget=bPreviousAutoCamera;
            PC->SetViewTarget(PreviousViewTarget.IsValid()?PreviousViewTarget.Get():PC->GetPawn());
            PC->bShowMouseCursor=bPreviousMouseCursor;
        }
    }
    PreviousViewTarget.Reset();
    bMenuActive=false;bPresentationReady=false;PresentationElapsedSeconds=0;
    SetFrontendInputBlocked(false);
}

void AInkMenuDirector::SetFrontendInputBlocked(bool bBlocked)
{
    bFrontendInputBlocked=bMenuActive||bBlocked;
    ApplyInputBlock(bFrontendInputBlocked);
    if(bFrontendInputBlocked)SuspendOverlays();
    else if(!bMenuActive)RestoreOverlays();
}

void AInkMenuDirector::ApplyInputBlock(bool bBlocked)
{
    auto* PC=LocalController.Get();
    if(bInputBlockApplied==bBlocked||(bBlocked&&!PC))return;
    if(bBlocked)
    {
        for(TActorIterator<AActor> It(GetWorld());It;++It)
        {
            TInlineComponentArray<UCultivationGameplayComponent*> Actions(*It);
            for(auto* Action:Actions){BlockedActions.Add({Action,Action->bFrontendInputBlocked});Action->SetFrontendInputBlocked(true);}
            TInlineComponentArray<UQiCultivationComponent*> QiComponents(*It);
            for(auto* Qi:QiComponents)
            {
                // Its presentation component owns this keyboard lease and restores it on exit.
                if(bMenuActive&&PresentationCultivation.IsValid()&&Qi->GetOwner()==PresentationCultivation->GetOwner())continue;
                BlockedQi.Add({Qi,Qi->bEnableKeyboard});Qi->bEnableKeyboard=false;
            }
        }
        PC->StopMovement();PC->SetIgnoreMoveInput(true);PC->SetIgnoreLookInput(true);
        if(auto* Character=Cast<ACharacter>(PC->GetPawn()))Character->GetCharacterMovement()->StopMovementImmediately();
        PC->FlushPressedKeys();PC->bShowMouseCursor=true;
        FInputModeUIOnly Input;Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Input);
    }
    else
    {
        for(const auto& Entry:BlockedActions)if(auto* Action=Entry.Component.Get())Action->SetFrontendInputBlocked(Entry.bWasBlocked);
        for(const auto& Entry:BlockedQi)if(auto* Qi=Entry.Component.Get())Qi->bEnableKeyboard=Entry.bKeyboardEnabled;
        BlockedActions.Reset();BlockedQi.Reset();
        if(PC)
        {
            PC->SetIgnoreMoveInput(false);PC->SetIgnoreLookInput(false);PC->FlushPressedKeys();
            FInputModeGameAndUI Input;Input.SetHideCursorDuringCapture(false);Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Input);
        }
    }
    bInputBlockApplied=bBlocked;
}

void AInkMenuDirector::SuspendOverlays()
{
    for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)
    {
        auto* Overlay=*It;
        FOverlayState* Entry=SuspendedOverlays.FindByPredicate([Overlay](const FOverlayState& State){return State.Overlay.Get()==Overlay;});
        if(!Entry)
        {
            FOverlayState Saved;Saved.Overlay=Overlay;Saved.bTickEnabled=Overlay->IsActorTickEnabled();
            if(Overlay->Panel)Saved.PanelVisibility=Overlay->Panel->GetVisibility();
            Saved.Swarm=Overlay->Swarm;
            if(Overlay->Swarm){Saved.bSwarmHidden=Overlay->Swarm->IsHidden();Saved.bSwarmTickEnabled=Overlay->Swarm->IsActorTickEnabled();}
            SuspendedOverlays.Add(Saved);
        }
        Overlay->SetActorTickEnabled(false);
        if(Overlay->Panel)Overlay->Panel->SetVisibility(ESlateVisibility::Collapsed);
        if(Overlay->Swarm){Overlay->Swarm->SetActorHiddenInGame(true);Overlay->Swarm->SetActorTickEnabled(false);}
    }
}

void AInkMenuDirector::RestoreOverlays()
{
    for(const auto& Entry:SuspendedOverlays)
    {
        if(auto* Overlay=Entry.Overlay.Get())
        {
            Overlay->SetActorTickEnabled(Entry.bTickEnabled);
            if(Overlay->Panel)Overlay->Panel->SetVisibility(Entry.PanelVisibility);
        }
        if(auto* Swarm=Entry.Swarm.Get()){Swarm->SetActorHiddenInGame(Entry.bSwarmHidden);Swarm->SetActorTickEnabled(Entry.bSwarmTickEnabled);}
    }
    SuspendedOverlays.Reset();
}

void AInkMenuDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    bEnterMenuRequested=false;
    SuspendGameplaySystems();
    ClearGameplayEncounters();
    if(auto* Health=ObservedHealth.Get())Health->OnDied.RemoveDynamic(this,&ThisClass::HandleAnchorDied);ObservedHealth.Reset();
    if(auto* C=GameplayCultivation.Get()){C->SaveRecords();C->OnAnnouncement.RemoveDynamic(this,&ThisClass::HandleGameplayAnnouncement);}
    if(DirectorAudience)DirectorAudience->Unbind();
    if(bOwnsGameplaySwarm&&IsValid(GameplaySwarm))GameplaySwarm->Destroy();GameplaySwarm=nullptr;GameplayAudience=nullptr;
    for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)if(It->GetOwner()==this)It->Destroy();
    LeaveMenu();
    if(auto* GameInstance=GetGameInstance())if(auto* Saves=GameInstance->GetSubsystem<UInkSaveSubsystem>())Saves->EndGameplay();
    if(FrontendWidget){FrontendWidget->RemoveFromParent();FrontendWidget=nullptr;}
    Super::EndPlay(Reason);
}
