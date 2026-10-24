# Review-v4 handoff

Owner: `panel-journey`; input finding: `model/review/bounded-v3-review.md` J3-01. This is a complete replacement snapshot copied from frozen `review-v3`, with the J3-01 correction isolated under `review-v4/staged-source/`. Review-v3 and earlier revisions remain frozen.

## Delivered

- Full replacement `staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.h/.cpp`.
- `review-v3-to-v4.patch`: source diff for both complete replacement files.
- `review-resolution.md`: root cause, J3-01 resolution, retained N02/N04 behavior, and evidence boundary.
- `host-integration-changes.md`: exact current-subject and MonthlyChest host checks.
- `status.json`: source hashes, revision state, and verification limits.

## J3-01: month is an optional fingerprint

The v3 UI incorrectly required both a non-empty `ExpectedServiceDay` and non-empty `ServerMonth` before enabling/starting MonthlyChest confirmation. The native adapter's `MonthlyChest` evaluation does not require a month string. It requires the current non-empty service day, valid milestone, provided chest data, and a chest that is eligible and unclaimed, along with the standard context/confirmed gates.

V4 requires a non-empty service day and retains the milestone's known/eligible/unclaimed presentation gates. The first click captures `(service day, month fingerprint, milestone)`, including an empty month value. Same-tuple refresh retains confirmation. Any exact change to day, month fingerprint (including empty-to-non-empty or non-empty-to-empty), or milestone invalidates it. The second click revalidates the captured milestone against current authority presentation and emits the captured tuple. It never substitutes a refreshed service day.

## Retained N02 and N04

N02 remains: the three-field identity is captured on first click; same-subject refresh retains it, identity change invalidates it, and the event carries captured values. Pending keeps the request identity until matching feedback clears it.

N04 remains: font requests use canonical `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody`. Import/readback and actual typography rendering remain unverified.

## Host integration update

Update the three-parameter delegate binding to receive `ExpectedDay`, `ExpectedMonth`, and `Milestone`. Before fresh adapter work, compare the current expected service day, current month fingerprint, and selected milestone exactly against the emitted tuple. Empty month equals empty captured month and must not itself deny the request; a changed month fingerprint rejects stale UI intent. Then call `InkJourneyAdapter::Capture`, evaluate `MonthlyChest` with the captured expected day and milestone and `confirmed=true`, and Submit once only if the current native gate passes. Do not add a month requirement or replace the captured day with a fresh VM date. Preserve request identity through back-stack and async response handling; clear pending only for the matching actual enqueue/service result.

This revision is source-only and `approved:false`. No production `Source/`, `Config/`, `Content/`, migration map, or shared contract was changed. No compile, test, Editor, import, AssetRegistry, service action, automation, PIE, device, package, or runtime operation was run. Visual acceptance remains pending.
