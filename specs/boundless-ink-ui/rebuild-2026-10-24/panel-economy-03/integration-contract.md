# Prototype shop, consignment and orders · integration contract

## Shared dependency

`RebuildEconomyWidgets.{h,cpp}` depends on `panel-activities/review-v3/staged-source/RebuildActivityWidgets.{h,cpp}` and `Frontend/UIRebuild20261024/InkRebuildTheme.h`. These are candidate source files only; place them together in a future integration change and bind inherited `OnActionRequested` once. Host creates each widget with `CreateWidget<WidgetClass>(OwningPlayer)`, binds the delegate and calls its typed `ApplyViewModel(...)`.

## Pages and requests

| Widget | View model | Request behavior |
| --- | --- | --- |
| `URebuildShopWidget` | `FRebuildShopViewModel` | `query-shop` carries offset and OwnedOnly. Purchase sends SKU in `EntryId`, quote version in `Version`, displayed authoritative price text and confirmation. SKU, quote, price and entitlement/permission are host-owned. |
| `URebuildConsignmentWidget` | `FRebuildConsignmentViewModel` | `query-market` carries offset and OwnedOnly. Paging requires a positive host-provided `PageSize`; next additionally requires host-provided `bHasNextPage`. `create-listing` carries real inventory `ItemId`, positive parsed user quantity, original quantity text, user-entered unit-price text and confirmation. `buy-listing` carries exact ListingId/version and confirmation. |
| `URebuildOrdersWidget` | `FRebuildOrdersViewModel` | Shows service rows and state. `cancel-listing` carries exact ListingId/version and confirmation; it never changes local order state optimistically. |

All commands are UI intents, not authority decisions. The host must freshly capture actor, identity, connection, Busy, inventory, quote/listing version and permission; run existing validation and submit only under the shared action contract. Query acknowledgements remain pending until fresh service data arrives. Unknown values stay unknown; the page does not synthesize prices, currency, balance, stock, listing rules, order status or successful payment/settlement.

## Visual assets

The three RGB PNGs are isolated composition concepts under `assets/pages/`; `asset-manifest.yaml` records their measured dimensions and SHA-256. Every entry remains `approved:false`, `runtime_texture:false`, and `import_verified:false`. No import or route change is included.

## Evidence boundary

Source and concepts received static inspection only. No Unreal compile, editor, route binding, PIE, Cook, device, package or production acceptance is claimed.
