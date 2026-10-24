# Economy pages · slice 03 handoff

## Delivered for review

- `URebuildShopWidget`: authority-backed catalog/detail state, filter and supplied-page-size paging, quote-bound two-step purchase confirmation.
- `URebuildConsignmentWidget`: market list and filters, supplied-page-size paging, actual inventory selection, user-entered quantity/unit price, and listing-version-bound buy/create confirmations.
- `URebuildOrdersWidget`: service-backed order rows and listing-version-bound cancel confirmation with no optimistic state update.
- Three portrait composition concepts and measured image metadata in `asset-manifest.yaml`.

## Exact source and dependency

- Economy source: `staged-source/RebuildEconomyWidgets.h/.cpp`.
- Shared dynamic action relay/request: `../panel-activities/review-v3/staged-source/RebuildActivityWidgets.h/.cpp` (v3 also contains the launch-start N01 correction and extended economy request fields).
- Shared theme: `Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildTheme.h`.
- Host creates each page using `CreateWidget<WidgetClass>(OwningPlayer)`, binds inherited `OnActionRequested` once, supplies its typed view model and handles only actions listed in `integration-contract.md`.

## Authority boundaries

The pages pass intent and the current visible subject to the host. They do not perform local purchase, grant, market mutation, payment, settlement or order-state changes. Host integration must re-capture identity, Busy, permissions, current inventory, quote/listing versions and rules before existing authoritative evaluation/submit. All money, availability and statuses must come from service state; user draft inputs are passed for host validation.

## Evidence and approval

Static source review and image metadata checks only. No compile, editor, route binding, PIE, Cook, device or package verification was performed. All three concepts remain `approved:false`, and no image was imported. Do not mark pages composed or player entry verified from these files.
