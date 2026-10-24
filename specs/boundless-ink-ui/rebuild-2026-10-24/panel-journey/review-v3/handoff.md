# Review-v3 handoff

Owner: `panel-journey`; input findings: `model/review/remaining-v2.md` N02 and N04. This is a complete replacement source snapshot copied from frozen `review-v2`, with the N02/N04 changes isolated under `review-v3/staged-source/`. The prior `review-v2/` source and handoff remain frozen for the integration owner to finish its current comparison.

## Delivered files

- `staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.h`
- `staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.cpp`
- `review-v2-to-v3.patch`: filesystem diff of both replacement files against review-v2.
- `review-resolution.md`: exact N02/N04 source resolutions and verification boundary.
- `host-integration-changes.md`: precise adapter and font handoff for the integration owner.
- `status.json`: source hashes and status flags.

This revision keeps the route set and previous C05/C06 source repairs. It does not edit production `Source/`, `Config/`, `Content/`, the migration map, or shared contracts.

## N02: monthly confirmation subject

`UInkJourneyMonthlySignInPageWidget::OnMilestoneClaimRequested` now carries `(ExpectedDay, ExpectedMonth, Milestone)`. The widget captures all three on the first confirmation click. A presentation refresh with the same day, month, and selected milestone retains the open confirmation. A changed day/month/selection invalidates it. On the second click, the current eligibility is checked against the captured milestone and the event is emitted from captured local values, never from a refreshed date. A pending request retains its captured identity until actual feedback clears it.

Host integration must update the delegate binding to receive all three values. Before a fresh `InkJourneyAdapter::Capture`, `Evaluate`, and `Submit`, compare the current authoritative expected day, month, and selected milestone against the event payload. Reject mismatched/stale intent; never substitute refreshed VM values for the captured day or month. For a matching subject, use `MonthlyChest` with the captured `ExpectedDay`, captured `Milestone`, and `confirmed=true`. The current action contract has no month argument, so month is a host-side identity fingerprint, not a new service command field. Resolve widget pending state only from the actual enqueue/service response and correlate that completion to the same captured request identity.

## N04: canonical font packages

The source now requests `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody`, matching the migration map's canonical targets. The `FCoreStyle` fallback remains a source-only tolerance path and is not evidence that migrated fonts were imported, found by AssetRegistry, or displayed at runtime. Import/readback and visual typography review remain with integration/asset cleanup.

## Integration boundary

Integrate this revision as a whole replacement of review-v2's two source files after rebasing the host changes above. It changes the reflected dynamic delegate signature; update every binding/call site in the same integration change. Preserve host request identity through back-stack movement and asynchronous completion. Do not treat this source package as route wiring, service correctness, font import, build, Editor, or player-entry evidence.

No production source/config/content was changed. No Unreal Editor, UHT/UBT, build, import, automation, PIE, device, or runtime action was run. `approved` remains `false` and visual acceptance remains pending.
