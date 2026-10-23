#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveWeaponDefinition.h"
#include "LiveWeapons/LiveMonster.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAlertRegressionTest,"BoundlessImmortality.LiveWeapons.Alert.Regression",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveAlertRegressionTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;
    auto* Swarm=W.World->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
    auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);Def->FollowUpChance=0;Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);
    auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);
    for(int I=0;I<20;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    float MinHold=100,MaxHold=0;
    for(int I=0;I<20;++I){const float H=Swarm->GetIdleHoldRemaining(FString::FromInt(I));MinHold=FMath::Min(MinHold,H);MaxHold=FMath::Max(MaxHold,H);}
    TestTrue(TEXT("default idle holds vary within 1 to 5 seconds"),MinHold>=1&&MinHold<3&&MaxHold<=5);
    // Old screen-space protection considered even distant points along the camera ray inside.
    TestFalse(TEXT("avoidance ends outside the configured local XYZ volume"),Swarm->IsInsideCharacterReadabilityArea(FRotationMatrix(FRotator(-35,-45,0)).GetUnitAxis(EAxis::X)*1000));
    Swarm->ResetParticipants();Swarm->JoinParticipant({TEXT("single"),TEXT("viewer"),TEXT("")});
    auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);Enemy->Health=100000;
    int64 PreviousLaunch=0;double LastLaunch=-100,ShortestGap=100;double MinimumZ=10000;bool Returned=false;int MissingReturn=0;
    for(int Frame=0;Frame<1800;++Frame)
    {
        Swarm->Tick(1.f/60);const auto P=Swarm->GetWeaponPosition(TEXT("single"));MinimumZ=FMath::Min(MinimumZ,P.Z);
        if(PreviousLaunch>0&&P.Size2D()<320&&P.Z>100)Returned=true;
        if(Swarm->TotalPrimaryLaunched>PreviousLaunch)
        {
            const double Now=Frame/60.;if(PreviousLaunch>0){ShortestGap=FMath::Min(ShortestGap,Now-LastLaunch);if(!Returned)++MissingReturn;}
            LastLaunch=Now;PreviousLaunch=Swarm->TotalPrimaryLaunched;Returned=false;
        }
    }
    TestTrue(TEXT("combat still lands real hits"),Swarm->TotalAttacks>=2);
    TestTrue(TEXT("attack and post-hit drift stay above ground"),MinimumZ>=25);
    TestTrue(TEXT("each new sortie leaves enough time for return and alert"),ShortestGap>=3);
    TestEqual(TEXT("every repeat sortie first returns near the owner"),MissingReturn,0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveAlertOrbitTest,"BoundlessImmortality.LiveWeapons.Alert.OrbitAndParameters",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveAlertOrbitTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);
    Swarm->AlertRadiusMin=290;Swarm->AlertRadiusMax=330;Swarm->AlertHeightMin=168;Swarm->AlertHeightMax=192;Swarm->AlertOrbitSpeed=60;
    Swarm->CharacterAvoidanceExtent=FVector(100,200,250);
    TestTrue(TEXT("XYZ avoidance honors separate Y extent"),Swarm->IsInsideCharacterReadabilityArea(FVector(0,190,0)));
    TestFalse(TEXT("XYZ avoidance honors separate X extent"),Swarm->IsInsideCharacterReadabilityArea(FVector(110,0,0)));
    TestFalse(TEXT("XYZ avoidance honors separate Z extent"),Swarm->IsInsideCharacterReadabilityArea(FVector(0,0,260)));
    for(const TCHAR* Name:{TEXT("CharacterAvoidanceExtent"),TEXT("AlertDetectionRange"),TEXT("AlertRadiusMin"),TEXT("AlertRadiusMax"),TEXT("AlertHeightMin"),TEXT("AlertHeightMax"),TEXT("AlertOrbitSpeed"),TEXT("PostReturnAlertMin"),TEXT("PostReturnAlertMax")})
    {
        const FProperty* P=Swarm->GetClass()->FindPropertyByName(Name);
        TestTrue(FString(Name)+TEXT(" editable and Blueprint writable"),P&&P->HasAllPropertyFlags(CPF_Edit|CPF_BlueprintVisible)&&!P->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
    }
    for(int I=0;I<12;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(1200,0,45),FRotator::ZeroRotator);Enemy->Health=10000;
    for(int I=0;I<300;++I)Swarm->Tick(1.f/60);
    const FVector Before=Swarm->GetWeaponPosition(TEXT("0"));
    double MinRadius=10000,MaxRadius=0,MinZ=10000,MaxZ=-10000;bool AllAlert=true;
    for(int Frame=0;Frame<120;++Frame)
    {
        Swarm->Tick(1.f/60);
        for(int I=0;I<12;++I)
        {
            const FString Id=FString::FromInt(I);const FVector P=Swarm->GetWeaponPosition(Id);
            MinRadius=FMath::Min(MinRadius,P.Size2D());MaxRadius=FMath::Max(MaxRadius,P.Size2D());MinZ=FMath::Min(MinZ,P.Z);MaxZ=FMath::Max(MaxZ,P.Z);
            AllAlert&=Swarm->GetWeaponPhase(Id)==ELiveWeaponFlightPhase::Alert;
        }
    }
    TestTrue(TEXT("enemy outside attack range triggers alert"),AllAlert);
    TestEqual(TEXT("alert never attacks an out-of-range enemy"),Swarm->TotalPrimaryLaunched,int64(0));
    TestTrue(TEXT("spirits circle the owner"),FVector::Dist(Before,Swarm->GetWeaponPosition(TEXT("0")))>200);
    TestTrue(TEXT("orbit respects full radial width"),MinRadius>=287&&MaxRadius<=332);
    TestTrue(TEXT("orbit respects full vertical thickness"),MinZ>=167&&MaxZ<=193);
    Swarm->AlertRadiusMin=Swarm->AlertRadiusMax=220;Swarm->AlertHeightMin=Swarm->AlertHeightMax=160;
    for(int I=0;I<180;++I)Swarm->Tick(1.f/60);
    TestTrue(TEXT("runtime Blueprint parameter changes reshape ring"),FMath::Abs(Swarm->GetWeaponPosition(TEXT("0")).Size2D()-220)<3);
    Enemy->Destroy();for(int I=0;I<600;++I)Swarm->Tick(1.f/60);
    TestTrue(TEXT("clearing enemies exits alert"),Swarm->GetWeaponPhase(TEXT("0"))!=ELiveWeaponFlightPhase::Alert);
    Swarm->ResetParticipants();Swarm->AlertRadiusMin=Swarm->AlertRadiusMax=1000;Swarm->AlertOrbitSpeed=75;Swarm->JoinParticipant({TEXT("large-ring"),TEXT("viewer"),TEXT("")});
    auto* Near=W.World->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);Near->Health=10000;
    for(int I=0;I<1800;++I)Swarm->Tick(1.f/60);
    TestTrue(TEXT("large Blueprint orbit still recovers and repeats attacks"),Swarm->TotalPrimaryLaunched>=3);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveCurveHeightTest,"BoundlessImmortality.LiveWeapons.Alert.PureHeightCurve",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveCurveHeightTest::RunTest(const FString&)
{
    auto Path=FLiveBezierFlight::BuildSpatial(FVector(0,0,200),FVector(600,0,50),FVector(1000,0,-2000),FVector(1000,0,-1800),.8f,FVector(0,0,-150),FVector(0,0,300));
    Path.KeepAboveLinearFloor(100,0,40);
    double MinimumMargin=10000;
    for(int I=0;I<=1000;++I)
    {
        const float T=I/1000.f;MinimumMargin=FMath::Min(MinimumMargin,Path.Position(Path.Duration*T).Z-FMath::Lerp(100.f,0.f,T));
    }
    TestTrue(TEXT("entire mathematical curve stays above the endpoint foot baseline, including downward initial velocity and weave"),MinimumMargin>=39.99);
    FLiveWeaponTestWorld W;
    auto* Swarm=W.World->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
    auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);Def->FollowUpChance=0;Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);
    auto* Owner=W.World->SpawnActor<APawn>();auto* Root=NewObject<USceneComponent>(Owner);Owner->SetRootComponent(Root);Root->RegisterComponent();Owner->SetActorLocation(FVector(0,0,100));Swarm->SetAnchor(Owner);
    Swarm->JoinParticipant({TEXT("s"),TEXT("s"),TEXT("")});auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(600,0,145),FRotator::ZeroRotator);Enemy->Health=10000;
    double MinTip=10000;
    for(int I=0;I<1200;++I){Swarm->Tick(1.f/60);MinTip=FMath::Min(MinTip,Swarm->GetWeaponPosition(TEXT("s")).Z-41);}
    TestTrue(TEXT("elevated actor foot references protect sword height without any scene geometry"),MinTip>=100);
    TestTrue(TEXT("analytical height curve still recovers and repeats impacts"),Swarm->TotalAttacks>=2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveRunningReturnTest,"BoundlessImmortality.LiveWeapons.Alert.RunningReturn",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveRunningReturnTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* Swarm=W.World->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
    auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);Def->FollowUpChance=0;Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);
    auto* Owner=W.World->SpawnActor<APawn>();auto* Root=NewObject<USceneComponent>(Owner);Owner->SetRootComponent(Root);Root->RegisterComponent();Swarm->SetAnchor(Owner);
    Swarm->JoinParticipant({TEXT("runner"),TEXT("runner"),TEXT("")});auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);Enemy->Health=10000;
    for(int I=0;I<1800;++I)
    {
        Owner->SetActorLocation(FVector(I*350.f/60,0,0));Enemy->SetActorLocation(Owner->GetActorLocation()+FVector(600,0,45));Swarm->Tick(1.f/60);
    }
    TestTrue(TEXT("owner really ran over 10,000 cm"),Owner->GetActorLocation().X>10000);
    TestTrue(FString::Printf(TEXT("moving owner must allow subsequent sorties, observed %lld"),Swarm->TotalPrimaryLaunched),Swarm->TotalPrimaryLaunched>=3);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSceneIndependentFlightTest,"BoundlessImmortality.LiveWeapons.Alert.NoSceneInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSceneIndependentFlightTest::RunTest(const FString&)
{
    TArray<FVector> Baseline;double MaxDifference=0;
    for(int Scenario=0;Scenario<2;++Scenario)
    {
        FLiveWeaponTestWorld W;
        if(Scenario==1)
        {
            auto* Obstacle=W.World->SpawnActor<AStaticMeshActor>(FVector(300,0,150),FRotator::ZeroRotator);
            auto* Mesh=Obstacle->GetStaticMeshComponent();Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            Obstacle->SetActorScale3D(FVector(25,25,1));Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetCollisionObjectType(ECC_WorldStatic);
        }
        auto* Swarm=W.World->SpawnActorDeferred<ALiveWeaponSwarm>(ALiveWeaponSwarm::StaticClass(),FTransform::Identity);
        auto* Def=NewObject<ULiveWeaponDefinition>(Swarm);Def->FollowUpChance=0;Swarm->Definitions={Def};Swarm->FinishSpawning(FTransform::Identity);
        auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);Swarm->JoinParticipant({TEXT("ghost"),TEXT("ghost"),TEXT("")});
        auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);Enemy->Health=10000;
        for(int Frame=0;Frame<900;++Frame)
        {
            Swarm->Tick(1.f/60);const FVector P=Swarm->GetWeaponPosition(TEXT("ghost"));
            if(Scenario==0)Baseline.Add(P);else MaxDifference=FMath::Max(MaxDifference,FVector::Dist(P,Baseline[Frame]));
        }
        for(auto Mesh:Swarm->WeaponMeshes)TestEqual(TEXT("spirit mesh never has physics or query collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
    }
    TestTrue(FString::Printf(TEXT("collidable scene geometry must not change the flight at all (difference=%f)"),MaxDifference),MaxDifference<.01);
    return true;
}
#endif
