#include "ScopedKnownRenameWarning.h"
#include "CoreGlobals.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"

namespace
{
const TCHAR* ExpectedNames[] = {
    TEXT("T_HudLaunchCountdownFrame_v1"), TEXT("T_InkTitle"), TEXT("T_InkMeditation"), TEXT("T_InkAttributeIcons"),
    TEXT("T_ProvidedButtons"), TEXT("T_ProvidedRank"), TEXT("T_ProvidedInventory"), TEXT("M_InkTitleFeather"),
    TEXT("M_InkScrollFade"), TEXT("F_InkBody"), TEXT("NS_InkTitleAura")
};

bool MatchesObservedWarningOnly(const FText& Message)
{
    FString Body = Message.ToString().Replace(TEXT("\r\n"), TEXT("\n"));
    TArray<FString> Lines;
    Body.ParseIntoArrayLines(Lines, true);
    for (FString& L : Lines) L.TrimStartAndEndInline();
    Lines.RemoveAll([](const FString& L) { return L.IsEmpty(); });
    if (Lines.Num() != UE_ARRAY_COUNT(ExpectedNames) + 2 ||
        Lines[0] != TEXT("源代码、配置INI和文本文件可能需要查找/替换：") ||
        Lines.Last() != TEXT("否则烘焙版本可能缺失资产。继续重命名？")) return false;
    TSet<FString> Remaining;
    for (const TCHAR* Name : ExpectedNames) Remaining.Add(Name);
    for (int32 I = 1; I < Lines.Num() - 1; ++I) if (Remaining.Remove(Lines[I]) != 1) return false;
    return Remaining.IsEmpty();
}
}

FKnownRenameWarningResult Rename582WithScopedKnownWarningAck(
    IAssetTools& AssetTools, const TArray<FAssetRenameData>& Exact582)
{
    FKnownRenameWarningResult Result;
    if (FParse::Param(FCommandLine::Get(), TEXT("unattended")))
    {
        Result.Refusal = TEXT("Process has -unattended: dialog default may return before scoped delegate. Do not retry in this process.");
        return Result;
    }
    if (Exact582.Num() != 582 || AssetTools.IsDiscoveringAssetsInProgress())
    {
        Result.Refusal = TEXT("Exact 582 canonical rows and completed discovery required.");
        return Result;
    }
    TSet<FString> Names, Destinations;
    for (const FAssetRenameData& R : Exact582)
    {
        UObject* A = R.Asset.Get();
        if (!A || A->IsA<UObjectRedirector>() || R.bOnlyFixSoftReferences ||
            !A->GetOutermost()->GetName().StartsWith(TEXT("/Game/UI/")) ||
            !R.NewPackagePath.StartsWith(TEXT("/Game/newUI/")) || R.NewName != A->GetName() ||
            Destinations.Contains(R.NewPackagePath.ToLower() + TEXT("/") + R.NewName.ToLower()))
        {
            Result.Refusal = TEXT("Not a valid canonical-only exact row; preserve caller preflight/pin/backup checks.");
            return Result;
        }
        Names.Add(A->GetName());
        Destinations.Add(R.NewPackagePath.ToLower() + TEXT("/") + R.NewName.ToLower());
    }
    for (const TCHAR* Name : ExpectedNames) if (!Names.Contains(Name))
    {
        Result.Refusal = TEXT("Observed warning asset name is outside exact rename batch.");
        return Result;
    }
    // Python execution may independently mark a script unattended; restore it immediately after this one native call.
    TGuardValue<bool> RestoreScriptState(GIsRunningUnattendedScript, false);
    if (FApp::IsUnattended())
    {
        Result.Refusal = TEXT("Application remains unattended; scoped modal delegate cannot be assumed effective.");
        return Result;
    }
    const auto PreviousHandler = FCoreDelegates::ModalMessageDialog;
    ON_SCOPE_EXIT { FCoreDelegates::ModalMessageDialog = PreviousHandler; };
    FCoreDelegates::ModalMessageDialog.BindLambda([&Result](EAppMsgCategory Category, EAppMsgType::Type Type,
        const FText& Message, const FText& Title) -> EAppReturnType::Type
    {
        Result.DialogEvidence.Add(FString::Printf(TEXT("type=%d category=%d title=%s text=%s"),
            int32(Type), int32(Category), *Title.ToString(), *Message.ToString()));
        if (Type == EAppMsgType::OkCancel && MatchesObservedWarningOnly(Message))
        {
            ++Result.AcknowledgedKnownWarnings;
            return EAppReturnType::Ok;
        }
        ++Result.RejectedUnknownDialogs;
        return EAppReturnType::Cancel; // No broad acceptance and no fallback interactive prompt.
    });
    Result.bRenameSucceeded = AssetTools.RenameAssets(Exact582);
    return Result;
}
