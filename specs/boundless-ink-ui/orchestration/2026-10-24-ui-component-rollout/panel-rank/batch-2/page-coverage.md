# panel-rank batch-2 page coverage

Latest baseline read: `logic-integration/screen-coverage.json`, timestamped in that file by the integration owner; the current entries still say `partial_component_mounted` for `leaderboard` and `wuling`, with `import_verified`, `route_wired`, `logic_verified`, and runtime/compile evidence false.

| Page | Batch-2 composition | Real provider/action seam | State after this handoff | Next piece |
|---|---|---|---|---|
| Damage leaderboard | Shared title, category/scope tabs, field header, provider state, stable existing row viewport, self-position strip | `InkCore::ReadBoard(..., Damage)`; existing actions `RankDamage`, `RankLocal`, `RankGlobal`; existing `RefreshLeaderboardRows` remains owner of persistent row widgets and avatar requests | Source staged; not promoted, compiled, imported, route-mounted or runtime tested. Self position explicitly requires a service-provided value; no local derivation | Confirm whether service response gains self position and period options; no period control until an action/data contract exists |
| Cultivation speed leaderboard | Same shared composite with `CultivationSpeed` title and its category metric label | `InkCore::ReadBoard(..., CultivationSpeed)`; `RankCultivation`; the metric is whatever existing provider supplies | Source staged; same evidence limits as damage | Preserve existing row/cache behavior when integrating |
| Anchor spirit leaderboard | Same shared composite with `Anchor` category | `InkCore::ReadBoard(..., Anchor)`; `RankAnchor`; label/metric from existing theme and service entry | Source staged; no streamer personal stat is added | Confirm category metric meaning with service owner; do not merge streamer value |
| Wuling overview | Separate identity/status, three service count fields, public supply/reputation, and existing action-widget slot | `UWulingPrototypeSubsystem::Snapshot()` via existing host; `CreateGuild` remains in `UInkWulingSectPanel`; supplied action widgets retain `Ext_` dispatch | Concrete composition source staged; missing values display em dash; no new action was invented | Integrate call in `UInkWulingSectPanel::BuildPanelContent` and retain its `RefreshLiveData` text updates |
| Wuling member list | Separate current-page state, up to eight existing clickable `RecordRow_i` widgets, page controls, and the existing `RecordRemove` control | Host `Rows`/`SelectedMember`; `member_cursor`; `RequestMemberPage`; `HandlePanelAction` row selection and remove handling | Staged composite reparents the actual row and remove widgets; no new labels substitute for the selection chain. Live integration remains pending | Capture and pass the original widgets; verify selection, enabled state, removal submission, cursor and service permission behavior |

## Explicit backlog

1. Wuling construction page (`WulingConstruction`, currently routed to `UInkWulingToolsPanel`).
2. My sword spirit page and its real `SwordVm` fields.
3. Gift rules page (`GiftRules`) and its `GiftEffectVm` source.

Contribution is not aliased to orders, offline gains, or rights. No contribution field/period was found in the current Wuling snapshot usage or adapter contract.
