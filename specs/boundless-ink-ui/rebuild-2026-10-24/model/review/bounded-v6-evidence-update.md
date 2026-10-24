# Bounded v6: formal execution and default seven-route evidence

The newer records supersede v5's missing-execution-record status. **The three NewUI native tests actually ran and succeeded; the third default PIE run finished with 159 checks and 15 captures reported passed.** This review reads those existing records and selected hashes only. It starts no engine, build, tests or image review. Inputs are frozen in `selected-inputs-v6.json`; qualified results are in `runtime-evidence-v6.json`.

## Confirmed evidence and its scope

| Evidence | Confirmed result | Coverage limit |
|---|---|---|
| build11 | Exit 0; 64 Source files / 118 ledger records; finished 2026-10-24 14:24:18 +08:00 | This is the tested epoch, not later in-flight Source edits. |
| `native-tests-01-report/index.json` | succeeded=3, failed=0, notRun=0; all three exact NewUI test paths have state Success and zero errors/warnings | Native Editor-context fixtures, not rendered or service-transport evidence. |
| N03 `VisibleIdentitySurvivesProjectionChanges` | Formal native execution Success | Validates its model reorder/removal and TriggerAction identity fixture. No longer “execution record missing.” |
| I3 `BusyRetainsConfirmedPresentation` | Formal native execution Success | Validates confirmed builder content and mutation gates with the supplied Busy flag. Real service Busy through the default Root is a separate unverified scenario. |
| `ExactCanonicalPaths` | Formal native execution Success | Validates that fixture's mapping cases; not a new rehash of all migrated assets. |
| default PIE03 | finished=true, passed=true; 159 checks, 15 captures | Reported success is retained; geometry acceptance is qualified below. |
| Actual default WBP and routes | Main, HUD, Saves, Settings, Character, Inventory, Leaderboard each have passed page/root/runtime-version/WBP/page-version assertions | Seven route/shell smoke validations; not 44 pages or visual approval. |
| Rank variants and scope | Actual Damage/Local, CultivationSpeed/Local, Anchor/Local, Anchor/Global assertions passed through typed-button Slate actions | The global unavailable-state UI branch does not establish a live global board service. |

The core summary, PIE03 and selected Source epoch all identify SHA256 `a0d9b6e31b37536400f6075975dc37cf9e9ae9d894ed37d60b61920f9738f495`. The native process explicitly references build11 and the same 64/118 counts; the two selected native fixture sources match that epoch. Its process finished at 14:25:35 +08:00. PIE03 then ran 06:26:22–06:27:31 UTC with the actual default `/Game/newUI/Runtime/WBP_NewUIFrontend.WBP_NewUIFrontend_C`. No directly constructed page fixture is substituted for that default route evidence.

The publisher's `core_runtime_verified=true` is preserved as a reported result. The review qualifies it as default core route/shell smoke plus three formal native successes; it does not extend that flag to rigorous geometry, real service mutations, server global ranking, device/package or user visual approval.

## Geometry acceptance remains provisional

The v5 finite-filter finding is still present at the selected unchanged hashes of the verifier and Editor readback helper. No independent expected-paint/culled classification or rigorous replacement run is in this selected evidence.

PIE03's own `nonfinite_metrics` still contains:

- `inventory-landscape`: 40 records flagged visible and nonfinite, zero with geometry_arranged=true.
- `inventory-landscape-end`: 8 records flagged visible and nonfinite, zero with geometry_arranged=true.
- Several later captures also retain nonfinite records flagged nonvisible.

The new filter reports its checks passed because it excludes those unarranged records. Thus “159 passed” is not evidence that the original 40-record condition was eliminated. Strict JSON/null diagnostic handling is valid, and the independently recorded scroll-to-slot39/viewport-bounds observations are useful. They do not classify all other flagged records. Until independent expected-paint/culled evidence and a non-circular finite check are verified, geometry remains **provisional**, while the seven route/class/Slate observations stay confirmed.

## Avatar finding and repair receipts

The selected Rank host/page hashes also match the v5 inputs. V5-AVATAR-01 remains open; no repair receipt or avatar-backoff regression result is included. The three successful native tests do not contain an avatar test. No old API description or already settled owner-v5 correction is reopened here; the six owner-v5 source/overlay dispositions are carried forward unchanged.

## Current Source work versus the tested checkpoint

The selected current Root `.cpp` and `InkFrontendExtensions.cpp` differ from their build11 epoch hashes. They represent newer integration work on the remaining feature hosts. These newer bytes are not granted build11/PIE03's results. The table explicitly labels production wiring as the **build11 checkpoint**: 7 connected core routes, 0 unconnected core placeholders, 37 feature routes outside this validated checkpoint. Their evolving Source integration can continue without repeating settled owner patches or the already successful NewUI3 tests merely to restore an old status label.

`stage-to-runtime-v6.csv/.json` separates:

- 44 staged screen declarations.
- 7 core routes connected and observed at the build11 checkpoint.
- 7 default route/shell smoke validations.
- 3 formal native successes, with Inventory's identity/Busy scopes stated per row.
- Geometry provisional for the observed routes; rigorous geometry completion remains 0.
- Real service Busy coverage remains unverified; visual approval and complete 44-screen acceptance remain pending.

The legacy broad `runtime_verified`/full-page-acceptance fields stay false so they cannot silently mean geometry or visual acceptance. The new explicit route-validation fields record the seven confirmed successes. The v5 statements that no execution records were then visible remain historical cutoff statements; they must not be used as the current status.

## Changes after this bounded capture

Final drift detection found newer hashes for `verify_default_root_pie.py`, `InkFrontendEditorLibrary.cpp` and `InkRebuildLeaderboard.cpp`. Those repair bytes are outside this selected source verdict; their paths and before/after hashes are in `readiness-v6.json`. Reconcile their new receipt/diff next, rather than redispatching an old source description without checking the changed bytes. Geometry stays provisional until the new independent classification/check has rigorous execution evidence; the formal NewUI3 successes and seven default-route observations remain confirmed historical executions.
