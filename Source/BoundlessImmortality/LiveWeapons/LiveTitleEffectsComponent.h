#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiveTitleEffectsComponent.generated.h"
class UNiagaraSystem;
class UNiagaraComponent;
class APlayerController;
class UInkNiagaraNameplateWidget;

UCLASS(ClassGroup=(Presentation),meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API ULiveTitleEffectsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULiveTitleEffectsComponent();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Titles") bool bEnabled=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Titles",meta=(ClampMin="0",ClampMax="128")) int32 MaximumEffects=24;
    /** Retained for saved asset compatibility. The old world-material system cannot render in UI. */
    UPROPERTY(meta=(DeprecatedProperty,DeprecationMessage="World TitleSystem is superseded by the shared NiagaraUIRenderer title systems; configure TitleColor and Intensity instead.")) TSoftObjectPtr<UNiagaraSystem> TitleSystem;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Titles") FLinearColor TitleColor=FLinearColor(1,.67f,.18f,1);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Titles",meta=(ClampMin="0",ClampMax="4")) float Intensity=.65f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Titles") bool bOccludeWithScene=false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Titles") int32 ActiveEffectCount=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Titles") int32 ReclaimedEffectCount=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Titles") int32 ActiveSystemCount=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Presentation|Titles") TArray<FString> ActiveParticipantIds;
    UFUNCTION(BlueprintPure,Category="Presentation|Diagnostics") TArray<UNiagaraComponent*> GetActiveEffects()const;
    UFUNCTION(BlueprintPure,Category="Presentation|Diagnostics") TArray<UInkNiagaraNameplateWidget*> GetActiveNameplates()const;
    static bool IsPresentationBlocked(UWorld* World,bool bIncludeGM=true);
    static bool IsAnchorVisible(APlayerController* PC,const FVector& Position,bool bOcclude);
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UInkNiagaraNameplateWidget>> Effects;
    void Retire(const FString& Id);
};
