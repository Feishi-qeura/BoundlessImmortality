# Journey C: resource detail

Owner: panel-journey. Contract revision: `newui-r1`. Merge slice: `journey-c`. Candidate source only; unapproved.

## Route composition

Use the distinct class `UInkJourneyResourceDetailPageWidget` for route/page ID `resource-detail`. It inherits `UInkJourneyDetailResultPageWidget` composition but does not become an `UInkFeaturePanel`; the new root host creates it, owns it with `UPROPERTY`, and returns Back through the parent resource-map page.

## Data and actions

- Preserve the exact selected resource `EntryId` and `node_id`. Populate rows from `journey.resources[]` using only returned fields: index/count, quantity/item, interval, qi additive, lease maximum, owners and open state.
- Join `snapshot.lease` and `snapshot.race` only by exact node/source identity provided by authority. Do not infer lease ownership, expiry, competition outcome, or rewards from map art.
- `SetPageData(Title, ExactSourceId, Status, Rows, CanTravel)` receives exact `EntryId` as its source key. Set `CanTravel` from current supported action eligibility only; missing presentation fields are not extra permission gates.
- Primary action uses the same confirmed resource action contract as the map: fresh `InkJourneyAdapter::Capture`, `Evaluate(ResourceTravel, exact EntryId, confirmed=true)`, then `Submit`; current service implementation queues `QueueRace(node_id)`. It does not directly claim a lease.
- Filter confirmed reward receipts by exact event/node/actor source. The widget never drains the global receipt inbox.

Same-source refresh retains confirmation/pending. The root host must preserve request identity through response and call `SetTravelFeedback` only with an actual enqueue/service outcome.

## Evidence boundary

The distinct route class and shell exist in staged source. No actual route, data mapper, compile, import, or runtime evidence exists. Production host integration is owned by `logic-integration`.
