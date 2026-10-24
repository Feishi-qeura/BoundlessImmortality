# Panel journey handoff

Owner: `panel-journey`; contract revision: `newui-r1`. Two independent 2-page merge slices are ready: `journey-a` (world/resources) and `journey-b` (daily/monthly sign-in). Follow-on route shells are also staged for `journey-c` (resource detail) and `journey-d` (world/race results). See the four page notes in this directory.

Current integration candidate: `review-v4/` is the complete replacement for the panel-journey source snapshot. `review-v2/` and `review-v3/` remain frozen. The older root `staged-source/` is historical and is not the current candidate; use the v4 host integration notes and diff before wiring.

## Delivered source

Staged under `staged-source/Frontend/UIRebuild20261024/Journey/` for proposed target `Source/BoundlessImmortality/Frontend/UIRebuild20261024/Journey/`:

- `InkJourneyNewWidgets.h/.cpp`: native responsive map page base, world and resource subclasses, daily and monthly sign-in pages, and a shared detail/result shell with distinct resource-detail, world-result and race-result classes.
- `UInkJourneyMapPinButton`: exact EntryId selection event.
- `UInkJourneyMilestoneButton`: explicit selected milestone event.
- `UInkJourneyResourceDetailPageWidget`, `UInkJourneyWorldResultPageWidget`, `UInkJourneyRaceResultPageWidget`: distinct route classes sharing the detail/result shell.
- All page roots are built once in `NativeOnInitialized` with a root guard; delegates use `AddUniqueDynamic`.
- Candidate map backgrounds are listed with dimensions, pixel format and SHA256 in `assets/asset-manifest.yaml`. Expected targets are `/Game/newUI/Textures/Journey/T_NUI_JourneyMapPortrait_V1` and `/Game/newUI/Textures/Journey/T_NUI_JourneyMapLandscape_V1`. Review-v3 requests canonical font packages `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody` per migration map; import/readback remains unverified. The review-v2 path `/Game/newUI/Shared/Fonts` is stale and is not a delivered asset location.

## Host API and action wiring

`UInkJourneyMapPageWidget::SetAuthorityData(State, Entries, SelectedEntryId)` is a VM presentation setter. The host owns mapping from the authoritative snapshot and calls it on data change (not every frame). Same-entry refresh preserves confirmation/pending/feedback. Changing selection resets confirmation; pending remains until its adapter result and keeps the primary action blocked. Hosts must preserve the request subject/token across any selection refresh. Detail/result hosts should retain one presenter per in-flight exact source or otherwise route completion back by request identity. `ResetInteractionForReopen()` clears confirmation but retains an in-flight pending request. Completion/failure must resolve with `SetTravelFeedback` only after the real adapter result. Back emits `OnBackRequested` for the original root/feature stack.

Map actions emit `OnTravelRequested(ExactEntryId)` only after the page's confirmation step. Host derives the action from concrete page class: `world-travel` for world; resource request uses the existing resource action's `QueueRace(node_id)` semantics. For either action: recapture with `InkJourneyAdapter::Capture`, evaluate the exact subject and `confirmed=true`, then submit once if current gates pass. UI confirmation is not a service authorization.

`UInkJourneyDailySignInPageWidget::SetPresentation(ServerDay, State, Status, HasMonthCount, MonthCount, HasVerse, Verse)` accepts `activities.day`, `activities.signed`, `month_sign_count`, and `activities.verse`. The verse is a separate read-only field and does not imply fortune. `OnClaimRequested(ExpectedDay)` binds the exact shown service day; host calls `Capture/Evaluate/Submit(SignIn)` and returns the authoritative result through `SetClaimFeedback` and refreshed presentation.

`UInkJourneyMonthlySignInPageWidget::SetPresentation(ExpectedServiceDay, ServerMonth, CalendarDayCount, ServerDay, HasMonthCount, MonthCount, Milestones, Status)` uses explicit provider fields. No day-history status is synthesized. Each milestone carries known/eligible/claimed/status/reward rows. `OnMilestoneClaimRequested(ExpectedDay, Milestone)` maps to `MonthlyChest` after a fresh Capture/Evaluate with confirmation; resolve pending only from actual completion/failure. No receipt is dequeued by these widgets.

`UInkJourneyDetailResultPageWidget::SetPageData(Title, ExactSourceId, Status, Rows, CanTravel)` is a common composition shell, not a complete route adapter. New integration must create actual distinct result/detail presenters for resource detail, world result, and race result, fill rows from authoritative `lease`, `round`, `race`, and exact-source `reward_receipts`, and preserve the prior page stack on Back. Receipt matching must not drain the global inbox or substitute a generic detail page for `world-result`/`race-result`.

## PJ review items resolved in this staged revision

See `review-resolution.md`. PJ-01 and PJ-03 are addressed by source structure. PJ-02 is addressed as an explicit new-root UUserWidget host contract; actual host/feature routes remain for `logic-integration`.

## Not done / evidence limits

- No UHT/UBT/compiler, Editor, import, or runtime execution was performed.
- Source is not compiled; assets are not imported; routes are not wired; business logic is not verified; runtime and player-entry evidence are absent.
- No actual visual acceptance has been claimed. `approved:false`, `visual_pending_user:true`.
- Further journey screens remain outstanding: region detail, resource ranking/rewards, and any fortune surface only when a separate authoritative provider contract exists. World and race result sources remain awaiting integration mapper/receipt wiring.







## Review-v2 continuation

`review-v2/` contains the untouched-prior-stage correction copy and subsequent Journey page compositions. Use `review-v2/handoff.md` for the current source revision, class constructors, VM update methods, events, route identities, authority action mapping and evidence limits. It adds the contract's 3-page `journey-a` slice and continues through slices `journey-f`. The previously handed-off `staged-source/` was not overwritten; production Source/Config/Content remains unchanged by panel-journey.

