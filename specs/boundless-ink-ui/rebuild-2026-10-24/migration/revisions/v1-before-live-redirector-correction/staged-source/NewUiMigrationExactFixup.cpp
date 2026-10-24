#include "NewUiMigrationExactFixup.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/CoreRedirects.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FString TeamDir()
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() /
        TEXT("specs/boundless-ink-ui/rebuild-2026-10-24/migration"));
}

bool ReadJson(const FString& Path, TSharedPtr<FJsonObject>& Out)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text, *Path) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Out) && Out.IsValid();
}

void WriteReport(const TSharedRef<FJsonObject>& Report)
{
    FString Text;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
    FFileHelper::SaveStringToFile(Text, *(TeamDir() / TEXT("engine-fixup-result.json")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool Fail(const TSharedRef<FJsonObject>& Report, const FString& Message)
{
    Report->SetBoolField(TEXT("passed"), false);
    Report->SetStringField(TEXT("error"), Message);
    WriteReport(Report);
    UE_LOG(LogTemp, Error, TEXT("newUI exact fixup refused/failed: %s"), *Message);
    return false;
}

bool ValidLease()
{
    TSharedPtr<FJsonObject> Lease;
    if (!ReadJson(TeamDir() / TEXT("../engine-lease.json"), Lease)) return false;
    FString Owner, State, Sole, Expiry;
    if (!Lease->TryGetStringField(TEXT("owner_team"), Owner) || Owner != TEXT("logic-integration") ||
        !Lease->TryGetStringField(TEXT("sole_engine_and_content_executor"), Sole) || Sole != Owner ||
        !Lease->TryGetStringField(TEXT("state"), State) || (State != TEXT("granted") && State != TEXT("active")) ||
        !Lease->TryGetStringField(TEXT("expires_at"), Expiry)) return false;
    FDateTime End;
    if (!FDateTime::ParseIso8601(*Expiry, End) || FDateTime::UtcNow() >= End) return false;
    const TArray<TSharedPtr<FJsonValue>>* Ops = nullptr;
    if (!Lease->TryGetArrayField(TEXT("allowed_operations"), Ops)) return false;
    return Ops->ContainsByPredicate([](const TSharedPtr<FJsonValue>& V) { return V->AsString() == TEXT("exact_old_redirector_fixup"); });
}
}

bool NewUiMigration20261024::FixupExactMigratedRedirectors()
{
    const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("passed"), false);
    Report->SetNumberField(TEXT("engine_pid"), FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("checked_at"), FDateTime::UtcNow().ToIso8601());
    if (!ValidLease() || !GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized())
        return Fail(Report, TEXT("Requires rebuild integration lease and idle GUI Editor with Slate; no PIE."));
    TArray<UPackage*> Dirty;
    TArray<UPackage*> DirtyMaps;
    UEditorLoadingAndSavingUtils::GetDirtyContentPackages(Dirty);
    UEditorLoadingAndSavingUtils::GetDirtyMapPackages(DirtyMaps);
    Dirty.Append(DirtyMaps);
    if (!Dirty.IsEmpty()) return Fail(Report, TEXT("Dirty packages exist; save explicit migrated/referencer packages first."));

    TSharedPtr<FJsonObject> Map, Preflight, Journal, Backup;
    if (!ReadJson(TeamDir() / TEXT("migration-map.json"), Map) ||
        !ReadJson(TeamDir() / TEXT("engine-preflight.json"), Preflight) ||
        !ReadJson(TeamDir() / TEXT("engine-migration-journal.json"), Journal))
        return Fail(Report, TEXT("Missing exact map, live preflight or rename journal."));
    if (!Preflight->GetBoolField(TEXT("passed")) || Journal->GetStringField(TEXT("state")) != TEXT("renamed_complete") ||
        Journal->GetIntegerField(TEXT("engine_pid")) != int32(FPlatformProcess::GetCurrentProcessId()) ||
        Preflight->GetIntegerField(TEXT("engine_pid")) != int32(FPlatformProcess::GetCurrentProcessId()) ||
        Journal->GetStringField(TEXT("map_sha256")) != Preflight->GetStringField(TEXT("migration_map_sha256")))
        return Fail(Report, TEXT("Preflight and complete rename must be for this engine process and the same exact map."));
    const FString BackupPath = Map->GetStringField(TEXT("backup_manifest"));
    const FString BackupRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Saved/Codex/UIRebuild20261024/Backups"));
    if (!FPaths::IsUnderDirectory(BackupPath, BackupRoot) || !ReadJson(BackupPath, Backup) || !Backup->GetBoolField(TEXT("verified")))
        return Fail(Report, TEXT("No verified backup manifest inside authorized Saved backup root."));

    TSet<FName> BackedPackages;
    for (const TSharedPtr<FJsonValue>& V : Backup->GetArrayField(TEXT("files")))
    {
        const TSharedPtr<FJsonObject> Row = V->AsObject();
        FString Package;
        if (Row->TryGetStringField(TEXT("package"), Package) && !Package.IsEmpty())
        {
            if (!IFileManager::Get().FileExists(*Row->GetStringField(TEXT("backup"))))
                return Fail(Report, TEXT("Archived package file missing."));
            BackedPackages.Add(FName(Package));
        }
    }
    struct FMove { FString OldPackage, NewPackage, ObjectName; };
    TArray<FMove> Moves;
    TSet<FName> OldNames, NewNames;
    for (const TSharedPtr<FJsonValue>& V : Map->GetArrayField(TEXT("assets")))
    {
        const TSharedPtr<FJsonObject> Row = V->AsObject();
        FMove Move{Row->GetStringField(TEXT("old_package")), Row->GetStringField(TEXT("new_package")), Row->GetStringField(TEXT("asset_name"))};
        if (!Move.OldPackage.StartsWith(TEXT("/Game/UI/")) ||
            Move.NewPackage != TEXT("/Game/newUI") + Move.OldPackage.Mid(8) ||
            OldNames.Contains(FName(Move.OldPackage)) || NewNames.Contains(FName(Move.NewPackage)) ||
            !BackedPackages.Contains(FName(Move.OldPackage)))
            return Fail(Report, TEXT("Invalid, duplicate, out-of-scope or unbacked exact move row."));
        OldNames.Add(FName(Move.OldPackage));
        NewNames.Add(FName(Move.NewPackage));
        Moves.Add(Move);
    }
    if (Moves.Num() != Map->GetIntegerField(TEXT("count")) || Moves.Num() != Journal->GetIntegerField(TEXT("exact_count")))
        return Fail(Report, TEXT("Exact map membership differs from rename count."));

    // Suspend only exact process aliases for migrated canonical old packages. No ini changes.
    TArray<FCoreRedirect> ScopedAliases;
    for (const FMove& Move : Moves)
    {
        TArray<const FCoreRedirect*> Found;
        FCoreRedirects::GetMatchingRedirects(ECoreRedirectFlags::Type_Package, FCoreRedirectObjectName(Move.OldPackage), Found);
        for (const FCoreRedirect* Alias : Found)
        {
            if (Alias->RedirectFlags != ECoreRedirectFlags::Type_Package || Alias->OldName.PackageName != FName(Move.OldPackage))
                return Fail(Report, TEXT("A broad/non-exact package redirect affects a source; inspect before fixup."));
            ScopedAliases.Add(*Alias);
        }
    }
    if (!ScopedAliases.IsEmpty() && !FCoreRedirects::RemoveRedirectList(ScopedAliases, TEXT("NewUIExactMigratedInspect")))
        return Fail(Report, TEXT("Could not suspend exact process aliases."));
    ON_SCOPE_EXIT
    {
        if (!ScopedAliases.IsEmpty()) FCoreRedirects::AddRedirectList(ScopedAliases, TEXT("NewUIExactMigratedRestore"));
    };
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    Registry.WaitForCompletion();
    TArray<FAssetData> All;
    if (!Registry.GetAllAssets(All, true)) return Fail(Report, TEXT("Full on-disk registry query failed."));
    TArray<UObjectRedirector*> Redirectors;
    TArray<TStrongObjectPtr<UObjectRedirector>> KeepAlive;
    TArray<TSharedPtr<FJsonValue>> Inspected;
    for (const FMove& Move : Moves)
    {
        if (!ValidLease()) return Fail(Report, TEXT("Execution lease expired before fixup."));
        const FAssetData* Destination = All.FindByPredicate([&](const FAssetData& A) {
            return A.PackageName == FName(Move.NewPackage) && A.AssetName == FName(Move.ObjectName) &&
                A.AssetClassPath != UObjectRedirector::StaticClass()->GetClassPathName(); });
        if (!Destination || !FPackageName::DoesPackageExist(Move.NewPackage))
            return Fail(Report, TEXT("Saved canonical destination missing: ") + Move.NewPackage);
        const FAssetData* Source = All.FindByPredicate([&](const FAssetData& A) { return A.PackageName == FName(Move.OldPackage); });
        if (!Source)
        {
            if (FPackageName::DoesPackageExist(Move.OldPackage))
                return Fail(Report, TEXT("Old package exists outside registry: ") + Move.OldPackage);
            continue; // Rename may already remove a source with no retained redirector.
        }
        if (Source->AssetClassPath != UObjectRedirector::StaticClass()->GetClassPathName())
            return Fail(Report, TEXT("Mapped old package is still a real asset: ") + Move.OldPackage);
        const FString OldObject = Move.OldPackage + TEXT(".") + Move.ObjectName;
        const FString NewObject = Move.NewPackage + TEXT(".") + Move.ObjectName;
        UObjectRedirector* Redirector = LoadObject<UObjectRedirector>(nullptr, *OldObject, nullptr, LOAD_NoWarn | LOAD_Quiet | LOAD_NoRedirects);
        if (!Redirector || Redirector->GetPathName() != OldObject || !Redirector->DestinationObject ||
            Redirector->DestinationObject->GetPathName() != NewObject)
            return Fail(Report, TEXT("Raw redirector identity/destination mismatch: ") + OldObject);
        TArray<FName> Incoming;
        Registry.GetReferencers(FName(Move.OldPackage), Incoming, UE::AssetRegistry::EDependencyCategory::All);
        TArray<TSharedPtr<FJsonValue>> RefJson;
        for (FName Name : Incoming)
        {
            if (!BackedPackages.Contains(Name) && !NewNames.Contains(Name))
                return Fail(Report, TEXT("Unbacked incoming referencer: ") + Name.ToString());
            RefJson.Add(MakeShared<FJsonValueString>(Name.ToString()));
        }
        const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("old_object"), OldObject);
        Row->SetStringField(TEXT("destination_object"), NewObject);
        Row->SetArrayField(TEXT("incoming_before_fixup"), RefJson);
        Inspected.Add(MakeShared<FJsonValueObject>(Row));
        KeepAlive.Emplace(Redirector);
        Redirectors.Add(Redirector);
    }
    Report->SetArrayField(TEXT("inspected_exact_redirectors"), Inspected);
    Report->SetNumberField(TEXT("exact_redirector_count"), Redirectors.Num());
    Report->SetNumberField(TEXT("mapped_count"), Moves.Num());
    WriteReport(Report); // Durable before the only redirector-deleting API call.
    if (!Redirectors.IsEmpty()) FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().FixupReferencers(
        Redirectors, false, ERedirectFixupMode::DeleteFixedUpRedirectors);
    Registry.ScanPathsSynchronous({TEXT("/Game/UI"), TEXT("/Game/newUI")}, true, true);
    Registry.WaitForCompletion();
    TArray<FAssetData> After;
    if (!Registry.GetAllAssets(After, true)) return Fail(Report, TEXT("Post-fixup registry query failed."));
    int32 OldRemaining = 0, Canonical = 0;
    for (const FAssetData& A : After)
    {
        const FString Name = A.PackageName.ToString();
        if (Name == TEXT("/Game/UI") || Name.StartsWith(TEXT("/Game/UI/"))) ++OldRemaining;
        if (NewNames.Contains(A.PackageName) && A.AssetClassPath != UObjectRedirector::StaticClass()->GetClassPathName()) ++Canonical;
    }
    Report->SetNumberField(TEXT("old_registry_package_count"), OldRemaining);
    Report->SetNumberField(TEXT("canonical_destination_count"), Canonical);
    if (OldRemaining != 0 || Canonical != Moves.Num()) return Fail(Report, TEXT("Post-fixup old-root/destination membership proof failed."));
    Report->SetBoolField(TEXT("passed"), true);
    Report->SetStringField(TEXT("checked_at"), FDateTime::UtcNow().ToIso8601());
    WriteReport(Report);
    UE_LOG(LogTemp, Display, TEXT("UI_TO_NEWUI_EXACT_FIXUP_PASSED mapped=%d fixed_redirectors=%d old_root=0"), Moves.Num(), Redirectors.Num());
    return true;
}

namespace
{
void RunNewUiExactFixup() { NewUiMigration20261024::FixupExactMigratedRedirectors(); }
FAutoConsoleCommand NewUiExactFixupCommand(TEXT("BoundlessNewUI.FixupExactMigrationRedirectors"),
    TEXT("Fix only exact migrated UI old-path redirectors from backed-up migration map; integration lease required."),
    FConsoleCommandDelegate::CreateStatic(&RunNewUiExactFixup));
}
