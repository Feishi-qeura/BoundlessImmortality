#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/InkPawnSafety.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/BoxComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkPawnSafetyTest,"BoundlessImmortality.Frontend.SafeReentry.GroundAndVoid",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkPawnSafetyTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(500,500,20));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();
    auto* Pawn=World->SpawnActor<ACharacter>(FVector(0,0,125),FRotator(0,35,0));
    FTransform Supported;
    TestTrue(TEXT("Walkable ground accepted"),InkPawnSafety::FindSupportedTransform(Pawn,Pawn->GetActorTransform(),Supported));
    TestTrue(TEXT("Real yaw retained"),Supported.GetRotation().Equals(Pawn->GetActorQuat()));
    const FTransform Safe=Supported;
    const FTransform Void(FRotator(0,90,0),FVector(50000,50000,-1000));
    TestFalse(TEXT("Void rejected without clamping to arbitrary XY"),InkPawnSafety::FindSupportedTransform(Pawn,Void,Supported));
    TestTrue(TEXT("Void resolves through validated fallback"),InkPawnSafety::ResolveTransform(Pawn,Void,Safe,Supported));
    TestTrue(TEXT("Fallback remains on floor"),Supported.GetLocation().Equals(Safe.GetLocation(),1));
    Floor->SetActorRotation(FRotator(40,0,0));
    TestTrue(TEXT("Walkable slope accepted without capsule penetration"),InkPawnSafety::FindSupportedTransform(Pawn,Pawn->GetActorTransform(),Supported));
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestFalse(TEXT("No valid surface returns failure"),InkPawnSafety::ResolveTransform(Pawn,Void,Safe,Supported));
    World->DestroyWorld(false);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkPawnSafetyStageCoordinateTest,"BoundlessImmortality.Frontend.SafeReentry.StageCoordinatesKeepPawnRoles",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkPawnSafetyStageCoordinateTest::RunTest(const FString&)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* Floor=World->SpawnActor<AActor>();auto* Box=NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(Box);Box->SetBoxExtent(FVector(500,500,20));Box->SetCollisionProfileName(TEXT("BlockAll"));Box->RegisterComponent();
    auto* Stage=World->SpawnActor<ACharacter>(FVector(0,0,125),FRotator(0,180,0));
    auto* Player=World->SpawnActor<ACharacter>(FVector(50000,50000,-1000),FRotator(0,35,0));
    const FTransform StageBefore=Stage->GetActorTransform();
    const auto* PlayerClass=Player->GetClass();
    FTransform Resolved;
    TestFalse(TEXT("Off-island PlayerStart is rejected"),InkPawnSafety::FindSupportedTransform(Player,Player->GetActorTransform(),Resolved,Stage));
    TestTrue(TEXT("Legacy stage coordinates can be validated with actual gameplay capsule"),InkPawnSafety::FindSupportedTransform(Player,StageBefore,Resolved,Stage));
    Player->SetActorTransform(Resolved,false,nullptr,ETeleportType::TeleportPhysics);
    TestTrue(TEXT("Gameplay class remains unchanged"),Player->GetClass()==PlayerClass);
    TestTrue(TEXT("Stage actor is not moved by coordinate fallback"),Stage->GetActorTransform().Equals(StageBefore));
    Stage->Destroy();
    TestTrue(TEXT("Independent gameplay pawn survives stage destruction"),IsValid(Player));
    World->DestroyWorld(false);return true;
}
#endif
