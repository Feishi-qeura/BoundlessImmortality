# panel-rank handoff

## Milestone

Rank and Wuling component contract, one measured text-free candidate container, and a reusable native UMG widget factory are ready. The v2 screen source remains `approved: false`; the 2026-10-24 trial authorization permits this implementation work without changing that review state.

## Page coverage

| Candidate page | Existing business/widget reuse | New component/class and seam | State |
|---|---|---|---|
| Damage leaderboard `rw-leaderboard-damage-candidate-v2.png` | `UInkFrontendWidget` leaderboard tree; `InkLeaderboardTheme::CategoryTitle/MetricLabel`; `InkLeaderboard::FRowLayout`; `SetLeaderboardEntriesForScope` | `InkRankWulingComponents::AddPaperPanel`, tab/header/row/footer pieces are composable with the new factory; host remains `UInkFrontendWidget` | Source and component contract ready; category action exists as `RankDamage`; candidate import, integration of this component pack, and GUI review pending |
| Cultivation speed leaderboard `rw-leaderboard-cultivation-candidate-v2.png` | Same host and data setter; category key `CultivationSpeed`; separate gameplay HUD API via `RequestedGameplayLeaderboard` | Same reusable components; metric label stays sourced from `InkLeaderboardTheme` | Source and component contract ready; `RankCultivation` and HUD `HUDRankSpeed` exist; import/GUI review pending |
| Anchor spirit leaderboard `rw-leaderboard-spirit-candidate-v2.png` | Same host; category key `Anchor`; existing label currently maps to `剑灵数` | Same reusable components; do not infer/merge streamer-personal metrics | Existing rendering/action exists; v2 service metric semantics still need logic/service owner confirmation; import/GUI review pending |
| Wuling overview `rw-wuling-home-candidate-v2.png` | `UInkWulingSectPanel`; `UWulingPrototypeSubsystem`; current summary fields and route action `Open_WulingGuild` | Reusable paper panel, identity/summary cards and live labels; factory API accepts host-supplied dynamic `FText` and frontend actions | Existing prototype-backed overview identified; no new business action added; import/integration and visual review pending |
| Wuling members / roles | `UInkWulingRecordsPanel` route `WulingGuild`; `UWulingPrototypeSubsystem::RequestMemberPage` | Reusable row/card/tab pieces; actual permission-gated actions remain owned by host/service | Route exists; verify service permissions and data cases in the logic team's acceptance |
| Wuling contribution / records | Existing `WulingOrders`, `WulingOffline`, `WulingRights` are separate views in `UInkWulingRecordsPanel`; do not relabel these as contribution unless service contract confirms | Reusable paper/card components only | No distinct contribution route or contribution-period model confirmed in this audit; explicitly remains unimplemented/unverified |
| Wuling management/tools | `UInkWulingToolsPanel` for construction, equipment, activity and race/tool pages; `UInkWulingRecordsPanel` for organization actions | Existing local feature route API, dynamic actions are factory-compatible | Routes and prototype APIs exist for current functions; production authorization/permissions are not established by this handoff |

## Component and adapter APIs

- New factory: `InkRankWulingComponents` in `Source/BoundlessImmortality/Frontend/UiRollout20261024/RankWuling/InkRankWulingComponents.h` and `.cpp`.
- Factory calls: `AddPaperPanel(Tree, Parent, Name, Texture, Placement, SlicePixels)`, `AddDynamicText(Tree, Parent, Name, Text, FontSize, Placement, Color, Justification)`, `AddActionButton(Tree, Parent, Frontend, WidgetName, Action, Caption, Selected, Primary, Placement)`, and `AddNativeSurface(Tree, Parent, Name, Surface, Placement)`.
- Leaderboard host/data API: `UInkFrontendWidget` in `Source/BoundlessImmortality/Frontend/InkFrontendWidget.h`; `SetLeaderboardEntries`, `SetLeaderboardEntriesForScope(Entries, Scope)`, `SetGlobalLeaderboardConnection`; actions `RankDamage`, `RankCultivation`, `RankAnchor`, `RankClose`. HUD selectors `HUDRankSpeed`, `HUDRankDamage` stay separate.
- Leaderboard presentation mapping: `InkLeaderboardTheme::{CategoryTitle,CategoryLabel,MetricLabel,Color}` and `InkLeaderboard::FullRowLayout` in `InkLeaderboardTheme.h` and `InkLeaderboardPresentation.h`.
- Wuling overview API: `UInkWulingSectPanel` in `Source/BoundlessImmortality/Frontend/UI/InkWulingSectPanel.h`; refresh from `WulingFrontendData::Service(this)` / `UWulingPrototypeSubsystem::Snapshot()`; create operation calls `UWulingPrototypeSubsystem::CreateGuild(Name)` only through its existing action path.
- Wuling member/record adapter: `UInkWulingRecordsPanel` in `Source/BoundlessImmortality/Frontend/UI/InkWulingRecordsPanel.h`; pagination calls `UWulingPrototypeSubsystem::RequestMemberPage(Cursor)`; commands use existing `SubmitPrototypeCommand(Operation, ArgsJson)` routes. Service currently identifies itself as a local simulation, so this is not production authority evidence.
- Wuling tool adapter: `UInkWulingToolsPanel` in `Source/BoundlessImmortality/Frontend/UI/InkWulingToolsPanel.h`; route state is configured by `UInkFeaturePanel::Configure`/`SetContext`; current catalog actions use `RequestCatalog` and current prototype action APIs.
- Candidate import script: `specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/panel-rank/import_rank_wuling_candidate.py`. It targets `/Game/UI/Ink/ComponentRollout20261024/RankWuling`; it was prepared but not run.

## Asset evidence

- `assets/rank-wuling-paper-panel-default-v1.png`: measured 1448×1086 RGBA; alpha range 0..255; alpha bounds `[0,27,1410,1068]`; SHA-256 `9b243e6844d92ff182d88b3bd28807583e2ff1cf51f7884b9326a36f0a4ad293`.
- Candidate 9-slice margins are 136 px on each edge. The central paper area is clear in `evidence/rank-wuling-paper-panel-9slice-guide-v1.png`; this is only a proposed slice and still needs in-engine stretch preview.
- `component-contract.yaml` keeps every candidate component unapproved. Text-free visual inspection passed for the generated panel image; GUI typography, cropping, touch targets, and page-level visual acceptance remain pending the user and engine integration.
- `packages/rank-wuling-components-v1.zip` contains the isolated PNG, component contract, sprite contract, and measurements; archive integrity check returned no errors.

## Verification and limitations

- Ran `inspect_candidate.py` using GameUI's bundled Python 3.12/Pillow. Dimensions, RGBA mode, alpha bounds, guide render, and SHA-256 were written to `asset-measurements.json`.
- `import_rank_wuling_candidate.py` was not executed. No Content asset, Asset Registry, saved WBP, Editor, UBT, PIE, or device verification is claimed.
- New C++ factory source is prepared but was not compiled. Existing business routes remain the integration target; this handoff does not register the new factory or mutate a shared route.
- Logic tests, service permission coverage, rank response semantics, and review of candidate text remain with the integration/logic owners. Visual status remains `approved: false`.

## Next step

After the coordinator grants the engine lease, logic-integration can run the import script, verify texture parameters and Asset Registry, reference the texture through `AddPaperPanel`, then integrate the factory into the existing rank and Wuling trees one page at a time. Resolve the anchor metric mapping and decide whether a service-confirmed Wuling contribution route exists before adding its page. Do not label existing order/offline/rights panels as contribution data without that confirmation.
