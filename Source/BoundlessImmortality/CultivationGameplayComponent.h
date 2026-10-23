#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "CultivationGameplayComponent.generated.h"

class ACharacter;
class APlayerController;
class UAnimSequence;
class UAnimationAsset;
class UInputComponent;
class UQiCultivationComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UCultivationGameplayComponent;

/** Local screen caption aligned to the ink effect. It never receives input. */
UCLASS(Transient)
class BOUNDLESSIMMORTALITY_API UCultivationInkCaptionWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(UCultivationGameplayComponent* Component);
protected:
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const override;
private:
    TWeakObjectPtr<UCultivationGameplayComponent> Source;
    UPROPERTY(Transient) TObjectPtr<UFont> CaptionFont;
};

UENUM(BlueprintType)
enum class ECultivationActionState : uint8
{
    Idle, MeditationStart, MeditationLoop, MeditationEnd, ChargeStart, ChargeLoop, ChargeRelease
};

/** Local player controls for the Sky Temple animation set. Damage/replication are separate systems. */
UCLASS(ClassGroup=(Cultivation), meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API UCultivationGameplayComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCultivationGameplayComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|输入") FName EnabledMapName=TEXT("Level_Sky_Temple_Demo_01");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|输入", meta=(ClampMin="0.05",ClampMax="2.0",Units="s")) float HoldToggleSeconds=.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|动画") TObjectPtr<UAnimSequence> MeditationStart;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|动画") TObjectPtr<UAnimSequence> MeditationLoop;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|动画") TObjectPtr<UAnimSequence> MeditationEnd;
    /** Existing Niagara component on the character; activated on entry and deactivated on exit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|粒子", meta=(DisplayName="修炼粒子组件名")) FName InkEffectComponentName=TEXT("InkCultivation");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|水墨过渡", meta=(ClampMin="0.6",ClampMax="2",Units="s")) float InkTransitionSeconds=.9f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|水墨过渡") TSoftObjectPtr<UMaterialInterface> InkTransitionMaterial;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="修炼|状态") float InkCoverage=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|水墨过渡") FText MeditationEntryCaption=NSLOCTEXT("Cultivation","EntryCaption","修炼开始");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|水墨过渡") FText MeditationExitCaption=NSLOCTEXT("Cultivation","ExitCaption","修炼结束");
    UFUNCTION(BlueprintPure, Category="修炼|水墨过渡") FText GetInkCaptionText()const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|动画") TObjectPtr<UAnimSequence> ChargeStart;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|动画") TObjectPtr<UAnimSequence> ChargeLoop;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|动画") TObjectPtr<UAnimSequence> ChargeRelease;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="修炼|状态") ECultivationActionState State=ECultivationActionState::Idle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="修炼|状态") float StateElapsed=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="修炼|状态") FVector LastMoveDirection=FVector::ZeroVector;
    UFUNCTION(BlueprintCallable, Category="修炼|输入") bool RequestCultivationToggle();
    UFUNCTION(BlueprintCallable, Category="修炼|输入") bool SetAttackHeld(bool Held);
    /** Death/gameplay interruption: release movement and effects without a queued toggle. */
    UFUNCTION(BlueprintCallable,Category="修炼|状态") void InterruptAction();
    /** Plays the existing meditation loop on a menu actor without possessing it or polling keys. */
    UFUNCTION(BlueprintCallable, Category="修炼|菜单演示") bool SetMenuPresentation(bool bEnabled);
    UFUNCTION(BlueprintPure, Category="修炼|菜单演示") bool IsMenuPresentation()const{return bMenuPresentation;}
    UFUNCTION(BlueprintCallable, Category="修炼|输入") void SetFrontendInputBlocked(bool bBlocked);
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly,Category="修炼|输入") bool bFrontendInputBlocked=false;
    UFUNCTION(BlueprintPure, Category="修炼|状态") bool IsMovementLocked() const { return State!=ECultivationActionState::Idle; }
    UFUNCTION(BlueprintPure, Category="修炼|状态") UAnimSequence* GetCurrentAnimation() const;
protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Driver;
    UPROPERTY(Transient) TObjectPtr<UInputComponent> InputBlocker;
    UPROPERTY(Transient) TObjectPtr<UQiCultivationComponent> Qi;
    UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> InkEffect;
    UPROPERTY(Transient) TObjectPtr<UClass> PreviousAnimClass;
    UPROPERTY(Transient) TObjectPtr<UAnimationAsset> PreviousSingleAnimation;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> InkCover;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> InkMaterial;
    UPROPERTY(Transient) TObjectPtr<UCultivationInkCaptionWidget> InkCaption;
    TWeakObjectPtr<APlayerController> BoundController;
    EAnimationMode::Type PreviousAnimationMode=EAnimationMode::AnimationBlueprint;
    EMovementMode PreviousMovementMode=MOVE_Walking;
    uint8 PreviousCustomMovementMode=0;
    bool bActiveMap=false;
    bool bPreviousQiKeyboard=false;
    bool bMovementLease=false;
    bool bAttackHeld=false;
    bool bPreviousMouseDown=false;
    bool bToggleLatched=false;
    bool bExitMeditationRequested=false;
    bool bMeditationPoseSwitched=false;
    bool bMenuPresentation=false;
    bool bTickBeforeMenuPresentation=true;
    EVisibilityBasedAnimTickOption AnimationTickBeforeMenuPresentation=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    float ToggleHeldSeconds=0;
    void BindController(APlayerController* Controller);
    void UnbindController();
    void PollInput(APlayerController* Controller,float DeltaTime);
    void AdvanceAction(float DeltaTime);
    void EnterState(ECultivationActionState NewState);
    void FinishAction();
    bool CanStart(bool Meditation) const;
    void BeginAction();
    bool EnsureInkCover();
    void UpdateInkCover();
    void ClearInkCover();
    void RestoreAnimation();
    void SetInkEffectPlaying(bool bPlaying);
    void ResetInkEffect();
};
