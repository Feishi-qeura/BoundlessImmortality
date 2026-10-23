#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LiveAnchorController.generated.h"
class ACameraActor;
class ULiveAnchorAbilityComponent;
UCLASS()
class BOUNDLESSIMMORTALITY_API ALiveAnchorController : public APlayerController
{
    GENERATED_BODY()
public:
    ALiveAnchorController();
    UFUNCTION(Exec) void LiveAudience(int32 Count);
    UFUNCTION(Exec) void LiveComment(const FString& Id,const FString& Text);
    UFUNCTION(Exec) void LiveSpawnMonsters(int32 Count);
    UFUNCTION(Exec) void LiveSkill(const FString& Id,FName Skill);
    UFUNCTION(BlueprintCallable) bool ExecuteGMAction(FName Action);
    virtual void PlayerTick(float DeltaSeconds)override;
    bool IsUsingGameplayCamera()const;
protected:
    virtual void OnPossess(APawn* InPawn)override;
private:
    UPROPERTY(Transient) TObjectPtr<ACameraActor> LiveCamera;
    UPROPERTY(Transient) TObjectPtr<ULiveAnchorAbilityComponent> Ability;
};
