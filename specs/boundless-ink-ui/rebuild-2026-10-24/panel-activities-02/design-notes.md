# Activity pages · slice 02

## Pages

1. **Launch Contribution Rank** (`launch-rank`): standalone responsive list page, service-provided scope and row order, preserves ties exactly as returned. No client ranking calculation, score synthesis, avatar requests, or scope action absent from the page contract. The only action is `back`.
2. **Launch Special Offer** (`offers-unavailable` variant): a route-less page body owned by the existing LaunchEvent host. The present contract has no launch-offer route or integrated payment/SKU/price source, so the page displays unknown/unavailable fields and no purchase CTA. A typed future purchase seam is retained: it appears only when a live SKU, authoritative price/quote version, entitlement, purchase service, connection, non-busy state and host permission are all provided; it captures SKU and quote for confirmation and emits contract `purchase` with `bConfirmed=true`.
3. **Offline Rewards** (`offline`): receipt list sourced from confirmed reward receipts and the current offline object. Acknowledge is a read acknowledgement (`ack-offline`), not a claim/grant. It has no request parameters per contract; host must capture the current exact subject/source and revalidate identity, Busy, connection and fresh state before submit.

## Composition and truth

- Native UMG page trees; the PNGs are full-page concepts only. No page bitmap is a runtime widget.
- Portrait uses a fixed masthead/footer and scrollable body at the 720×1280 logical target. Landscape reflows into list/detail columns. Action targets meet the shared 88 logical-pixel minimum.
- All text uses `InkRebuildUI::Font`; surface and control styles use the shared new-root theme. Actual data strings are model inputs.
- Known/unknown is explicit with `b...Provided` flags. Rank positions and ties are displayed as returned, never re-sorted or calculated. Missing contribution, score, Qi, source, SKU, price and currency stay unknown.
- Shared actions only: `back`, `purchase` (only under the future authorized offer gates above), `ack-offline`. Purchase emits SKU and explicit confirmation only; host recaptures the authoritative quote/version/price. Offline acknowledge emits no invented payload.

## Inputs consumed

- `contracts/screen-briefs/launch-rank.md`, `offline.md`, `launch.md`, and the shared `screen-contract.yaml`.
- `model/vm-contract.yaml`: `LaunchVm`, `RewardVm`, `EconomyVm` rules.
- `model/action-contract.yaml`: `back`, `purchase`, `ack-offline` semantics and command gates.
- Shared runtime font/style helper: `Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildTheme.h`.

## Ownership boundary

Source is staged for `logic-integration`; runtime assets, if later accepted/imported, belong under `/Game/newUI/Activities/`. `LaunchSpecialOffer` is explicitly a subview of the launch event, not a new route/page-contract entry. All candidates stay `approved:false`; no production source, Content, root, route, or engine operation occurs in this slice.
