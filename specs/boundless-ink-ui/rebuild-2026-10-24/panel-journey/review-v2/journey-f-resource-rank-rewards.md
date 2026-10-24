# Journey F: resource rank and rewards

Owner: panel-journey; merge slice: `journey-f`; contract revision: `newui-r1`; candidate `approved:false`.

## Routes

- `resource-rank` / `ResourceRanking`: `UInkJourneyResourceRankPageWidget`.
- `resource-rewards` / `ResourceRewards`: `UInkJourneyResourceRewardsPageWidget`.

The pages use a shared read-only composition base but remain different UUserWidget route classes.

## Resource rank

The current service has no distinct authoritative resource contribution ranking contract. Host passes `bCollectionProvided=false`, displays the explicit unavailable status and no fabricated rows. Do not repurpose guild growth or a generic leaderboard as resource contribution data.

## Resource rewards

Host supplies only rows from `RewardVm` receipts filtered by exact event/node/actor source. It passes an explicitly provided empty collection only when the exact source query is authoritative and empty; unknown source or missing receipt collection remains unknown. Do not claim entitlement from inventory delta, drain the receipt inbox, or acknowledge on page open.

## Evidence boundary

These read-only routes have no write action. Staged view classes do not establish that the missing rank business contract exists or that actual receipts were received.
