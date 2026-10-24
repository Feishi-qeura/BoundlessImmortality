# Review-v2 exact source and integration handoff

Owner: `panel-journey`. Contract revision: `newui-r1`. All pages below are staged for review only. No production source, config, UMG asset, content path, route, or engine state was changed. `approved:false`, visual acceptance pending.

## Corrected source location

- Header: `review-v2/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.h`
- Implementation: `review-v2/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.cpp`
- Proposed integration target: `Source/BoundlessImmortality/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.h/.cpp`
- SHA256 is recorded in `status.json`. The previously delivered `../staged-source/` remains unchanged; integration should review this revision before consuming it.

## Construction rule

All page classes are `UUserWidget` and use a default `CreateWidget<T>(OwningPlayer)` construction path. They accept no constructor parameters. Create the widget under the new root owner, retain it with `UPROPERTY`, add it under the root's page host, bind delegates, then call the VM setter below. The map-pin, milestone, and race-action controls are also `UUserWidget`; each builds a child `UButton` and live Text inside its own `WidgetTree` during `NativeOnInitialized`, and retains the child as a reflected `TObjectPtr`. None derives from `UButton` or calls `WidgetTree` through a `UButton` base.

## Page APIs and action/route mapping

| Slice / page ID | Concrete class | VM setter or update | Events and host action |
|---|---|---|---|
| `journey-a` / `journey` | `UInkJourneyHubPageWidget` | `SetAuthorityState(WorldStatus, GuildStatus)` | `OnBackRequested`; `OnNavigationRequested(ActionId)` emits exact `open-world`, `open-resources`, `open-wuling`; host maps to new-root routes. |
| `journey-a` / `world` | `UInkJourneyWorldMapPageWidget` | `SetAuthorityData(State, Entries, SelectedEntryId)`; `SetTravelFeedback(Feedback, bRequestPending)` | map emits exact `EntryId`; confirmed event maps `world-travel`; fresh Capture/Evaluate/Submit. |
| `journey-a` / `resources` | `UInkJourneyResourceMapPageWidget` | same map setter, with separate rows sharing `GroupId=node_id` | exact resource `EntryId` maps `resource-travel`; existing adapter/service queues `QueueRace(node_id)`, not a lease grant. |
| `journey-b` / `sign-in` | `UInkJourneyDailySignInPageWidget` | `SetPresentation(ServerDay, State, Status, HasMonthCount, MonthCount, HasVerse, Verse)`; `SetClaimFeedback` | `OnClaimRequested(ExpectedDay)` maps `sign-in`; service day binds request. Verse display is not fortune. |
| `journey-b` / `monthly-sign-in` | `UInkJourneyMonthlySignInPageWidget` | `SetPresentation(ExpectedServiceDay, ServerMonth, CalendarDayCount, ServerDay, HasMonthCount, MonthCount, Milestones, Status)`; `SetMilestoneFeedback` | `OnMilestoneClaimRequested(ExpectedDay, Milestone)` maps `claim-monthly-chest`, fresh evaluate with confirmation. Calendar history is not synthesized. |
| `journey-b` / `daily-verse` | `UInkJourneyDailyVersePageWidget` | inherited `SetAuthorityData(Status, bCollectionProvided, ExactSourceId, Rows)` | read-only `activities.verse`; no fortune draw; Back only. |
| `journey-c` / `region` | `UInkJourneyRegionPageWidget` | `SetAuthorityData(ExactEntryId, Name, State, Rows, bCanTravel)`; `SetTravelFeedback` | `OnTravelRequested(ExactEntryId)` maps confirmed `world-travel`. |
| `journey-c` / `resource-detail` | `UInkJourneyResourceDetailPageWidget` | inherited `SetPageData(Title, ExactSourceId, Status, Rows, CanTravel)`; `SetTravelFeedback` | selected exact resource id maps confirmed `resource-travel`; source rows/receipt query remain exact. |
| `journey-c` / `world-result` | `UInkJourneyWorldResultPageWidget` | inherited `SetPageData(..., bCanTravel=false)` | read-only `snapshot.round` and exact source receipts; Back only. |
| `journey-d` / `race-lobby` | `UInkJourneyRaceLobbyPageWidget` | `SetAuthorityData(QueueSubjectId, QueueState, Rows, bCanCancel)`; `SetCancelFeedback` | `OnCancelRequested` maps `cancel-race-queue` after fresh host evaluation; queued is not matched. |
| `journey-d` / `race-live` | `UInkJourneyRaceLivePageWidget` | `SetAuthorityData(ExactRaceId, Phase, Rows, Actions)`; `SetActionFeedback` | `OnRaceActionRequested(ExactRaceId, Kind)` maps provider-supported `race-action`; exact IDs and current gates required. |
| `journey-e` / `lease` | `UInkJourneyLeasePageWidget` | inherited read-only `SetAuthorityData(...)` | `snapshot.lease` with exact node, status, owner, online seconds, end UTC; Back only. |
| `journey-e` / `race-result` | `UInkJourneyRaceResultPageWidget` | inherited `SetPageData(..., bCanTravel=false)` | exact race/team and source receipt rows; read-only Back. |
| `journey-f` / `resource-rank` | `UInkJourneyResourceRankPageWidget` | inherited `SetAuthorityData(Status, bCollectionProvided, SourceId, Rows)` | no ranking contract exists: `bCollectionProvided=false`, show unavailable status, no fabricated rows. Back only. |
| `journey-f` / `resource-rewards` | `UInkJourneyResourceRewardsPageWidget` | same read-only setter | host supplies only exact-source `RewardVm` receipt rows; no global dequeue/ack or inventory-delta grant. Back only. |

Call VM setters only on relevant source changes, not each frame. The common `OnBackRequested` delegate restores the new root's parent page/feature stack. If an in-flight action's exact subject changes, invalidate that page's pending UI and let the host reconcile the response by retained request identity. Same-subject view updates preserve pending state. Service enqueue is always pending until an authoritative result/receipt says otherwise.

## Authority mapping notes

- World rows: `snapshot.journey.worlds[]` exact `id/name/open/x/y/difficulty/description/rewards[]`; coordinates affect pin layout only.
- Resource rows: `snapshot.journey.resources[]` exact `id/node_id/name/open/quantity/item_id/item_name/interval_seconds/qi_additive/lease_max_seconds/owners[]`. Same-node records share one map pin but remain individually selectable.
- Activities: `snapshot.activities.day/signed/verse`, `month_sign_count`, `monthly_chests[]`. Missing day history is not replaced with inferred calendar status.
- Race queue/action/live: `snapshot.race_queue`, `snapshot.race.teams[]`, current identity and only actual `anchor_actions[]`/eligible actions. Server decides score, qualification, rank and ties.
- Result/receipt: `snapshot.round`, `snapshot.race`, `snapshot.lease`, and `snapshot.reward_receipts.entries`; correlate event/node/actor exactly. Root alone owns global receipt dequeue.

## Runtime asset paths

- Map art expected at `/Game/newUI/Textures/Journey/T_NUI_JourneyMapPortrait_V1` and `/Game/newUI/Textures/Journey/T_NUI_JourneyMapLandscape_V1`; PNG source measurements and hashes remain in `../assets/asset-manifest.yaml`.
- Font inputs currently resolve by this candidate source under `/Game/newUI/Shared/Fonts/F_InkDisplay` and `/Game/newUI/Shared/Fonts/F_InkBody`. The current shared contract requires one canonical imported mapping to be confirmed before runtime.
- No `/Game/UI` asset path appears in review-v2 staged source. Full-screen concept images are not used as runtime UI.

## C05/C06 and validation boundary

C05/C06 fixes are described in `review-resolution.md`. Static source scan finds no `: public UButton` in this staged tree, no invalid old pointer-addition expression, and no `/Game/UI` reference. `status.json` parses. No UHT/UBT/compiler, automated test, Editor, import, route, service, or player-entry verification was run. Source remains uncompiled; assets remain unimported; routes/logic/runtime remain unverified. Visual acceptance remains pending.


