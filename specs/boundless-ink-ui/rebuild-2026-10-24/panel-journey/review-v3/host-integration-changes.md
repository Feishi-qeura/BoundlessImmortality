# Review-v3 host integration change notes

1. Replace both `InkJourneyNewWidgets.h/.cpp` files with the complete files under `review-v3/staged-source/`; do not cherry-pick only a hunk. `review-v2-to-v3.patch` documents the source delta.
2. Update the reflected event binding to `OnMilestoneClaimRequested(ExpectedDay, ExpectedMonth, Milestone)`. Search and update all Blueprint/C++ bindings affected by the delegate signature.
3. Before a fresh adapter Capture/Evaluate/Submit, compare the authoritative current expected service day, month, and selected milestone with the emitted captured tuple. Reject if any differ. Never take the new date/month from a refreshed VM and replace the captured tuple.
4. For a matching tuple, call `InkJourneyAdapter::Capture`, evaluate `MonthlyChest` using captured `ExpectedDay`, captured `Milestone`, and `confirmed=true`, then Submit exactly once if the current native gate passes. Do not add Month to the service command: it is an identity fingerprint used by the host comparison under the current contract.
5. Preserve request identity across route/back-stack transitions and async response handling. Call `SetMilestoneFeedback` only for the matching real enqueue/service result; the pending UI state does not authorize a claim.
6. Resolve/import and read back canonical font packages `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody` with asset cleanup/integration ownership. Do not treat the source fallback font as delivered typography.

This snapshot has not been compiled or run. Integration owns UHT/UBT, route/action binding, font AssetRegistry evidence, Slate/player-entry review, and service-result verification.
