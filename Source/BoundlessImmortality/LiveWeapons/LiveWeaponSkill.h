#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LiveWeaponSkill.generated.h"
class ALiveMonster;
class APawn;
USTRUCT(BlueprintType)
struct FLiveSkillContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString ParticipantId;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<APawn> Anchor=nullptr;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ALiveMonster> Target=nullptr;
    UPROPERTY(BlueprintReadOnly) float BaseDamage=1;
};
UCLASS(Blueprintable,BlueprintType)
class BOUNDLESSIMMORTALITY_API ULiveWeaponSkill : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent) bool CanExecute(const FLiveSkillContext& Context)const;
    UFUNCTION(BlueprintNativeEvent) bool Execute(const FLiveSkillContext& Context);
};
