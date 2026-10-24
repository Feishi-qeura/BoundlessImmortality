#include "LegacyUiRedirectorFixupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "UObject/CoreRedirects.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FLegacyRedirectorTarget
	{
		const TCHAR* PackageName;
		const TCHAR* ObjectName;
		const TCHAR* CanonicalPackageName;
	};

	constexpr FLegacyRedirectorTarget Targets[] =
	{
		{ TEXT("/Game/UI/Ink/Widgets/WBP_InkFrontend"), TEXT("WBP_InkFrontend"), TEXT("/Game/UI/Ink/Widgets/Shared/WBP_InkFrontend") },
		{ TEXT("/Game/UI/Ink/Cohesion20261021/NS_InkTitleAura"), TEXT("NS_InkTitleAura"), TEXT("/Game/UI/Ink/Niagara/Shared/NS_InkTitleAura") },
		{ TEXT("/Game/UI/Ink/Refactor20261021/Fonts/F_InkBody"), TEXT("F_InkBody"), TEXT("/Game/UI/Ink/Fonts/Shared/F_InkBody") },
		{ TEXT("/Game/UI/Ink/Textures/T_InkAttributeIcons"), TEXT("T_InkAttributeIcons"), TEXT("/Game/UI/Ink/Textures/Shared/T_InkAttributeIcons") },
		{ TEXT("/Game/UI/Ink/Textures/T_InkMeditation"), TEXT("T_InkMeditation"), TEXT("/Game/UI/Ink/Textures/Cultivation/T_InkMeditation") },
		{ TEXT("/Game/UI/Ink/Textures/T_InkTitle"), TEXT("T_InkTitle"), TEXT("/Game/UI/Ink/Textures/Shared/T_InkTitle") },
	};

	constexpr int32 TargetCount = UE_ARRAY_COUNT(Targets);
	const TCHAR* ConfirmationToken = TEXT("-ConfirmExactLegacyUiRedirectorFixup");
}

ULegacyUiRedirectorFixupCommandlet::ULegacyUiRedirectorFixupCommandlet()
{
	IsEditor = true;
	IsClient = false;
	IsServer = false;
	LogToConsole = true;
}

bool ULegacyUiRedirectorFixupCommandlet::ParseMode(const FString& Params, bool& bPreflightOnly)
{
	TArray<FString> Arguments;
	Params.ParseIntoArrayWS(Arguments, nullptr, true);
	TArray<FString> Modes;
	for (const FString& Argument : Arguments)
	{
		// UE passes its -run selector through to Main. These flags change only
		// process interaction/rendering; none changes the six-target scope.
		if (Argument.Equals(TEXT("-run=LegacyUiRedirectorFixup"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("-run=LegacyUiRedirectorFixupCommandlet"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("-unattended"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("-NoLiveCoding"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("-nop4"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("-nullrhi"), ESearchCase::IgnoreCase)
			|| Argument.Equals(TEXT("-nosplash"), ESearchCase::IgnoreCase)) continue;
		if (Argument != TEXT("-PreflightOnly") && Argument != ConfirmationToken) return false;
		Modes.Add(Argument);
	}
	if (Modes.Num() != 1) return false;
	bPreflightOnly = Modes[0] == TEXT("-PreflightOnly");
	return true;
}

int32 ULegacyUiRedirectorFixupCommandlet::Main(const FString& Params)
{
	bool bPreflightOnly = false;
	if (!ParseMode(Params, bPreflightOnly))
	{
		UE_LOG(LogTemp, Error, TEXT("Refusing unknown arguments or ambiguous mode; pass exactly one of -PreflightOnly or %s"), ConfirmationToken);
		return 9;
	}

	// PackageRedirects in the project ini resolve these legacy package names
	// before object loading can inspect them. Suspend only the exact six aliases
	// inside this standalone process; never edit the ini or the C++ resolver.
	TArray<FCoreRedirect> ScopedAliases;
	for (const FLegacyRedirectorTarget& Target : Targets)
	{
		TArray<const FCoreRedirect*> Matches;
		FCoreRedirects::GetMatchingRedirects(ECoreRedirectFlags::Type_Package,
			FCoreRedirectObjectName(FString(Target.PackageName)), Matches);
		if (Matches.Num() != 1 || Matches[0]->RedirectFlags != ECoreRedirectFlags::Type_Package
			|| Matches[0]->OldName.PackageName != FName(Target.PackageName)
			|| Matches[0]->NewName.PackageName != FName(Target.CanonicalPackageName))
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing non-exact or missing process alias for %s"), Target.PackageName);
			return 23;
		}
		ScopedAliases.Add(*Matches[0]);
	}
	ON_SCOPE_EXIT
	{
		FCoreRedirects::AddRedirectList(ScopedAliases, TEXT("LegacyUiExactSixRestore"));
		for (const FLegacyRedirectorTarget& Target : Targets)
		{
			const auto Restored = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Package,
				FCoreRedirectObjectName(FString(Target.PackageName)));
			if (Restored.PackageName != FName(Target.CanonicalPackageName))
				UE_LOG(LogTemp, Error, TEXT("Process alias restoration failed: %s"), Target.PackageName);
		}
		UE_LOG(LogTemp, Display, TEXT("Restored six process-only package aliases; project config was not modified"));
	};
	if (!FCoreRedirects::RemoveRedirectList(ScopedAliases, TEXT("LegacyUiExactSixInspect")))
	{
		UE_LOG(LogTemp, Error, TEXT("Refusing to continue: exact process aliases could not be suspended"));
		return 24;
	}
	for (const FLegacyRedirectorTarget& Target : Targets)
	{
		const auto RawName = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Package,
			FCoreRedirectObjectName(FString(Target.PackageName)));
		if (RawName.PackageName != FName(Target.PackageName))
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to continue: %s still has a package alias"), Target.PackageName);
			return 25;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("Suspended exactly six process-only aliases for raw redirector inspection"));

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	Registry.WaitForCompletion();
	TArray<FAssetData> ScannedAssets;
	if (!Registry.GetAllAssets(ScannedAssets, true) || ScannedAssets.Num() < TargetCount)
	{
		UE_LOG(LogTemp, Error, TEXT("Refusing to continue: full on-disk Asset Registry scan returned only %d assets"), ScannedAssets.Num());
		return 14;
	}
	UE_LOG(LogTemp, Display, TEXT("Fresh synchronous full Asset Registry scan completed: %d on-disk assets"), ScannedAssets.Num());
	UE_LOG(LogTemp, Display, TEXT("Reference preflight will cover Package dependency categories for every exact target"));
	const FTopLevelAssetPath RedirectorClassPath = UObjectRedirector::StaticClass()->GetClassPathName();

	TArray<UObjectRedirector*> Redirectors;
	Redirectors.Reserve(TargetCount);

	for (const FLegacyRedirectorTarget& Target : Targets)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), Target.PackageName, Target.ObjectName);
		const FAssetData* ScannedTarget = ScannedAssets.FindByPredicate([&Target, &RedirectorClassPath](const FAssetData& Asset)
		{
			return Asset.PackageName == FName(Target.PackageName)
				&& Asset.AssetName == FName(Target.ObjectName)
				&& Asset.AssetClassPath == RedirectorClassPath;
		});
		if (!ScannedTarget)
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: exact target was not found as an ObjectRedirector in the fresh on-disk scan: %s"), *ObjectPath);
			return 16;
		}

		// A normal load follows the redirector and returns its canonical object.
		// This helper must operate on the exact legacy object, never that target.
		UObjectRedirector* Redirector = LoadObject<UObjectRedirector>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet | LOAD_NoRedirects);
		if (!Redirector || Redirector->GetPathName() != ObjectPath)
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: exact target is missing or is not UObjectRedirector: %s"), *ObjectPath);
			return 11;
		}
		const FString ExpectedDestinationPath = FString::Printf(TEXT("%s.%s"), Target.CanonicalPackageName, Target.ObjectName);
		if (!Redirector->DestinationObject || Redirector->DestinationObject->GetPathName() != ExpectedDestinationPath)
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: %s does not redirect directly to the expected object %s"), *ObjectPath, *ExpectedDestinationPath);
			return 22;
		}

		TArray<FName> Referencers;
		if (!Registry.GetReferencers(FName(Target.PackageName), Referencers, UE::AssetRegistry::EDependencyCategory::All))
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: Asset Registry could not complete the referencer query for %s"), Target.PackageName);
			return 17;
		}
		if (!Referencers.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: %s has %d incoming package referencers"), Target.PackageName, Referencers.Num());
			for (const FName Referencer : Referencers)
			{
				UE_LOG(LogTemp, Error, TEXT("Incoming referencer: %s"), *Referencer.ToString());
			}
			return 12;
		}

		TArray<FName> Dependencies;
		if (!Registry.GetDependencies(FName(Target.PackageName), Dependencies, UE::AssetRegistry::EDependencyCategory::Package))
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: Asset Registry could not complete the dependency query for %s"), Target.PackageName);
			return 18;
		}
		if (Dependencies.Num() != 1 || Dependencies[0] != FName(Target.CanonicalPackageName))
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: %s does not point only to its expected canonical package"), Target.PackageName);
			for (const FName Dependency : Dependencies)
			{
				UE_LOG(LogTemp, Error, TEXT("Observed dependency: %s"), *Dependency.ToString());
			}
			return 15;
		}

		const bool bCanonicalTargetInScan = ScannedAssets.ContainsByPredicate([&Target, &ExpectedDestinationPath](const FAssetData& Asset)
		{
			return Asset.PackageName == FName(Target.CanonicalPackageName)
				&& Asset.AssetName == FName(Target.ObjectName)
				&& Asset.GetSoftObjectPath().ToString() == ExpectedDestinationPath;
		});
		if (!bCanonicalTargetInScan)
		{
			UE_LOG(LogTemp, Error, TEXT("Refusing to run: expected canonical object is missing from the fresh scan: %s"), *ExpectedDestinationPath);
			return 19;
		}

		UE_LOG(LogTemp, Display, TEXT("Preflight passed: %s is UObjectRedirector; incoming referencers=0; canonical dependency=%s"), *ObjectPath, Target.CanonicalPackageName);
		Redirectors.Add(Redirector);
	}

	if (Redirectors.Num() != TargetCount)
	{
		UE_LOG(LogTemp, Error, TEXT("Refusing to run: validated %d of %d targets"), Redirectors.Num(), TargetCount);
		return 13;
	}

	if (bPreflightOnly)
	{
		UE_LOG(LogTemp, Display, TEXT("Read-only preflight passed for exactly %d redirectors; no packages were modified"), Redirectors.Num());
		return 0;
	}
	if (!FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("FixupReferencers requires initialized Editor Slate; use BoundlessInkUI.CleanupLegacyRedirectors in an idle GUI Editor"));
		return 26;
	}

	UE_LOG(LogTemp, Display, TEXT("Calling FixupReferencers once for exactly %d redirectors"), Redirectors.Num());
	FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().FixupReferencers(
		Redirectors,
		false,
		ERedirectFixupMode::DeleteFixedUpRedirectors);

	Registry.SearchAllAssets(true);
	Registry.WaitForCompletion();
	TArray<FAssetData> PostFixupAssets;
	if (!Registry.GetAllAssets(PostFixupAssets, true))
	{
		UE_LOG(LogTemp, Error, TEXT("FixupReferencers returned, but the final full Asset Registry scan failed"));
		return 20;
	}
	for (const FLegacyRedirectorTarget& Target : Targets)
	{
		const bool bOldRedirectorRemains = PostFixupAssets.ContainsByPredicate([&Target](const FAssetData& Asset)
		{
			return Asset.PackageName == FName(Target.PackageName);
		});
		const bool bCanonicalRemains = PostFixupAssets.ContainsByPredicate([&Target](const FAssetData& Asset)
		{
			return Asset.PackageName == FName(Target.CanonicalPackageName)
				&& Asset.AssetName == FName(Target.ObjectName)
				&& Asset.AssetClassPath != UObjectRedirector::StaticClass()->GetClassPathName();
		});
		if (bOldRedirectorRemains || !bCanonicalRemains)
		{
			UE_LOG(LogTemp, Error, TEXT("Post-fixup verification failed for %s: old package remains=%s, canonical package remains=%s"),
				Target.PackageName,
				bOldRedirectorRemains ? TEXT("true") : TEXT("false"),
				bCanonicalRemains ? TEXT("true") : TEXT("false"));
			return 21;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("FixupReferencers returned; final full on-disk scan confirms all six old packages are absent and canonical assets remain"));
	return 0;
}

namespace
{
void RunLegacyUiGuiCleanup()
{
	if (!GEditor || GEditor->PlayWorld || !FSlateApplication::IsInitialized())
	{
		UE_LOG(LogTemp, Error, TEXT("Legacy UI cleanup refused: requires idle Editor with initialized Slate and no PIE"));
		return;
	}
	TStrongObjectPtr<ULegacyUiRedirectorFixupCommandlet> Helper(NewObject<ULegacyUiRedirectorFixupCommandlet>());
	const int32 Result = Helper->Main(TEXT("-ConfirmExactLegacyUiRedirectorFixup"));
	UE_LOG(LogTemp, Display, TEXT("Exact legacy UI GUI cleanup finished: exit_code=%d"), Result);
	if (FParse::Param(FCommandLine::Get(), TEXT("InkLegacyCleanupExit")))
		FPlatformMisc::RequestExit(false);
}

FAutoConsoleCommand LegacyUiGuiCleanupCommand(
	TEXT("BoundlessInkUI.CleanupLegacyRedirectors"),
	TEXT("Fix only six verified legacy UI redirectors in an idle Editor, with full preflight and post-scan."),
	FConsoleCommandDelegate::CreateStatic(&RunLegacyUiGuiCleanup));
}
