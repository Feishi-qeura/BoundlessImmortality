#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeapons/LiveWeaponCombatModel.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveCombatTest,"BoundlessImmortality.LiveWeapons.CombatClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveCombatTest::RunTest(const FString&)
{
    FLiveWeaponRoster Roster;Roster.Join({TEXT("a"),TEXT("a"),TEXT("")});
    FLiveWeaponCombatModel Model(Roster);TArray<FLiveAttackRequest> Out;
    Model.Advance(0,true,Out);
    Model.Advance(1.2,true,Out);
    TestEqual(TEXT("one attack at first interval"),Out.Num(),1);
    const double Ready=Roster.Find(TEXT("a"))->NextAttackTime;
    Roster.SwitchWeapon(TEXT("a"),ELiveWeaponType::Bow);
    TestEqual(TEXT("switch keeps clock"),Roster.Find(TEXT("a"))->NextAttackTime,Ready);
    Out.Reset();Model.Advance(1.3,true,Out);TestEqual(TEXT("not before cooldown"),Out.Num(),0);
    Model.Advance(2.4,true,Out);TestEqual(TEXT("second interval"),Out.Num(),1);
    Out.Reset();Model.Advance(20,true,Out);TestEqual(TEXT("long frame does not burst"),Out.Num(),1);
    Out.Reset();Model.Advance(30,false,Out);TestEqual(TEXT("no target no attack"),Out.Num(),0);
    Model.Advance(30,true,Out);TestTrue(TEXT("range entry at most one"),Out.Num()<=1);
    Out.Reset();Model.Advance(30,true,Out);TestEqual(TEXT("same game time no replay"),Out.Num(),0);
    FLiveWeaponRoster Crowd;for(int I=0;I<1000;++I)Crowd.Join({FString::FromInt(I),TEXT("v"),TEXT("")});
    FLiveWeaponCombatModel CrowdModel(Crowd);Out.Reset();CrowdModel.Advance(0,true,Out);
    TSet<int32> Buckets;for(const auto& State:Crowd.GetStates()){TestTrue(TEXT("phase within 1.2s"),State.NextAttackTime>=0&&State.NextAttackTime<=1.2);Buckets.Add(int32(State.NextAttackTime*100));}
    TestTrue(TEXT("attacks staggered"),Buckets.Num()>50);
    FLiveWeaponRoster Fast;Fast.Join({TEXT("fast"),TEXT("v"),TEXT("")});Fast.GetStates()[0].bClockInitialized=true;
    FLiveWeaponCombatModel Guarded(Fast);Guarded.AttackIntervals.Add(ELiveWeaponType::Sword,.05f);Out.Reset();Guarded.Advance(0,true,Out);Out.Reset();Guarded.Advance(.1,true,Out);
    TestEqual(TEXT("configured base interval cannot bypass 1.2 second minimum"),Out.Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSkillsTest,"BoundlessImmortality.LiveWeapons.Skills",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSkillsTest::RunTest(const FString&)
{
    FLiveWeaponRoster R;R.Join({TEXT("a"),TEXT("a"),TEXT("")});FLiveWeaponCombatModel M(R);
    FLiveSkillRule Rule;Rule.SkillId=TEXT("Soul1");Rule.TriggerChance=1;Rule.Weight=1;Rule.Cooldown=100;
    M.SkillRules.Add(Rule);Rule.SkillId=TEXT("Soul2");M.SkillRules.Add(Rule);Rule.SkillId=TEXT("Soul3");M.SkillRules.Add(Rule);
    TArray<FLiveAttackRequest> Out;M.Advance(0,true,Out);Out.Reset();M.Advance(1.2,true,Out);
    TestTrue(TEXT("locked skill stays basic"),Out.Num()==1&&Out[0].SkillId.IsNone());
    M.GrantSkill(TEXT("a"),TEXT("Soul1"));M.GrantSkill(TEXT("a"),TEXT("Soul2"));M.GrantSkill(TEXT("a"),TEXT("Soul3"));
    for(int I=2;I<=4;++I){Out.Reset();M.Advance(I*1.2,true,Out);TestTrue(TEXT("one available skill replaces attack"),Out.Num()==1&&!Out[0].SkillId.IsNone());if(!Out.IsEmpty())M.CommitSkill(Out[0].Participant,Out[0].SkillId,I*1.2);}
    Out.Reset();M.Advance(6,true,Out);TestTrue(TEXT("all cooling down falls back"),Out.Num()==1&&Out[0].SkillId.IsNone());
    for(auto& S:M.SkillRules)S.TriggerChance=0;
    Out.Reset();M.Advance(200,true,Out);TestTrue(TEXT("zero probability basic"),Out.Num()==1&&Out[0].SkillId.IsNone());
    TestFalse(TEXT("grant unknown viewer rejected"),M.GrantSkill(TEXT("missing"),TEXT("Soul1")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSkillCommitTest,"BoundlessImmortality.LiveWeapons.SkillCommitAfterExecution",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSkillCommitTest::RunTest(const FString&)
{
    FLiveWeaponRoster R;R.Join({TEXT("a"),TEXT("a"),TEXT("")});FLiveWeaponCombatModel M(R);
    FLiveSkillRule Rule;Rule.SkillId=TEXT("skill");Rule.TriggerChance=1;M.SkillRules.Add(Rule);M.GrantSkill(TEXT("a"),TEXT("skill"));
    TArray<FLiveAttackRequest> Out;M.Advance(0,true,Out);M.Advance(1.2,true,Out);
    TestTrue(TEXT("skill selected"),Out.Num()==1&&!Out[0].SkillId.IsNone());
    TestFalse(TEXT("selection cannot consume an unexecuted skill cooldown"),R.Find(TEXT("a"))->SkillReadyAt.Contains(TEXT("skill")));
    M.CommitSkill(Out[0].Participant,Out[0].SkillId,1.2);TestTrue(TEXT("successful execution commits cooldown"),R.Find(TEXT("a"))->SkillReadyAt.Contains(TEXT("skill")));
    Rule.SkillId=TEXT("eligible");M.SkillRules.Add(Rule);M.GrantSkill(TEXT("a"),TEXT("eligible"));Out.Reset();
    M.Advance(5,true,Out,{},[](const FLiveWeaponState&,FName Id){return Id==TEXT("eligible");});
    TestTrue(TEXT("ineligible skill cannot steal eligible selection"),Out.Num()==1&&Out[0].SkillId==TEXT("eligible"));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveEligibilityMutationTest,"BoundlessImmortality.LiveWeapons.EligibilityMutation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveEligibilityMutationTest::RunTest(const FString&)
{
    FLiveWeaponRoster R;R.Join({TEXT("a"),TEXT("v"),TEXT("")});FLiveWeaponCombatModel M(R);
    for(FName Id:{FName(TEXT("first")),FName(TEXT("second"))}){FLiveSkillRule Rule;Rule.SkillId=Id;Rule.TriggerChance=1;M.SkillRules.Add(Rule);M.GrantSkill(TEXT("a"),Id);}
    TArray<FLiveAttackRequest> Out;M.Advance(0,true,Out);int Calls=0;
    M.Advance(1.2,true,Out,{},[&](const FLiveWeaponState&,FName){++Calls;R.Reset();return true;});
    TestEqual(TEXT("reset aborts eligibility immediately"),Calls,1);TestEqual(TEXT("cancelled eligibility queues nothing"),Out.Num(),0);
    return true;
}
#endif
