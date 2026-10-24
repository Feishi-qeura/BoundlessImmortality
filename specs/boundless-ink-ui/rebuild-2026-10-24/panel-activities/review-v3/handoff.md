# Activity start confirmation · review v3 handoff

## Disposition

This is a new patch revision based on `review-v2`. Neither the v1 nor v2 staged/handoff files were overwritten. It responds to `model/review/remaining-v2.md` finding N01/P1 only.

## Exact change

- Removed the UI precondition `ViewModel.EventId.IsNone()` from StartLaunch. A launch event ID is created by the authority service when a legal absent-event start is accepted; the UI sends no fabricated future ID.
- `bCanStart`, connected, non-Busy and `StartSubject.bLaunchKnownAbsent` remain the visible/action checks. The exact start subject holds the current anchor user, guild ID, rules version, profile ID and known-absent flag.
- First confirmation captures that subject and the current service day string. The day can be empty and is only a confirmation fingerprint; it is not an independent start-eligibility gate.
- Same subject/day refresh retains confirmation. Any subject/day change clears it. Second click emits `start-launch`, `bConfirmed=true`, the captured subject and optional captured day, with `EntryId=None`.
- Host must compare the requested subject against a fresh Capture, then run the existing adapter `Evaluate`/Submit. Native logic still requires the prototype profile, known absent launch, anchor/guild identity, valid service/window time and the normal Busy/transport gates.
- The shared action request also carries exact economy intent fields (`Version`, `PriceText`, `QuantityText`, `Quantity`, `Offset`, `bOwnedOnly`) for the separate `panel-economy-03` staged pages. These fields do not change the launch-start command semantics.

## Evidence boundary

The report identifies `StartLaunchEvent` as creating the event ID and the adapter as checking `bLaunchKnownAbsent`; the patch follows those source contracts. This is source-only. No compile, engine, UBT, network, Editor or runtime verification was performed. Exact v3 file hashes are in `status.json`.
