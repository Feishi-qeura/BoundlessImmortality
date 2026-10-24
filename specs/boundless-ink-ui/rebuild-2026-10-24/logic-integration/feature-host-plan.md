# Full new feature host next epoch

This is an implementation continuation, not runtime evidence. Sole production writer remains integration. Do not message other chats or spawn agents.

Implement `UInkRebuildFeatureHost : UInkExtensionWidget` under UIRebuild20261024, with a wholly independent widget tree and retained owner components. Keep inherited navigation stack and authority/service owners; do not mount old FeaturePanel chrome or old WBP trees.

Minimal shared seams for one next Source epoch:

- UInkFrontendWidget protected virtual `CreateRebuiltFeaturePage(FName)` returns UInkExtensionWidget*. `EnsureFeaturePage` must take ONLY this factory when UsesRebuiltPresentation, returning false if unsupported; no old ClassForRoute fallback. Existing extension map/switcher/input blocking/ShowPage/back stack can then retain their behavior.
- In OpenFeaturePanel, execute old alias/grouping/ClassForRoute checks only for !UsesRebuiltPresentation. New canonical screen names remain distinct, especially MonthlySignIn, Tribulation, RaceLobby/Live/Settlement, ResourceLease and EventSettlement. Use canonical factory whitelist from stage-to-runtime-v4.csv; legacy aliases, if needed, must resolve exact single meanings explicitly.
- Add virtual SetContext(const FString&) to UInkExtensionWidget (default sets SelectedEntry). Existing UInkFeaturePanel::SetContext automatically overrides with its current behavior; use the generic extension pointer in navigation so the new host receives actual context too.
- Root override factory creates UInkRebuildFeatureHost only for canonical supported route. Core7 remain in root. Feature host Configure/NativeOnInitialized own frame and retained component, actual context, typed snapshot/selection/drafts/confirmation, .35s bounded semantic refresh; source writes alone do not establish runtime proof.
- New host Back/Close use existing Ext_Back/Ext_Close buttons and frontend CloseFeaturePanel(false/true); new owner delegate navigation calls OpenFeaturePanel(canonical, exactEntryId). Do not collapse distinct result routes.
- Use InkCore/InkJourneyAdapter and existing Wuling service snapshot/commands. Unknown provider fields stay unknown. Fresh subject/version/day/month/stage and current active/connected/non-Busy gates precede command dispatch. Owner callback payload must carry captured identities; never substitute fresh values into an old confirmation.
- Full owner UUserWidget pages already have composition/scroll/header; avoid a second paper/frame or nested expanding ScrollBox. Player pure composers receive actual host style/layout/VM/retained callback. RootHost may provide only safe body/back overlay for full pages, and full native page shell for pure composers.

Latest safe promotion inputs: Rank review-v3 already promoted/connected in source; Journey review-v4; Activities original review-v3 plus separate review-v5 replacements for panel-activities-02/economy-03 (six files, read manifest/receipt); Player-c frozen + review-v4 minimal diff; Player-d; Player-e latest review-v5 stage fingerprint revision when ready. Preserve original staging and exact ledger backups/hashes. Do not re-report PC3-01 AddLambda, source already uses retained dynamic callbacks.

44 actual stage declarations are in model/review/stage-to-runtime-v4.csv/.json, not proof of mounting. Read exact actual_stage_class, actual_stage_builder, stage_vm_input, stage_header columns. Remaining stages include seven Player process pages; five Rank/Wuling feature surfaces; Journey map/sign-in/race/result/read-only surfaces; Activity hub/details/launch/rank/offline; Shop/Market/Orders. `wuling` screen also includes member page as an internal retained route, not a fabricated45th required screen.

Next runtime checkpoint after build10: native NewUI3 tests, actual defaultPIE03 core7 and Rank3tabs/local-global plus actual landscape slot39 scroll. Previous PIE01/02 evidence stays immutable. PIE02 NaN check was a false positive on culled zero-size slot24..39, now diagnostics explicitly geometry_arranged. The screenshot is valid1280x720; require actual scroll39 into bounds to validate previously culled content.
