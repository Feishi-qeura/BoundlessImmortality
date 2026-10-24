# Review-v4 host integration change notes

1. Replace both Journey source files with the complete files under `review-v4/staged-source/`. `review-v3-to-v4.patch` records the delta from frozen review-v3. Do not cherry-pick only the eligibility line.
2. Keep the reflected event binding `OnMilestoneClaimRequested(ExpectedDay, ExpectedMonth, Milestone)` and update all C++/Blueprint bindings as needed.
3. Compare current authoritative expected day, current month fingerprint, and selected milestone exactly with the emitted captured tuple before adapter work. The captured month may be empty; empty-to-empty matches and does not deny MonthlyChest. If the fingerprint changes, reject stale intent. Never replace the captured expected day with a refreshed VM value.
4. For an exact identity match, run `InkJourneyAdapter::Capture`, then `Evaluate(MonthlyChest)` with captured `ExpectedDay`, captured `Milestone`, and `confirmed=true`; Submit once only if fresh native gates pass. Keep the month solely as an identity fingerprint; do not require it to be non-empty or add it to the command contract.
5. Retain request identity across back-stack movement and asynchronous completion. Resolve widget pending state only from the actual matching enqueue/service response.
6. Canonical font package paths remain `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody`; asset import/readback and visual typography proof remain outstanding.

No UHT/UBT, tests, Editor, imports, service actions, automation, PIE, device, package, or runtime run was performed for this snapshot.
