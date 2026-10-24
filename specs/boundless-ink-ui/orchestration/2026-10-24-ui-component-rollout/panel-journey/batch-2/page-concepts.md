# Batch 2 page concepts and authority contract

This batch is intentionally native-widget only. Existing approved style and interaction contracts already cover the water-ink shell, so no new art or image generation is required. All labels, values, result messages, and counts are runtime `FText`; the batch adds no baked UI text or reward art.

## Resource detail

- Entry: `resources` → `resource-detail`; back follows the existing navigation stack.
- Header: resource name from the selected `journey.resources[]` row. Show current status separately from the resource facts.
- Facts may be shown only when supplied by the selected authoritative row: `item_name`, `quantity`, `interval_seconds`, `qi_additive`, `lease_max_seconds`, and exact `owners[]` (`name`, `end`). Missing fields render as unavailable in the host mapper; zero and empty are not inferred from absence.
- Main action is two-step: “查看参与确认” opens a confirmation message, then “确认前往竞采” emits the selected `entry.id`. Host re-captures `InkJourneyAdapter::Capture`, evaluates `ResourceTravel` against that exact id with `bConfirmed=true`, and only then calls `Submit`. Enqueued means pending, never matched or granted.
- Status states: unavailable/loading/offline/current facts, confirmation open, request pending, and the adapter/service result. Back emits `OnBackRequested`.
- No detail illustration is needed; reuse the approved resource-map candidate and existing native paper shell after visual approval.

## World result

- Entry: the world-round result route. Back returns to the originating journey page.
- Preferred source is `snapshot.round` for the current anchor: its `id`, `status`, `completed[]`, and `participants` stage records. Do not treat a partially completed or interrupted round as completed.
- Separate completed stages, anchor stage participation, each participant's contribution, guild inventory/vault movement, and any extra/core receipt. Only show a receipt if its `source_id` exactly identifies the current round or its stage; never use aggregate inventory deltas as proof of reward.
- Missing current round or result fields use “权威结果待同步” / “服务未提供”. The page has no claim/grant action. “返回” only closes the result view and does not mutate rewards or receipt state.
- Visual: compact result title/status card, then one card row per authoritative result group; scroll the rows independently from the fixed back action when hosted in the responsive shell.

## Race result

- Entry: `race-live` → `race-result`; back follows the navigation stack.
- Source is `snapshot.race` for the current user's actual race. Read `id`, `status`, `node_id`, `start`, and the matching team by current identity/room. Show score and score contributors only if present in that team. If race/team is missing, show pending synchronization rather than cached labels or invented score.
- Eligibility threshold, tie ordering, and the 30/20/10-minute lease policy are existing interaction-contract rules. Derive the winning tier from the server's settled team/race and actual `lease`; never locally award a tier. During reconciliation show “结算确认中”.
- Link race reward receipts only on an exact race-derived source id that matches the current race and anchor. `reward_receipts.entries` is a presentation inbox and must not be consumed here; never call `TakeNextRewardReceiptJson()`.
- Main action is “返回”; no claim action. Actual lease state and end time remain separate from race score/result.
- Visual: settlement title/status, score and contribution rows, eligibility/lease card, and any exact-source receipt section. Distinct row labels preserve common score, anchor actions, and representative responses as separate sources.

## Shared settlement card

`UInkJourneySettlementCardWidget` is a data-only native paper card. The host supplies heading/status/rows from an authority projection. It performs no state reads, grants, acknowledgements, or navigation. The world and race outcome shells use it; all text is live widget text.

## Image/concept decision

No new raster concept is necessary for these data-dense detail/result pages. Retain existing page backgrounds and established style contract; use native text/card treatments. No first-batch asset, manifest, PNG, or ZIP is copied or regenerated.
