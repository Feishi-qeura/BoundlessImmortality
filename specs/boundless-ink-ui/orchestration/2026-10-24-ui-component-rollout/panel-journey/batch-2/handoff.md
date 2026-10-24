# Batch 2 staged handoff

## Scope completed

This isolated batch contains page contracts for `resource-detail`, `world-result`, `race-result`, and a shared settlement card; one pair of native widget source files; and a no-raster concept decision. It does not change the live routes, first-batch artifacts, shared source, Content, or the active engine lease.

## Staged source

These files are staged only and are not in a UBT/UHT source directory:

| Batch file | Future project-relative target | Role |
|---|---|---|
| `staged-source/InkJourneyBatch2Widgets.h` | `Source/BoundlessImmortality/Frontend/UiRollout20261024/Journey/InkJourneyBatch2Widgets.h` | Reflected row model, resource-detail event shell, outcome shells, settlement card |
| `staged-source/InkJourneyBatch2Widgets.cpp` | `Source/BoundlessImmortality/Frontend/UiRollout20261024/Journey/InkJourneyBatch2Widgets.cpp` | Native live-text/card construction and confirmation/back events |

The source is written against public UE widget APIs and the module's existing export macro. It has not been compiled. Do not call it compile-verified or import it while the logic-integration engine lease is active.

## Promotion sequence after lease release and explicit promotion authorization

1. Re-read the current engine lease and integration coverage; verify the batch-2 authorization field has changed and the permitted paths include these exact targets.
2. Copy the header and `.cpp` pair together into the future target directory. Keep the staged copies for provenance.
3. Add host-side mappers in an authorized integration change: resource selection → display rows; `snapshot.round` → world outcome rows; `snapshot.race` plus matching identity/team and `snapshot.lease` → race outcome rows.
4. Bind `UInkResourceDetailBatch2Widget::OnTravelRequested` to a fresh `InkJourneyAdapter::Capture` + `Evaluate(ResourceTravel, exact EntryId, confirmed)` and call the existing adapter submission only if allowed. Feed enqueue/service feedback back with `SetTravelFeedback`.
5. Bind outcome `OnBackRequested` to the existing navigation stack. Register distinct result route classes only in a separately authorized routing patch; do not alias `race-result` to a generic race route.
6. After authorization, run UHT/UBT and report that evidence separately. Then verify live route/data behavior and request visual approval; these are not established by the staged source.

## Data and action safeguards

- Adapter submission is only enqueued feedback; it does not establish a matched race or a grant.
- Resource facts may only map from selected `journey.resources[]` fields. Re-evaluate the exact entry immediately before submit.
- World results read `round`; reward entries must match a source id for that exact round/stage. Do not infer reward from inventory changes.
- Race results read the actual `race` team and actual `lease`. The 600 score threshold, tie policy, and lease duration policy come from the interaction contract; local UI never awards a tier.
- Match a receipt only when its exact source id identifies this race and anchor. Never drain the global inbox or call `TakeNextRewardReceiptJson()` from these pages.
- No claim, grant, or receipt-acknowledgement action is included. “返回” only delegates back navigation.
- Unknown fields stay absent or show host-supplied unavailable status. No fabricated owner, amount, score, duration, or result.

## Visual/assets

No new image asset is needed: these are native, data-dense detail/result surfaces using dynamic TextBlocks and the existing approved style contract. `prompts/no-raster-required.md` records the decision. No first-batch file was copied or regenerated.

## Current evidence limits

Static source and documentation are staged. No Editor/UBT, import, route wiring, logic verification, PIE, device, package, or visual approval was performed. The batch remains `approved:false` and visual review is pending.
