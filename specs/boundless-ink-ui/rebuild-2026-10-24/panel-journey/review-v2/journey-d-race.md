# Journey D: race lobby and live race

Owner: panel-journey; merge slice: `journey-d`; contract revision: `newui-r1`; candidate `approved:false`.

## Routes

`race-lobby` uses `UInkJourneyRaceLobbyPageWidget`; `race-live` uses `UInkJourneyRaceLivePageWidget`. Both are distinct UUserWidget pages under the new root host. The layouts use width-aware columns for landscape and one column for portrait.

## Race lobby

`SetAuthorityData(QueueSubjectId, QueueState, Rows, bCanCancel)` maps `RaceVm` queue state from the current authority snapshot. Render enqueued/waiting separately from matched; only server match data permits the matched view. `OnCancelRequested` maps to the exact `cancel-race-queue` authority action, whose existing command takes no page parameters. Host performs fresh Capture/Evaluate/Submit and resolves with `SetCancelFeedback`; pending UI is not proof of cancellation. A changed subject invalidates old pending UI state.

## Live race

`SetAuthorityData(ExactRaceId, Phase, Rows, Actions)` maps exact `snapshot.race` / team data: score, common progress, anchor actions, response arrays, representative IDs, eligibility, forfeiture, room and anchor. Keep missing data unknown; do not derive rank/ties/scoring locally.

`Actions` is an explicit provider/host projection of supported action kinds and must include known/enabled/reason state. Only those options are rendered; do not invent action buttons. Selecting one emits `OnRaceActionRequested(ExactRaceId, Kind)` and maps to `race-action` with exact `race_id` and `kind`. The host recaptures and submits once through the existing prototype command. No local confirmation is added because the action contract marks this action as not requiring one. The page disables all action controls while pending, and a race-id change invalidates stale UI state.

## Evidence boundary

These are source compositions and typed event seams only. They do not create or mutate race state. Adapter authorization, actual matching/phase changes, command completion and returned score/result evidence remain unverified until integration and runtime acceptance.
