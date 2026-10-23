#include "LiveWeaponCombatModel.h"
float FLiveWeaponCombatModel::AttackFrequencyScale(const FString& Id,double Now)
{
    const uint32 Seed=GetTypeHash(Id);
    const float Bias=(float(Seed&65535)/65535.f-.5f)*.2f;
    const float Phase=float((Seed>>16)&65535)/65535.f*2*PI;
    const float Frequency=.35f+float((Seed>>8)&255)/255.f*.4f;
    return FMath::Clamp(1.f+Bias+.1f*FMath::Sin(float(Now)*Frequency+Phase),.8f,1.2f);
}
void FLiveWeaponCombatModel::Advance(double Now,bool bHasTarget,TArray<FLiveAttackRequest>& Out,
    TFunction<bool(const FLiveWeaponState&)> CanAttack,TFunction<bool(const FLiveWeaponState&,FName)> CanUseSkill)
{
    if(!FMath::IsFinite(Now))return;
    const int32 InitialCount=Roster.Num();
    for(int32 Index=0;Index<InitialCount&&Index<Roster.Num();++Index)
    {
        auto& State=Roster.GetStates()[Index];
        if(!State.bParticipating)continue;
        const float Configured=AttackIntervals.Contains(State.WeaponType)?AttackIntervals[State.WeaponType]:AttackInterval;
        const double BaseInterval=FMath::IsFinite(Configured)?FMath::Max(1.2,double(Configured)):1.2;
        // Frequency scales reciprocally to the interval; sample again for each next attack.
        const double Interval=BaseInterval/(bDynamicAttackFrequency?AttackFrequencyScale(State.Profile.ParticipantId,Now):1.f);
        if(!State.bClockInitialized)
        {
            // Irrational stride spreads consecutive slots over the interval without reshuffling on joins.
            State.NextAttackTime=Now+FMath::Frac((State.Handle.Index+1)*.6180339887498949)*Interval;
            State.bClockInitialized=true;
        }
        if(Now+1.e-6<State.NextAttackTime)continue;
        State.NextAttackTime=Now+Interval;
        if(!bHasTarget)continue;
        // Blueprint eligibility callbacks may join/reset/switch participants. Only ready
        // participants are copied, avoiding both dangling array references and per-frame crowd copies.
        const FLiveWeaponState Snapshot=State;
        const auto Current=[&](){return Roster.GetStates().IsValidIndex(Index)&&Roster.GetStates()[Index].Handle==Snapshot.Handle&&Roster.GetStates()[Index].WeaponRevision==Snapshot.WeaponRevision;};
        if((CanAttack&&!CanAttack(Snapshot))||!Current())continue;
        FLiveAttackRequest Request{Snapshot.Handle,Snapshot.WeaponType,NAME_None,Snapshot.WeaponRevision};
        TArray<const FLiveSkillRule*,TInlineAllocator<8>> Candidates;
        float Total=0;
        for(const auto& Rule:SkillRules)
        {
            if(Rule.WeaponType!=Snapshot.WeaponType || !Snapshot.UnlockedSkills.Contains(Rule.SkillId) ||
                Snapshot.SkillReadyAt.FindRef(Rule.SkillId)>Now || !FMath::IsFinite(Rule.Weight) || Rule.Weight<=0)continue;
            if(CanUseSkill)
            {
                const bool Eligible=CanUseSkill(Snapshot,Rule.SkillId);
                if(!Current())break;
                if(!Eligible)continue;
            }
            Candidates.Add(&Rule);Total+=Rule.Weight;
        }
        if(!Current())continue;
        if(Total>0)
        {
            float Choice=Random.FRand()*Total;
            for(const auto* Rule:Candidates)
            {
                Choice-=Rule->Weight;
                if(Choice<=0){if(Random.FRand()<FMath::Clamp(Rule->TriggerChance,0.f,1.f))Request.SkillId=Rule->SkillId;break;}
            }
        }
        if(Current())Out.Add(Request);
    }
}
void FLiveWeaponCombatModel::CommitSkill(FLiveWeaponHandle Participant,FName SkillId,double Now)
{
    if(!Roster.GetStates().IsValidIndex(Participant.Index))return;auto& State=Roster.GetStates()[Participant.Index];if(State.Handle!=Participant)return;
    for(const auto& Rule:SkillRules)if(Rule.SkillId==SkillId&&Rule.WeaponType==State.WeaponType){State.SkillReadyAt.Add(SkillId,Now+FMath::Max(0.f,Rule.Cooldown));return;}
}
bool FLiveWeaponCombatModel::GrantSkill(const FString& Id,FName SkillId)
{
    auto* State=Roster.Find(Id);if(!State || SkillId.IsNone())return false;
    State->UnlockedSkills.Add(SkillId);return true;
}
