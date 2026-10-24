# Journey A: journey hub, world and resource maps

Owner: panel-journey. Contract revision: `newui-r1`. Candidate only; `approved:false`, visual review pending.

## Page set

This contract slice contains three pages: `journey` (`UInkJourneyHubPageWidget`), `world` (`UInkJourneyWorldMapPageWidget`) and `resources` (`UInkJourneyResourceMapPageWidget`). All are standalone UUserWidgets intended for the new root host.

### Journey hub

The new `/Extensions` entry uses `UInkJourneyHubPageWidget`. It displays only dynamic world and guild state text and emits `OnNavigationRequested` with the exact action ID `open-world`, `open-resources`, or `open-wuling`. The new root host maps those navigation intents to the current route stack; missing world/guild status never disables unrelated navigation.

## World map

Portrait: page header and provider state, a tall illustrated map viewport with live selectable node pins, then a selected destination detail card and primary travel action. Landscape: map left, selected destination and action right. Pins display live names and state; unknown remains explicit. Background art contains no labels or controls.

Data mapping from `WorldVm` / authority snapshot `journey.worlds[]`:

- `EntryId` = exact `id`; `Name` = `name`; `State` and `StateText` from authoritative `open` (do not infer from coordinates).
- `NormalizedPosition` = clamped provider `x`,`y`; `Details` = provider `difficulty`, `description`, and `rewards[]`, each preserving unknown vs supplied empty.
- Selection is retained by exact entry ID across same-entry refresh. User confirmation is local UI only; integration must recapture and evaluate the same exact ID with `confirmed=true` immediately before Submit.

### Resource map

Uses the same portrait/landscape composition and node interaction model. Resources sharing `node_id` are visually grouped at one location while retaining individually selectable exact `id` values. A group count is a count of returned records, never a fabricated contribution metric.

Data mapping from `LeaseVm` / `RaceVm` and `journey.resources[]`:

- `EntryId` = exact resource row `id`; `GroupId` = `node_id`; `Name` = `name`; state only from explicit `open` and provider status.
- `NormalizedPosition` uses the provider location for the node; repeated rows under the same `node_id` share the coordinate but remain distinct entries.
- Details may show exact `index`, `entry_count`, `quantity`, `item_id`, `item_name`, `interval_seconds`, `qi_additive`, `lease_max_seconds`, and `owners[]` only when provided. No locally calculated lease expiry or reward.
- Primary request goes through `Capture → Evaluate(ResourceTravel, exact EntryId, confirmed=true) → Submit`; the existing service action queues `QueueRace(node_id)`. It does not immediately claim a lease.

## Shared composition rules

- Portrait logical baseline 720×1280, 24-unit content insets, shortest-side scaling owned once by root. Landscape is independently reflowed to 1280×720, never portrait-scaled.
- Live text remains UMG text. Text helper enforces the contract's 26-unit minimum; actions use the contract's 30-unit button text and 88-unit minimum hit target intent. Final contrast/raster fitting still needs assembled visual review.
- New fonts resolve under `/Game/newUI/Shared/Fonts`; map art resolves under `/Game/newUI/Textures/Journey`. No runtime reference to `/Game/UI`.
- Candidate map backgrounds derive only landscape decoration from the prior map composition inputs; old candidate business labels/states are not carried forward. The daily-sign-in reference's fortune/example cards are deliberately excluded.
- Candidate page and assets remain unapproved until user visual review.

## Readiness boundary

The staged classes and background PNGs are not compiled, imported, routed, or runtime-verified. A root-owned presenter must own each child widget with UPROPERTY and connect route/back stack, VM mapping, focus/viewport selection, adapter actions, exact result/receipt sources, and pending completion. The integration owner remains the only production Source/Config/Content writer.





