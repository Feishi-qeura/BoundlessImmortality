# Host handoff · review-v5

## Replacement set

Consume these six complete files together from `staged-source/`:

- `RebuildActivityWidgets.h/.cpp` — shared page/action base. Header adds the typed `OfferId` field to `FRebuildActivityActionRequest`; `.cpp` is byte-identical to frozen review-v3.
- `RebuildActivitySecondaryWidgets.h/.cpp` — activity rank, route-less offer and offline pages.
- `RebuildEconomyWidgets.h/.cpp` — shop, market and orders pages.

This is a complete replacement candidate for the activity secondary and economy page sources plus the paired shared base. It supersedes those source files for any later integration; it does not edit or delete the frozen v3, panel-activities-02 or panel-economy-03 directories. Resolve shared base/interface use once when integrating.

## Intent mapping

- Offline acknowledgement emits `ack-offline` with no payload. Map `bCanAcknowledge` from live native eligibility and a fresh current-user `offline_reward` record; do not demand or synthesize `SourceId`. Before submit, the host recaptures current actor/identity, connection and Busy state and validates that record still exists. Existing service code reads the current user's record itself.
- Launch offer purchase emits `Action=purchase`, `OfferId=<captured offer>`, `EntryId=<captured SKU>`, `Version=<captured quote version>`, `PriceText=<captured display text>`, `bConfirmed=true`. Compare all five subject/value fields against a fresh offer/quote and actor state before native submit. The page only exposes the button when the real service and platform payment channels, entitlement and quote gates are true; keep it unavailable until those exist.
- Shop purchase confirmation compares selected SKU, quote version and displayed price across local changes, refreshed view models and the second click.
- Market buy confirmation compares selected listing ID, listing version and displayed price. Create-listing confirmation compares selected inventory item, parsed positive quantity and exact user price text. Order cancellation compares selected listing ID and listing version. On a subject switch clear only that action's pending confirmation; on a same-subject refresh retain it.
- Continue to route requests through the existing action contract and authority adapter. Preserve active-page, actor-role, identity, connection, Busy, `expected_version`, outbox request identity and server-side inventory/eligibility checks. UI confirmation does not establish payment, settlement, acknowledgement success or order state.

## Files and provenance

- Exact per-file diff: `v5.patch`.
- Frozen-input and replacement hashes/byte lengths: `source-delta-v5.json`.
- Finding dispositions: `review.md`.
- Production source and Content remain owned by `logic-integration`; this handoff contains no production change or import.
