#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputCoreTypes.h"
#include "InkCultivationEditorLibrary.generated.h"

class UNiagaraSystem;
class UMaterialInterface;
class AStaticMeshActor;
class UNiagaraComponent;
class APlayerController;
class UBlueprint;

/** Asset authoring only; no dependency from the runtime game module. */
UCLASS()
class BOUNDLESSIMMORTALITYEDITOR_API UInkCultivationEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static UNiagaraSystem* CreateSignInChestSystem(const FString& PackagePath,UMaterialInterface* Material);
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static UNiagaraSystem* CreateTitleSystem(const FString& PackagePath,UMaterialInterface* Material);
    /** CPU NiagaraUI motion. Kinds 0..3: title effects; 4: character slot borders; 5: dantian. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static UNiagaraSystem* CreateTitleParticleSystem(const FString& PackagePath,UMaterialInterface* Material,int32 Kind);
    UFUNCTION(BlueprintCallable, Category="Qi Cultivation|Editor")
    static void SetLevelPreviewRealtime(bool Enabled);
    UFUNCTION(BlueprintCallable, Category="Qi Cultivation|Editor")
    static AActor* GetBlueprintPreviewActor(UBlueprint* Blueprint);
    UFUNCTION(BlueprintCallable, Category="Qi Cultivation|Editor")
    static bool UpgradeQiSystem(UNiagaraSystem* System, UMaterialInterface* LightningMaterial);
    UFUNCTION(BlueprintCallable, Category="Qi Cultivation|Editor")
    static bool UpgradeQiAbsorption(UNiagaraSystem* System);
    UFUNCTION(BlueprintCallable, Category="Qi Cultivation|Editor")
    static void InjectQiToggleKey(APlayerController* Controller, bool Pressed);
    UFUNCTION(BlueprintCallable, Category="Cultivation Gameplay|Editor")
    static void InjectGameplayKey(APlayerController* Controller, FKey Key, bool Pressed);
    UFUNCTION(BlueprintCallable, Category="Qi Cultivation|Editor")
    static UNiagaraSystem* CreateQiSystem(const FString& PackagePath, const TArray<UMaterialInterface*>& Materials);
    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static UNiagaraSystem* CreateInkSystem(const FString& PackagePath, UMaterialInterface* Ring, UMaterialInterface* Mist, UMaterialInterface* Flecks);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static FString InspectSystem(UNiagaraSystem* System);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static bool SetEmitterInput(UNiagaraSystem* System, int32 EmitterIndex, const FString& ModuleName, const FString& InputName, const FString& Value, bool bStaticSwitch=false);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static bool CompileSystem(UNiagaraSystem* System);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static bool AddInkMotion(UNiagaraSystem* System);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static AActor* CreateDecorBatch(const TArray<AStaticMeshActor*>& Sources, const FString& Label, bool bDecorationOnly);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static TArray<AActor*> GetProtectedActors(UWorld* World);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static FString InspectComponent(UNiagaraComponent* Component);

    UFUNCTION(BlueprintCallable, Category="Ink Cultivation|Editor")
    static bool ConfigureInkUserControls(UNiagaraSystem* System);
};
