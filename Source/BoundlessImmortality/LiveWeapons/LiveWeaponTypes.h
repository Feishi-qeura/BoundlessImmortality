#pragma once
#include "CoreMinimal.h"
#include "LiveWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class ELiveWeaponType : uint8 { Sword, Axe, Spear, Bow };

USTRUCT(BlueprintType)
struct FLiveParticipantProfile
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ParticipantId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString AvatarUrl;
    FLiveParticipantProfile()=default;
    FLiveParticipantProfile(FString Id,FString Name,FString Url):ParticipantId(MoveTemp(Id)),DisplayName(MoveTemp(Name)),AvatarUrl(MoveTemp(Url)){}
};

struct FLiveWeaponHandle
{
    int32 Index=INDEX_NONE;
    uint32 Generation=0;
    bool IsValid()const{return Index!=INDEX_NONE;}
    bool operator==(const FLiveWeaponHandle& Other)const{return Index==Other.Index && Generation==Other.Generation;}
    bool operator!=(const FLiveWeaponHandle& Other)const{return !(*this==Other);}
};

struct FLiveWeaponState
{
    FLiveParticipantProfile Profile;
    FLiveWeaponHandle Handle;
    ELiveWeaponType WeaponType=ELiveWeaponType::Sword;
    uint32 WeaponRevision=0;
    bool bParticipating=true;
    bool bPersonalSword=false;
    bool bClockInitialized=false;
    double NextAttackTime=0;
    double LastInteractionTime=0;
    TSet<FName> UnlockedSkills;
    TMap<FName,double> SkillReadyAt;
};

struct FLiveWeaponLabelSample
{
    FString ParticipantId,DisplayName,AvatarUrl;
    FVector Position=FVector::ZeroVector;
    double LastInteractionTime=0;
};
