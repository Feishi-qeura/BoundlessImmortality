# PJ-01 / PJ-02 / PJ-03 resolution for rebuild revision

Review source: `specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-core/batch-2-review/review.md`. This resolution applies only to this new `panel-journey` revision; the prior `orchestration/.../panel-journey/batch-2/` files are unchanged by this handoff.

## PJ-01 — root lifecycle

**Resolution in staged source:** all new page owners build their native root once in `NativeOnInitialized`, behind `if (!WidgetTree->RootWidget)`. Page and child-button delegates use `AddUniqueDynamic`. No `NativeConstruct` root replacement is used.

**Evidence boundary:** source inspection only. UHT/UBT, Slate rebuild, repeated insertion/removal and runtime event count have not been exercised.

## PJ-02 — route-host type mismatch

**Resolution in this architecture:** the new map/sign-in/detail/result compositions remain standalone `UUserWidget` owners. They are not presented as `UInkFeaturePanel` subclasses and must not be assigned directly to `TSubclassOf<UInkFeaturePanel>` / `ClassForRoute`. The integration owner creates them beneath the new root presentation host, retains them with `UPROPERTY`, provides exact route and source IDs, binds each page's delegates, and returns Back through the original page/feature stack.

Current contract route IDs include `world`, `resources`, `sign-in`, and `monthly-sign-in`. The generic detail/result shell is not permission to alias distinct `resource-detail`, `world-result`, and `race-result` routes. Integration must map the actual route to the correct presenter and preserve the old route stack semantics. This closes the incorrect direct-cast assumption while host implementation and route registration remain outstanding.

**Evidence boundary:** contract/handoff only. No production route or host was changed or wired by panel-journey.

## PJ-03 — same-entry refresh state

**Resolution in staged source:** map `SetAuthorityData` clears confirmation when `SelectedEntryId` changes, while same-entry refresh updates rows/state and retains confirmation, pending, and feedback. A source-ID change clears confirmation but does not discard an in-flight request; feedback resets only when no request is pending. `ResetInteractionForReopen` / `ResetForReopen` are explicit route-reopen resets that preserve in-flight pending state. The host must only clear pending after actual enqueue failure, service completion/failure, or page cancellation; it must not clear it merely because a snapshot refreshed. Hosts retain the exact in-flight subject/request token even if the visible selection changes.

**Evidence boundary:** source inspection only. Busy/idempotency behavior and pending completion are still delegated to adapter/host and have not been executed.

## Status

PJ-01 and PJ-03 are structurally addressed in the staged source. PJ-02's type mismatch is resolved by the explicit standalone-widget host architecture, while its production route/stack integration remains a `logic-integration` task. This team's status remains source-only, not compiled, not imported, not routed, not logic-verified, and not runtime-verified. Candidate visuals remain `approved:false`; user visual acceptance is pending.



