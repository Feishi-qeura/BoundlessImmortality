# Batch-2 promotion plan

The panel team does not promote files into the real Source tree. `logic-integration` owns source promotion and route wiring; the asset owner controls imports.

## Merge units and order

1. **Shared activity detail shell** — add the staged header/cpp and generated-header build inputs under the approved activity UI source folder. Map `DailyActivityDetail` to `UInkDailyActivityDetailBatch2Panel` and `TimedActivityDetail` to `UInkTimedActivityDetailBatch2Panel` in `UInkFeaturePanel::ClassForRoute`. Keep selected parent/list state intact.
2. **Launch rank page** — map `LaunchRank` to `UInkLaunchRankBatch2Panel`; launch page navigation uses the existing `Open_LaunchRank` action convention.
3. **Launch page** — map `LaunchEvent` to `UInkLaunchEventBatch2Panel` after the retained launch route is switched deliberately. The staged page routes an active event to `WulingWorld` and requires a second explicit click before submitting a new event start. No purchase affordance should be enabled until a verified SKU, price/currency, entitlement gate, and service purchase command are present.
4. Import only the three transparent component PNGs after the asset import lease is granted. Keep page concepts outside runtime texture imports.

## Staged files

- `staged-source/InkActivitiesBatch2Panels.h`
- `staged-source/InkActivitiesBatch2Panels.cpp`
- `assets/` (six candidate PNGs; three page concepts and three transparent components)
- `assets-manifest.json`, `component-contract.yaml`, `coverage.md`, `visual-research.md`, `handoff.md`, `review-resolution.md`

## Promotion validation sequence

1. Confirm field semantics and route ownership. Keep `approved:false`; visual review can proceed in parallel and is not a gate for wiring these page compositions.
2. Verify the selected action routing against the live navigation contract; resolve launch route behavior and rank action naming.
3. Import transparent components only; check dimensions, alpha, and no unexpected material/texture settings.
4. Compile after source promotion, then run focused adapter automation and PIE for daily sign-in, timed detail states, launch tabs/offer disabled state, and returned rank ties.
5. Record compile, automation, and PIE as separate evidence. They do not imply Cook/package/device acceptance.

This plan does not grant import, route wiring, or source promotion authority.
