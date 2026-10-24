# Bounded source review: ready review-v3 and Player-c

Scope: only new Player-c source, ready review-v3 deltas and the five-file production Source epoch that arrived during this review. Unchanged production files and previous source/API checks were not reread. Exact inputs/hashes are in `source-delta-v3.json`. Historical `screen-coverage-v2.*` remains unchanged. No engine, build, migration, logs sweep, image approval or production edit was performed.

## Ready patch dispositions

| Finding | Source disposition | Integration obligation |
|---|---|---|
| N01 Activities | Resolved in review-v3 source. Start no longer requires a future EventId or nonempty display service day; it captures the real anchor/guild/rules/profile/known-absent subject, compares it on refresh and second click, and emits that captured subject. | Populate StartSubject, compare to a fresh capture, run native Evaluate/Submit. No fake future EventId. Actual host wiring is not verified. |
| N02 Journey | Original cross-day/month confirmation defect fixed in review-v3 source: capture day/month/milestone, invalidate changed subject, emit captured tuple, keep pending until feedback. A separate new missing-month gate is J3-01 below. | Update all reflected delegate bindings to three parameters. Month is a host confirmation fingerprint, not a new MonthlyChest command field. Correlate async completion with the captured request. |
| N04 Journey | Source path now matches `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay` and Body, the cleanup map targets. | Imported package/readback/typography remain unverified; fallback font is not acceptance. |
| N07 Rank | Null completion releases the in-flight ID. Review-v3 adds RetryParticipantAvatarAfterHostBackoff with visible participant/state/cache/in-flight/delegate checks. | The later current review-v2 had already moved the null release before return; that differs from this review's earlier archived v2 hash. Host must handle success/failure and enforce backoff on every request callback, including requests emitted by row refresh. No async completion was run. |
| N08 Rank | Explicit Engine/Font.h included in the new base translation unit. | Compile remains unrun. |
| N09 Rank | Label resolver unwraps the SizeBox/button and uses the inner button GetContent(). | Tab text color/render behavior remains unrun. |

The Rank overlay changes three files and preserves its other v2 files by hash; Activities v3 is the start-subject delta, not an implementation of its pending commerce/rank pages. Journey v3 is the confirmation/font delta on its v2 page set. Their owner handoffs/statuses exist; hashes are independently recorded here instead of assuming a handoff is compiler evidence.

## New findings for dispatch

Paths below are relative to `specs/boundless-ink-ui/rebuild-2026-10-24/`.

**PC3-01 / P1 / Player-c API:** `panel-player/player-c/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerMethodsEquipmentAppearanceWidgets.cpp:62` attaches AddLambda to ordinary UButton::OnClicked. This is the same installed UE5.8 dynamic-multicast API mismatch as C01, now in a previously unreviewed slice. A later one-line change adds `mutable` to the lambda; that does not give a dynamic delegate an AddLambda API. Fix the shared ActionButton once using a retained typed callback button/relay and a UFUNCTION click handler, preserving the action ID and copied typed payload. Do not turn this into a borrowed stack delegate pointer. The FActionRequested native delegate may execute normally once the dynamic click bridge is correct. No new engine query/build was needed; the Button.h declaration was already verified.

**PC3-02 / P2 / Player-c action availability:** Methods `.cpp:330-346` only creates `learn-method` inside `if (Selected)` where Selected must be found in the display Catalog. A retained actual SelectedMethodId and host `bLearnActionAvailable=true` still have no learn button if the optional catalog response is absent; the available inventory projection only displays details at 367-377. Keep actual command identity and fresh host eligibility separate from catalog display completeness: render the authorized known-item intent with an unknown-name/detail placeholder when the exact selected item is known. If the host cannot validate that identity, it must explicitly mark the action unavailable. Do not fabricate a book ID or infer selection from a label.

**PC3-03 / P2 / Player-c unknown field:** Equipment `.cpp:422-424` treats `bEntryKnown=true` plus empty ItemName as “当前没有已配置物品”, even when SlotValue.EntryId/ItemId identify an equipped item. Missing display name must render an unknown-name state. Render empty only when provider-known absence of the entry is established; use the actual entry identity rather than name emptiness. This affects what the UI claims, not server authorization.

**J3-01 / P2 / Journey new gate:** review-v3 Journey `.cpp:223-224,248` adds `!ServerMonth.IsEmpty()` as a mandatory MonthlyChest UI eligibility condition. The existing native Evaluate requires current ExpectedDay, valid milestone, provided/eligible/unclaimed chest and standard context gates (`Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.cpp:121-134`); it has no month-string requirement. A valid expected day and eligible chest with missing month display string now cannot reach that legal request. Capture the month as an optional exact fingerprint (including an empty value) and invalidate when it changes, or derive a period from an explicitly authoritative day contract. Keep the real ExpectedDay and chest eligibility checks; do not add an independent display-month permission. N02's captured identity behavior should remain.

**I3-01 / P2 / Integration new Pending projection:** new Root PlayerState returns Pending for any transport Busy. The Character and Inventory composers render their confirmed attributes/rows only when State is Data. Thus starting a request changes an already populated screen into a status-only screen; the current confirmed inventory/attributes disappear until feedback clears Busy. Retain the last confirmed content and stable selection while showing Pending, and gate only the applicable mutation controls with fresh authority/Busy state. Do not optimistically update values. This follows the changed Root mapping and existing builder State conditions; no flicker was claimed observed in Slate.

## Positive Player-c observations and boundaries

Equipment selection payload contains stable EntryId and ItemId, and equip/unequip passes that EntryId and quantity. Action availability is host-supplied and is not globally tied to every provider field being Data. ConfigureMethods carries the host-owned main/support draft without inventing support entitlement. Fonts/colors are injected rather than loading an old package. Appearance copy explicitly describes local preview; its actions make no unlock, ownership or persistence claim. These are source properties only: the ordinary button API defect still prevents treating this source as compilation-ready.

The pure builder functions return trees rather than replacing an existing UUserWidget RootWidget. Integration owns mounting, disposal, selection/draft/scroll preservation, callback lifetime, orientation and fresh handler binding. No host for Player-c was observed in the captured production source epoch; staged UI intents and adapter documentation do not prove dispatch reaches the handler.

## New production Source epoch

Five production files changed against the archived v2 hashes: InkInventoryWidget.cpp, InkRebuildFrontendWidget.cpp, Player/InkPlayerCharacterInventoryWidgets.h/.cpp and Player/InkPlayerVisibleEntryWidgets.cpp. Only their filesystem deltas were reviewed.

- N03 is resolved in production source: populated cells send `InventorySelectId_<GUID>`, the handler parses exactly that GUID, rechecks it still exists and rejects stale identity; empty physical slots use a separate revalidated intent. The new prefix lengths are 18 and 21, matching the Mid offsets. Actual click behavior remains unrun.
- N06's missing eligibility-key and unconditional Data defects are resolved in production source: each reflected inventory entry is exported into the content key, which also includes provider/Busy/capacity/status/width. The new provider projection introduces the separate Pending-content issue I3-01 above. No command eligibility was inferred from display completeness.
- HUD/Main/Character/Inventory now request the semantic Player textures under `/Game/newUI/Components/Player`. Presence/import/hash matching/rendering of those Texture2Ds was not verified here. Dynamic inventory columns were added; layout fitting remains unverified.
- Visible-entry button sizing now takes the maximum of the requested size and the shared TouchSize. This preserves the minimum source target without establishing its physical device size or visual fit.
- A new InventoryIdentity automation test source covers visible-GUID selection after reorder/removal and empty-slot replacement. It was read as source only and was not executed; its presence is not a passing result.
- N05 remains open: this epoch does not mount additional page IDs or replace the public legacy FeaturePanel host. Counts remain 6 source-connected compositions / 1 placeholder / 37 without observed new production component wiring. The CSV now carries current evidence hashes while preserving the v2 files as historical evidence.

The original Player-b staged builder differs from the new production version. Do not re-copy its full older pair over the stable-ID/provider/width corrections; rebase any owner changes against the production hashes in this review's manifest.

## Activities follow-on availability

At this bounded capture, Activities has only its three original page classes (Hub, CategoryDetail, Launch) under staged/review-v2/review-v3 source. Launch contribution rank, offline/shop, consignment and orders remain listed as future work in the team status; no ready paired source/handoff for those slices was observed. They are marked absent/not-ready in the 44-screen stage map, not silently assigned to the existing three classes.

## Deliverable map

`stage-to-runtime-v3.csv` / `.json` give every one of the 44 IDs: actual stage class or pure builder, typed VM input, canonical VM/action contract, production source mount state and runtime proof. Historical source coverage remains 6 new compositions / 1 placeholder / 37 without observed new production component wiring. Runtime/player-entry proof remains false for every row. Source-ready, compiled, mounted, rendered and user-approved are separate states.
