# Handoff · panel-activities slice 02

## Delivered for review

- `URebuildLaunchRankWidget`: service-owned launch contribution scope, current-participant summary, actual rank row list, state handling, stable root and portrait/landscape reflow.
- `URebuildLaunchSpecialOfferWidget`: route-less LaunchEvent offers-unavailable child view; unknown fields stay unknown; purchase only under full live offer/payment/price/entitlement/connection/Busy gates and a subject-bound confirmation.
- `URebuildOfflineRewardsWidget`: confirmed offline receipt list, explicit unknown/empty distinction, receipt totals only when known, and a read-acknowledgement intent with no invented parameters.
- Three page concepts and actual metadata in `asset-manifest.yaml`. All image concepts are review-only.

## Exact source and data interface

- Source: `staged-source/RebuildActivitySecondaryWidgets.h/.cpp`; shared action relay/base is from `../panel-activities/review-v3/staged-source/RebuildActivityWidgets.h/.cpp`.
- Classes have default UUserWidget construction. Host creates with `CreateWidget<WidgetClass>(OwningPlayer)`, binds inherited `OnActionRequested`, calls `ApplyViewModel(const F...ViewModel&)`, then handles only shared contract action IDs.
- `URebuildLaunchRankWidget` accepts `FRebuildLaunchRankViewModel`: exact event ID, scope/period, own rank/contribution-known flags, rows-provided flag, status and ordered `FRebuildLaunchRankRowViewModel[]`. Per row, participant ID and rank/name/contribution have independent known flags. It does not reorder, derive ties, or synthesize missing rows/fields. Only `back` returns.
- `URebuildLaunchSpecialOfferWidget` accepts `FRebuildLaunchOfferViewModel`: offer ID, SKU, quote version, title/description, authoritative price+currency text, entitlement status and known/availability/connection/Busy flags. Its second confirmed click returns `purchase` with SKU in `EntryId` and `bConfirmed=true`; current integration is expected to pass unavailable flags until the real launch-offer SKU, price quote and purchase channel exist. The host recaptures and revalidates the current quote and actor state before dispatch.
- `URebuildOfflineRewardsWidget` accepts `FRebuildOfflineRewardsViewModel`: exact source ID, offline/acknowledged Qi text with separate known flags, receipt-provided flag, service state, Busy/connection/can-ack flags, and receipt rows keyed by receipt/source IDs with independent source/time/Qi-known flags. Its `ack-offline` event has no payload; the host uses a fresh current subject and the existing subsystem operation, never a client grant.
- All text uses shared `InkRebuildUI::Font`; controls use the shared button style and 88-unit target; landscape builds use a reflowed summary/detail split, not a portrait bitmap scaled to fit.

## Remaining handoff facts

- `launch-rank` uses the existing `LaunchRank` semantic route and needs a distinct new-root host binding by `logic-integration`.
- The offer view is embedded under the existing `LaunchEvent` host; no new route is requested.
- `offline` uses the existing `OfflineRewards`/`WulingOffline` semantic entry and needs an integration host binding by `logic-integration`.
- Nothing here is compiled, imported, routed, displayed in Unreal Editor, or accepted visually. Do not mark contract `page_composed` or `player_entry_verified` from source or concept PNGs.
- All three candidate visual directions remain `approved:false` until the user reviews them.

## Next requested slice

Continue with Prototype Shop, Consignment, and Orders in separate authority-backed pages. Preserve live SKU/price/quote versions, listing/item IDs, order states, identity, connection, Busy, and confirmation subject. Unknown money/data stays unknown; use `query-shop`, `purchase`, `create-listing`, `buy-listing`, `cancel-listing`, and actual order actions only where the shared contracts authorize them. Keep every page native UMG and do not add client-ledger or payment behavior.
