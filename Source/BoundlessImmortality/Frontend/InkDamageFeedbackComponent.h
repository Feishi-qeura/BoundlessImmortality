#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraModifier.h"
#include "InkDamageFeedbackComponent.generated.h"
class UCultivationHealthComponent;

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkHitShakeSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hit") float RotationAmplitude=.25f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hit") float LocationAmplitude=.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hit") float Frequency=19.f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Hit") float Duration=.22f;
};

UCLASS()
class BOUNDLESSIMMORTALITY_API UInkDamageCameraModifier : public UCameraModifier
{
    GENERATED_BODY()
public:
    void Pulse(const FInkHitShakeSettings& Settings,float Scale);
    virtual bool ModifyCamera(float DeltaTime,FMinimalViewInfo& POV)override;
    void Stop(){Remaining=0;}
private:
    FInkHitShakeSettings Hit;
    float Remaining=0,Age=0,Strength=1;
};

UCLASS(ClassGroup=(Presentation),meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API UInkDamageFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UInkDamageFeedbackComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Damage") bool bCameraFeedback=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Damage",meta=(ClampMin="0",ClampMax="2")) float Strength=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Damage") TArray<FInkHitShakeSettings> Tiers;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Damage") int32 LastDamageTier=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Damage") double LastDamageRatio=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Damage") int32 DamageEventCount=0;
    void Observe(UCultivationHealthComponent* Health);
    UFUNCTION(BlueprintCallable,Category="Presentation|Damage") void PreviewDamageRatio(double Ratio);
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    TWeakObjectPtr<UCultivationHealthComponent> Observed;
    UPROPERTY(Transient) TObjectPtr<UInkDamageCameraModifier> Modifier;
    UFUNCTION() void OnDamage(double Applied,double Maximum);
};
