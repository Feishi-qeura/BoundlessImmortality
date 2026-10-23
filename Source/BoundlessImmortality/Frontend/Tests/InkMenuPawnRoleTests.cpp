#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/InkMenuDirector.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkMenuPawnRoleTest,"BoundlessImmortality.Frontend.SafeReentry.MenuPawnRoles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkMenuPawnRoleTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Director=World->SpawnActor<AInkMenuDirector>();
    auto* Demo=World->SpawnActor<ACharacter>();
    auto* DemoCopy=World->SpawnActor<ACharacter>(FVector(1000,0,1000),FRotator::ZeroRotator);
    auto* Player=World->SpawnActor<APawn>();
    Director->PresentationActor=Demo;
    Director->PresentationPawnClass=Demo->GetClass();
    Director->PresentationSpawnTransform=FTransform(FVector(50000,50000,1000));
    FTransform Safe;
    TestFalse(TEXT("Menu stage is not a gameplay safe point"),Director->GetSafeReentryTransform(Safe));
    TestFalse(TEXT("Presentation instance cannot become gameplay pawn"),Director->IsGameplayPawnCandidate(Demo));
    TestFalse(TEXT("Another presentation-class instance cannot become gameplay pawn"),Director->IsGameplayPawnCandidate(DemoCopy));
    TestTrue(TEXT("Independent gameplay class remains eligible"),Director->IsGameplayPawnCandidate(Player));
    Director->LastSafeGameplayTransform=FTransform(FRotator(0,65,0),FVector(100,200,300));
    Director->bHasSafeGameplayTransform=true;
    TestTrue(TEXT("Confirmed gameplay point is available"),Director->GetSafeReentryTransform(Safe));
    TestTrue(TEXT("Confirmed gameplay position and yaw retained"),Safe.Equals(Director->LastSafeGameplayTransform));
    World->DestroyWorld(false);
    return true;
}
#endif
