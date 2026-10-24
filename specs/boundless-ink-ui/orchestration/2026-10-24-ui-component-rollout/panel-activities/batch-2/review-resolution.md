# Panel Activities batch-2 review resolution

Reviewed `logic-core/batch-2-review/review.md` against the staged source and current public declarations/service fields. This is a static source review only.

| Finding | Result | Evidence |
|---|---|---|
| PA-01: `ESurface` is unavailable unqualified in this translation unit | Fixed | Every staged `Surface` call now uses `InkPresentationStyle::ESurface::FeatureCard`. The enum declaration is in `Frontend/InkPresentationStyle.h`; the base API also uses its fully qualified name. |
| PA-02: launch, rank, and timed detail lack an effective live refresh path | Fixed in staged source | All three panel types override `RefreshLiveData`. Each checks `IsRolloutPageActive` and compares a content key built from route/active state, snapshot version, enabled/connected/busy/status, selected tab or activity ID, service day, event/window boundary state, and serialized relevant authority fields. Only a key change calls `RebuildPanel`, preserving selected state and preventing unchanged per-frame rebuilds. Async snapshot/receipt changes update version, status, busy state, or relevant data and therefore invalidate the key. |
| PA-03: `launch.complete_rounds` was read as a number despite being an array | Fixed | `ArrayCountOrPending` calls `TryGetArrayField`. Explicit empty array means `0`; absent or wrong-type data remains `待同步`. Milestone-row `complete_rounds` remains numeric because those rows carry scalar thresholds. |

## Scope and verification

- Changes remain inside `panel-activities/batch-2/`; no actual `Source`, `Content`, `Binaries`, or `Intermediate` file was edited.
- Page concepts remain `approved:false`. The user authorized shell and integration iteration without waiting for another visual approval, so visual review is informational and does not gate the handoff.
- No UHT, UBT, UE editor, automation, PIE, Cook, import, or runtime visual check was run. Compile/runtime status remains unverified.
