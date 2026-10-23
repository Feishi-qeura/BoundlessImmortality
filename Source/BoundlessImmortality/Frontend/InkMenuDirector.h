#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "InkMenuDirector.generated.h"

class APawn;
class APlayerController;
class UCameraComponent;
class UCharacterMovementComponent;
class UCultivationGameplayComponent;
class UQiCultivationComponent;
class ALiveWeaponSwarm;
class ALiveGMOverlay;
class UInkFrontendWidget;
class UStaticMesh;
class FComponentInstanceDataCache;
class ULiveAudienceAdapterComponent;
class UCultivationSubsystem;
class ALiveMonster;
class UCultivationHealthComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkGameplayAnnouncement,const FString&,Text);

/** Owns only the frontend presentation; the map's existing GameMode and player remain intact. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API AInkMenuDirector : public AActor
{
    GENERATED_BODY()
public:
    AInkMenuDirector();
    virtual ~AInkMenuDirector() override;
    static void AddReferencedObjects(UObject* Object,FReferenceCollector& Collector);

    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|场景") TObjectPtr<AActor> PresentationActor;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|场景") TSubclassOf<ALiveWeaponSwarm> SwarmClass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|界面") TSubclassOf<UInkFrontendWidget> WidgetClass;
    /** Empty uses the map GameMode's default pawn at PlayerStart. The presentation class is never playable. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|游戏") TSubclassOf<APawn> GameplayPawnClass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|场景",meta=(ClampMin="0.1",Units="s")) float PresentationInterval=10.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="演示剑模型")) TObjectPtr<UStaticMesh> PresentationSwordMesh;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="演示剑长度",ClampMin="1",Units="cm")) float PresentationSwordLength=55.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="模型朝向修正",ToolTip="将模型剑尖对齐本地正Z飞行方向。只影响菜单临时实例。")) FRotator PresentationSwordRotation=FRotator(0,0,180);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="待机半径范围",Units="cm")) FVector2D PresentationIdleRadius=FVector2D(40,58);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="待机高度范围",Units="cm")) FVector2D PresentationIdleHeight=FVector2D(-30,35);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="警戒半径范围",Units="cm")) FVector2D PresentationAlertRadius=FVector2D(42,58);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="警戒高度范围",Units="cm")) FVector2D PresentationAlertHeight=FVector2D(-20,30);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="主菜单|剑灵",meta=(DisplayName="闲游曲线幅度",ClampMin="0",ClampMax="1")) float PresentationPathScale=.2f;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") TObjectPtr<UInkFrontendWidget> FrontendWidget;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") TObjectPtr<ALiveWeaponSwarm> PresentationSwarm;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="游戏|直播") TObjectPtr<ALiveWeaponSwarm> GameplaySwarm;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="游戏|直播") TObjectPtr<ULiveAudienceAdapterComponent> GameplayAudience;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="游戏|直播") bool bGameplaySystemsReady=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|数据") bool bPublishGameplayData=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|数据",meta=(ClampMin="0.1",Units="s")) float GameplayDataPublishInterval=.5f;
    UPROPERTY(BlueprintAssignable,Category="游戏|直播") FInkGameplayAnnouncement OnGameplayAnnouncement;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇") bool bEnableGameplayEncounters=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇") TSubclassOf<ALiveMonster> EncounterMonsterClass;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="0",ClampMax="32")) int32 InitialEncounterCount=4;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="0",ClampMax="32")) int32 MaximumEncounterCount=8;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="1",Units="s")) float EncounterSpawnInterval=12;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="200",Units="cm")) float EncounterMinimumRadius=1000;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="200",Units="cm")) float EncounterMaximumRadius=1600;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="1")) float EncounterMonsterHealth=40;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="0")) float EncounterMonsterDamage=2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="游戏|遭遇",meta=(ClampMin="0",Units="cm/s")) float EncounterMonsterSpeed=55;
    UFUNCTION(BlueprintPure,Category="游戏|遭遇") int32 GetGameplayEncounterCount()const;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") bool bMenuActive=false;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") bool bPresentationReady=false;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") bool bFrontendInputBlocked=false;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") float PresentationElapsedSeconds=0;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="主菜单|状态") FString PresentationStatus;

    UFUNCTION(BlueprintCallable,Category="主菜单") bool StartGame();
    bool GetSafeReentryTransform(FTransform& Out)const;
    UFUNCTION(BlueprintCallable,Category="主菜单") void ReturnToMainMenu();
    UFUNCTION(BlueprintCallable,Category="主菜单") void SetFrontendInputBlocked(bool bBlocked);
    UFUNCTION(BlueprintPure,Category="主菜单") APawn* GetPresentationPawn()const;
    UFUNCTION(BlueprintPure,Category="主菜单") int32 GetPresentationSwordCount()const;
    UFUNCTION(BlueprintPure,Category="主菜单") int32 GetPresentationLabelCount()const;
    UFUNCTION(BlueprintPure,Category="主菜单") int32 GetAlertSwordCount()const;
    UFUNCTION(BlueprintPure,Category="主菜单") bool IsPresentationAlert()const;
    virtual void Tick(float DeltaSeconds)override;

protected:
    virtual void BeginPlay()override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;

private:
    friend class FInkMenuPawnRoleTest;
    struct FActionInputState
    {
        TWeakObjectPtr<UCultivationGameplayComponent> Component;
        bool bWasBlocked=false;
    };
    struct FQiInputState
    {
        TWeakObjectPtr<UQiCultivationComponent> Component;
        bool bKeyboardEnabled=false;
    };
    struct FOverlayState
    {
        TWeakObjectPtr<ALiveGMOverlay> Overlay;
        bool bTickEnabled=false;
        ESlateVisibility PanelVisibility=ESlateVisibility::Visible;
        TWeakObjectPtr<ALiveWeaponSwarm> Swarm;
        bool bSwarmHidden=false;
        bool bSwarmTickEnabled=false;
    };
    TArray<FActionInputState> BlockedActions;
    TArray<FQiInputState> BlockedQi;
    TArray<FOverlayState> SuspendedOverlays;
    TWeakObjectPtr<APlayerController> LocalController;
    TWeakObjectPtr<AActor> PreviousViewTarget;
    TWeakObjectPtr<APawn> GameplayPawn;
    UPROPERTY(Transient) TSubclassOf<APawn> PresentationPawnClass;
    FTransform PresentationSpawnTransform;
    FTransform LastSafeGameplayTransform;
    bool bHasSafeGameplayTransform=false;
    TWeakObjectPtr<UCharacterMovementComponent> FrozenMenuMovement;
    uint8 MenuPreviousMovementMode=0,MenuPreviousCustomMovementMode=0;
    TUniquePtr<FComponentInstanceDataCache> PresentationInstanceData;
    TWeakObjectPtr<APawn> HiddenGameplayPawn;
    TWeakObjectPtr<UCameraComponent> PresentationCamera;
    TWeakObjectPtr<UCultivationGameplayComponent> PresentationCultivation;
    bool bEnterMenuRequested=true;
    bool bInputBlockApplied=false;
    bool bPreviousMouseCursor=false;
    bool bPreviousAutoCamera=true;
    bool bPreviousAspectConstraint=false;
    bool bCameraWasActive=true;
    bool bGameplayPawnWasHidden=false;
    bool bOwnsGameplaySwarm=false,bGameplayAdapterBound=false,bGameplayAutomaticAttacks=true;
    float GameplayPublishElapsed=0;
    FString GameplaySessionKey;
    TWeakObjectPtr<UCharacterMovementComponent> WulingMovement;
    float WulingBaseWalkSpeed=0,WulingAppliedMoveFactor=1;
    TArray<TWeakObjectPtr<ALiveMonster>> GameplayEncounters;
    float EncounterElapsed=0;
    bool bEncounterWaveInitialized=false;
    void TickGameplayEncounters(float DeltaSeconds);
    void ClearGameplayEncounters();
    bool SpawnGameplayEncounter();
    UPROPERTY(Transient) TObjectPtr<ULiveAudienceAdapterComponent> DirectorAudience;
    TWeakObjectPtr<UCultivationSubsystem> GameplayCultivation;
    TWeakObjectPtr<UCultivationHealthComponent> ObservedHealth;
    bool bRespawnOnNextStart=false;
    bool RespawnGameplayPawn(APawn* Pawn);
    void RememberSafeGameplayTransform();
    UFUNCTION() void HandleAnchorDied();
    bool EnsureGameplaySystems();
    void SuspendGameplaySystems();
    void PublishGameplayData();
    UFUNCTION() void HandleGameplayAnnouncement(const FString& Text);
    double PresentationStartedAt=0;
    bool TryEnterMenu();
    bool EnsureFrontend();
    bool EnsurePresentationPawn();
    bool IsGameplayPawnCandidate(const APawn* Pawn)const;
    void ConfigurePresentationSwarm();
    void LeaveMenu();
    void ApplyInputBlock(bool bBlocked);
    void SuspendOverlays();
    void RestoreOverlays();
};
