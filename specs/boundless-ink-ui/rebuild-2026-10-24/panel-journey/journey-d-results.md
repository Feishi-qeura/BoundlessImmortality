# Journey D: world and race results

Owner: panel-journey. Contract revision: `newui-r1`. Merge slice: `journey-d`. Candidate source only; unapproved.

## Distinct route classes

- Route/page ID `world-result` uses `UInkJourneyWorldResultPageWidget`.
- Route/page ID `race-result` uses `UInkJourneyRaceResultPageWidget`.

Both inherit the live-text `UInkJourneyDetailResultPageWidget` composition shell. They remain distinct UUserWidget classes; do not assign them directly to the legacy `TSubclassOf<UInkFeaturePanel>` route factory. New-root integration owns each instance and its parent-stack relationship.

## World result mapping

Populate `SetPageData(..., bCanTravel=false)` from authoritative `snapshot.round`: exact round `id`/status, completed stages, and participant stages. Preserve integer stage values and unknown fields as unknown. Keep exact result source identity in `ExactSourceId`; match `snapshot.reward_receipts.entries` only against that source and relevant actor. Return Back to the preserved world/region page. Back must not claim, acknowledge, or dequeue receipts.

## Race result mapping

Populate `SetPageData(..., bCanTravel=false)` from authoritative `snapshot.race` and the exact current team. Keep score, common progress, anchor actions, response arrays, representative IDs, eligibility, forfeiture, room and anchor fields as provided. Do not infer rank or tie outcomes. Join `snapshot.lease` and reward receipts only with exact node/event/actor identity. Do not drain global receipts or use inventory deltas as proof of a grant.

Both routes are read-only results; their action button should be disabled, and the Back delegate restores the original page stack. The generic shell's optional action remains available for detail routes, but result host maps it off.

## Evidence boundary

The separate result classes and source-key behavior are staged only. No route, mapper, screenshot, compile, service response or runtime result is claimed. Integration owns the host and real adapter subscriptions.
