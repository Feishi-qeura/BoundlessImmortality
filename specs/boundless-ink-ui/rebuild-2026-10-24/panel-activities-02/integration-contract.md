# Activity secondary pages · integration contract

## Shared dependency

`RebuildActivitySecondaryWidgets.{h,cpp}` depends on the v3 shared presentation types and relay in `panel-activities/review-v3/staged-source/RebuildActivityWidgets.{h,cpp}`. Integration should promote the paired activity base/button and secondary page files into the same production source folder, then bind the inherited `OnActionRequested` once. No custom page constructor arguments; create with `CreateWidget<WidgetClass>(OwningPlayer)` then call the typed `ApplyViewModel(...)`.

Shared theme is read from `InkRebuildTheme.h`: text uses `InkRebuildUI::Font`, control style uses `InkRebuildUI::ButtonStyle`, safe inset and touch size use shared tokens. The rank and offline pages, plus the offer subview, retain a stable root and scroll/confirmation state on refresh.

## Pages and API

| Widget | View-model update | Request/action behavior |
| --- | --- | --- |
| `URebuildLaunchRankWidget` | `ApplyViewModel(const FRebuildLaunchRankViewModel&)` | Rows preserve service ordering and returned rank text including ties. Scope is display-only because the launch-rank page contract exposes only `back`. Back emits `back`; no score/rank calculation, row sorting, or user filter is invented. |
| `URebuildLaunchSpecialOfferWidget` | `ApplyViewModel(const FRebuildLaunchOfferViewModel&)` | Route-less child of LaunchEvent `offers-unavailable`; not a new page/route. Purchase only appears if offer ID, SKU, nonempty quote version, authoritative price+currency text, known unowned entitlement, platform payment, service purchase, connection, non-busy and host permission are all true. Confirmation captures offer ID, SKU, quote version and displayed price; changes reset it. Second click emits `purchase`, `EntryId=SKU`, `bConfirmed=true`; host must re-capture price/version/entitlement and recheck identity, Busy and permission. |
| `URebuildOfflineRewardsWidget` | `ApplyViewModel(const FRebuildOfflineRewardsViewModel&)` | Displays confirmed receipts and known totals only. `ack-offline` has no payload per shared action contract; host recaptures exact current offline source/subject, identity, connection and Busy before submit. It acknowledges reading and does not grant or claim rewards. |

All view models include state plus collection/field-known flags, so missing is not zero or empty. `PriceText` must contain service-authoritative amount and currency as one display value. If launch purchase API or its quote source is absent, integration supplies `b...Available=false` and the subview stays disabled without SKU/price/checkout claims.

## Assets

Three RGB PNGs are composition concepts only, all `approved:false`, `runtime_texture:false`, `import_verified:false`. Intended paths are `/Game/newUI/Activities/Concepts/`; no imports occurred. `launch-special-offer` is specifically a subview of the existing launch event and is not added to route metadata.

## Evidence boundary

These source files have static review only. No source promotion, actual route/page binding, asset import, compile, Editor, PIE, screenshot capture, Cook, device, or package evidence is claimed. The generated PNGs do not establish page composition or player entry.
