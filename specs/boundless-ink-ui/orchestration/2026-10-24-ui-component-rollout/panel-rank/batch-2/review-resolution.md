# Rank batch-2 review resolution

Review: `logic-core/batch-2-review/review.md` (2026-10-24). Source claims below were checked against the review and the staged panel-rank files. No engine or runtime verification was run.

## PR-01: Wuling member selection and removal

**Confirmed.** The original host creates clickable `RecordRow_i` actions; their handler assigns `SelectedMember` from the matching `Rows` entry. `RecordRemove` uses that selection to submit the existing remove command. The original staged text-only roster had no way to select a member if it replaced the visible list.

**Resolved in staged interface and composition.** `BuildWulingRosterComposite` now accepts `ExistingMemberRowWidgets`, paging action widgets, and the existing remove action widget. It reparents the supplied row widgets into its roster canvas and moves the existing remove widget into the management area. It does not construct name-only substitutes or bind new handlers. The concrete host capture and preservation sequence is in `host-callsite-suggestions.md` and `api-map.md`.

The integration patch still needs to capture each widget returned by the existing `Action(...)` calls and pass those pointers through. `Rows`, `SelectedMember`, the selected/enabled state, `HandlePanelAction`, and the service request stay host-owned. Until integration applies that callsite, this is a corrected staged implementation/interface, not a repaired live page.

## FRankChrome lifetime and reflection

**Confirmed.** `FRankChrome` is a plain C++ struct, not a `USTRUCT`, and cannot itself be declared as a UPROPERTY.

**Resolved in handoff guidance.** Do not persist the struct as a reflected member. The attached widgets remain children of the host `WidgetTree`; the host can resolve canonical named widgets from that tree during refresh. If cached pointers are needed, integration must add separate typed `UPROPERTY(Transient) TObjectPtr<...>` fields and reset them before a destructive rebuild. This avoids implying that unreflected UObject pointers are GC roots.

## Existing rows, avatar subscriptions, and adaptive layout

The leaderboard factory continues to reparent the same persistent scroll and row subtree. It does not clone rows or call clear. The host must capture that scroll before any old `Clear(Page)` path, retain canonical scroll/row names for avatar updates, and keep existing refresh/subscription owners.

The review also identified the host's canonical names consumed by `UpdateLeaderboardAdaptiveLayout`. The handoff now requires using those names or explicitly extending the adaptive method for every new shell control, then invoking it after assembly and on the current resize/orientation path. The factory still contains 720-coordinate composition values; no source or runtime evidence proves responsive integration yet. This remains an integration acceptance item, not a claimed pass.

## Verification boundary

- Staged source/interface: updated for PR-01.
- Host callsites: suggestions only; not applied to `Source/`.
- UHT/UBT compile: not run; `compile_verified=false`.
- Import/runtime/route/PIE: not run; corresponding flags remain false.
- Visual approval: pending user; candidate state remains `approved:false`.
