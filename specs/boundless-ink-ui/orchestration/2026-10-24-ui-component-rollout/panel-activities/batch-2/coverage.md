# Batch 2 coverage and gaps

| Surface | Existing integration evidence | Batch-2 staged coverage | Remaining verification |
|---|---|---|---|
| Activities overview | `source_mounted`; category actions reach retained cycle/detail panels | Not modified; batch 2 adds a common detail destination shell | Route to new shell only after promotion; PIE navigation |
| Daily activities | `source_mounted`; ACT_SIGN and ACT_VERSE/detail seams exist; missing fields stay pending | Shared shell has daily cycle filter and ACT_SIGN adapter submit | Verify real non-empty snapshot and sign-in response in PIE |
| Limited activities | `source_mounted`; limited-cycle list exists; detail needs component batch | Shared shell has exact `limited` cycle filter; actions remain unavailable until endpoint exists | Verify real deadline fields and expired state after service contract exists |
| Daily detail | Existing shell adapter wired; ACT_SIGN captures service day | Batch-2 replacement candidate uses `InkJourneyAdapter::Capture/Read/Submit` and exact template identity | Route mapping and click-time service-day response in PIE |
| Timed detail | Existing detail was marked `retained_needs_component_batch` | Batch-2 limited variant shares shell and retains `SelectedEntry` | Route mapping and live data in PIE |
| Launch page | Existing `UInkLaunchEventPanel` retained; countdown route exists, but tasks/permissions/auto-award UI not reverified | New page includes overview, tasks, guild milestones, honors entry, disabled offer area; launch start uses adapter gate | Route/tab parity, full scroll layout, service action result in PIE |
| Launch rank | Existing leaderboard retained with field gaps; tie/contribution runtime not evidenced | Separate page renders returned leaderboard name/rank/score and personal contribution fields | Route mapping and same-score ranks in PIE |

## Excluded claims

- No UE import, UBT compile, automation, PIE, Cook, package, device, or platform acceptance was performed.
- Generated page images are layout concepts. Runtime labels and values must be created from live UMG text, not baked into those images.
- The new panels are staged source only. Existing shared `Source`, `Content`, and first-batch artifacts were not edited.
- Review PA-01 through PA-03 are resolved in staged source; see `review-resolution.md` for the exact static evidence and limits.
- Detail expiry is not inferred from the local clock. Show the authority state until an explicit expiry field/contract is available.
