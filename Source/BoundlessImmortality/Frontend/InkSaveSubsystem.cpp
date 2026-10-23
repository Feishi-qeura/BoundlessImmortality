#include "Frontend/InkSaveSubsystem.h"
#include "Frontend/InkPawnSafety.h"
#include "Frontend/InkMenuDirector.h"
#include "EngineUtils.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"

namespace InkSave
{
constexpr TCHAR SupportedMap[] = TEXT("Level_Sky_Temple_Demo_01");
constexpr int32 LocalUserIndex = 0;
constexpr uint8 EnvelopeMagic[] = {'B', 'I', 'S', 'A', 'V', 'E', '0', '1'};
constexpr int32 SizeOffset = sizeof(EnvelopeMagic);
constexpr int32 HashOffset = SizeOffset + sizeof(uint32);
constexpr int32 HeaderSize = HashOffset + FSHA1::DigestSize;
constexpr int32 MaxPayloadBytes = 1024 * 1024;

bool IsSupportedMap(const FString& Name)
{
    return UInkSaveSubsystem::NormalizeMapName(Name).Equals(SupportedMap, ESearchCase::CaseSensitive);
}
}

bool UInkSaveSubsystem::IsValidSlotIndex(int32 Slot)
{
    return Slot >= 0 && Slot < ManualSlotCount;
}

FString UInkSaveSubsystem::NormalizeMapName(const FString& MapName)
{
    const FString PackageName = FPackageName::ObjectPathToPackageName(MapName);
    return UWorld::RemovePIEPrefix(FPackageName::GetShortName(PackageName));
}

bool UInkSaveSubsystem::ValidateSaveGame(const UInkSaveGame* Save, FString& OutError)
{
    OutError.Reset();
    if (!Save)
    {
        OutError = TEXT("存档内容无法读取。");
    }
    else if (Save->SaveVersion != CurrentSaveVersion)
    {
        OutError = TEXT("存档版本不兼容，无法继续。");
    }
    else if (!InkSave::IsSupportedMap(Save->MapName))
    {
        OutError = TEXT("此存档的关卡暂不支持。");
    }
    else if (!Save->PawnTransform.IsValid() || Save->PawnTransform.GetScale3D().GetAbsMin() <= UE_SMALL_NUMBER)
    {
        OutError = TEXT("存档中的角色位置无效。");
    }
    else if (Save->SavedAt.GetTicks() <= 0 || Save->SavedAt > FDateTime::MaxValue())
    {
        OutError = TEXT("存档时间无效。");
    }
    else if (!FMath::IsFinite(Save->PlayedSeconds) || Save->PlayedSeconds < 0)
    {
        OutError = TEXT("存档中的游玩时长无效。");
    }
    return OutError.IsEmpty();
}

bool UInkSaveSubsystem::EncodeSave(UInkSaveGame* Save, TArray<uint8>& OutBytes, FString& OutError)
{
    OutBytes.Reset();
    if (!ValidateSaveGame(Save, OutError)) return false;

    TArray<uint8> Payload;
    if (!UGameplayStatics::SaveGameToMemory(Save, Payload) || Payload.IsEmpty() || Payload.Num() > InkSave::MaxPayloadBytes)
    {
        OutError = TEXT("无法序列化存档内容。");
        return false;
    }

    // Check integrity before handing bytes to UObject deserialization. This also detects truncation.
    OutBytes.SetNumUninitialized(InkSave::HeaderSize + Payload.Num());
    FMemory::Memcpy(OutBytes.GetData(), InkSave::EnvelopeMagic, sizeof(InkSave::EnvelopeMagic));
    const uint32 PayloadSize = static_cast<uint32>(Payload.Num());
    for (int32 Byte = 0; Byte < 4; ++Byte)
    {
        OutBytes[InkSave::SizeOffset + Byte] = static_cast<uint8>(PayloadSize >> (Byte * 8));
    }
    FSHA1::HashBuffer(Payload.GetData(), Payload.Num(), OutBytes.GetData() + InkSave::HashOffset);
    FMemory::Memcpy(OutBytes.GetData() + InkSave::HeaderSize, Payload.GetData(), Payload.Num());
    return true;
}

UInkSaveGame* UInkSaveSubsystem::DecodeSave(const TArray<uint8>& Bytes, FString& OutError)
{
    OutError.Reset();
    if (Bytes.Num() <= InkSave::HeaderSize || Bytes.Num() > InkSave::HeaderSize + InkSave::MaxPayloadBytes ||
        FMemory::Memcmp(Bytes.GetData(), InkSave::EnvelopeMagic, sizeof(InkSave::EnvelopeMagic)) != 0)
    {
        OutError = TEXT("存档文件损坏或格式不兼容。");
        return nullptr;
    }

    uint32 PayloadSize = 0;
    for (int32 Byte = 0; Byte < 4; ++Byte)
    {
        PayloadSize |= static_cast<uint32>(Bytes[InkSave::SizeOffset + Byte]) << (Byte * 8);
    }
    if (PayloadSize != static_cast<uint32>(Bytes.Num() - InkSave::HeaderSize))
    {
        OutError = TEXT("存档文件不完整，无法继续。");
        return nullptr;
    }

    uint8 ActualHash[FSHA1::DigestSize];
    FSHA1::HashBuffer(Bytes.GetData() + InkSave::HeaderSize, PayloadSize, ActualHash);
    if (FMemory::Memcmp(ActualHash, Bytes.GetData() + InkSave::HashOffset, FSHA1::DigestSize) != 0)
    {
        OutError = TEXT("存档校验失败，文件可能已损坏。");
        return nullptr;
    }

    TArray<uint8> Payload;
    Payload.Append(Bytes.GetData() + InkSave::HeaderSize, static_cast<int32>(PayloadSize));
    UInkSaveGame* Save = Cast<UInkSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
    return ValidateSaveGame(Save, OutError) ? Save : nullptr;
}

FString UInkSaveSubsystem::StorageSlotName(int32 Slot) const
{
    return IsValidSlotIndex(Slot) ? FString::Printf(TEXT("%s%d"), *SlotPrefix, Slot + 1) : FString();
}

FString UInkSaveSubsystem::BackupSlotName(int32 Slot) const
{
    return IsValidSlotIndex(Slot) ? StorageSlotName(Slot) + TEXT("_Recovery") : FString();
}

bool UInkSaveSubsystem::CheckSlotIndex(int32 Slot)
{
    if (IsValidSlotIndex(Slot)) return true;
    LastError = TEXT("存档槽编号无效。");
    return false;
}

bool UInkSaveSubsystem::ValidatePawn(APawn* Pawn, bool bRequireRegisteredSession)
{
    UWorld* World = GetWorld();
    if (!IsValid(Pawn) || !World || Pawn->GetWorld() != World || !World->IsGameWorld() ||
        !Pawn->IsPlayerControlled() || !Pawn->IsLocallyControlled())
    {
        LastError = TEXT("当前没有可存取的本地玩家角色。");
        return false;
    }
    if (!InkSave::IsSupportedMap(World->GetMapName()))
    {
        LastError = TEXT("当前仅支持天宫演示关卡的位置存档。");
        return false;
    }
    if (bRequireRegisteredSession && (!bGameplaySessionActive || GameplayPawn.Get() != Pawn || GameplayWorld.Get() != World))
    {
        LastError = TEXT("请先进入游戏，再保存或读取进度。");
        return false;
    }
    return true;
}

bool UInkSaveSubsystem::BeginGameplay(APawn* Pawn)
{
    LastError.Reset();
    EndGameplay();
    if (!ValidatePawn(Pawn, false)) return false;
    ActivateGameplaySession(Pawn, Pawn->GetWorld(), Pawn->GetWorld()->GetTimeSeconds());
    return true;
}

void UInkSaveSubsystem::ActivateGameplaySession(APawn* Pawn, UWorld* World, double WorldTime)
{
    if (GameplayPawn.Get() != Pawn || GameplayWorld.Get() != World) StoredPlayedSeconds = 0;
    GameplayPawn = Pawn;
    GameplayWorld = World;
    SessionStartedAt = WorldTime;
    bGameplaySessionActive = true;
}

double UInkSaveSubsystem::GetPlayedSeconds(double WorldTime) const
{
    return StoredPlayedSeconds + (bGameplaySessionActive ? FMath::Max(0.0, WorldTime - SessionStartedAt) : 0.0);
}

void UInkSaveSubsystem::FreezeGameplaySession(double WorldTime)
{
    StoredPlayedSeconds = GetPlayedSeconds(WorldTime);
    bGameplaySessionActive = false;
    SessionStartedAt = 0;
}

void UInkSaveSubsystem::EndGameplay()
{
    // Keep identity/baseline so returning from the menu resumes this same runtime progress.
    // The active flag revokes all save/load access while its pawn is used for presentation.
    const UWorld* World = GameplayWorld.Get();
    FreezeGameplaySession(World ? World->GetTimeSeconds() : SessionStartedAt);
}

bool UInkSaveSubsystem::SaveSlot(int32 Slot, APawn* Pawn)
{
    LastError.Reset();
    if (!CheckSlotIndex(Slot) || !ValidatePawn(Pawn, true)) return false;

    UInkSaveGame* Save = Cast<UInkSaveGame>(UGameplayStatics::CreateSaveGameObject(UInkSaveGame::StaticClass()));
    if (!Save)
    {
        LastError = TEXT("无法创建存档。");
        return false;
    }
    Save->SaveVersion = CurrentSaveVersion;
    Save->MapName = NormalizeMapName(Pawn->GetWorld()->GetMapName());
    Save->PawnTransform = Pawn->GetActorTransform();
    Save->SavedAt = FDateTime::UtcNow();
    // World gameplay time excludes SetPause intervals; menu time is outside this registered session.
    Save->PlayedSeconds = GetPlayedSeconds(Pawn->GetWorld()->GetTimeSeconds());
    TArray<uint8> Bytes;
    if (!EncodeSave(Save, Bytes, LastError)) return false;
    return WriteSlotBytes(Slot, Bytes);
}

bool UInkSaveSubsystem::WriteSlotBytes(int32 Slot, const TArray<uint8>& Bytes)
{
    LastError.Reset();
    if (!CheckSlotIndex(Slot) || !DecodeSave(Bytes, LastError)) return false;
    const FString PrimaryName = StorageSlotName(Slot);
    const FString RecoveryName = BackupSlotName(Slot);
    TArray<uint8> PreviousBytes;
    FString PreviousError;
    if (UGameplayStatics::DoesSaveGameExist(PrimaryName, InkSave::LocalUserIndex))
    {
        if (!UGameplayStatics::LoadDataFromSlot(PreviousBytes, PrimaryName, InkSave::LocalUserIndex))
        {
            LastError = TEXT("无法读取原存档，已取消覆盖以保留原记录。");
            return false;
        }
        if (DecodeSave(PreviousBytes, PreviousError))
        {
            // The platform API may truncate the primary before a failed write. Preserve and
            // verify the old valid bytes before any attempt to replace that primary.
            TArray<uint8> VerifiedBackup;
            if (!UGameplayStatics::SaveDataToSlot(PreviousBytes, RecoveryName, InkSave::LocalUserIndex) ||
                !UGameplayStatics::LoadDataFromSlot(VerifiedBackup, RecoveryName, InkSave::LocalUserIndex) ||
                VerifiedBackup != PreviousBytes || !DecodeSave(VerifiedBackup, PreviousError))
            {
                LastError = TEXT("无法建立恢复备份，已保留原存档并取消覆盖。");
                return false;
            }
        }
        // If the primary is already corrupt, keep an existing recovery copy untouched.
    }

    TArray<uint8> VerifiedPrimary;
    if (!UGameplayStatics::SaveDataToSlot(Bytes, PrimaryName, InkSave::LocalUserIndex) ||
        !UGameplayStatics::LoadDataFromSlot(VerifiedPrimary, PrimaryName, InkSave::LocalUserIndex) ||
        VerifiedPrimary != Bytes || !DecodeSave(VerifiedPrimary, PreviousError))
    {
        FString RecoveryError;
        LastError = ReadStorageSlot(RecoveryName, RecoveryError)
            ? TEXT("保存失败，原记录已由恢复备份保留。请检查存储空间和权限。")
            : TEXT("保存或校验失败，请检查存储空间和写入权限。");
        return false;
    }
    // If cleanup fails, the verified primary still takes precedence on the next read.
    if (UGameplayStatics::DoesSaveGameExist(RecoveryName, InkSave::LocalUserIndex))
        UGameplayStatics::DeleteGameInSlot(RecoveryName, InkSave::LocalUserIndex);
    return true;
}

UInkSaveGame* UInkSaveSubsystem::ReadStorageSlot(const FString& SlotName, FString& OutError) const
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, InkSave::LocalUserIndex))
    {
        OutError = TEXT("此存档槽为空。");
        return nullptr;
    }
    TArray<uint8> Bytes;
    if (!UGameplayStatics::LoadDataFromSlot(Bytes, SlotName, InkSave::LocalUserIndex))
    {
        OutError = TEXT("无法读取存档文件。");
        return nullptr;
    }
    return DecodeSave(Bytes, OutError);
}

UInkSaveGame* UInkSaveSubsystem::ReadSlot(int32 Slot, FString& OutError) const
{
    if (!IsValidSlotIndex(Slot))
    {
        OutError = TEXT("存档槽编号无效。");
        return nullptr;
    }
    if (UInkSaveGame* Save = ReadStorageSlot(StorageSlotName(Slot), OutError)) return Save;
    if (UGameplayStatics::DoesSaveGameExist(BackupSlotName(Slot), InkSave::LocalUserIndex))
        return ReadStorageSlot(BackupSlotName(Slot), OutError);
    return nullptr;
}

bool UInkSaveSubsystem::LoadSlot(int32 Slot, APawn* Pawn)
{
    LastError.Reset();
    if (!CheckSlotIndex(Slot) || !ValidatePawn(Pawn, true)) return false;
    const UInkSaveGame* Save = ReadSlot(Slot, LastError);
    if (!Save) return false;
    if (NormalizeMapName(Save->MapName) != NormalizeMapName(Pawn->GetWorld()->GetMapName()))
    {
        LastError = TEXT("存档关卡与当前关卡不一致。");
        return false;
    }
    FTransform Fallback=Pawn->GetActorTransform(),Resolved;
    for(TActorIterator<AInkMenuDirector> It(Pawn->GetWorld());It;++It)if(It->GetSafeReentryTransform(Fallback))break;
    if(!InkPawnSafety::ResolveTransform(Pawn,Save->PawnTransform,Fallback,Resolved))
    {LastError=TEXT("存档位置无法站立，且安全落点尚未就绪。");return false;}
    if (!Pawn->SetActorTransform(Resolved, false, nullptr, ETeleportType::TeleportPhysics))
    {
        LastError = TEXT("无法恢复角色位置。");
        return false;
    }
    if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent()) Movement->StopMovementImmediately();
    InkPawnSafety::ClearFallingMotion(Pawn);
    StoredPlayedSeconds = Save->PlayedSeconds;
    SessionStartedAt = Pawn->GetWorld()->GetTimeSeconds();
    return true;
}

bool UInkSaveSubsystem::DeleteSlot(int32 Slot)
{
    LastError.Reset();
    if (!CheckSlotIndex(Slot)) return false;
    // Delete the fallback first; otherwise a failed fallback deletion could resurrect a slot
    // whose primary was already removed. On failure the primary remains available.
    const FString RecoveryName = BackupSlotName(Slot);
    if (UGameplayStatics::DoesSaveGameExist(RecoveryName, InkSave::LocalUserIndex) &&
        !UGameplayStatics::DeleteGameInSlot(RecoveryName, InkSave::LocalUserIndex))
    {
        LastError = TEXT("删除恢复备份失败，原存档尚未删除。");
        return false;
    }
    const FString SlotName = StorageSlotName(Slot);
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, InkSave::LocalUserIndex)) return true;
    if (!UGameplayStatics::DeleteGameInSlot(SlotName, InkSave::LocalUserIndex))
    {
        LastError = TEXT("删除存档失败，请检查本地文件权限。");
        return false;
    }
    return true;
}

TArray<FInkSaveSlotInfo> UInkSaveSubsystem::GetSlots() const
{
    TArray<FInkSaveSlotInfo> Slots;
    Slots.Reserve(ManualSlotCount);
    for (int32 Slot = 0; Slot < ManualSlotCount; ++Slot)
    {
        FInkSaveSlotInfo& Info = Slots.AddDefaulted_GetRef();
        Info.SlotIndex = Slot;
        Info.DisplayName = FString::Printf(TEXT("存档 %d"), Slot + 1);
        Info.bExists = UGameplayStatics::DoesSaveGameExist(StorageSlotName(Slot), InkSave::LocalUserIndex) ||
            UGameplayStatics::DoesSaveGameExist(BackupSlotName(Slot), InkSave::LocalUserIndex);
        if (!Info.bExists) continue;
        if (const UInkSaveGame* Save = ReadSlot(Slot, Info.ValidationError))
        {
            Info.bValid = true;
            Info.SavedAt = Save->SavedAt;
            Info.MapName = NormalizeMapName(Save->MapName);
            Info.PlayedSeconds = Save->PlayedSeconds;
        }
    }
    return Slots;
}

int32 UInkSaveSubsystem::FindLatestSlot() const
{
    int32 LatestSlot = INDEX_NONE;
    FDateTime LatestTime;
    for (const FInkSaveSlotInfo& Slot : GetSlots())
    {
        if (Slot.bValid && (LatestSlot == INDEX_NONE || Slot.SavedAt > LatestTime))
        {
            LatestSlot = Slot.SlotIndex;
            LatestTime = Slot.SavedAt;
        }
    }
    return LatestSlot;
}
