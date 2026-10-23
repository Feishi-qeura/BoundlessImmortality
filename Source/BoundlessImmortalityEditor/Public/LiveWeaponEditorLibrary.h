#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LiveWeaponEditorLibrary.generated.h"
class UStaticMesh;
class UFont;
class APlayerController;
class ALiveGMOverlay;
class ULiveInteractionSubsystem;
class UCultivationSubsystem;
class UCultivationPrototypeClient;
class UWulingPrototypeSubsystem;
class ALiveMonster;
class APawn;
UCLASS()
class BOUNDLESSIMMORTALITYEDITOR_API ULiveWeaponEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static UWulingPrototypeSubsystem* GetWulingState(UObject* Context);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static UCultivationSubsystem* GetCultivationState(UObject* Context);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static UCultivationPrototypeClient* GetPrototypeState(UObject* Context);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static ULiveInteractionSubsystem* GetLiveInteractionState(UObject* Context);
    /** Creates a transient PIE fixture with health initialized before BeginPlay. */
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static ALiveMonster* SpawnTestMonster(UObject* Context,FVector Location,float MaxHealth,float AttackDamage,float MoveSpeed,APawn* FollowTarget);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static UStaticMesh* CreatePlaceholderMesh(const FString& PackagePath,bool bTriangle);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static UFont* CreateNameFont();
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static bool ClickHUDAt(APlayerController* Controller,FVector2D Position);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static void CaptureGameFrame(const FString& Filename,bool bShowUI=false);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static bool ClickGMButton(ALiveGMOverlay* Overlay,FName Action);
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static void FinishAssetCompilation();
    /** Exercises the normal session-validated presence delivery, only in explicit local simulation. */
    UFUNCTION(BlueprintCallable,Category="Live Weapons|Editor") static bool SimulatePresence(ULiveInteractionSubsystem* Host,const FString& UserId,const FString& Nickname,int32 EnterType=1);
};
