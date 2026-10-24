# panel-rank review-v3 handoff

**State:** three-file incremental replacement staged under `review-v3/staged-source`; `approved:false`. The review-v2 source, diff and handoff remain frozen and unchanged.

## Apply

Apply `replacement.diff` on top of `panel-rank/review-v2/staged-source`; it only changes `InkRankWulingPages.h/.cpp` and `InkRankWulingPageBase.cpp`. The full staged-source directory is a byte-identical v2 baseline plus those changes. `replacement-contract.yaml` records the finding mapping.

## N07–N09

At the current v2 hash, `SetParticipantAvatar` already removes the participant from `AvatarRequestsInFlight` before handling nullptr; N07's reported early-return ordering is stale against that v2 file. V3 makes the retry path explicit: the host reports success with a texture or failure with nullptr, waits its own backoff, then calls `RetryParticipantAvatarAfterHostBackoff(id)`. The method verifies the participant still appears in current Data rows, there is no cached avatar or request in flight, state is Data, and the request delegate is bound. Success continues to update participant-keyed cache and the current participant-keyed image map. No rank/index lookup is introduced.

N08 adds `Engine/Font.h` explicitly to the base translation unit. N09 reads the label directly from the inner UButton's `GetContent()` after unwrapping the outer 88×88 SizeBox.

## Evidence and next owner

All six files in the current review-v2 directory match its status hashes. The review's archived `source-snapshot-v2` differs for the Rank source and reproduces the N07/N09 report; v3 is based on the later current review-v2 hashes, so it preserves intervening fixes. Relevant UE5.8 headers were checked for UFont declaration, UContentWidget::GetContent, dynamic button content, and the existing request/cache types. Static assertions pass. No engine, UBT, compile, PIE, or screenshot was run.

Integration owns provider failure callbacks and backoff timing. Consume or reject this small overlay, connect failure -> nullptr and delayed retry -> retry API, then compile and verify late completions, current participant updates, and tab-state text in runtime. No independent Rank/Wuling work remains; waiting for the unique integration owner.
