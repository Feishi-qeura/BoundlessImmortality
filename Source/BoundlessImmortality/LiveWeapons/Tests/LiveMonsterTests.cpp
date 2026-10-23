#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "LiveWeaponTestWorld.h"
#include "LiveWeapons/LiveMonster.h"
#include "LiveWeapons/LiveMonsterRegistrySubsystem.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveMonsterTest,"BoundlessImmortality.LiveWeapons.Monster",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveMonsterTest::RunTest(const FString&)
{
    FLiveWeaponTestWorld TestWorld;UWorld* World=TestWorld.World;
    auto* Monster=World->SpawnActor<ALiveMonster>();
    TestNotNull(TEXT("monster spawned"),Monster);
    if(Monster)
    {
        TestTrue(TEXT("alive initially"),Monster->IsAlive());
        const float Before=Monster->GetHealthFraction();
        TestFalse(TEXT("negative damage ignored"),Monster->ApplyWeaponDamage(-5,TEXT("test")));
        TestEqual(TEXT("negative damage never heals"),Monster->GetHealthFraction(),Before);
        Monster->ApplyPush(FVector(-100,0,0),150,.25);
        Monster->Tick(.1);TestEqual(TEXT("push does not damage"),Monster->GetHealthFraction(),Before);
        TestTrue(TEXT("push moves away"),Monster->GetActorLocation().X>0);
        TArray<TWeakObjectPtr<ALiveMonster>> Found;
        Monster->SetActorLocation(FVector(0,0,500));
        World->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(FVector::ZeroVector,100,Found);
        TestEqual(TEXT("range is XY only"),Found.Num(),1);
        Monster->ApplyWeaponDamage(100000,TEXT("test"));
        TestFalse(TEXT("dead after lethal"),Monster->IsAlive());TestEqual(TEXT("health floor zero"),Monster->GetHealthFraction(),0.f);
        TestFalse(TEXT("dead ignores damage"),Monster->ApplyWeaponDamage(1,TEXT("test")));
        Found.Reset();World->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(FVector::ZeroVector,100,Found);
        TestEqual(TEXT("dead removed from candidates"),Found.Num(),0);
    }
    return true;
}
#endif
