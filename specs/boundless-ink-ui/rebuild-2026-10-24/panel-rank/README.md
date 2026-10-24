# Rank and Wuling UI rebuild · first page slice

## Scope

Two independently mergeable slices are staged: (A) the three leaderboard variants (Damage, Cultivation Speed, Anchor Spirit), Wuling Overview, and Wuling Members; (B) Guild Build, the viewer's Sword Spirit page, and read-only Gift Rules. They use independent UUserWidget components with dynamic text and explicit host action callbacks. Prior candidate page artwork is used only as a composition source, never as a runtime page background or substitute for controls.

The work is staged for the sole production integration owner. No `Source/`, `Config/`, or `Content/` files are changed here. Runtime asset references are planned under `/Game/newUI/`; no `/Game/UI` paths are introduced.

## Page composition

- **Leaderboard:** shared page shell, three category tabs, local/global scope tabs, metric-specific header, ordered provider rows, self-position footer, and explicit unavailable/loading/empty/error states. Display order/rank/metric come from the provider. No prizes, period controls, or claim action are added.
- **Wuling Overview/Members:** organization identity and service summary, then server-paged selectable members. Remove requires a local danger confirmation bound to the selected user id; permission, profile version, request identity, pending/error state, and mutation remain host/service owned.
- **Guild Build:** server catalog rows with actual current/target levels and costs where supplied, paging, and a build confirmation bound to building id, target level, and the supplied snapshot version.
- **Sword Spirit/Gift Rules:** show only an exact viewer participant record. The current contract has no complete personal profile or synced gift catalog, so unavailable/unknown remains explicit; no guild or anchor numbers are substituted.

## Layout source

Reuse the current unapproved candidate page images under `specs/boundless-ink-ui/revisions/2026-10-23-parallel-rank-wuling/assets/pages/` as composition references. Their current empty/loading examples are not treated as populated runtime data. The approved project style contract supplies palette, typography, 720×1280 portrait baseline, touch targets, row minimums, safe area, and landscape breakpoint. New semantic assets are text-free and separately registered.

## Status and evidence

Design and component candidates remain `approved: false`. Integration, import, compile, route, PIE, player-entry screenshots, and visual acceptance remain unverified. See `status.json`, `screen-contract.yaml`, `component-contract.yaml`, `asset-manifest.yaml`, and `handoff.md`.
