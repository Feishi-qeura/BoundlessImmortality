#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InkSaveSubsystem.generated.h"

class APawn;
class UWorld;

/** The first save format stores only the player's position, orientation, and elapsed gameplay. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    // Invalid defaults keep incomplete/foreign saves from being treated as playable progress.
    UPROPERTY(SaveGame) int32 SaveVersion = 0;
    UPROPERTY(SaveGame) FString MapName;
    UPROPERTY(SaveGame) FTransform PawnTransform = FTransform::Identity;
    UPROPERTY(SaveGame) FDateTime SavedAt;
    UPROPERTY(SaveGame) double PlayedSeconds = 0;
};

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FInkSaveSlotInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") int32 SlotIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") bool bExists = false;
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") bool bValid = false;
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") FString DisplayName;
    /** UTC timestamp; the UI can convert this to local time for display. */
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") FDateTime SavedAt;
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") FString MapName;
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") double PlayedSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="水墨前端|存档") FString ValidationError;
};

/** Three local, explicit manual slots for the currently supported Sky Temple map. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    static constexpr int32 ManualSlotCount = 3;
    static constexpr int32 CurrentSaveVersion = 1;

    /** Register the restored player. The same pawn/world resumes its accumulated play time. */
    UFUNCTION(BlueprintCallable, Category="水墨前端|存档") bool BeginGameplay(APawn* Pawn);
    /** Revoke saving/loading before presenting the menu's demonstration pawn. */
    UFUNCTION(BlueprintCallable, Category="水墨前端|存档") void EndGameplay();

    /** Slot indices are zero based, in [0, 2]. Saving is always explicit. */
    UFUNCTION(BlueprintCallable, Category="水墨前端|存档") bool SaveSlot(int32 Slot, APawn* Pawn);
    UFUNCTION(BlueprintCallable, Category="水墨前端|存档") bool LoadSlot(int32 Slot, APawn* Pawn);
    UFUNCTION(BlueprintCallable, Category="水墨前端|存档") bool DeleteSlot(int32 Slot);
    UFUNCTION(BlueprintPure, Category="水墨前端|存档") TArray<FInkSaveSlotInfo> GetSlots() const;
    /** INDEX_NONE when no valid slot exists. Corrupt and unsupported slots are excluded. */
    UFUNCTION(BlueprintPure, Category="水墨前端|存档") int32 FindLatestSlot() const;

    UPROPERTY(BlueprintReadOnly, Transient, Category="水墨前端|存档") FString LastError;

    // Native helpers also allow integrity/compatibility tests without accessing player saves.
    static bool IsValidSlotIndex(int32 Slot);
    static FString NormalizeMapName(const FString& MapName);
    static bool ValidateSaveGame(const UInkSaveGame* Save, FString& OutError);
    static bool EncodeSave(UInkSaveGame* Save, TArray<uint8>& OutBytes, FString& OutError);
    static UInkSaveGame* DecodeSave(const TArray<uint8>& Bytes, FString& OutError);

private:
    friend class FInkSaveStorageTest;
    friend class FInkSaveSessionTest;

    FString SlotPrefix = TEXT("BoundlessImmortality_Ink_Manual_");
    TWeakObjectPtr<APawn> GameplayPawn;
    TWeakObjectPtr<UWorld> GameplayWorld;
    double SessionStartedAt = 0;
    double StoredPlayedSeconds = 0;
    bool bGameplaySessionActive = false;

    FString StorageSlotName(int32 Slot) const;
    FString BackupSlotName(int32 Slot) const;
    bool ValidatePawn(APawn* Pawn, bool bRequireRegisteredSession);
    bool CheckSlotIndex(int32 Slot);
    void ActivateGameplaySession(APawn* Pawn, UWorld* World, double WorldTime);
    void FreezeGameplaySession(double WorldTime);
    double GetPlayedSeconds(double WorldTime) const;
    bool WriteSlotBytes(int32 Slot, const TArray<uint8>& Bytes);
    UInkSaveGame* ReadStorageSlot(const FString& SlotName, FString& OutError) const;
    UInkSaveGame* ReadSlot(int32 Slot, FString& OutError) const;
};
