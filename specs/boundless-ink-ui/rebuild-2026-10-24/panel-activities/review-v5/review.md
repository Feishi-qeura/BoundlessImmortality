# Bounded review v4 corrections · review-v5

## Verified findings and changes

- **V4-API-01 (P1):** Both copied translation units call `SetPadding` on a `UHorizontalBoxSlot`. Added the defining `Components/HorizontalBoxSlot.h` include to `RebuildActivitySecondaryWidgets.cpp` and `RebuildEconomyWidgets.cpp`.
- **V4-ACT-01 (P1):** The service stores the current user's `offline_reward` aggregate and `acknowledge_offline(ctx,args)` accepts no source identifier. Removed `SourceId` from the staged offline view models and removed the nonempty-ID gates. The visible/dispatch gate is `bCanAcknowledge && connected && !busy`; the host must freshly verify current actor/identity and actual offline-record eligibility before the no-argument `ack-offline` command. The command remains a read acknowledgement, not a grant.
- **V4-ACT-02 (P2):** The confirmed launch offer now emits `OfferId`, `EntryId=SKU`, `Version=confirmed quote version`, `PriceText=confirmed displayed text`, and `bConfirmed=true`. The host must compare that exact captured tuple with a fresh quote before invoking the existing purchase path. No payment-provider field or new command was added. Offer UI remains disabled unless real offer, entitlement, service purchase, platform payment, connection and non-Busy gates are all supplied.
- **V4-ECO-01 (P2):** Shop confirmation binds SKU/version/price; market buy binds listing ID/version/displayed price; create binds inventory item/parsed quantity/user price; order cancel binds listing ID/version. Local selection changes clear that workflow's confirmation and captured values. `ApplyViewModel` compares incoming selected subject and row values directly with the captured tuple; unchanged tuples retain confirmation.

## Frozen-input identity

`source-delta-v5.json` records the exact SHA-256 of each frozen v3/02/economy03 input and each replacement file. The input hashes were also matched against `model/review/source-delta-v4.json` before copying. `v5.patch` records the precise hunks against those inputs; the unchanged shared base `.cpp` is included in the full replacement set and has the same hash.

## Evidence boundary

This is source-only static review. No build, compile, test suite, Unreal Editor, PIE, asset import, route binding, Cook, package or production-source change was performed. All `approved` flags remain false.
