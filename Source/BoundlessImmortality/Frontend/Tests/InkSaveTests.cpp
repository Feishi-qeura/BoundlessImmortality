#if WITH_DEV_AUTOMATION_TESTS

#include "Frontend/InkSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

namespace InkSaveTests
{
UInkSaveGame* MakeSave()
{
    UInkSaveGame* Save = NewObject<UInkSaveGame>();
    Save->SaveVersion = UInkSaveSubsystem::CurrentSaveVersion;
    Save->MapName = TEXT("Level_Sky_Temple_Demo_01");
    Save->SavedAt = FDateTime(2026, 10, 19, 12, 0, 0);
    Save->PlayedSeconds = 125.5;
    Save->PawnTransform = FTransform(FRotator(0, 45, 0), FVector(125, -230, 95));
    return Save;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkSaveValidationTest, "BoundlessImmortality.Frontend.Save.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkSaveValidationTest::RunTest(const FString&)
{
    TestTrue(TEXT("Three zero-based manual slots"), UInkSaveSubsystem::IsValidSlotIndex(0) && UInkSaveSubsystem::IsValidSlotIndex(2));
    TestFalse(TEXT("Negative slot rejected"), UInkSaveSubsystem::IsValidSlotIndex(-1));
    TestFalse(TEXT("Fourth slot rejected"), UInkSaveSubsystem::IsValidSlotIndex(3));
    TestEqual(TEXT("PIE package normalizes to persistent map name"), UInkSaveSubsystem::NormalizeMapName(TEXT("/Game/Isometric_World/Sky_Temple/Maps/UEDPIE_12_Level_Sky_Temple_Demo_01")), FString(TEXT("Level_Sky_Temple_Demo_01")));
    TestEqual(TEXT("Object path normalizes"), UInkSaveSubsystem::NormalizeMapName(TEXT("/Game/Isometric_World/Sky_Temple/Maps/Level_Sky_Temple_Demo_01.Level_Sky_Temple_Demo_01")), FString(TEXT("Level_Sky_Temple_Demo_01")));

    UInkSaveGame* Save = InkSaveTests::MakeSave();
    FString Error;
    TestTrue(TEXT("Current map and schema accepted"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    Save->SaveVersion = UInkSaveSubsystem::CurrentSaveVersion + 1;
    TestFalse(TEXT("Unknown schema cannot continue"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    TestFalse(TEXT("Invalid schema has feedback"), Error.IsEmpty());
    Save->SaveVersion = UInkSaveSubsystem::CurrentSaveVersion;
    Save->MapName = TEXT("Level_Sky_Temple_Showcase_01");
    TestFalse(TEXT("Different map cannot continue"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    Save->MapName = TEXT("Level_Sky_Temple_Demo_01");
    Save->PlayedSeconds = -1;
    TestFalse(TEXT("Negative play time rejected"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    Save->PlayedSeconds = std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("Nonfinite play time rejected"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    Save->PlayedSeconds = 0;
    Save->PawnTransform.SetScale3D(FVector::ZeroVector);
    TestFalse(TEXT("Degenerate player transform rejected"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    Save->PawnTransform = FTransform::Identity;
    Save->SavedAt = FDateTime();
    TestFalse(TEXT("Missing save timestamp rejected"), UInkSaveSubsystem::ValidateSaveGame(Save, Error));
    TestFalse(TEXT("Empty SaveGame cannot pass as new progress"), UInkSaveSubsystem::ValidateSaveGame(NewObject<UInkSaveGame>(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkSaveMemoryTest, "BoundlessImmortality.Frontend.Save.MemoryIntegrity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkSaveMemoryTest::RunTest(const FString&)
{
    UInkSaveGame* Save = InkSaveTests::MakeSave();
    TArray<uint8> Bytes;
    FString Error;
    if (!TestTrue(TEXT("Serialize real SaveGame to memory"), UInkSaveSubsystem::EncodeSave(Save, Bytes, Error))) return false;
    UInkSaveGame* Loaded = UInkSaveSubsystem::DecodeSave(Bytes, Error);
    if (!TestNotNull(TEXT("Read valid SaveGame from memory"), Loaded)) return false;
    TestTrue(TEXT("Pawn transform survives serialization"), Loaded->PawnTransform.Equals(Save->PawnTransform));
    TestEqual(TEXT("Timestamp survives serialization"), Loaded->SavedAt.GetTicks(), Save->SavedAt.GetTicks());
    TestEqual(TEXT("Played time survives serialization"), Loaded->PlayedSeconds, Save->PlayedSeconds);
    Bytes.Last() ^= 0x40;
    TestNull(TEXT("Changed payload cannot continue"), UInkSaveSubsystem::DecodeSave(Bytes, Error));
    TestFalse(TEXT("Corruption provides feedback"), Error.IsEmpty());
    Bytes.Last() ^= 0x40;
    Bytes.RemoveAt(Bytes.Num() - 1);
    TestNull(TEXT("Truncated save cannot continue"), UInkSaveSubsystem::DecodeSave(Bytes, Error));
    TestNull(TEXT("Empty save cannot continue"), UInkSaveSubsystem::DecodeSave(TArray<uint8>(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkSaveStorageTest, "BoundlessImmortality.Frontend.Save.IsolatedSlots", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkSaveStorageTest::RunTest(const FString&)
{
    UGameInstance* Instance = NewObject<UGameInstance>();
    UInkSaveSubsystem* Subsystem = NewObject<UInkSaveSubsystem>(Instance);
    // Every test run has its own namespace. Never read, overwrite, or delete player slots.
    Subsystem->SlotPrefix = FString::Printf(TEXT("InkSaveAutomation_%s_"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    ON_SCOPE_EXIT
    {
        for (int32 Slot = 0; Slot < UInkSaveSubsystem::ManualSlotCount; ++Slot)
        {
            UGameplayStatics::DeleteGameInSlot(Subsystem->StorageSlotName(Slot), 0);
            UGameplayStatics::DeleteGameInSlot(Subsystem->BackupSlotName(Slot), 0);
        }
    };

    TestEqual(TEXT("Exactly three slot entries"), Subsystem->GetSlots().Num(), 3);
    TestEqual(TEXT("Empty saves have no continue slot"), Subsystem->FindLatestSlot(), INDEX_NONE);
    TestFalse(TEXT("Menu without active game cannot save"), Subsystem->SaveSlot(0, nullptr));
    TestFalse(TEXT("Menu without active game cannot load"), Subsystem->LoadSlot(0, nullptr));
    TestFalse(TEXT("Invalid slot cannot delete"), Subsystem->DeleteSlot(-1));
    TestFalse(TEXT("Invalid slot feedback"), Subsystem->LastError.IsEmpty());

    UInkSaveGame* Save = InkSaveTests::MakeSave();
    FString Error;
    TArray<uint8> Bytes;
    if (!UInkSaveSubsystem::EncodeSave(Save, Bytes, Error)) return false;
    if (!TestTrue(TEXT("Write isolated older slot"), UGameplayStatics::SaveDataToSlot(Bytes, Subsystem->StorageSlotName(0), 0))) return false;
    Save->SavedAt += FTimespan::FromMinutes(2);
    if (!UInkSaveSubsystem::EncodeSave(Save, Bytes, Error)) return false;
    if (!TestTrue(TEXT("Write isolated latest slot"), UGameplayStatics::SaveDataToSlot(Bytes, Subsystem->StorageSlotName(1), 0))) return false;
    Bytes.Last() ^= 0x40;
    if (!TestTrue(TEXT("Write isolated corrupt slot"), UGameplayStatics::SaveDataToSlot(Bytes, Subsystem->StorageSlotName(2), 0))) return false;

    const TArray<FInkSaveSlotInfo> Slots = Subsystem->GetSlots();
    TestTrue(TEXT("Valid slots are continuable"), Slots[0].bExists && Slots[0].bValid && Slots[1].bValid);
    TestTrue(TEXT("Corrupt slot is present but cannot continue"), Slots[2].bExists && !Slots[2].bValid);
    TestFalse(TEXT("Corrupt slot explains the failure"), Slots[2].ValidationError.IsEmpty());
    TestEqual(TEXT("Continue selects latest valid save"), Subsystem->FindLatestSlot(), 1);
    TestTrue(TEXT("Delete selected isolated slot"), Subsystem->DeleteSlot(1));
    TestEqual(TEXT("Continue falls back to remaining valid save"), Subsystem->FindLatestSlot(), 0);
    TestTrue(TEXT("Delete corrupt isolated slot"), Subsystem->DeleteSlot(2));
    TestTrue(TEXT("Deleted slot is empty"), !Subsystem->GetSlots()[2].bExists);

    // Simulate an interrupted overwrite: the new primary is corrupt but the old backup is valid.
    Save->PlayedSeconds = 7200;
    if (!UInkSaveSubsystem::EncodeSave(Save, Bytes, Error)) return false;
    if (!TestTrue(TEXT("Write isolated recovery backup"), UGameplayStatics::SaveDataToSlot(Bytes, Subsystem->BackupSlotName(2), 0))) return false;
    const TArray<uint8> TruncatedBytes = {1, 2, 3};
    if (!TestTrue(TEXT("Write interrupted primary"), UGameplayStatics::SaveDataToSlot(TruncatedBytes, Subsystem->StorageSlotName(2), 0))) return false;
    auto RecoveredSlots = Subsystem->GetSlots();
    TestTrue(TEXT("Corrupt primary with valid backup is continuable"), RecoveredSlots[2].bExists && RecoveredSlots[2].bValid);
    TestEqual(TEXT("Backup retains old progress"), RecoveredSlots[2].PlayedSeconds, 7200.0);
    TestEqual(TEXT("Continue includes recoverable backup"), Subsystem->FindLatestSlot(), 2);
    TestTrue(TEXT("Remove interrupted primary"), UGameplayStatics::DeleteGameInSlot(Subsystem->StorageSlotName(2), 0));
    RecoveredSlots = Subsystem->GetSlots();
    TestTrue(TEXT("Backup-only slot remains visible and loadable"), RecoveredSlots[2].bExists && RecoveredSlots[2].bValid);
    TestTrue(TEXT("Delete backup-only slot"), Subsystem->DeleteSlot(2));
    TestFalse(TEXT("Delete prevents backup resurrection"), Subsystem->GetSlots()[2].bExists);

    if (!TestTrue(TEXT("Restore isolated recovery fixture"), UGameplayStatics::SaveDataToSlot(Bytes, Subsystem->BackupSlotName(2), 0))) return false;
    Save->PlayedSeconds = 7300;
    if (!UInkSaveSubsystem::EncodeSave(Save, Bytes, Error)) return false;
    TestTrue(TEXT("Replace recovered slot with a verified save"), Subsystem->WriteSlotBytes(2, Bytes));
    TestEqual(TEXT("Successful overwrite reads new progress"), Subsystem->GetSlots()[2].PlayedSeconds, 7300.0);
    TestFalse(TEXT("Successful overwrite cleans recovery backup"), UGameplayStatics::DoesSaveGameExist(Subsystem->BackupSlotName(2), 0));
    TestFalse(TEXT("Malformed replacement is rejected before touching valid slot"), Subsystem->WriteSlotBytes(2, TruncatedBytes));
    TestEqual(TEXT("Rejected replacement preserves previous save"), Subsystem->GetSlots()[2].PlayedSeconds, 7300.0);

    Save->PlayedSeconds = 7400;
    if (!UInkSaveSubsystem::EncodeSave(Save, Bytes, Error)) return false;
    TestTrue(TEXT("Overwrite valid primary via backup strategy"), Subsystem->WriteSlotBytes(2, Bytes));
    TestEqual(TEXT("Valid-primary overwrite retains new data"), Subsystem->GetSlots()[2].PlayedSeconds, 7400.0);
    if (!TestTrue(TEXT("Create stale backup to test primary preference"), UGameplayStatics::SaveDataToSlot(Bytes, Subsystem->BackupSlotName(0), 0))) return false;
    TestEqual(TEXT("Valid primary wins over stale backup"), Subsystem->GetSlots()[0].PlayedSeconds, 125.5);
    TestTrue(TEXT("Delete primary and backup together"), Subsystem->DeleteSlot(0));
    TestFalse(TEXT("Both copies removed from slot list"), Subsystem->GetSlots()[0].bExists);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkSaveSessionTest, "BoundlessImmortality.Frontend.Save.MenuPlayTime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInkSaveSessionTest::RunTest(const FString&)
{
    UInkSaveSubsystem* Subsystem = NewObject<UInkSaveSubsystem>(NewObject<UGameInstance>());
    // Identity-only fixtures: no actor/world state is changed and no player save is touched.
    APawn* Pawn = GetMutableDefault<APawn>();
    APawn* OtherPawn = GetMutableDefault<ACharacter>();
    UWorld* World = NewObject<UWorld>();
    UWorld* OtherWorld = NewObject<UWorld>();
    Subsystem->ActivateGameplaySession(Pawn, World, 10);
    TestEqual(TEXT("Initial session measures actual elapsed gameplay"), Subsystem->GetPlayedSeconds(70), 60.0);
    Subsystem->FreezeGameplaySession(70);
    TestFalse(TEXT("Menu revokes permission to save or load"), Subsystem->bGameplaySessionActive);
    TestEqual(TEXT("Menu time is excluded"), Subsystem->GetPlayedSeconds(370), 60.0);
    Subsystem->FreezeGameplaySession(370);
    TestEqual(TEXT("Repeated menu teardown does not double-count time"), Subsystem->GetPlayedSeconds(370), 60.0);
    Subsystem->ActivateGameplaySession(Pawn, World, 370);
    TestEqual(TEXT("Same pawn and world resume previous play time"), Subsystem->GetPlayedSeconds(400), 90.0);
    Subsystem->StoredPlayedSeconds = 7200; // The same baseline assigned by a successful LoadSlot.
    Subsystem->SessionStartedAt = 400;
    Subsystem->FreezeGameplaySession(460);
    Subsystem->ActivateGameplaySession(Pawn, World, 1000);
    TestEqual(TEXT("Loaded cumulative play time survives menu round trip"), Subsystem->GetPlayedSeconds(1030), 7290.0);
    Subsystem->FreezeGameplaySession(1030);
    Subsystem->ActivateGameplaySession(OtherPawn, World, 1030);
    TestEqual(TEXT("Different pawn begins fresh progress"), Subsystem->GetPlayedSeconds(1040), 10.0);
    Subsystem->FreezeGameplaySession(1040);
    Subsystem->ActivateGameplaySession(OtherPawn, OtherWorld, 5);
    TestEqual(TEXT("Different world begins fresh progress"), Subsystem->GetPlayedSeconds(25), 20.0);
    return true;
}

#endif
