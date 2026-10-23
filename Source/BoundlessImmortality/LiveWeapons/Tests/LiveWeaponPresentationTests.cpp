#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveMonster.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveMenuPresentationTest,"BoundlessImmortality.LiveWeapons.Presentation.MenuCycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveMenuPresentationTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;
    auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();
    auto* Pawn=W.World->SpawnActor<APawn>();
    Swarm->SetAnchor(Pawn);
    Swarm->SetPresentationMode(ELiveWeaponPresentationMode::Idle);
    for(int32 I=0;I<10;++I)
        TestTrue(TEXT("anonymous demonstration sword joins"),Swarm->AddParticipant({FString::Printf(TEXT("menu_%02d"),I),TEXT(""),TEXT("")}));
    TestEqual(TEXT("exactly ten demonstration swords"),Swarm->GetParticipantCount(),10);
    TestEqual(TEXT("all ten swords are rendered"),Swarm->GetRenderedCount(),10);
    TArray<FLiveWeaponLabelSample> Labels;
    Swarm->GetPresentationSamples(Labels);
    TestTrue(TEXT("demonstration supplies no identity or avatar labels"),Labels.IsEmpty());
    float LargestStep=0;bool bAllPositionsFinite=true;
    for(int32 Phase=0;Phase<4;++Phase)
    {
        if(Phase==2)
        {
            auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(200,0,45),FRotator::ZeroRotator);
            Enemy->Health=100000;
        }
        const bool Alert=(Phase%2)==1;
        Swarm->SetPresentationMode(Alert?ELiveWeaponPresentationMode::Alert:ELiveWeaponPresentationMode::Idle);
        for(int32 Frame=0;Frame<600;++Frame)
        {
            const FVector Previous=Swarm->GetWeaponPosition(TEXT("menu_00"));
            Swarm->Tick(1.f/60);
            const FVector Current=Swarm->GetWeaponPosition(TEXT("menu_00"));
            bAllPositionsFinite&=!Current.ContainsNaN();
            LargestStep=FMath::Max(LargestStep,float(FVector::Distance(Previous,Current)));
        }
        for(int32 I=0;I<10;++I)
        {
            const auto Current=Swarm->GetWeaponPhase(FString::Printf(TEXT("menu_%02d"),I));
            TestTrue(TEXT("phase reaches the requested presentation state"),Alert?Current==ELiveWeaponFlightPhase::Alert:(Current==ELiveWeaponFlightPhase::Wander||Current==ELiveWeaponFlightPhase::Hover));
        }
    }
    TestTrue(TEXT("positions remain finite throughout both cycles"),bAllPositionsFinite);
    TestTrue(TEXT("idle/alert transitions preserve continuous motion"),LargestStep<150.f);
    TestEqual(TEXT("presentation ignores nearby monsters"),Swarm->TotalPrimaryLaunched,int64(0));
    TestEqual(TEXT("presentation never inflicts damage"),Swarm->TotalAttacks,int64(0));
    TestEqual(TEXT("presentation never launches arrows"),Swarm->GetActiveArrowCount(),0);
    Swarm->SetPresentationMode(ELiveWeaponPresentationMode::Gameplay);
    Swarm->GetPresentationSamples(Labels);
    TestEqual(TEXT("ordinary gameplay labels are restored"),Labels.Num(),10);
    for(int32 Frame=0;Frame<1200;++Frame)Swarm->Tick(1.f/60);
    TestTrue(TEXT("ordinary gameplay attacks still run after presentation"),Swarm->TotalPrimaryLaunched>0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLivePresentationInterruptTest,"BoundlessImmortality.LiveWeapons.Presentation.InterruptCombat",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLivePresentationInterruptTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;
    auto* Swarm=W.World->SpawnActor<ALiveWeaponSwarm>();
    auto* Pawn=W.World->SpawnActor<APawn>();Swarm->SetAnchor(Pawn);
    Swarm->AddParticipant({TEXT("combat"),TEXT("player"),TEXT("")});
    auto* Enemy=W.World->SpawnActor<ALiveMonster>(FVector(600,0,45),FRotator::ZeroRotator);Enemy->Health=100000;
    for(int32 Frame=0;Frame<600&&Swarm->TotalPrimaryLaunched==0;++Frame)Swarm->Tick(1.f/60);
    TestTrue(TEXT("test interrupts a real sortie"),Swarm->TotalPrimaryLaunched>0);
    Swarm->SetPresentationMode(ELiveWeaponPresentationMode::Alert);
    const int64 Hits=Swarm->TotalAttacks,Launches=Swarm->TotalPrimaryLaunched;
    for(int32 Frame=0;Frame<600;++Frame)Swarm->Tick(1.f/60);
    TestEqual(TEXT("existing combat cannot leak damage into demonstration"),Swarm->TotalAttacks,Hits);
    TestEqual(TEXT("no new combat begins during demonstration"),Swarm->TotalPrimaryLaunched,Launches);
    TestEqual(TEXT("interrupted sword reaches alert"),Swarm->GetWeaponPhase(TEXT("combat")),ELiveWeaponFlightPhase::Alert);
    return true;
}
#endif
