# Player component rollout handoff

## Delivery status

- `source_ready`: true (one standalone presentation atom under the assigned Player source directory).
- `assets_ready`: true for the 16 copied approved HUD components in the candidate ZIP; the newly prompted breakthrough frame is `pending-export` and excluded.
- `import_verified`: false. No Unreal Editor/UBT/import operation was run; `engine-lease.json` says `not_granted`.
- `route_wired`: false for this rollout. Existing Player routes and panel classes are identified below; public route files are shared and integration-owned.
- `logic_verified`: false for the new visual atom. Existing native gameplay behavior was read and preserved, but no new gameplay action was added.
- `visual_pending_user`: true. All three v3 source page candidates remain `approved:false`; no user visual approval is claimed.

## Page coverage and integration seam

See `page-component-map.md` for full source, component, field, and state mapping. Summary:

| Page | Existing implementation to reuse | New assets/class | Integration point | State |
|---|---|---|---|---|
| Main menu | `UInkFrontendWidget`, `AInkMenuDirector` | None; live scene remains authoritative | `ShowPage("Main")` and existing action IDs | Reuse identified; no route changes |
| HUD | `UInkFrontendWidget` | 16 approved HUD PNGs in ZIP | Existing action IDs; text/live data stays in widget tree | Source package ready; not imported |
| Character | `UInkCharacterProfilePanel` | No replacement art | `ShowPage("Character")`; existing field refresh and profile actions | Protected behavior preserved |
| Inventory | `UInkFrontendWidget`, `UInkInventoryModel` | Existing approved bag icon only | `ShowPage`, `GetInventoryModel`, `RefreshInventory`, `RebuildInventoryPage`, `ExecuteInventoryAction` | Protected behavior preserved |
| Breakthrough preparation | `UInkWulingToolsPanel` through `UInkFeaturePanel::ClassForRoute` | `UInkPlayerMaterialSlotWidget` | `ConfigureSlot(...)`; bind `OnSlotActivated(SlotId, ItemId)` to the authority adapter; current alias is `BreakthroughPrepare` -> `WulingBreakthrough` | Atom source ready; integration and player-growth field mapping pending |
| Tribulation live/result | Existing `UInkWulingToolsPanel` adapter surface must be reconciled with player authority | No separate result image; render one authoritative outcome state | `RefreshLiveData()` and existing panel action dispatch; only authority-approved actions | Runtime adapter review pending; candidate remains unapproved |

## New class interface

- Header: `Source/BoundlessImmortality/Frontend/UiRollout20261024/Player/InkPlayerMaterialSlotWidget.h`
- Class: `UInkPlayerMaterialSlotWidget : UUserWidget`
- Configuration: `ConfigureSlot(FName SlotId, FName ItemId, const FText& ItemName, const FText& OwnedCount, const FText& RequiredCount, UTexture2D* ItemIcon, UTexture2D* FrameTexture, bool bCanSelect)`
- Gate: `SetSelectionEnabled(bool)`
- Delegate: `OnSlotActivated(FName SlotId, FName ItemId)`. This emits a request only; the adapter performs authoritative validation/action and refresh.
- The widget has no prices, material names, counts, readiness, forecasts, or reward defaults.

## Owned artifacts

All team documents, scripts, prompts, manifests, validation, copied PNGs and ZIP live under `specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/panel-player/`. The C++ atom is the only source change and stays under the assigned `Frontend/UiRollout20261024/Player/` directory. No shared route/class files were modified.

- `component-contract.yaml`: component IDs, states, source approval, usage, and slice rules.
- `asset-manifest.json`: measured PNG properties, alpha bounds, SHA-256 and upstream status.
- `player-components-candidate.zip`: 16 local approved component copies plus contract/manifest/map/validation.
- `pixel-validation.json`: per-file dimensions, mode, alpha bounds, hash, state/slice status.
- `import_player_components_unreal.py`: prepared only; must be run by logic-integration while holding the engine lease.
- `prompts/components/breakthrough-material-slot-frame-default-v1.md`: saved prompt; output is pending local export.

## Verification run

```powershell
$gameUiSkillRoot = 'C:\Users\zxxuh\.codex\skills\gameui'
$gameUiPython = Join-Path $gameUiSkillRoot '.venv\Scripts\python.exe'
& $gameUiPython 'specs\boundless-ink-ui\orchestration\2026-10-24-ui-component-rollout\panel-player\build_player_component_package.py' --project-root 'D:\demo\BoundlessImmortality'
```

Result: 16/16 selected PNGs exist; their actual dimensions, file format/mode, alpha channel/bounds, SHA-256, source status and slice contract were read from disk. All 16 match upstream `approved`/`user-approved` records. All 16 use `full` fixed-ratio slices; 0 use nine-slice, so no margins were invented. The candidate breakthrough frame is not counted as ready.

No UBT, editor import, route migration, PIE, Cook, or runtime acceptance was performed. Do not report the package ZIP or static PNG verification as Unreal import/integration.

## Next actions for integration owner

1. Review this handoff and ask the supervisor for the bounded engine lease before importing.
2. Run the prepared import script in Unreal Editor, inspect AssetRegistry and Texture2D properties, save the readback result.
3. Bind the new material slot atom inside the existing breakthrough route only after logic-core supplies the authoritative field/action adapter; reconcile live and mutually exclusive outcome fields for tribulation.
4. Wire/review page components against actual saved WBP and route files, then run the authorized build/runtime verification.
5. Keep candidate `approved:false`; user owns visual review.
