#pragma once
#include "LiveWeaponRoster.h"

struct FLiveSkillRule
{
    FName SkillId;
    ELiveWeaponType WeaponType=ELiveWeaponType::Sword;
    float TriggerChance=0.2f;
    float Weight=1;
    float Cooldown=3;
};
struct FLiveAttackRequest
{
    FLiveWeaponHandle Participant;
    ELiveWeaponType WeaponType=ELiveWeaponType::Sword;
    FName SkillId;
    uint32 WeaponRevision=0;
};
class BOUNDLESSIMMORTALITY_API FLiveWeaponCombatModel
{
public:
    explicit FLiveWeaponCombatModel(FLiveWeaponRoster& InRoster):Roster(InRoster),Random(7319){}
    void Advance(double Now,bool bHasTarget,TArray<FLiveAttackRequest>& OutRequests,
        TFunction<bool(const FLiveWeaponState&)> CanAttack={},TFunction<bool(const FLiveWeaponState&,FName)> CanUseSkill={});
    void CommitSkill(FLiveWeaponHandle Participant,FName SkillId,double Now);
    bool GrantSkill(const FString& Id,FName SkillId);
    void SetSeed(int32 Seed){Random.Initialize(Seed);}
    TArray<FLiveSkillRule> SkillRules;
    float AttackInterval=1.2f;
    TMap<ELiveWeaponType,float> AttackIntervals;
    bool bDynamicAttackFrequency=false;
    static float AttackFrequencyScale(const FString& ParticipantId,double Now);
private:
    FLiveWeaponRoster& Roster;
    FRandomStream Random;
};
