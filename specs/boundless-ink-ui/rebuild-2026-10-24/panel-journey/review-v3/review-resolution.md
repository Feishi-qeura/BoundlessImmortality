# Review-v3 resolution

## N02 — captured monthly sign-in subject

Resolved in the isolated replacement source. The first confirmation click captures `(ExpectedServiceDay, ServerMonth, SelectedMilestone)`. A setter refresh retains the confirmation only while all three identity fields still match. A changed service day, month, or selected milestone clears the confirmation and its capture. The second click revalidates the captured milestone against the refreshed authority projection and emits the captured day/month/milestone values. It cannot replace the expected day with a newer setter value. The host event signature therefore changes to three parameters. Pending retains the request tuple until `SetMilestoneFeedback(..., false)` resolves it.

Host checks the event tuple against a fresh current day/month/milestone before adapter work. If identity differs, reject the stale UI intent. If identity matches, call Capture, Evaluate `MonthlyChest` with captured expected day and milestone and `confirmed=true`, then Submit once if current gates pass. Month remains a host-side identity check because the existing action contract has no month field. Completion must resolve the same pending request; UI state is not service authorization.

## N04 — canonical font paths

Resolved in the isolated replacement source by using `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody`, which match the migration map targets. No migration or production asset was changed. The default font fallback is only source tolerance; the actual packages were not confirmed imported/readable at this checkpoint.

## Scope and evidence

`review-v2/` remains frozen. Production `Source/`, `Config/`, and `Content/` were not edited. This package is source-only and remains `approved:false`. Static inspection covers the source text, patch, hashes, and JSON only. It does not establish UHT/UBT compilation, asset import/readback, route wiring, service behavior, Slate behavior, Editor/PIE, device, package, or visual acceptance. No test/build/runtime command was run.
