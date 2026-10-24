# Panel Activities batch-2 handoff

## Status

Three full native panel compositions are ready for the single `logic-integration` source owner to promote: launch activity/tasks/disabled offer, launch contribution rank, and shared daily/limited detail. This adds page compositions; it is not only a countdown skin. The offer stays unavailable because the service currently exposes no verified SKU/price/purchase contract. ACT_SIGN is the only detail action submitted through the existing journey adapter.

PA-01 through PA-03 are resolved in `review-resolution.md`. Staged source remains unpromoted and `approved:false`. The user authorized continued shell/integration work without waiting for a further visual approval; the concepts remain available for review but do not block wiring the native compositions.

## Staged source and proposed target

- `staged-source/InkActivitiesBatch2Panels.h`
- `staged-source/InkActivitiesBatch2Panels.cpp`
- Suggested target folder: `Source/BoundlessImmortality/Frontend/UI/Activities/`.
- Classes: `UInkLaunchEventBatch2Panel`, `UInkLaunchRankBatch2Panel`, `UInkDailyActivityDetailBatch2Panel`, `UInkTimedActivityDetailBatch2Panel`, shared `UInkActivityDetailBatch2PanelBase`.

Copy only these staged files when promotion is authorized; retain the staged copies unchanged until the source owner confirms promotion. This handoff does not authorize edits to the real Source tree.

## Route and merge order

1. Promote the shared detail shell. Map `DailyActivityDetail` and `TimedActivityDetail` in `UInkFeaturePanel::ClassForRoute` to the concrete daily and limited classes. Preserve the frontend `SelectedEntry` context.
2. Promote rank and map `LaunchRank` to `UInkLaunchRankBatch2Panel`. The launch page uses the existing `Open_LaunchRank` action convention.
3. Deliberately switch `LaunchEvent` to `UInkLaunchEventBatch2Panel`. Starting requires a second confirmation before `InkJourneyAdapter::Submit(StartLaunch)`; an active, unexpired authority event enters the existing `WulingWorld` route.
4. Import the three transparent component sprites only through the asset-import owner. Keep page concepts as preview artifacts, not runtime textures.

## Data and refresh notes

- Live refresh keys include route activity, snapshot version, connection/busy/status, selected tab or activity ID, service day, event/window boundary state, and relevant serialized launch/activity fields. Rebuilds occur only when this key changes; selected tab/entry and confirmation state remain intact.
- Launch date display comes from server `start/end`; organization progress counts `complete_rounds` only after successful array retrieval.
- Do not add fake offers, task claim actions, rank rewards, inferred personal rank, or local expiry decisions.
- See `component-contract.yaml`, `field-action-mapping.md`, and `coverage.md` for contracts and remaining runtime coverage.

## Evidence boundary

Only static source review was performed. Import, route wiring, compile, automation, PIE, Cook, package, device, and platform acceptance remain outstanding. Promotion and engine verification belong to `logic-integration`; do not overwrite the staged copy or start a compile from this handoff.
