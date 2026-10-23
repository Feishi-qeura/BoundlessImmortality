#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveMonster.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveIndependentTempoTest,"BoundlessImmortality.LiveWeapons.Tempo.IndependentAlert",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveIndependentTempoTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;
    auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();
    Swarm->bAutomaticAttacks=false;Swarm->AlertRadiusMin=Swarm->AlertRadiusMax=230;Swarm->AlertHeightMin=Swarm->AlertHeightMax=130;
    auto* Owner=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Owner);
    for(int I=0;I<32;++I)Swarm->JoinParticipant({FString::FromInt(I),TEXT("viewer"),TEXT("")});
    W.World->SpawnActor<ALiveMonster>(FVector(1200,0,45),FRotator::ZeroRotator);
    for(int I=0;I<600;++I)Swarm->Tick(1.f/60);
    double Minimum=100,Maximum=0,FirstRate=0,LastRate=0;
    for(int Window=0;Window<4;++Window)
    {
        TArray<double> Angles;
        for(int I=0;I<32;++I){auto P=Swarm->GetWeaponPosition(FString::FromInt(I));Angles.Add(FMath::Atan2(P.Y,P.X));}
        for(int I=0;I<60;++I)Swarm->Tick(1.f/60);
        for(int I=0;I<32;++I)
        {
            auto P=Swarm->GetWeaponPosition(FString::FromInt(I));
            const double Rate=FMath::UnwindRadians(FMath::Atan2(P.Y,P.X)-Angles[I])/FMath::DegreesToRadians(Swarm->AlertOrbitSpeed);
            Minimum=FMath::Min(Minimum,Rate);Maximum=FMath::Max(Maximum,Rate);
            if(I==0){if(Window==0)FirstRate=Rate;LastRate=Rate;}
        }
    }
    TestTrue(TEXT("alert speed stays within baseline 0.5 to 1.5"),Minimum>=.49&&Maximum<=1.51);
    TestTrue(FString::Printf(TEXT("spirits have distinct speeds; spread %.4f"),Maximum-Minimum),Maximum-Minimum>.5);
    TestTrue(TEXT("each spirit changes tempo over time"),FMath::Abs(LastRate-FirstRate)>.005);
    return true;
}
#endif
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveMonster.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveWideTempoTest,"BoundlessImmortality.LiveWeapons.Night.WideTempo",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveWideTempoTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* S=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Owner=W.World->SpawnActor<APawn>();S->SetAnchor(Owner);
    for(int I=0;I<1000;++I)S->JoinParticipant({FString::FromInt(I),TEXT("v"),TEXT("")});
    float Low=100,High=0;
    for(int Frame=0;Frame<600;++Frame)
    {
        S->Tick(1.f/60);
        for(int I=0;I<1000;I+=19){const float M=S->GetWeaponSpeedMultiplier(FString::FromInt(I));Low=FMath::Min(Low,M);High=FMath::Max(High,M);}
    }
    TestTrue(TEXT("tempo remains within 0.5 to 1.5"),Low>=.5f&&High<=1.5f);
    TestTrue(FString::Printf(TEXT("tempo visibly spans below 0.7 and above 1.3: %.3f %.3f"),Low,High),Low<.7f&&High>1.3f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveDynamicAttackRateTest,"BoundlessImmortality.LiveWeapons.Night.DynamicAttackFrequency",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveDynamicAttackRateTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* S=W.World->SpawnActor<ALiveWeaponSwarm>();S->SetAnchor(W.World->SpawnActor<APawn>());
    for(int I=0;I<32;++I)S->JoinParticipant({FString::FromInt(I),TEXT("v"),TEXT("")});
    TArray<double> Previous;Previous.Init(-1,32);double Low=100,High=0,First=0,Last=0;
    for(int Frame=0;Frame<900;++Frame)
    {
        S->Tick(1.f/60);
        for(int I=0;I<32;++I)
        {
            const double Next=S->GetRoster().GetStates()[I].NextAttackTime;
            if(Previous[I]>=0&&Previous[I]!=Next)
            {
                const double Interval=Next-S->GetSimulationTime();Low=FMath::Min(Low,Interval);High=FMath::Max(High,Interval);
                if(I==0){if(First==0)First=Interval;Last=Interval;}
            }
            Previous[I]=Next;
        }
    }
    TestTrue(TEXT("frequency 0.8 to 1.2 means interval base/1.2 to base/0.8"),Low>=.999&&High<=1.501);
    TestTrue(FString::Printf(TEXT("participants use different attack intervals: %.3f %.3f"),Low,High),High-Low>.2);
    TestTrue(TEXT("same participant changes attack frequency over time"),FMath::Abs(Last-First)>.015);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveFastFollowTest,"BoundlessImmortality.LiveWeapons.Night.RunningCatchUp",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveFastFollowTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* S=W.World->SpawnActor<ALiveWeaponSwarm>();S->bAutomaticAttacks=false;
    auto* Owner=W.World->SpawnActor<APawn>();auto* Root=NewObject<USceneComponent>(Owner);Owner->SetRootComponent(Root);Root->RegisterComponent();S->SetAnchor(Owner);
    for(int I=0;I<32;++I)S->JoinParticipant({FString::FromInt(I),TEXT("v"),TEXT("")});
    auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(1200,0,45),FRotator::ZeroRotator);
    for(int I=0;I<300;++I)S->Tick(1.f/60);
    const FVector Before=S->GetWeaponPosition(TEXT("0"));Owner->SetActorLocation(FVector(1500,0,0));Enemy->SetActorLocation(FVector(2700,0,45));S->Tick(1.f/60);
    TestTrue(TEXT("player displacement cannot teleport spirits"),FVector::Dist(Before,S->GetWeaponPosition(TEXT("0")))<100);
    for(int Frame=0;Frame<600;++Frame)
    {
        Owner->SetActorLocation(FVector(1500+(Frame+1)*600.f/60,0,0));Enemy->SetActorLocation(Owner->GetActorLocation()+FVector(1200,0,45));S->Tick(1.f/60);
    }
    double Farthest=0;TSet<int32> Bins;
    for(int I=0;I<32;++I)
    {
        const FVector D=S->GetWeaponPosition(FString::FromInt(I))-Owner->GetActorLocation();Farthest=FMath::Max(Farthest,D.Size2D());
        Bins.Add(FMath::FloorToInt((FMath::Atan2(D.Y,D.X)+PI)/(2*PI)*8)%8);
    }
    TestTrue(FString::Printf(TEXT("all spirits catch a player running at 600 cm/s, farthest %.1f"),Farthest),Farthest<400);
    TestTrue(FString::Printf(TEXT("moving returns preserve separate orbit phases, angular bins %d"),Bins.Num()),Bins.Num()>=6);
    return true;
}
#endif

