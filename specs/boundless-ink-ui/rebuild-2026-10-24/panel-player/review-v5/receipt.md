# Player-d/e bounded review-v5 receipt

Updated: 2026-10-24 Asia/Hong_Kong  
Input: `model/review/bounded-v4-review.md`, findings V4-PD-01 and V4-PE-01.  
Status: review-only candidate, `approved:false`; frozen player-c/d/e and review-v4 files were not modified.

## Dispositions

| Finding | Disposition | Evidence |
|---|---|---|
| V4-PD-01 | Corrected by a contract overlay only. | Frozen Player-d `action-adapter.yaml` names `InkPlayerProcessPageWidgets`, while `InkPlayerCraftSourcesWidgets.h` declares `FActionRequested` and `FActionPayload` in `InkPlayerCraftSourcesWidgets`. The overlay replaces only those two documentation type paths and preserves canonical IDs, payload meanings, routes, and gates. No Player-d source or original contract was edited. |
| V4-PE-01 | Minimal source diff and explicit host contract prepared; diff not applied. | First-click intent now carries a host-owned `StageSubjectRevision`; the VM adds the current and captured stage revision; second-click UI requires the captured stage revision, selection revision, and mode to still match, and submits the captured values. Host contract binds that revision to the actual authority `(stage_id, from_realm_index, to_realm_index)` tuple and requires an exact first-click capture/second-click comparison before fresh eligibility dispatch. |

## Required host behavior

`StageSubjectRevision` is not a display field or action parameter. The host changes it whenever the actual stage/realm tuple changes, even if selected materials and their revision do not. It captures the actual tuple, material map/revision, and mode on the first click; it invalidates the confirmation on any subject change. On the second click it compares the captured identity against current host and fresh authority state before evaluating Prepare/Rework availability. It must not replace the captured subject with fresh current values. Unknown display fields such as `q_normalized`, readiness, slots, sources, or growth range do not gate a host-authorized Prepare/Rework action.

## Frozen input fingerprints

| Frozen file | bytes | SHA-256 |
|---|---:|---|
| player-d `contracts/action-adapter.yaml` | 2308 | `69B98D244A2A91D4783704DBC01B010AA9B71A9DDDB3F0C22721FBBDB47811AB` |
| player-d `InkPlayerCraftSourcesWidgets.h` | 3761 | `15AEEA0FC79CF62E1160192DD8326069CB20F0578E8618F7A8E46FAD835D8DB4` |
| player-d `InkPlayerCraftSourcesWidgets.cpp` | 17726 | `2556D05F2A294FA0148AA947D11153702A8513A120E9801AAFC331C30289B4F8` |
| player-e `contracts/action-adapter.yaml` | 3070 | `785A54736A2636FDBBFD00F5C3634E92C6A7DC4D7B67A2939B3BB78FF609E53A` |
| player-e `InkPlayerBreakthroughTribulationWidgets.h` | 4978 | `CB8D0E2276E633706F4B9A53383FFB8E8F73222063FF2C563A25A9D2C591C117` |
| player-e `InkPlayerBreakthroughTribulationWidgets.cpp` | 23505 | `762BFA4CCBFD6FC30263EAD518771C13BBA5857C15A7F00564B43EEC60DBBB03` |

Player-c PC3-01 remains satisfied by the frozen typed relay with `AddDynamic`/UFUNCTION; no duplicate patch is proposed. The PC3-02/03 diff in review-v4 remains ready and untouched.

## Verification boundary

The review-v5 player-e diff is intended for `git apply --check` only; no source edit is included here. Static contract/hash/source checks do not prove compilation. No Unreal build, test, Editor, PIE, cook, package, device, platform check, or image generation was run.
