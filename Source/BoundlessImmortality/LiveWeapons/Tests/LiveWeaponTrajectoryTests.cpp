#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeapons/LiveWeaponTrajectory.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSpatialBezierTest,"BoundlessImmortality.LiveWeapons.Refinement.SpatialBezier",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveSpatialBezierTest::RunTest(const FString&)
{
    const FVector Start(0,0,170),End(650,50,45),Velocity(130,20,0),Exit(1700,100,0);
    const auto P=FLiveBezierFlight::BuildSpatial(Start,End,Velocity,Exit,.5f,FVector(0,0,200),FVector(0,100,0));
    TestTrue(TEXT("3D curve retains starting position and velocity"),P.Position(0).Equals(Start,.001)&&P.Velocity(0).Equals(Velocity,.01));
    TestTrue(TEXT("3D curve retains exact impact and inertia"),P.Position(P.Duration).Equals(End,.001)&&P.Velocity(P.Duration).Equals(Exit,.01));
    TestTrue(TEXT("arched center rises above both endpoints"),P.Position(P.Duration*.5f).Z>Start.Z+50);
    const FVector A=P.Position(P.Duration*.25f)-Start,B=P.Position(P.Duration*.5f)-Start,C=P.Position(P.Duration*.75f)-Start;
    TestTrue(TEXT("flight is spatial rather than a tilted flat arc"),FMath::Abs(FVector::DotProduct(FVector::CrossProduct(A,B),C))>10000);
    for(int I=0;I<=128;++I){const float T=P.Duration*I/128;TestTrue(TEXT("spatial acceleration bounded"),P.Acceleration(T).Size()<12001);TestTrue(TEXT("spatial speed bounded"),P.Velocity(T).Size()<2604);}
    const FBox2D Body(FVector2D(100,100),FVector2D(200,300));
    const FVector2D Plate=ALiveWeaponSwarm::KeepNameplateOutside(FVector2D(140,180),FVector2D(90,17),Body);
    TestFalse(TEXT("name and avatar remain present without covering the character"),FBox2D(Plate,Plate+FVector2D(90,17)).Intersect(Body));
    const FBox2D Character(FVector2D(120,300),FVector2D(250,480)),Panel(FVector2D(36,18),FVector2D(382,420));
    const FVector2D Safe=ALiveWeaponSwarm::PlaceNameplate(FVector2D(300,200),FVector2D(90,17),Character,Panel,FVector2D(400,700));const FBox2D Rect(Safe,Safe+FVector2D(90,17));
    TestTrue(TEXT("portrait viewport never pushes a name offscreen to escape the GM panel"),Safe.X>=0&&Safe.Y>=0&&Rect.Max.X<=400&&Rect.Max.Y<=700&&!Rect.Intersect(Character)&&!Rect.Intersect(Panel));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveBezierDynamicsTest,"BoundlessImmortality.LiveWeapons.BezierDynamics",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveBezierDynamicsTest::RunTest(const FString&)
{
    const FVector A(0,0,160),B(600,100,45),V0(100,250,0),V1(1700,100,0);
    const auto P=FLiveBezierFlight::Build(A,B,V0,V1,.55f);
    TestTrue(TEXT("Bezier starts at world position"),P.Position(0).Equals(A,.001));
    TestTrue(TEXT("Bezier ends at target"),P.Position(P.Duration).Equals(B,.001));
    TestTrue(TEXT("initial velocity retained"),P.Velocity(0).Equals(V0,.01));
    TestTrue(TEXT("impact has forward inertia"),P.Velocity(P.Duration).Equals(V1,.01));
    const auto Drift=FLiveBezierFlight::Build(B,B+V1*.2,V1,V1*.8,.2f);
    TestTrue(TEXT("segment transition keeps velocity"),Drift.Velocity(0).Equals(P.Velocity(P.Duration),.01));
    const auto Launch=FLiveBezierFlight::Build(A,B,FVector::ZeroVector,V1,.5f);
    TestTrue(TEXT("starts with acceleration rather than teleport speed"),Launch.Velocity(0).IsNearlyZero()&&Launch.Velocity(.05).Size()>0);
    for(int I=0;I<=20;++I)TestTrue(TEXT("acceleration bounded"),P.Acceleration(P.Duration*I/20).Size()<=12001);
    TestTrue(TEXT("swept box catches fast crossing"),FLiveBezierFlight::SweptBoxHits(FVector::ZeroVector,FVector(1000,0,0),FVector(500,0,0),40,FVector(12,12,35)));
    TestFalse(TEXT("swept box rejects lateral miss"),FLiveBezierFlight::SweptBoxHits(FVector::ZeroVector,FVector(1000,0,0),FVector(500,200,0),40,FVector(12,12,35)));
    TestFalse(TEXT("box is not unlimited vertical cylinder"),FLiveBezierFlight::SweptBoxHits(FVector::ZeroVector,FVector(1000,0,0),FVector(500,0,250),40,FVector(12,12,35)));
    TestFalse(TEXT("rounded sphere corner is not a box hit"),FLiveBezierFlight::SweptBoxHits(FVector::ZeroVector,FVector::ZeroVector,FVector(50,50,0),40,FVector(14,14,32)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveComboProbabilityTest,"BoundlessImmortality.LiveWeapons.ComboProbability",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveComboProbabilityTest::RunTest(const FString&)
{
    FRandomStream Random(718);int Hits=0;
    for(int I=0;I<10000;++I)Hits+=ALiveWeaponSwarm::ShouldContinueCombo(ELiveWeaponType::Sword,1,.5f,4,Random)?1:0;
    TestTrue(TEXT("50 percent independent continuation"),Hits>4800&&Hits<5200);
    TestFalse(TEXT("four hit cap even at probability one"),ALiveWeaponSwarm::ShouldContinueCombo(ELiveWeaponType::Spear,4,1,4,Random));
    TestFalse(TEXT("zero chance never chains"),ALiveWeaponSwarm::ShouldContinueCombo(ELiveWeaponType::Sword,1,0,4,Random));
    TestFalse(TEXT("axe does not chain"),ALiveWeaponSwarm::ShouldContinueCombo(ELiveWeaponType::Axe,1,1,4,Random));
    return true;
}
#endif
