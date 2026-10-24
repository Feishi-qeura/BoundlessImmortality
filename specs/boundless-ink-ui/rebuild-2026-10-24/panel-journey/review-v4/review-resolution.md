# Review-v4 resolution

## J3-01 — do not require a display month for MonthlyChest

**Root cause:** v3 encoded `!ServerMonth.IsEmpty()` both in the UI action eligibility expression and first-click guard. The bounded review's adapter source shows that native `MonthlyChest` evaluation checks `bConfirmed`, non-empty/current `ExpectedDay`, valid milestone, monthly-chest provider presence, and the selected chest's current `eligible` / `claimed` state. It has no month-string predicate. Thus v3 made a missing display/fingerprint field an extra UI permission gate.

**Resolution:** v4 changes the UI subject gate to require only a non-empty expected service day. It keeps known/eligible/unclaimed milestone checks and current-presentation revalidation. First click captures the month string exactly, including empty. Confirmation identity equality continues comparing day, month fingerprint, and milestone exactly, so a month becoming known (or becoming empty) during an open confirmation invalidates that captured confirmation. The emitted event uses the captured day/month/milestone values.

**Host rule:** compare current day, current month fingerprint, and selected milestone with the captured event tuple before adapter work. Empty captured/current month values compare as the same fingerprint; do not reject solely because month is empty. A changed fingerprint invalidates the intent. For an exact match, let the existing fresh `Capture → Evaluate(MonthlyChest, captured ExpectedDay, captured Milestone, confirmed=true) → Submit` path enforce native authority and service gates. Month is not a new command field or a service permission.

## Retained N02 and N04

N02's captured tuple, same-identity retention, identity-change invalidation, eligibility revalidation, captured-value event payload, and pending request identity remain from v3. N04's canonical migrated font paths remain. Neither font import nor runtime behavior is established.

## Scope and verification

`review-v3/` and `review-v2/` remain frozen. This revision writes only inside `panel-journey/review-v4/` plus the panel-journey current pointer documents. Production source/config/content and the migration map are untouched. Static source/hash/JSON checks only; no build/test/engine/runtime evidence.
