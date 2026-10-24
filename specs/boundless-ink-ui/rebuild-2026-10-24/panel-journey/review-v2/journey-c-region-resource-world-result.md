# Journey C: region, resource detail, world result

Owner: panel-journey; merge slice: `journey-c`; contract revision: `newui-r1`; candidate `approved:false`.

## Page classes and routes

- `region` / `RegionDetail`: `UInkJourneyRegionPageWidget`.
- `resource-detail` / `ResourceDetail`: `UInkJourneyResourceDetailPageWidget`.
- `world-result` / `EventSettlement`: `UInkJourneyWorldResultPageWidget`.

Each is a distinct UUserWidget route beneath the new root host. Back delegates return through the original root page stack. Never register these with legacy `TSubclassOf<UInkFeaturePanel>`.

## Region data and action

Call `SetAuthorityData(ExactEntryId, Name, State, Rows, bCanTravel)` from the selected `WorldVm` destination and any exact region facts the provider supplies. Keep `ExactEntryId` unchanged from selection. Missing provider fields stay unknown; do not add realm recommendations, completed stages or restrictions from art.

`OnTravelRequested(ExactEntryId)` fires only after a local confirmation step. Host maps to `world-travel`, recaptures `InkJourneyAdapter::Capture`, evaluates the exact entry with `confirmed=true`, then submits once if current service gates pass. `SetTravelFeedback` resolves pending only from the actual adapter result.

## Resource detail data and action

`UInkJourneyResourceDetailPageWidget::SetPageData(Title, ExactSourceId, Status, Rows, CanTravel)` receives the exact selected resource row id. Map rows from `journey.resources[]`; correlate `snapshot.lease` / `snapshot.race` only using authoritative node/source ids. Resource submit remains `Capture → Evaluate(ResourceTravel, exact EntryId, confirmed=true) → Submit`; existing service behavior queues `QueueRace(node_id)`, not an immediate lease claim.

Reward rows must be filtered to exact source/event/node/actor receipt identity. The view must never dequeue the global inbox or infer rewards from inventory delta. Same-source refresh preserves interaction state.

## World result data

`UInkJourneyWorldResultPageWidget::SetPageData(..., bCanTravel=false)` receives `snapshot.round` id/status, completed stages, participant stage lists, and only exact-source matching `reward_receipts.entries`. Back is read-only navigation; no claim, acknowledgement, or receipt dequeue is attached.

## Evidence boundary

Composition and class names are staged under review-v2. Production route, mapper, adapter subscription, compile/import, service and player-entry verification remain integration-owned and incomplete.
