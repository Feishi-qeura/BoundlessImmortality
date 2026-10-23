#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveMonster.h"
#include "LiveWeapons/LiveAnchorAbilityComponent.h"
#include "LiveWeapons/LiveWeaponDefinition.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "LiveImpactRegressionSkill.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveRejoinCapacityTest,"BoundlessImmortality.LiveWeapons.RejoinCapacity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveRejoinCapacityTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Anchor=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);Swarm->bAutomaticAttacks=false;
    for(int32 I=0;I<1000;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    TestTrue(TEXT("leave deactivates stable identity"),Swarm->RemoveParticipant(TEXT("0")));
    TestTrue(TEXT("new viewer fills released capacity"),Swarm->JoinParticipant({TEXT("new"),TEXT("new"),TEXT("")}).IsValid());
    TestFalse(TEXT("old stable slot cannot bypass current capacity"),Swarm->JoinParticipant({TEXT("0"),TEXT("return"),TEXT("")}).IsValid());
    TestEqual(TEXT("active viewer count remains bounded"),Swarm->GetParticipantCount(),1000);
    Swarm->RemoveParticipant(TEXT("1"));TestTrue(TEXT("return succeeds after another leave"),Swarm->JoinParticipant({TEXT("0"),TEXT("return"),TEXT("")}).IsValid());
    for(int32 I=0;I<3;++I)Swarm->Tick(1.f/60);TestEqual(TEXT("inactive slots do not stall advancement"),Swarm->GetParticipantCount(),1000);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSmoothPresentationTest,"BoundlessImmortality.LiveWeapons.Refinement.FrameSmoothness",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSmoothPresentationTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);
    Swarm->JoinParticipant({TEXT("first"),TEXT("viewer"),TEXT("")});Swarm->JoinParticipant({TEXT("smooth"),TEXT("viewer"),TEXT("")});Swarm->bAutomaticAttacks=false;
    double LargestError=0,LargestMotion=0;const FVector Origin=Swarm->GetWeaponPosition(TEXT("smooth"));
    for(int I=0;I<600;++I)
    {
        Swarm->Tick(1.f/120);FTransform Presented;Swarm->WeaponMeshes[0]->GetInstanceTransform(1,Presented,true);
        LargestError=FMath::Max(LargestError,FVector::Dist(Presented.GetLocation(),Swarm->GetWeaponPosition(TEXT("smooth"))));
        LargestMotion=FMath::Max(LargestMotion,FVector::Dist(Origin,Swarm->GetWeaponPosition(TEXT("smooth"))));
    }
    TestTrue(TEXT("the sampled spirit really moves"),LargestMotion>1);
    TestTrue(TEXT("every rendered frame uses the current flight position, including 120 Hz"),LargestError<.01);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveIdleDwellTest,"BoundlessImmortality.LiveWeapons.Refinement.IdleDwell",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveIdleDwellTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);Swarm->bAutomaticAttacks=false;
    for(int I=0;I<12;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    TArray<FVector> Start;for(int I=0;I<12;++I)Start.Add(Swarm->GetWeaponPosition(FString::FromInt(I)));
    double LargestMove=0;
    for(int Frame=0;Frame<54;++Frame){Swarm->Tick(1.f/60);for(int I=0;I<12;++I)LargestMove=FMath::Max(LargestMove,FVector::Dist(Start[I],Swarm->GetWeaponPosition(FString::FromInt(I))));}
    TestTrue(TEXT("idle spirits hold their cached position for at least 1 second, with only gentle breathing"),LargestMove<12);
    for(int Frame=0;Frame<600;++Frame)Swarm->Tick(1.f/60);
    int Changed=0;for(int I=0;I<12;++I)Changed+=FVector::Dist(Start[I],Swarm->GetWeaponPosition(FString::FromInt(I)))>25?1:0;
    TestTrue(TEXT("cached idle positions eventually refresh without waiting for enemies"),Changed>=10);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveIdleEvacuationTest,"BoundlessImmortality.LiveWeapons.Refinement.OwnerApproach",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveIdleEvacuationTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);Swarm->bAutomaticAttacks=false;
    auto* Root=NewObject<USceneComponent>(Owner);Owner->SetRootComponent(Root);Root->RegisterComponent();
    for(int I=0;I<1000;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    const FVector Right=FRotationMatrix(FRotator(-35,-45,0)).GetUnitAxis(EAxis::Y);TestTrue(TEXT("test owner really moves"),Owner->SetActorLocation(Right*149));Swarm->Tick(1.f/60);
    int SleepingInSilhouette=0;for(int I=0;I<1000;++I){const FString Id=FString::FromInt(I);if(Swarm->IsInsideCharacterReadabilityArea(Swarm->GetWeaponPosition(Id))&&Swarm->GetIdleHoldRemaining(Id)>0)++SleepingInSilhouette;}
    TestEqual(TEXT("approaching owner interrupts cached hovering before the general follow threshold"),SleepingInSilhouette,0);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveDepthShadowTest,"BoundlessImmortality.LiveWeapons.Refinement.DepthAndShadow",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveDepthShadowTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);
    double Low=MAX_dbl,High=-MAX_dbl;int32 ObscuringOwner=0;const FRotationMatrix View(FRotator(-35,-45,0));
    for(int I=0;I<1000;++I)
    {
        const FString Id=FString::FromInt(I);Swarm->JoinParticipant({Id,TEXT("viewer"),TEXT("")});const FVector P=Swarm->GetWeaponPosition(Id);
        Low=FMath::Min(Low,P.Z);High=FMath::Max(High,P.Z);
        if(Swarm->IsInsideCharacterReadabilityArea(P))++ObscuringOwner;
    }
    TestTrue(TEXT("1000 spirits have more than 200 cm of height separation on arrival"),High-Low>200);
    TestEqual(TEXT("1000 idle spirits leave the owner's configured XYZ volume clear"),ObscuringOwner,0);
    for(auto Mesh:Swarm->WeaponMeshes)TestTrue(TEXT("all four weapon batches cast shadows"),Mesh->CastShadow);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveImpactCallbackTest,"BoundlessImmortality.LiveWeapons.ImpactCallbacks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveImpactCallbackTest::RunTest(const FString&)
{
    for(int Behavior=0;Behavior<5;++Behavior)
    {
        FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
        auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);Def->FollowUpChance=0;
        FLiveSkillEntry Skill;Skill.SkillId=TEXT("impact-test");Skill.TriggerChance=1;Skill.Cooldown=100;Skill.SkillClass=ULiveImpactRegressionSkill::StaticClass();Def->Skills.Add(Skill);
        Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);auto* Anchor=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);
        Swarm->JoinParticipant({TEXT("owner"),TEXT("x"),TEXT("")});Swarm->GrantSkill(TEXT("owner"),Skill.SkillId);
        auto* Target=W.World->SpawnActor<ALiveMonster>(FVector(500,0,45),FRotator::ZeroRotator);Target->Health=1000;
        ULiveImpactRegressionSkill::Behavior=Behavior;ULiveImpactRegressionSkill::Executions=0;
        for(int I=0;I<120&&Swarm->TotalPrimaryLaunched==0;++I)Swarm->Tick(1.f/60);Swarm->bAutomaticAttacks=false;
        TestEqual(TEXT("skill not executed at launch"),ULiveImpactRegressionSkill::Executions,0);
        for(int I=0;I<240&&ULiveImpactRegressionSkill::Executions==0;++I)Swarm->Tick(1.f/60);
        TestEqual(TEXT("skill executed once at impact"),ULiveImpactRegressionSkill::Executions,1);
        TestEqual(TEXT("success replaces basic; false falls back; cancellation suppresses fallback"),Target->Health,Behavior>=3?1000.f:(Behavior==1?999.f:993.f));
        if(Behavior<3){const auto* S=Swarm->GetRoster().Find(TEXT("owner"));TestEqual(TEXT("only success commits skill cooldown"),S&&S->SkillReadyAt.Contains(Skill.SkillId),Behavior!=1);}
        if(Behavior==2)TestEqual(TEXT("callback additions retained without dangling references"),Swarm->GetParticipantCount(),257);
    }
    ULiveImpactRegressionSkill::Behavior=0;return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveMovingFlightTest,"BoundlessImmortality.LiveWeapons.MovingTargetFlight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveMovingFlightTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Anchor=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);
    Swarm->JoinParticipant({TEXT("moving-target"),TEXT("x"),TEXT("")});auto* M=W.World->SpawnActor<ALiveMonster>(FVector(650,0,45),FRotator::ZeroRotator);M->FollowTarget=Anchor;M->Health=1000;
    const FVector Start=M->GetActorLocation();for(int I=0;I<240;++I){M->Tick(1.f/60);Swarm->Tick(1.f/60);}
    TestTrue(TEXT("target actually moved"),FVector::Dist(Start,M->GetActorLocation())>100);
    TestTrue(TEXT("moving target is hit by real swept flight"),M->Health<1000&&Swarm->TotalAttacks>0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveLivingFlightTest,"BoundlessImmortality.LiveWeapons.LivingFlight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveLivingFlightTest::RunTest(const FString&)
{
    for(const auto Type:{ELiveWeaponType::Sword,ELiveWeaponType::Spear,ELiveWeaponType::Axe,ELiveWeaponType::Bow})
    {
        FLiveWeaponTestWorld TestWorld;auto* W=TestWorld.World;
        auto* Swarm=W->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
        auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);Def->WeaponType=Type;Def->FollowUpChance=1;Def->ChainSearchRadius=1200;Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);
        auto* Anchor=W->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);Swarm->JoinParticipant({TEXT("spirit"),TEXT("v"),TEXT("")});Swarm->SwitchParticipantWeapon(TEXT("spirit"),Type);
        TArray<ALiveMonster*> Enemies;
        for(int I=0;I<4;++I){auto* M=W->SpawnActor<ALiveMonster>(FVector(300+I*130,(I%2)*180,45),FRotator::ZeroRotator);M->MaxHealth=M->Health=1000;Enemies.Add(M);}
        for(int I=0;I<120&&Swarm->TotalPrimaryLaunched==0;++I)Swarm->Tick(1.f/60);
        TestEqual(TEXT("one primary launched"),Swarm->TotalPrimaryLaunched,int64(1));
        TestEqual(TEXT("launch does not apply remote instant damage"),Enemies[0]->Health,1000.f);
        Swarm->bAutomaticAttacks=false;
        TArray<FLiveWeaponLabelSample> Before;Swarm->GetPresentationSamples(Before);
        // Flight duration grows to satisfy acceleration/speed limits, then each spirit
        // advances at its own changing tempo. Await the launched sortie rather than
        // assuming all four physical impacts must fit a fixed eight-second window.
        // Retain that minimum observation period for the separate idle-motion check.
        for(int I=0;I<1800&&(I<480||Swarm->HasActiveAttacks());++I)Swarm->Tick(1.f/60);
        TestFalse(TEXT("launched sortie finishes within thirty seconds"),Swarm->HasActiveAttacks());
        const int64 Expected=Type==ELiveWeaponType::Sword||Type==ELiveWeaponType::Spear?4:1;
        TestEqual(TEXT("only swords/spears chain and at most four hits"),Swarm->TotalAttacks,Expected);
        TestEqual(TEXT("extra hits are counted separately"),Swarm->TotalFollowUpHits,Expected-1);
        TArray<FLiveWeaponLabelSample> After;Swarm->GetPresentationSamples(After);
        TestTrue(TEXT("all weapons including bow keep moving"),FVector::Dist(Before[0].Position,After[0].Position)>5);
        const FVector Previous=After[0].Position;Anchor->SetActorLocation(FVector(2000,0,0));Swarm->Tick(.001f);Swarm->GetPresentationSamples(After);
        TestTrue(TEXT("owner teleport cannot drag a spirit"),FVector::Dist(Previous,After[0].Position)<10);
        for(auto Mesh:Swarm->WeaponMeshes)TestEqual(TEXT("weapon meshes have no collisions"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
        Swarm->ResetParticipants();TestEqual(TEXT("reset also clears flying arrows"),Swarm->GetActiveArrowCount(),0);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveQueuedSwitchTest,"BoundlessImmortality.LiveWeapons.QueuedAttackSwitch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveQueuedSwitchTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
    auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);FLiveSkillEntry Skill;Skill.SkillId=TEXT("queue-test");Skill.TriggerChance=1;Skill.SkillClass=ULiveImpactRegressionSkill::StaticClass();Def->Skills.Add(Skill);Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);
    auto* Anchor=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);
    for(const TCHAR* Id:{TEXT("a"),TEXT("b")}){Swarm->JoinParticipant({Id,Id,TEXT("")});Swarm->GrantSkill(Id,Skill.SkillId);}
    W.World->SpawnActor<ALiveMonster>(FVector(500,0,45),FRotator::ZeroRotator);
    ULiveImpactRegressionSkill::Behavior=5;Swarm->Tick(.01f);Swarm->Tick(1.3f);
    TestEqual(TEXT("queued sword attack cancelled when another eligibility callback switches it"),Swarm->TotalPrimaryLaunched,int64(1));
    TestEqual(TEXT("old sword skill cannot launch as a bow arrow"),Swarm->GetActiveArrowCount(),0);
    ULiveImpactRegressionSkill::Behavior=0;return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSwarmTest,"BoundlessImmortality.LiveWeapons.Swarm",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSwarmTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld TestWorld;auto* W=TestWorld.World;
    auto* Swarm=W->SpawnActor<ALiveWeaponSwarm>();
    auto* Anchor=W->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);
    for(int32 I=0;I<1000;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    TestEqual(TEXT("1000 participants"),Swarm->GetParticipantCount(),1000);
    TestEqual(TEXT("1000 rendered weapons"),Swarm->GetRenderedCount(),1000);
    for(int32 I=0;I<1000;++I)Swarm->SwitchParticipantWeapon(FString::FromInt(I),ELiveWeaponType(I%4));
    TestEqual(TEXT("switches preserve rendered count"),Swarm->GetRenderedCount(),1000);
    Swarm->Tick(.016f);TArray<FLiveWeaponLabelSample> Samples;Swarm->GetPresentationSamples(Samples);
    TestEqual(TEXT("all participants have positions"),Samples.Num(),1000);
    for(const auto& Sample:Samples)TestFalse(TEXT("finite formation"),Sample.Position.ContainsNaN());
    const int64 Attacks=Swarm->TotalAttacks;Swarm->Tick(1.3f);TestEqual(TEXT("no target no damage"),Swarm->TotalAttacks,Attacks);
    auto* Near=W->SpawnActor<ALiveMonster>(FVector(250,0,45),FRotator::ZeroRotator);
    auto* Far=W->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);
    Near->MaxHealth=Near->Health=100000;Far->MaxHealth=Far->Health=100000;
    for(int I=0;I<240;++I)Swarm->Tick(1.f/60);TestTrue(TEXT("targets damaged after actual flight"),Near->Health<100000||Far->Health<100000);
    Near->Destroy();const float PreviousFar=Far->Health;for(int I=0;I<240;++I)Swarm->Tick(1.f/60);TestTrue(TEXT("reselect after destruction"),Far->Health<PreviousFar);
    Swarm->ResetParticipants();TestEqual(TEXT("reset clears rendered weapons"),Swarm->GetRenderedCount(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveFlightCancellationTest,"BoundlessImmortality.LiveWeapons.FlightCancellation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveFlightCancellationTest::RunTest(const FString&)
{
    for(int Scenario=0;Scenario<3;++Scenario)
    {
        FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Anchor=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);
        Swarm->JoinParticipant({TEXT("a"),TEXT("a"),TEXT("")});if(Scenario==2)Swarm->SwitchParticipantWeapon(TEXT("a"),ELiveWeaponType::Bow);
        auto* Target=W.World->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);Target->Health=1000;
        for(int I=0;I<120&&Swarm->TotalPrimaryLaunched==0;++I)Swarm->Tick(1.f/60);Swarm->bAutomaticAttacks=false;
        if(Scenario==0)Target->Destroy();
        if(Scenario==1)Swarm->SwitchParticipantWeapon(TEXT("a"),ELiveWeaponType::Axe);
        if(Scenario==2)Swarm->ResetParticipants();
        for(int I=0;I<240;++I)Swarm->Tick(1.f/60);
        TestEqual(TEXT("cancelled flight cannot commit stale damage"),Swarm->TotalAttacks,int64(0));
        if(Scenario>0)TestEqual(TEXT("untouched target retains HP"),Target->Health,1000.f);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLivePushTest,"BoundlessImmortality.LiveWeapons.Anchor",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLivePushTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld TestWorld;auto* W=TestWorld.World;
    auto* A=W->SpawnActor<APawn>();auto* Ability=NewObject<ULiveAnchorAbilityComponent>(A);Ability->RegisterComponent();
    auto* M=W->SpawnActor<ALiveMonster>(FVector(200,0,45),FRotator::ZeroRotator);
    auto* Far=W->SpawnActor<ALiveMonster>(FVector(2000,0,45),FRotator::ZeroRotator);
    const float Health=M->Health;TestTrue(TEXT("first push accepted"),Ability->TryPush());TestFalse(TEXT("cooldown rejects repeat"),Ability->TryPush());
    M->Tick(.1f);Far->Tick(.1f);TestTrue(TEXT("near pushed"),M->GetActorLocation().X>200);TestEqual(TEXT("far not pushed"),Far->GetActorLocation().X,2000.0);TestEqual(TEXT("push no damage"),M->Health,Health);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveMixedRangeSkillTest,"BoundlessImmortality.LiveWeapons.MixedRangeSkillEligibility",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveMixedRangeSkillTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld TestWorld;auto* W=TestWorld.World;
    auto* Swarm=W->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
    auto* Sword=NewObject<ULiveWeaponDefinition>(Swarm);Sword->AttackRange=100;FLiveSkillEntry Skill;Skill.SkillId=TEXT("soul");Skill.TriggerChance=1;Sword->Skills.Add(Skill);
    auto* Bow=NewObject<ULiveWeaponDefinition>(Swarm);Bow->WeaponType=ELiveWeaponType::Bow;Bow->AttackRange=800;Swarm->Definitions={Sword,Bow};Swarm->FinishSpawning(FTransform::Identity);
    auto* Anchor=W->SpawnActor<APawn>();Swarm->SetAnchor(Anchor);Swarm->JoinParticipant({TEXT("sword"),TEXT("s"),TEXT("")});Swarm->GrantSkill(TEXT("sword"),TEXT("soul"));
    Swarm->JoinParticipant({TEXT("bow"),TEXT("b"),TEXT("")});Swarm->SwitchParticipantWeapon(TEXT("bow"),ELiveWeaponType::Bow);
    auto* M=W->SpawnActor<ALiveMonster>(FVector(500,0,45),FRotator::ZeroRotator);M->Health=10000;
    Swarm->Tick(.01f);Swarm->Tick(1.3f);
    TestEqual(TEXT("only ranged weapon launches"),Swarm->TotalPrimaryLaunched,int64(1));
    TestFalse(TEXT("out-of-range sword does not spend skill cooldown"),Swarm->GetRoster().Find(TEXT("sword"))->SkillReadyAt.Contains(TEXT("soul")));
    return true;
}
#endif
