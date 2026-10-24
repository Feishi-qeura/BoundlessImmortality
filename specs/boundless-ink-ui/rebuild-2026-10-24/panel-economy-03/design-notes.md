# Shop, consignment and orders · slice 03

## Page boundaries

- **Prototype Shop** (`shop`): service-backed catalog, selected detail and purchase confirmation. `query-shop` carries actual paging/filter values. Each SKU/cost/availability is independently known; purchase is exposed only when the host supplies actual SKU, authoritative price+currency, quote/listing version, permission, connection and non-Busy state. There is no platform checkout integration claim.
- **Consignment** (`market`): actual market catalog plus a user-entered unit-price draft and count for the selected owned item. Query uses `query-market` with actual offset and OwnedOnly state. Previous/next controls require a positive host-supplied page size, and next additionally requires an explicit `bHasNextPage`; filter changes reset offset to zero. Listing requires service-provided price/count rules and inventory eligibility; the UI does not guess bounds. `create-listing` returns real `item_id`, parsed user count, user-entered unit price and confirmation. `buy-listing` uses exact listing ID/version and confirmation.
- **Orders** (`orders`): service-provided own listing/order states and versions. Cancel is shown only if the host says the exact listing is cancellable; confirmation binds listing ID and listing version. `cancel-listing` is an authority command, not an optimistic local status update.

## Shared presentation shape

- Native UMG page trees, shared `URebuildActivityPageWidget` relay/button base and shared theme font/style injection.
- Stable page roots; scroll and filter/paging state stay outside regenerated rows. Portrait list/detail stacks; landscape uses reflowed list/detail columns.
- Money text is supplied as an authoritative formatted price+currency. Draft input is user text passed to host validation; no hardcoded price schedule, bounds, fees, proceeds, tax, exchange rate, payment provider or settlement claim is added.
- Query/command acknowledgements remain `pending` until service state arrives. Unknown catalog, wallet, inventory and listing state remain unknown, never zero/empty by default. Shop paging likewise requires host-supplied page size and `bHasNextPage`; filter changes reset offset to zero.

## Contracts consumed

- `contracts/screen-briefs/shop.md`, `market.md`, `orders.md` and matching screen entries in `screen-contract.yaml`.
- `model/vm-contract.yaml`: `EconomyVm`, `InventoryVm`.
- `model/action-contract.yaml`: `query-shop`, `query-market`, `purchase`, `create-listing`, `buy-listing`, `cancel-listing` and their exact gates/parameters.
- Shared font and button styling: `InkRebuildTheme.h`; shared action request transport: activity review-v3 `FRebuildActivityActionRequest`.

## Ownership and open integration facts

`logic-integration` is the sole production source/config writer. These source classes have typed adapters but do not call `UWulingPrototypeSubsystem` directly. The host must supply fresh known flags/values, permission booleans and command results, then revalidate fresh actor, identity, Busy, current inventory, quote/listing version and all service rules before submit. All assets remain visual concepts and `approved:false`.
