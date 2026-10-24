#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Frontend/InkFeaturePanel.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkMenuDirector.h"
#include "InkFrontendEditorLibrary.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Cultivation/CultivationPrototypeClient.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Components/TextBlock.h"
#include "Widgets/SWindow.h"

namespace InkSafePieRouteAcceptance
{
struct FRouteCase
{
	const TCHAR* ContractId;
	const TCHAR* RequestedRoute;
	const TCHAR* ExpectedPage;
	const TCHAR* ExpectedPanelClass;
	const TCHAR* Context;
	const TCHAR* ExpectedGroupRoute;
	const TCHAR* Availability;
};

// These are the 44 contract rows in evidence/2026-10-23-native-route-matrix.md.
// The structured player-entry table supplies actions, safety, and final page;
// this local table supplies the contract id and expected native panel class.
static const FRouteCase RouteCases[] = {
	{TEXT("main"), TEXT("Main"), TEXT("Main"), TEXT(""), TEXT(""), TEXT(""), TEXT("native")},
	{TEXT("home"), TEXT("HUD"), TEXT("HUD"), TEXT(""), TEXT(""), TEXT(""), TEXT("native")},
	{TEXT("character"), TEXT("CharacterProfile"), TEXT("CharacterProfile"), TEXT("UInkCharacterProfilePanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("inventory"), TEXT("Inventory"), TEXT("Inventory"), TEXT(""), TEXT(""), TEXT(""), TEXT("native")},
	{TEXT("leaderboard"), TEXT("Leaderboard"), TEXT("Leaderboard"), TEXT(""), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("saves"), TEXT("Saves"), TEXT("Saves"), TEXT(""), TEXT(""), TEXT(""), TEXT("native")},
	{TEXT("settings"), TEXT("Settings"), TEXT("Settings"), TEXT(""), TEXT(""), TEXT(""), TEXT("native")},
	{TEXT("wuling"), TEXT("Wuling"), TEXT("Wuling"), TEXT("UInkWulingSectPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("sword-spirit"), TEXT("SwordSpirit"), TEXT("WulingRights"), TEXT("UInkWulingRecordsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("gift-rules"), TEXT("GiftRules"), TEXT("GiftRules"), TEXT("UInkGiftRulesPanel"), TEXT(""), TEXT(""), TEXT("missing-authority-api")},
	{TEXT("methods"), TEXT("CultivationMethods"), TEXT("WulingEquipment"), TEXT("UInkWulingToolsPanel"), TEXT("methods"), TEXT(""), TEXT("authority-dependent")},
	{TEXT("race-lobby"), TEXT("RaceLobby"), TEXT("WulingRace"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("race-live"), TEXT("RaceLive"), TEXT("WulingRace"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("race-result"), TEXT("RaceSettlement"), TEXT("WulingRace"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("lease"), TEXT("ResourceLease"), TEXT("WulingRace"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("world-result"), TEXT("EventSettlement"), TEXT("WulingWorld"), TEXT("UInkWulingPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("offline"), TEXT("OfflineRewards"), TEXT("WulingOffline"), TEXT("UInkWulingRecordsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("shop"), TEXT("Shop"), TEXT("WulingShop"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("market"), TEXT("Market"), TEXT("WulingMarket"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("orders"), TEXT("TradeOrders"), TEXT("WulingOrders"), TEXT("UInkWulingRecordsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("world"), TEXT("WorldCultivation"), TEXT("WorldCultivation"), TEXT("UInkJourneyMapPanel"), TEXT(""), TEXT(""), TEXT("metadata-only")},
	{TEXT("region"), TEXT("RegionDetail"), TEXT("RegionDetail"), TEXT("UInkJourneyDestinationPanel"), TEXT(""), TEXT(""), TEXT("metadata-only")},
	{TEXT("resources"), TEXT("ResourceContest"), TEXT("ResourceContest"), TEXT("UInkResourceContestPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("resource-detail"), TEXT("ResourceDetail"), TEXT("ResourceDetail"), TEXT("UInkJourneyDestinationPanel"), TEXT(""), TEXT(""), TEXT("metadata-only")},
	{TEXT("sign-in"), TEXT("SignIn"), TEXT("SignIn"), TEXT("UInkSignInGroupPanel"), TEXT(""), TEXT("SignIn"), TEXT("authority-dependent")},
	{TEXT("monthly-sign-in"), TEXT("MonthlySignIn"), TEXT("SignIn"), TEXT("UInkSignInGroupPanel"), TEXT(""), TEXT("MonthlySignIn"), TEXT("missing-claim-fields")},
	{TEXT("daily-verse"), TEXT("DailyFortune"), TEXT("SignIn"), TEXT("UInkSignInGroupPanel"), TEXT(""), TEXT("DailyFortune"), TEXT("missing-authority-api")},
	{TEXT("activities"), TEXT("Activities"), TEXT("Activities"), TEXT("UInkActivitiesGroupPanel"), TEXT(""), TEXT("DailyActivities"), TEXT("authority-dependent")},
	{TEXT("daily-activities"), TEXT("DailyActivities"), TEXT("Activities"), TEXT("UInkActivitiesGroupPanel"), TEXT(""), TEXT("DailyActivities"), TEXT("authority-dependent")},
	{TEXT("timed-activities"), TEXT("TimedActivities"), TEXT("Activities"), TEXT("UInkActivitiesGroupPanel"), TEXT(""), TEXT("TimedActivities"), TEXT("authority-dependent")},
	{TEXT("daily-detail"), TEXT("DailyActivityDetail"), TEXT("DailyActivityDetail"), TEXT("UInkDailyActivityDetailPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("timed-detail"), TEXT("TimedActivityDetail"), TEXT("TimedActivityDetail"), TEXT("UInkTimedActivityDetailPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("launch"), TEXT("LaunchEvent"), TEXT("LaunchEvent"), TEXT("UInkLaunchEventPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("launch-rank"), TEXT("LaunchRank"), TEXT("LaunchRank"), TEXT("UInkLaunchEventPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("breakthrough"), TEXT("BreakthroughPrepare"), TEXT("WulingBreakthrough"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("missing-authority-fields")},
	{TEXT("tribulation"), TEXT("Tribulation"), TEXT("WulingBreakthrough"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("missing-authority-fields")},
	{TEXT("item-sources"), TEXT("ItemSources"), TEXT("WulingSources"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("guild-build"), TEXT("GuildConstruction"), TEXT("WulingConstruction"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("equipment"), TEXT("EquipmentFabao"), TEXT("CharacterEquipment"), TEXT("UInkCharacterEquipmentPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("journey"), TEXT("Extensions"), TEXT("Extensions"), TEXT("UInkJourneyMapPanel"), TEXT(""), TEXT(""), TEXT("metadata-only")},
	{TEXT("craft"), TEXT("WulingCraft"), TEXT("WulingCraft"), TEXT("UInkWulingToolsPanel"), TEXT(""), TEXT(""), TEXT("authority-dependent")},
	{TEXT("appearance"), TEXT("CharacterAppearance"), TEXT("CharacterAppearance"), TEXT("UInkCharacterAppearancePanel"), TEXT(""), TEXT(""), TEXT("local-presentation")},
	{TEXT("resource-rank"), TEXT("ResourceRanking"), TEXT("ResourceRanking"), TEXT("UInkResourceRankingPanel"), TEXT(""), TEXT(""), TEXT("missing-ranking-api")},
	{TEXT("resource-rewards"), TEXT("ResourceRewards"), TEXT("ResourceRewards"), TEXT("UInkResourceRewardsPanel"), TEXT(""), TEXT(""), TEXT("missing-receipt-api")},
};

struct FRouteResult
{
	FString ContractId;
	FString RequestedRoute;
	FString ExpectedPage;
	FString ActualPage;
	FString ActualPanelClass;
	FString Availability;
	FString Status;
	FString Screenshot;
	FString WindowScreenshot;
	FString Detail;
	FString EntryMethod;
	FString WidgetSnapshotJson;
	FString ReturnTarget;
	FString RouteOriginPage;
	bool bReturnVerified = false;
	bool bPlayerEntryVerified = false;
	FString PlayerEntryActions;
	FString PlayerEntrySafety;
	FString FinalSelectedState;
	TArray<TSharedPtr<FJsonValue>> ActionTrace;
	FVector2D ViewportSize = FVector2D::ZeroVector;
};

struct FPlayerEntryCase
{
	int32 ContractIndex = INDEX_NONE;
	FString Actions;
	FString Safety;
	FString FinalPage;
	FString FinalState;
};

struct FRunState
{
	bool bActive = false;
	bool bWaitingForPIE = false;
	bool bOwnsPIE = false;
	int32 RouteIndex = 0;
	double StartedSeconds = 0;
	double NextActionSeconds = 0;
	FString RunId;
	FDateTime StartedAtUtc;
	FString OutputDirectory;
	TWeakObjectPtr<UInkFrontendWidget> Frontend;
	TArray<FRouteResult> Results;
	TArray<FPlayerEntryCase> PlayerEntries;
	int32 ActionIndex = 0;
	FString RouteOriginPage;
	FString PendingActionId;
	FString PendingFromPage;
	uint32 PendingActionSerial = 0;
	double PendingActionStartedSeconds = 0;
	bool bActionPending = false;
	bool bScreenshotsCaptured = false;
	bool bPathVerified = false;
	FTSTicker::FDelegateHandle TickerHandle;
};

static FRunState Run;

// RequestEndPlayMap is deferred. Let Editor tear down PIE and its subsystems
// before requesting process exit; exiting in the same tick skips CleanupWorld.
static void ExitHarnessAfterPIE()
{
	if (GEditor && GEditor->PlayWorld) GEditor->RequestEndPlayMap();
	const double RequestedAt = FPlatformTime::Seconds();
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RequestedAt, SettledAt = 0.0](float) mutable
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - RequestedAt > 30.0)
		{
			UE_LOG(LogTemp, Error, TEXT("UI harness refused Editor exit: PIE teardown did not settle within 30 seconds"));
			return false;
		}
		if (GEditor && GEditor->PlayWorld) { SettledAt = 0.0; return true; }
		if (SettledAt == 0.0) { SettledAt = Now; return true; }
		if (Now - SettledAt < 1.0) return true;
		UE_LOG(LogTemp, Display, TEXT("UI harness PIE teardown settled; requesting normal Editor exit"));
		FPlatformMisc::RequestExit(false);
		return false;
	}), 0.1f);
}

static TSharedRef<FJsonObject> MakeResultJson(const FRouteResult& Result)
{
	auto Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("contract_id"), Result.ContractId);
	Object->SetStringField(TEXT("requested_route"), Result.RequestedRoute);
	Object->SetStringField(TEXT("expected_current_page"), Result.ExpectedPage);
	Object->SetStringField(TEXT("actual_current_page"), Result.ActualPage);
	Object->SetStringField(TEXT("actual_panel_class"), Result.ActualPanelClass);
	Object->SetStringField(TEXT("availability_boundary"), Result.Availability);
	Object->SetStringField(TEXT("status"), Result.Status);
	Object->SetStringField(TEXT("entry_method"), Result.EntryMethod);
	Object->SetBoolField(TEXT("player_entry_verified"), Result.bPlayerEntryVerified);
	Object->SetStringField(TEXT("player_entry_actions"), Result.PlayerEntryActions);
	Object->SetStringField(TEXT("player_entry_safety"), Result.PlayerEntrySafety);
	Object->SetStringField(TEXT("expected_selected_route_or_state"), Result.FinalSelectedState);
	Object->SetArrayField(TEXT("action_trace"), Result.ActionTrace);
	Object->SetStringField(TEXT("screenshot"), Result.Screenshot);
	Object->SetStringField(TEXT("window_screenshot"), Result.WindowScreenshot);
	Object->SetStringField(TEXT("detail"), Result.Detail);
	Object->SetStringField(TEXT("widget_snapshot_json"), Result.WidgetSnapshotJson);
	Object->SetStringField(TEXT("return_target"), Result.ReturnTarget);
	Object->SetStringField(TEXT("route_origin_page"), Result.RouteOriginPage);
	Object->SetBoolField(TEXT("return_verified"), Result.bReturnVerified);
	Object->SetNumberField(TEXT("viewport_width"), Result.ViewportSize.X);
	Object->SetNumberField(TEXT("viewport_height"), Result.ViewportSize.Y);
	return Object;
}

static void SaveReport(const FString& FinalStatus)
{
	auto Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("boundless-ink-safe-pie-route-report-v2"));
	Report->SetStringField(TEXT("run_id"), Run.RunId);
	Report->SetStringField(TEXT("started_utc"), Run.StartedAtUtc.ToIso8601());
	Report->SetStringField(TEXT("status"), FinalStatus);
	Report->SetStringField(TEXT("entry_scope"), TEXT("Player-entry reachability is asserted only for exact allowlisted Slate navigation clicks; blocked_entry and resolver probes are not passes"));
	Report->SetStringField(TEXT("mutation_policy"), TEXT("deny-by-default; only exact audited navigation and local read-only tab actions may be Slate-clicked; no sign-in claim, purchase, breakthrough, guild mutation, race, save/load, settings apply or authority command"));
	Report->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
	Report->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
	Report->SetStringField(TEXT("output_directory"), Run.OutputDirectory);
	Report->SetNumberField(TEXT("contract_route_count"), UE_ARRAY_COUNT(RouteCases));
	Report->SetNumberField(TEXT("attempted_count"), Run.Results.Num());
	int32 Opened = 0, ExpectedUnavailable = 0, BlockedEntry = 0, Blocked = 0, Failed = 0;
	TArray<TSharedPtr<FJsonValue>> Rows;
	for (const FRouteResult& Result : Run.Results)
	{
		if (Result.Status == TEXT("opened")) ++Opened;
		else if (Result.Status == TEXT("expected_unavailable")) ++ExpectedUnavailable;
		else if (Result.Status == TEXT("blocked_entry")) ++BlockedEntry;
		else if (Result.Status == TEXT("blocked")) ++Blocked;
		else if (Result.Status == TEXT("failed")) ++Failed;
		Rows.Add(MakeShared<FJsonValueObject>(MakeResultJson(Result)));
	}
	Report->SetNumberField(TEXT("opened_count"), Opened);
	Report->SetNumberField(TEXT("expected_unavailable_count"), ExpectedUnavailable);
	Report->SetNumberField(TEXT("blocked_entry_count"), BlockedEntry);
	Report->SetNumberField(TEXT("blocked_count"), Blocked);
	Report->SetNumberField(TEXT("failed_count"), Failed);
	Report->SetArrayField(TEXT("routes"), Rows);
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *(Run.OutputDirectory / TEXT("report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

static TArray<FString> SplitMatrixCells(const FString& Line)
{
	TArray<FString> Cells;
	FString Cell;
	for (int32 Index = 0; Index < Line.Len(); ++Index)
	{
		const TCHAR Character = Line[Index];
		if (Character == TEXT('\\') && Index + 1 < Line.Len() && Line[Index + 1] == TEXT('|'))
		{
			Cell.AppendChar(TEXT('|'));
			++Index;
		}
		else if (Character == TEXT('|'))
		{
			Cells.Add(Cell.TrimStartAndEnd());
			Cell.Reset();
		}
		else Cell.AppendChar(Character);
	}
	if (!Cell.IsEmpty()) Cells.Add(Cell.TrimStartAndEnd());
	if (Cells.Num() && Cells[0].IsEmpty()) Cells.RemoveAt(0);
	if (Cells.Num() && Cells.Last().IsEmpty()) Cells.Pop();
	for (FString& Value : Cells) Value.ReplaceInline(TEXT("`"), TEXT(""));
	return Cells;
}

static bool LoadPlayerEntryMatrix(TArray<FPlayerEntryCase>& OutEntries)
{
	FString Markdown;
	const FString MatrixPath = FPaths::ProjectDir() / TEXT("specs/boundless-ink-ui/evidence/2026-10-23-native-route-matrix.md");
	if (!FFileHelper::LoadFileToString(Markdown, *MatrixPath)) return false;
	TArray<FString> Lines;
	Markdown.ParseIntoArrayLines(Lines, false);
	bool bInPlayerTable = false;
	for (const FString& Line : Lines)
	{
		if (Line.Contains(TEXT("## Structured player navigation"))) { bInPlayerTable = true; continue; }
		if (!bInPlayerTable) continue;
		if (Line.StartsWith(TEXT("## "))) break;
		if (!Line.StartsWith(TEXT("|"))) continue;
		const TArray<FString> Cells = SplitMatrixCells(Line);
		if (Cells.Num() != 5) continue;
		int32 ContractIndex = INDEX_NONE;
		if (!LexTryParseString(ContractIndex, *Cells[0])) continue;
		FPlayerEntryCase Entry;
		Entry.ContractIndex = ContractIndex;
		Entry.Actions = Cells[1];
		Entry.Safety = Cells[2];
		Entry.FinalPage = Cells[3];
		Entry.FinalState = Cells[4];
		OutEntries.Add(MoveTemp(Entry));
	}
	OutEntries.Sort([](const FPlayerEntryCase& A, const FPlayerEntryCase& B) { return A.ContractIndex < B.ContractIndex; });
	if (OutEntries.Num() != UE_ARRAY_COUNT(RouteCases)) return false;
	for (int32 Index = 0; Index < OutEntries.Num(); ++Index)
		if (OutEntries[Index].ContractIndex != Index + 1 || OutEntries[Index].Actions.IsEmpty() || OutEntries[Index].FinalPage.IsEmpty()) return false;
	return true;
}

static bool IsAllowedExtensionRoute(const FString& Route)
{
	static const TCHAR* Allowed[] = {
		TEXT("WulingRights"), TEXT("GiftRules"), TEXT("WulingEquipment"), TEXT("WulingRace"), TEXT("WulingWorld"),
		TEXT("WulingOffline"), TEXT("WulingCraft"), TEXT("WulingGuild"), TEXT("LaunchRank"), TEXT("ResourceContest"),
		TEXT("DailyActivityDetail"), TEXT("TimedActivityDetail"), TEXT("CharacterEquipment"), TEXT("CharacterAppearance"),
		TEXT("ResourceRanking"), TEXT("ResourceRewards"), TEXT("WulingShop"), TEXT("WulingMarket"), TEXT("WulingOrders"),
		TEXT("WulingSources"), TEXT("WulingBreakthrough")
	};
	for (const TCHAR* Candidate : Allowed) if (Route == Candidate) return true;
	return false;
}

static bool IsAllowedGroupTab(const FString& Action)
{
	static const TCHAR* Allowed[] = { TEXT("GroupTab_SignIn"), TEXT("GroupTab_MonthlySignIn"), TEXT("GroupTab_DailyFortune"), TEXT("GroupTab_DailyActivities"), TEXT("GroupTab_TimedActivities") };
	for (const TCHAR* Candidate : Allowed) if (Action == Candidate) return true;
	return false;
}

static bool ResolveAllowedPlayerAction(UInkFrontendWidget* UI, const FString& LogicalAction, FString& OutButtonAction)
{
	static const TCHAR* RootActions[] = {
		TEXT("OpenCharacter"), TEXT("OpenInventory"), TEXT("OpenLeaderboard"), TEXT("OpenSaves"), TEXT("OpenSettings"),
		TEXT("OpenWuling"), TEXT("OpenActivities"), TEXT("OpenSignIn"), TEXT("OpenExtensions"), TEXT("OpenLaunchEvent")
	};
	for (const TCHAR* Candidate : RootActions)
	{
		if (LogicalAction == Candidate) { OutButtonAction = LogicalAction; return true; }
	}

	FString ExtensionAction;
	if (LogicalAction.StartsWith(TEXT("Open_")) && !LogicalAction.Contains(TEXT("<id>")))
	{
		FString Route = LogicalAction.Mid(5);
		const int32 ContextSeparator = Route.Find(TEXT("|"));
		if (ContextSeparator != INDEX_NONE) Route = Route.Left(ContextSeparator);
		if (!IsAllowedExtensionRoute(Route)) return false;
		ExtensionAction = TEXT("Ext_") + LogicalAction;
	}
	else if (IsAllowedGroupTab(LogicalAction) || LogicalAction == TEXT("AtlasWorld") || LogicalAction == TEXT("AtlasResource") || LogicalAction == TEXT("LaunchTab_honors"))
	{
		ExtensionAction = TEXT("Ext_") + LogicalAction;
	}
	else if (LogicalAction.Contains(TEXT("<id>")) || LogicalAction.Contains(TEXT("DYNAMIC:")))
	{
		if (!UI) return false;
		FString Prefix;
		if (LogicalAction.StartsWith(TEXT("Open_DailyActivityDetail"))) Prefix = TEXT("Ext_Open_DailyActivityDetail|");
		else if (LogicalAction.StartsWith(TEXT("Open_TimedActivityDetail"))) Prefix = TEXT("Ext_Open_TimedActivityDetail|");
		else if (LogicalAction.StartsWith(TEXT("AtlasNode_"))) Prefix = TEXT("Ext_AtlasNode_");
		else if (LogicalAction.StartsWith(TEXT("AtlasEntry_"))) Prefix = TEXT("Ext_AtlasEntry_");
		else return false;
		TArray<FString> Prefixes;
		Prefixes.Add(Prefix);
		if (LogicalAction.Contains(TEXT("or AtlasEntry_"))) Prefixes.Add(TEXT("Ext_AtlasEntry_"));
		for (const FString& CandidatePrefix : Prefixes)
		for (const UInkFrontendActionButton* Button : UI->ActionButtons)
		{
			if (!Button || !Button->GetIsEnabled() || !Button->IsVisible()) continue;
			const FString Candidate = Button->Action.ToString();
			if (!Candidate.StartsWith(CandidatePrefix)) continue;
			if (CandidatePrefix.EndsWith(TEXT("|")) ? Candidate.Contains(TEXT("|")) && Candidate.Len() > CandidatePrefix.Len() : Candidate.Len() > CandidatePrefix.Len())
			{
				OutButtonAction = Candidate;
				return true;
			}
		}
		if (Prefix == TEXT("Ext_Open_DailyActivityDetail|") || Prefix == TEXT("Ext_Open_TimedActivityDetail|"))
		{
			const FString EmptyFallback = Prefix.LeftChop(1);
			for (const UInkFrontendActionButton* Button : UI->ActionButtons)
				if (Button && Button->Action.ToString() == EmptyFallback && Button->GetIsEnabled() && Button->IsVisible())
				{
					OutButtonAction = EmptyFallback;
					return true;
				}
		}
		return false;
	}
	else return false;

	OutButtonAction = ExtensionAction;
	return true;
}

static TSharedPtr<FJsonValueObject> MakeActionTrace(const FString& ActionId, const FString& FromPage, const FString& ToPage, const FString& Outcome)
{
	auto Trace = MakeShared<FJsonObject>();
	Trace->SetStringField(TEXT("action_id"), ActionId);
	Trace->SetStringField(TEXT("from_page"), FromPage);
	Trace->SetStringField(TEXT("to_page"), ToPage);
	Trace->SetStringField(TEXT("outcome"), Outcome);
	return MakeShared<FJsonValueObject>(Trace);
}

static FString ExpectedPanelName(const FRouteCase& Route)
{
	return FString(Route.ExpectedPanelClass);
}

static bool HasNativeClassInAncestry(const UClass* Class, const FString& ExpectedName)
{
	for (const UClass* Current = Class; Current; Current = Current->GetSuperClass())
		// UClass reflection names omit the C++ U prefix used by the contract.
		if (Current->GetName() == ExpectedName || TEXT("U") + Current->GetName() == ExpectedName) return true;
	return false;
}

static void AppendFailure(const TCHAR* Status, const FString& Detail)
{
	FRouteResult Result;
	if (Run.RouteIndex < UE_ARRAY_COUNT(RouteCases))
	{
		const FRouteCase& Route = RouteCases[Run.RouteIndex];
		Result.ContractId = Route.ContractId;
		Result.RequestedRoute = Route.RequestedRoute;
		Result.ExpectedPage = Route.ExpectedPage;
		Result.Availability = Route.Availability;
	}
	Result.Status = Status;
	Result.Detail = Detail;
	Run.Results.Add(MoveTemp(Result));
	SaveReport(TEXT("in_progress"));
}

static void Finish(const TCHAR* Status)
{
	Run.bActive = false;
	Run.bWaitingForPIE = false;
	SaveReport(Status);
	UE_LOG(LogTemp, Display, TEXT("BoundlessInkPIE route acceptance %s: %d/%d rows recorded at %s"), Status, Run.Results.Num(), UE_ARRAY_COUNT(RouteCases), *Run.OutputDirectory);
	if (Run.bOwnsPIE && FParse::Param(FCommandLine::Get(), TEXT("InkUiSafeRoutesExit")))
	{
		Run.bOwnsPIE = false;
		ExitHarnessAfterPIE();
	}
}

static UInkFrontendWidget* FindFrontend(UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AInkMenuDirector> It(World); It; ++It)
		if (It->FrontendWidget && It->FrontendWidget->IsInViewport()) return It->FrontendWidget;
	return nullptr;
}

static bool Tick(float)
{
	if (!Run.bActive) return false;
	const double Now = FPlatformTime::Seconds();
	if (Now - Run.StartedSeconds > 240.0)
	{
		if (Run.RouteIndex < UE_ARRAY_COUNT(RouteCases)) AppendFailure(TEXT("blocked"), TEXT("PIE route harness timed out before all rows could be recorded"));
		Finish(TEXT("blocked_timeout"));
		return false;
	}
	if (!GEditor || !GEditor->PlayWorld)
	{
		if (!Run.bWaitingForPIE && Run.RouteIndex > 0)
		{
			AppendFailure(TEXT("blocked"), TEXT("PIE world ended before the route set completed"));
			Finish(TEXT("blocked_pie_ended"));
			return false;
		}
		return true;
	}

	UWorld* World = GEditor->PlayWorld;
	if (!Run.Frontend.IsValid()) Run.Frontend = FindFrontend(World);
	UInkFrontendWidget* UI = Run.Frontend.Get();
	if (!UI) return true;
	if (!FSlateApplication::IsInitialized() || !GEditor->GameViewport || !GEditor->GameViewport->GetGameViewportWidget()) return true;
	const TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(GEditor->GameViewport->GetGameViewportWidget().ToSharedRef());
	if (!Window.IsValid()) return true;

	if (Run.bWaitingForPIE)
	{
		Run.bWaitingForPIE = false;
		Run.NextActionSeconds = Now + 0.5;
		return true;
	}
	if (Now < Run.NextActionSeconds) return true;

	if (Run.RouteIndex >= UE_ARRAY_COUNT(RouteCases))
	{
		const bool bAnyFailure = Run.Results.ContainsByPredicate([](const FRouteResult& Row) { return Row.Status == TEXT("failed"); });
		const bool bAnyBlocked = Run.Results.ContainsByPredicate([](const FRouteResult& Row) { return Row.Status == TEXT("blocked") || Row.Status == TEXT("blocked_entry"); });
		Finish(bAnyFailure ? TEXT("completed_with_failures") : bAnyBlocked ? TEXT("completed_with_blocked_entries") : TEXT("completed_with_expected_unavailable"));
		return false;
	}

	const FRouteCase& Route = RouteCases[Run.RouteIndex];
	const FPlayerEntryCase* Entry = Run.PlayerEntries.IsValidIndex(Run.RouteIndex) ? &Run.PlayerEntries[Run.RouteIndex] : nullptr;
	if (Run.Results.Num() <= Run.RouteIndex)
	{
		FRouteResult Result;
		Result.ContractId = Route.ContractId;
		Result.RequestedRoute = Route.RequestedRoute;
		Result.ExpectedPage = Entry ? Entry->FinalPage : FString(Route.ExpectedPage);
		Result.Availability = Route.Availability;
		Result.PlayerEntryActions = Entry ? Entry->Actions : FString();
		Result.PlayerEntrySafety = Entry ? Entry->Safety : TEXT("matrix_missing");
		Result.FinalSelectedState = Entry ? Entry->FinalState : FString();
		Result.EntryMethod = TEXT("matrix-driven real Slate clicks; no direct ShowPage/OpenFeaturePanel fallback");
		Run.RouteOriginPage = UI->CurrentPage.ToString();
		Result.RouteOriginPage = Run.RouteOriginPage;
		Run.ActionIndex = 0;
		Run.bActionPending = false;
		Run.PendingActionSerial = UI->ActionDispatchSerial;
		Run.PendingActionStartedSeconds = 0;
		Run.bScreenshotsCaptured = false;
		Run.bPathVerified = false;
		if (!Entry)
		{
			Result.Status = TEXT("blocked_entry");
			Result.Detail = TEXT("structured player-entry matrix is missing this contract row; direct resolver routing is not used as a fallback");
		}
		else if (Entry->Safety != TEXT("navigation_only"))
		{
			Result.Status = TEXT("blocked_entry");
			Result.Detail = FString::Printf(TEXT("matrix safety=%s; deny-by-default policy refuses player action sequence '%s' (including StartGame and routes that may issue automatic authority requests)"), *Entry->Safety, *Entry->Actions);
		}
		else
		{
			Result.Status = TEXT("pending_player_entry");
			Result.Detail = TEXT("only the matrix-listed exact player navigation actions may be clicked");
		}
		Run.Results.Add(MoveTemp(Result));
		if (Run.Results.Last().Status == TEXT("blocked_entry"))
		{
			++Run.RouteIndex;
			SaveReport(TEXT("in_progress"));
		}
		Run.NextActionSeconds = Now + 0.5;
		return true;
	}

	FRouteResult& Result = Run.Results[Run.RouteIndex];
	if (Result.Status == TEXT("blocked_entry") || (Result.Status == TEXT("failed") && !Run.bPathVerified))
	{
		if (!Run.RouteOriginPage.IsEmpty() && UI->CurrentPage != FName(*Run.RouteOriginPage))
		{
			if (UI->CurrentPage == TEXT("HUD") && Run.RouteOriginPage == TEXT("Main"))
			{
				Result.Detail += TEXT("; cleanup to Main is blocked because HUD Back would invoke ReturnToMainMenu");
				++Run.RouteIndex;
				SaveReport(TEXT("in_progress"));
				return true;
			}
			const bool bExtension = UI->ExtensionPages.Contains(UI->CurrentPage);
			const FName BackAction = bExtension ? FName(TEXT("Ext_Back")) : FName(TEXT("Back"));
			const FString FromPage = UI->CurrentPage.ToString();
			if (UInkFrontendEditorLibrary::ClickAction(UI, BackAction))
			{
				Result.ActionTrace.Add(MakeActionTrace(BackAction.ToString(), FromPage, UI->CurrentPage.ToString(), TEXT("cleanup_return_click")));
				Run.NextActionSeconds = Now + 0.35;
				return true;
			}
		}
		++Run.RouteIndex;
		Run.NextActionSeconds = Now + 0.1;
		SaveReport(TEXT("in_progress"));
		return true;
	}
	if (!Entry)
	{
		Result.Status = TEXT("blocked_entry");
		Result.Detail = TEXT("player-entry matrix row could not be loaded");
		++Run.RouteIndex;
		SaveReport(TEXT("in_progress"));
		return true;
	}

	if (Run.bActionPending)
	{
		const FName ExpectedAction(*Run.PendingActionId);
		const uint32 DispatchDelta = UI->ActionDispatchSerial - Run.PendingActionSerial;
		if (DispatchDelta > 0)
		{
			if (DispatchDelta != 1 || UI->LastDispatchedAction != ExpectedAction)
			{
				Result.Status = TEXT("failed");
				Result.Detail = FString::Printf(TEXT("Slate click for '%s' observed %u TriggerAction dispatch(es), last='%s'"), *Run.PendingActionId, DispatchDelta, *UI->LastDispatchedAction.ToString());
				Result.ActionTrace.Add(MakeActionTrace(Run.PendingActionId, Run.PendingFromPage, UI->CurrentPage.ToString(), TEXT("unexpected_action_callback")));
				SaveReport(TEXT("in_progress"));
				return true;
			}
			Result.ActionTrace.Add(MakeActionTrace(Run.PendingActionId, Run.PendingFromPage, UI->CurrentPage.ToString(), TEXT("action_dispatched")));
			Run.bActionPending = false;
			++Run.ActionIndex;
			Run.NextActionSeconds = Now + 0.35;
			return true;
		}
		if (Now - Run.PendingActionStartedSeconds > 1.5)
		{
			Result.Status = TEXT("failed");
			Result.Detail = FString::Printf(TEXT("Slate hit-test/mouse-down handled '%s', but UInkFrontendWidget::TriggerAction did not observe its callback"), *Run.PendingActionId);
			Result.ActionTrace.Add(MakeActionTrace(Run.PendingActionId, Run.PendingFromPage, UI->CurrentPage.ToString(), TEXT("click_without_action_callback")));
			SaveReport(TEXT("in_progress"));
			return true;
		}
		Run.NextActionSeconds = Now + 0.05;
		return true;
	}

	if (!Run.bPathVerified)
	{
		TArray<FString> Actions;
		Entry->Actions.ParseIntoArray(Actions, TEXT(";"), false);
		bool bClickedAction = false;
		while (Run.ActionIndex < Actions.Num())
		{
			const FString Logical = Actions[Run.ActionIndex].TrimStartAndEnd();
			if (Logical.IsEmpty() || Logical.StartsWith(TEXT("DYNAMIC:")) || Logical == TEXT("PIE_STARTUP"))
			{
				++Run.ActionIndex;
				continue;
			}
			FString ButtonAction;
			if (!ResolveAllowedPlayerAction(UI, Logical, ButtonAction))
			{
				Result.Status = TEXT("blocked_entry");
				Result.Detail = FString::Printf(TEXT("action '%s' has no audited safe Slate button or required dynamic entry id is unavailable"), *Logical);
				Result.ActionTrace.Add(MakeActionTrace(Logical, UI->CurrentPage.ToString(), UI->CurrentPage.ToString(), TEXT("blocked_entry")));
				SaveReport(TEXT("in_progress"));
				return true;
			}
			const FString FromPage = UI->CurrentPage.ToString();
			// Slate may execute OnClicked synchronously. Capture the baseline before
			// dispatch, otherwise a successful callback is mistaken for no callback.
			const uint32 BeforeClickSerial = UI->ActionDispatchSerial;
			if (!UInkFrontendEditorLibrary::ClickAction(UI, FName(*ButtonAction)))
			{
				Result.Status = TEXT("blocked_entry");
				Result.Detail = FString::Printf(TEXT("visible player action '%s' could not be Slate-hit-tested from page '%s'; no direct route fallback was attempted"), *ButtonAction, *FromPage);
				Result.ActionTrace.Add(MakeActionTrace(ButtonAction, FromPage, FromPage, TEXT("blocked_entry")));
				SaveReport(TEXT("in_progress"));
				return true;
			}
			Run.PendingActionId = ButtonAction;
			Run.PendingFromPage = FromPage;
			Run.PendingActionSerial = BeforeClickSerial;
			Run.PendingActionStartedSeconds = Now;
			Run.bActionPending = true;
			bClickedAction = true;
			break;
		}
		if (bClickedAction) return true;

		Result.ActualPage = UI->CurrentPage.ToString();
		Result.WidgetSnapshotJson = UInkFrontendEditorLibrary::DescribeWidget(UI);
		Result.ViewportSize = UI->GetCachedGeometry().GetLocalSize();
		UInkExtensionWidget* Panel = UI->ExtensionPages.FindRef(FName(*Entry->FinalPage)).Get();
		Result.ActualPanelClass = Panel ? Panel->GetClass()->GetName() : FString();
		const bool bPageMatches = UI->CurrentPage == FName(*Entry->FinalPage);
		const bool bPanelMatches = Route.ExpectedPanelClass[0] == 0 || (Panel && HasNativeClassInAncestry(Panel->GetClass(), ExpectedPanelName(Route)));
		FString ExpectedGroupRoute;
		const int32 SelectedOffset = Entry->FinalState.Find(TEXT("SelectedRoute="));
		if (SelectedOffset != INDEX_NONE) ExpectedGroupRoute = Entry->FinalState.Mid(SelectedOffset + 14).TrimStartAndEnd();
		else if (Entry->FinalState.Contains(TEXT("default selected child")))
		{
			const int32 ChildOffset = Entry->FinalState.Find(TEXT("default selected child"));
			ExpectedGroupRoute = Entry->FinalState.Mid(ChildOffset + FCString::Strlen(TEXT("default selected child"))).TrimStartAndEnd();
		}
		bool bGroupMatches = true;
		if (!ExpectedGroupRoute.IsEmpty())
		{
			const UInkFeatureGroupPanel* Group = Cast<UInkFeatureGroupPanel>(Panel);
			bGroupMatches = Group && Group->SelectedRoute == FName(*ExpectedGroupRoute);
		}
		if (!bPageMatches || !bPanelMatches || !bGroupMatches)
		{
			Result.Status = TEXT("failed");
			Result.Detail = FString::Printf(TEXT("real player click path ended page=%s expected=%s panel=%s group=%s"), *Result.ActualPage, *Entry->FinalPage, bPanelMatches ? TEXT("ok") : TEXT("mismatch"), bGroupMatches ? TEXT("ok") : TEXT("mismatch"));
		}
		else
		{
			Result.Status = TEXT("pending_capture");
			Result.bPlayerEntryVerified = true;
			Result.Detail = TEXT("all listed navigation actions were Slate-clicked from the current player-visible state; no business CTA or authority command was dispatched");
		}
		if (FCString::Stricmp(Route.ContractId, TEXT("gift-rules")) == 0 && Result.Status != TEXT("failed"))
		{
			const UTextBlock* Unavailable = Panel && Panel->WidgetTree ? Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("GiftRulesUnavailable"))) : nullptr;
			const bool bHonestUnavailableState = Unavailable && Unavailable->GetVisibility() != ESlateVisibility::Collapsed && Unavailable->GetText().ToString().Contains(TEXT("暂未接入"));
			if (!bHonestUnavailableState)
			{
				Result.Status = TEXT("failed");
				Result.Detail += TEXT("; GiftRules did not show the explicit unavailable state");
			}
			else Result.Detail += TEXT("; native GiftRules screen is reachable, but the matrix correctly records missing authority API");
		}
		Run.bPathVerified = true;
		Run.NextActionSeconds = Now + 0.4;
		return true;
	}

	const FString Stem = FString::Printf(TEXT("%02d-%s-720x1280"), Run.RouteIndex + 1, Route.ContractId);
	const FString ScreenshotPath = Run.OutputDirectory / (Stem + TEXT(".png"));
	if (!Run.bScreenshotsCaptured)
	{
		const bool bCaptured = UInkFrontendEditorLibrary::CaptureFrontendViewport(ScreenshotPath);
		Result.Screenshot = ScreenshotPath;
		Result.WindowScreenshot = Run.OutputDirectory / (Stem + TEXT("-window.png"));
		const bool bViewportWritten = IFileManager::Get().FileSize(*ScreenshotPath) > 1024;
		const bool bWindowWritten = IFileManager::Get().FileSize(*Result.WindowScreenshot) > 1024;
		Run.bScreenshotsCaptured = true;
		if (!bCaptured || !bViewportWritten || !bWindowWritten)
		{
			Result.Status = TEXT("blocked");
			Result.Detail += TEXT("; current-run Slate viewport/window capture did not produce both screenshots");
		}
		Run.NextActionSeconds = Now + 0.2;
		return true;
	}

	if (UI->CurrentPage != FName(*Run.RouteOriginPage))
	{
		if (UI->CurrentPage == TEXT("HUD") && Run.RouteOriginPage == TEXT("Main"))
		{
			Result.Status = TEXT("failed");
			Result.Detail += TEXT("; safe return to Main is unavailable because HUD Back would invoke ReturnToMainMenu");
			++Run.RouteIndex;
			SaveReport(TEXT("in_progress"));
			return true;
		}
		const bool bExtension = UI->ExtensionPages.Contains(UI->CurrentPage);
		const FName BackAction = bExtension ? FName(TEXT("Ext_Back")) : FName(TEXT("Back"));
		const FString FromPage = UI->CurrentPage.ToString();
		if (!UInkFrontendEditorLibrary::ClickAction(UI, BackAction))
		{
			Result.Status = TEXT("failed");
			Result.Detail += TEXT("; safe player Back action was not visible or could not be clicked to restore the route origin");
			++Run.RouteIndex;
			SaveReport(TEXT("in_progress"));
			return true;
		}
		Result.ActionTrace.Add(MakeActionTrace(BackAction.ToString(), FromPage, UI->CurrentPage.ToString(), TEXT("return_click")));
		Run.NextActionSeconds = Now + 0.35;
		return true;
	}
	Result.ReturnTarget = Run.RouteOriginPage;
	Result.bReturnVerified = true;
	if (Result.Status == TEXT("failed"))
	{
		// Preserve the failed route assertion after a successful navigation return.
	}
	else if (Result.Status == TEXT("blocked"))
	{
		// Screenshot/capture blocks remain blocks.
	}
	else if (FCString::Stricmp(Route.Availability, TEXT("missing-authority-api")) == 0 || FCString::Stricmp(Route.Availability, TEXT("missing-claim-fields")) == 0 || FCString::Stricmp(Route.Availability, TEXT("missing-authority-fields")) == 0 || FCString::Stricmp(Route.Availability, TEXT("missing-ranking-api")) == 0 || FCString::Stricmp(Route.Availability, TEXT("missing-receipt-api")) == 0)
	{
		Result.Status = TEXT("expected_unavailable");
		Result.Detail += TEXT("; route was reached through player navigation, but the contract matrix records missing authoritative fields/API; this is not a feature pass");
	}
	else
	{
		Result.Status = TEXT("opened");
	}
	++Run.RouteIndex;
	Run.ActionIndex = 0;
	Run.RouteOriginPage.Reset();
	Run.bActionPending = false;
	Run.bScreenshotsCaptured = false;
	Run.bPathVerified = false;
	Run.NextActionSeconds = Now + 0.1;
	SaveReport(TEXT("in_progress"));
	return true;
}

static void Start()
{
	if (Run.bActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("BoundlessInkPIE route acceptance is already active"));
		return;
	}
	Run = FRunState();
	Run.RunId = FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%SZ")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
	Run.StartedAtUtc = FDateTime::UtcNow();
	Run.OutputDirectory = FPaths::ProjectSavedDir() / TEXT("Codex/BoundlessInkPIE") / Run.RunId;
	IFileManager::Get().MakeDirectory(*Run.OutputDirectory, true);
	if (!LoadPlayerEntryMatrix(Run.PlayerEntries))
	{
		Run.StartedSeconds = FPlatformTime::Seconds();
		Run.bActive = true;
		AppendFailure(TEXT("blocked_entry"), TEXT("failed to read the structured 44-row player-entry table from the native route matrix"));
		Finish(TEXT("blocked_matrix"));
		return;
	}
	Run.StartedSeconds = FPlatformTime::Seconds();
	Run.bActive = true;
	if (!GEditor || GEditor->PlayWorld)
	{
		AppendFailure(TEXT("blocked"), TEXT("requires the Editor console before PIE; refuses to attach to or replace an existing PIE session"));
		Finish(TEXT("blocked_precondition"));
		return;
	}
	if (!FSlateApplication::IsInitialized() || !UInkFrontendEditorLibrary::BeginFrontendPIE(720, 1280))
	{
		AppendFailure(TEXT("blocked"), TEXT("Slate is unavailable or Editor refused to request a 720x1280 PIE window"));
		Finish(TEXT("blocked_pie_request"));
		return;
	}
	Run.bWaitingForPIE = true;
	Run.bOwnsPIE = true;
	Run.TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick), 0.1f);
	SaveReport(TEXT("pie_start_requested"));
	UE_LOG(LogTemp, Display, TEXT("BoundlessInkPIE safe route acceptance started. No action-button dispatch or authority mutation is permitted. Report: %s"), *(Run.OutputDirectory / TEXT("report.json")));
}

static FAutoConsoleCommand RunCommand(
	TEXT("BoundlessInkPIE.RunSafeRoutes"),
	TEXT("Run the Editor-only, deny-by-default 44-screen PIE acceptance from structured matrix player_entry_actions using actual Slate clicks; unsafe or unreachable entries are blocked."),
	FConsoleCommandDelegate::CreateStatic(&Start));

// Component-only visual preview. This deliberately does not share or mutate the
// player-entry runner state and never dispatches an action button.
struct FVisualRouteResult
{
	FString ContractId, RequestedRoute, ExpectedPage, ActualPage, ExpectedPanelClass, ActualPanelClass;
	FString ExpectedGroupRoute, ActualGroupRoute, Availability, Status, EntryMethod, Screenshot, WindowScreenshot, Failure;
	bool bPlayerEntryVerified = false;
};

struct FVisualRunState
{
	bool bActive = false, bWaitingForPIE = false, bOwnsPIE = false, bExitEditorWhenDone = false;
	bool bServicesVerified = false, bServicesOffline = false, bRouteOpened = false;
	int32 RouteIndex = 0;
	double StartedSeconds = 0, NextActionSeconds = 0;
	FString RunId, OutputDirectory;
	FDateTime StartedAtUtc;
	FString WulingServiceStatus = TEXT("not_checked"), CultivationServiceStatus = TEXT("not_checked");
	TWeakObjectPtr<UInkFrontendWidget> Frontend;
	TArray<FVisualRouteResult> Results;
	FTSTicker::FDelegateHandle TickerHandle;
};

static FVisualRunState VisualRun;

static TSharedRef<FJsonObject> MakeVisualResultJson(const FVisualRouteResult& Result)
{
	auto Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("contract_id"), Result.ContractId);
	Object->SetStringField(TEXT("requested_route"), Result.RequestedRoute);
	Object->SetStringField(TEXT("expected_current_page"), Result.ExpectedPage);
	Object->SetStringField(TEXT("actual_current_page"), Result.ActualPage);
	Object->SetStringField(TEXT("expected_panel_class"), Result.ExpectedPanelClass);
	Object->SetStringField(TEXT("actual_panel_class"), Result.ActualPanelClass);
	Object->SetStringField(TEXT("expected_group_route"), Result.ExpectedGroupRoute);
	Object->SetStringField(TEXT("actual_group_route"), Result.ActualGroupRoute);
	Object->SetStringField(TEXT("availability_boundary"), Result.Availability);
	Object->SetStringField(TEXT("status"), Result.Status);
	Object->SetStringField(TEXT("entry_method"), Result.EntryMethod);
	Object->SetBoolField(TEXT("player_entry_verified"), false);
	Object->SetStringField(TEXT("screenshot"), Result.Screenshot);
	Object->SetStringField(TEXT("window_screenshot"), Result.WindowScreenshot);
	Object->SetStringField(TEXT("failure"), Result.Failure);
	return Object;
}

static void SaveVisualReport(const FString& Status)
{
	auto Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("boundless-ink-visual-suite-report-v1"));
	Report->SetStringField(TEXT("run_id"), VisualRun.RunId);
	Report->SetStringField(TEXT("started_utc"), VisualRun.StartedAtUtc.ToIso8601());
	Report->SetStringField(TEXT("status"), Status);
	Report->SetStringField(TEXT("suite_type"), TEXT("component_visual_preview"));
	Report->SetStringField(TEXT("scope"), TEXT("Direct component/page preview using public ShowPage/OpenFeaturePanel routes; screenshots are not player-entry or device acceptance evidence."));
	Report->SetBoolField(TEXT("player_entry_verified"), false);
	Report->SetBoolField(TEXT("device_acceptance"), false);
	Report->SetBoolField(TEXT("business_actions_dispatched"), false);
	Report->SetBoolField(TEXT("save_data_written"), false);
	Report->SetBoolField(TEXT("wuling_service_enabled"), VisualRun.WulingServiceStatus == TEXT("enabled"));
	Report->SetStringField(TEXT("wuling_service_status"), VisualRun.WulingServiceStatus);
	Report->SetBoolField(TEXT("cultivation_service_enabled"), VisualRun.CultivationServiceStatus == TEXT("enabled"));
	Report->SetStringField(TEXT("cultivation_service_status"), VisualRun.CultivationServiceStatus);
	Report->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
	Report->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
	Report->SetStringField(TEXT("output_directory"), VisualRun.OutputDirectory);
	Report->SetNumberField(TEXT("contract_route_count"), UE_ARRAY_COUNT(RouteCases));
	Report->SetNumberField(TEXT("attempted_count"), VisualRun.Results.Num());
	int32 Captured = 0, Failed = 0;
	TArray<TSharedPtr<FJsonValue>> Rows;
	for (const FVisualRouteResult& Result : VisualRun.Results)
	{
		if (Result.Status == TEXT("captured")) ++Captured; else ++Failed;
		Rows.Add(MakeShared<FJsonValueObject>(MakeVisualResultJson(Result)));
	}
	Report->SetNumberField(TEXT("captured_count"), Captured);
	Report->SetNumberField(TEXT("failed_count"), Failed);
	Report->SetArrayField(TEXT("routes"), Rows);
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *(VisualRun.OutputDirectory / TEXT("report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

static void FinishVisual(const TCHAR* Status)
{
	const bool bWasOwnPIE = VisualRun.bOwnsPIE;
	VisualRun.bActive = false;
	SaveVisualReport(Status);
	UE_LOG(LogTemp, Display, TEXT("BoundlessInkPIE visual suite %s: %d/%d routes captured at %s"), Status, VisualRun.Results.Num(), UE_ARRAY_COUNT(RouteCases), *(VisualRun.OutputDirectory / TEXT("report.json")));
	if (VisualRun.bOwnsPIE && GEditor && GEditor->PlayWorld)
	{
		if (!VisualRun.bExitEditorWhenDone) GEditor->RequestEndPlayMap();
		VisualRun.bOwnsPIE = false;
	}
	if (VisualRun.bExitEditorWhenDone && bWasOwnPIE) ExitHarnessAfterPIE();
}

static void AppendVisualFailure(const FRouteCase& Route, const FString& Reason)
{
	FVisualRouteResult Result;
	Result.ContractId = Route.ContractId;
	Result.RequestedRoute = Route.RequestedRoute;
	Result.ExpectedPage = Route.ExpectedPage;
	Result.ExpectedPanelClass = Route.ExpectedPanelClass;
	Result.ExpectedGroupRoute = Route.ExpectedGroupRoute;
	Result.Availability = Route.Availability;
	Result.EntryMethod = TEXT("direct component preview");
	Result.Status = TEXT("failed");
	Result.Failure = Reason;
	VisualRun.Results.Add(MoveTemp(Result));
}

static bool TickVisual(float)
{
	if (!VisualRun.bActive) return false;
	const double Now = FPlatformTime::Seconds();
	if (Now - VisualRun.StartedSeconds > 180.0)
	{
		while (VisualRun.RouteIndex < UE_ARRAY_COUNT(RouteCases)) AppendVisualFailure(RouteCases[VisualRun.RouteIndex++], TEXT("visual suite timed out waiting for Editor/Slate/PIE readiness or route capture"));
		FinishVisual(TEXT("failed_timeout"));
		return false;
	}
	if (!GEditor || !GEditor->PlayWorld) return true;
	UWorld* World = GEditor->PlayWorld;
	if (!World->GetGameInstance()) return true;
	if (!VisualRun.bServicesVerified)
	{
		UGameInstance* GameInstance = World->GetGameInstance();
		UWulingPrototypeSubsystem* Wuling = GameInstance->GetSubsystem<UWulingPrototypeSubsystem>();
		UCultivationPrototypeClient* Cultivation = GameInstance->GetSubsystem<UCultivationPrototypeClient>();
		if (!Wuling || !Cultivation)
		{
			VisualRun.WulingServiceStatus = Wuling ? (Wuling->IsEnabled() ? TEXT("enabled") : TEXT("disabled")) : TEXT("unavailable");
			VisualRun.CultivationServiceStatus = Cultivation ? (Cultivation->IsEnabled() ? TEXT("enabled") : TEXT("disabled")) : TEXT("unavailable");
			while (VisualRun.RouteIndex < UE_ARRAY_COUNT(RouteCases)) AppendVisualFailure(RouteCases[VisualRun.RouteIndex++], TEXT("could not verify both prototype services are disabled; preview refused to proceed"));
			FinishVisual(TEXT("failed_service_gate"));
			return false;
		}
		VisualRun.WulingServiceStatus = Wuling->IsEnabled() ? TEXT("enabled") : TEXT("disabled");
		VisualRun.CultivationServiceStatus = Cultivation->IsEnabled() ? TEXT("enabled") : TEXT("disabled");
		VisualRun.bServicesVerified = true;
		VisualRun.bServicesOffline = !Wuling->IsEnabled() && !Cultivation->IsEnabled();
		if (!VisualRun.bServicesOffline)
		{
			while (VisualRun.RouteIndex < UE_ARRAY_COUNT(RouteCases)) AppendVisualFailure(RouteCases[VisualRun.RouteIndex++], TEXT("one or more prototype services are enabled; component preview refused to capture"));
			FinishVisual(TEXT("failed_service_enabled"));
			return false;
		}
	}
	if (!VisualRun.Frontend.IsValid()) VisualRun.Frontend = FindFrontend(World);
	UInkFrontendWidget* UI = VisualRun.Frontend.Get();
	if (!UI || !FSlateApplication::IsInitialized() || !GEditor->GameViewport || !GEditor->GameViewport->GetGameViewportWidget()) return true;
	const TSharedPtr<SWindow> Window = FSlateApplication::Get().FindWidgetWindow(GEditor->GameViewport->GetGameViewportWidget().ToSharedRef());
	if (!Window.IsValid()) return true;
	if (VisualRun.bWaitingForPIE)
	{
		VisualRun.bWaitingForPIE = false;
		VisualRun.NextActionSeconds = Now + 0.5;
		return true;
	}
	if (Now < VisualRun.NextActionSeconds) return true;
	if (VisualRun.RouteIndex >= UE_ARRAY_COUNT(RouteCases))
	{
		const bool bFailed = VisualRun.Results.ContainsByPredicate([](const FVisualRouteResult& Row) { return Row.Status != TEXT("captured"); });
		FinishVisual(bFailed ? TEXT("completed_with_failures") : TEXT("completed"));
		return false;
	}
	const FRouteCase& Route = RouteCases[VisualRun.RouteIndex];
	if (VisualRun.Results.Num() <= VisualRun.RouteIndex)
	{
		FVisualRouteResult Result;
		Result.ContractId = Route.ContractId;
		Result.RequestedRoute = Route.RequestedRoute;
		Result.ExpectedPage = Route.ExpectedPage;
		Result.ExpectedPanelClass = Route.ExpectedPanelClass;
		Result.ExpectedGroupRoute = Route.ExpectedGroupRoute;
		Result.Availability = Route.Availability;
		Result.EntryMethod = TEXT("direct component preview via public ShowPage/OpenFeaturePanel; not player navigation");
		VisualRun.Results.Add(MoveTemp(Result));
		const FName Requested(*FString(Route.RequestedRoute));
		bool bOpened = false;
		const bool bBuiltin = Requested == TEXT("Main") || Requested == TEXT("HUD") || Requested == TEXT("Saves")
			|| Requested == TEXT("Settings") || Requested == TEXT("Character") || Requested == TEXT("Inventory") || Requested == TEXT("Leaderboard");
		if (bBuiltin) { UI->ShowPage(Requested); bOpened = UI->CurrentPage == Requested; }
		else bOpened = UI->OpenFeaturePanel(Requested, Route.Context);
		VisualRun.bRouteOpened = bOpened;
		VisualRun.NextActionSeconds = Now + 0.5;
		return true;
	}
	FVisualRouteResult& Result = VisualRun.Results[VisualRun.RouteIndex];
	Result.ActualPage = UI->CurrentPage.ToString();
	UInkExtensionWidget* Panel = UI->ExtensionPages.FindRef(UI->CurrentPage).Get();
	Result.ActualPanelClass = Panel ? Panel->GetClass()->GetName() : FString();
	if (const UInkFeatureGroupPanel* Group = Cast<UInkFeatureGroupPanel>(Panel)) Result.ActualGroupRoute = Group->SelectedRoute.ToString();
	const bool bPageMatches = UI->CurrentPage == FName(Route.ExpectedPage);
	const bool bPanelMatches = Route.ExpectedPanelClass[0] == 0 || (Panel && HasNativeClassInAncestry(Panel->GetClass(), Route.ExpectedPanelClass));
	const bool bGroupMatches = Route.ExpectedGroupRoute[0] == 0 || Result.ActualGroupRoute == Route.ExpectedGroupRoute;
	if (!VisualRun.bRouteOpened || !bPageMatches || !bPanelMatches || !bGroupMatches)
	{
		Result.Status = TEXT("failed");
		Result.Failure = FString::Printf(TEXT("direct route did not resolve as expected (opened=%s, page=%s, panel=%s, group=%s)"), VisualRun.bRouteOpened ? TEXT("true") : TEXT("false"), bPageMatches ? TEXT("ok") : TEXT("mismatch"), bPanelMatches ? TEXT("ok") : TEXT("mismatch"), bGroupMatches ? TEXT("ok") : TEXT("mismatch"));
	}
	else
	{
		const FString Stem = FString::Printf(TEXT("%02d-%s-720x1280"), VisualRun.RouteIndex + 1, Route.ContractId);
		Result.Screenshot = VisualRun.OutputDirectory / (Stem + TEXT(".png"));
		Result.WindowScreenshot = VisualRun.OutputDirectory / (Stem + TEXT("-window.png"));
		const bool bCaptured = UInkFrontendEditorLibrary::CaptureFrontendViewport(Result.Screenshot);
		const bool bViewportWritten = IFileManager::Get().FileSize(*Result.Screenshot) > 1024;
		const bool bWindowWritten = IFileManager::Get().FileSize(*Result.WindowScreenshot) > 1024;
		if (bCaptured && bViewportWritten && bWindowWritten) Result.Status = TEXT("captured");
		else { Result.Status = TEXT("failed"); Result.Failure = TEXT("viewport or PIE window screenshot was not written"); }
	}
	++VisualRun.RouteIndex;
	VisualRun.bRouteOpened = false;
	VisualRun.NextActionSeconds = Now + 0.5;
	SaveVisualReport(TEXT("in_progress"));
	return true;
}

static void StartVisual()
{
	if (VisualRun.bActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("BoundlessInkPIE visual suite is already active"));
		return;
	}
	VisualRun = FVisualRunState();
	VisualRun.RunId = FDateTime::UtcNow().ToString(TEXT("%Y%m%dT%H%M%SZ")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
	VisualRun.StartedAtUtc = FDateTime::UtcNow();
	VisualRun.OutputDirectory = FPaths::ProjectSavedDir() / TEXT("Codex/FullUI20261024") / (TEXT("visual-suite-") + VisualRun.RunId);
	IFileManager::Get().MakeDirectory(*VisualRun.OutputDirectory, true);
	VisualRun.StartedSeconds = FPlatformTime::Seconds();
	VisualRun.bExitEditorWhenDone = FParse::Param(FCommandLine::Get(), TEXT("InkUiVisualSuiteExit"));
	VisualRun.bActive = true;
	if (!GEditor || GEditor->PlayWorld)
	{
		for (const FRouteCase& Route : RouteCases) AppendVisualFailure(Route, TEXT("requires the Editor console with no existing PIE world; refuses to attach to an existing session"));
		FinishVisual(TEXT("failed_precondition"));
		return;
	}
	if (!FSlateApplication::IsInitialized() || !UInkFrontendEditorLibrary::BeginFrontendPIE(720, 1280))
	{
		for (const FRouteCase& Route : RouteCases) AppendVisualFailure(Route, TEXT("Slate is unavailable or Editor refused the visual preview PIE request"));
		FinishVisual(TEXT("failed_pie_request"));
		return;
	}
	VisualRun.bOwnsPIE = true;
	VisualRun.bWaitingForPIE = true;
	VisualRun.TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickVisual), 0.1f);
	SaveVisualReport(TEXT("pie_start_requested"));
	UE_LOG(LogTemp, Display, TEXT("BoundlessInkPIE component visual suite started. Report: %s"), *(VisualRun.OutputDirectory / TEXT("report.json")));
}

static FAutoConsoleCommand CaptureVisualSuiteCommand(
	TEXT("BoundlessInkPIE.CaptureVisualSuite"),
	TEXT("Capture a 44-route direct component visual preview in isolated PIE; does not verify player entry or dispatch business actions."),
	FConsoleCommandDelegate::CreateStatic(&StartVisual));
}
