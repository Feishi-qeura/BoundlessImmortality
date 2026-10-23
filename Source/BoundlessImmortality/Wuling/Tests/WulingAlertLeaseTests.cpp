#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "LiveWeapons/Tests/LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "Cultivation/CultivationSubsystem.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWulingAlertLeaseTest,"BoundlessImmortality.Wuling.AlertLeaseIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWulingAlertLeaseTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld W;auto* S=W.World->SpawnActor<ALiveWeaponSwarm>();auto* Pawn=W.World->SpawnActor<APawn>();S->SetAnchor(Pawn);
    S->JoinParticipant({TEXT("helper"),TEXT("剑灵"),TEXT("")});S->bAutomaticAttacks=false;
    const double End=UCultivationSubsystem::UtcNow()+60;
    TestTrue(TEXT("attempt lease accepted"),S->AcquireAlertLease(TEXT("helper"),TEXT("tribulation:one"),End,1));
    TestTrue(TEXT("independent alert accepted"),S->AcquireAlertLease(TEXT("helper"),TEXT("other"),End,1));
    TestEqual(TEXT("two sources coexist"),S->GetAlertLeaseCount(TEXT("helper")),2);
    for(int Frame=0;Frame<600;++Frame)S->Tick(1.f/60);
    TestEqual(TEXT("lease reaches alert through safe return"),S->GetWeaponPhase(TEXT("helper")),ELiveWeaponFlightPhase::Alert);
    S->ReleaseAlertLease(TEXT("helper"),TEXT("tribulation:one"));
    TestEqual(TEXT("cleanup preserves other source"),S->GetAlertLeaseCount(TEXT("helper")),1);
    S->ReleaseAlertLeasesForGeneration(1);
    TestEqual(TEXT("generation cleanup drains leases"),S->GetAlertLeaseCount(TEXT("helper")),0);
    TestTrue(TEXT("new generation accepted"),S->AcquireAlertLease(TEXT("helper"),TEXT("tribulation:two"),End,2));
    TestFalse(TEXT("stale generation rejected"),S->AcquireAlertLease(TEXT("helper"),TEXT("tribulation:old"),End,1));
    S->RemoveParticipant(TEXT("helper"));TestEqual(TEXT("destroyed participant releases state"),S->GetAlertLeaseCount(TEXT("helper")),0);
    return true;
}
#endif
