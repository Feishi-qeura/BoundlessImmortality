# Journey E: lease and race result

Owner: panel-journey; merge slice: `journey-e`; contract revision: `newui-r1`; candidate `approved:false`.

## Distinct routes

- `lease` / `ResourceLease`: `UInkJourneyLeasePageWidget`.
- `race-result` / `RaceSettlement`: `UInkJourneyRaceResultPageWidget`.

Both use distinct UUserWidget route classes under the new root. This removes the semantic ambiguity of the old `ResourceLease` and `RaceSettlement` aliases currently resolving to `WulingRace`; it does not wire production routes.

## Lease mapping

The host maps `LeaseVm` from `snapshot.lease` and the exact selected resource node. Display exact known node/status/owner/online seconds/end UTC fields. Countdown may use server time; never calculate a grant, lease tier, extension, expiration outcome, or ownership from artwork. Page is read-only and Back maps to the parent stack.

## Race result mapping

`UInkJourneyRaceResultPageWidget::SetPageData(..., bCanTravel=false)` receives `snapshot.race` and the exact anchor/team. Preserve provider score, team results and eligibility; do not infer winner, rank, ties or qualification. Receipt rows are joined by exact event/node/actor source; the root remains the only receipt dequeue owner. Back has no claim side effect.

## Evidence boundary

The classes and shell are staged only; no route, server data mapping, compile, import, receipt match or runtime result is claimed.
