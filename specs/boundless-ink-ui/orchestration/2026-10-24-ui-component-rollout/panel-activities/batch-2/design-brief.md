# Activity batch 2 component design

## Scope

Three isolated merge units: (1) the actual launch activity page with overview/daily tasks and special-offer gate, (2) launch contribution rank, (3) a shared detail shell for daily and limited activities. Existing authority data/action semantics are inputs; this batch does not alter current Source/Content.

## Visual direction

Use the approved shared water-ink style and existing component renderer contracts as the project style source. Any newly generated illustration is a candidate only. All labels, dates, numeric progress, task states, reward summaries, prices, and ranks are rendered from UMG TextBlocks bound to authority fields. Unknown fields show a neutral pending label; actions requiring absent SKU/price/entitlement are disabled.

## Planned hierarchy

- Launch: compact identity/status header; server-UTC deadline; segmented overview/tasks/community/honors tabs; tasks use an explicit list/empty-state card; offer section is server-data-driven and has a disabled, explained action until a verified SKU + price + transaction contract exists.
- Launch rank: server period/state banner; contribution rows from the launch authority leaderboard; own placement/unranked/empty/tie labels only when the matching snapshot fields exist; no invented rows.
- Daily/timed detail: parent list retained; selected authority template and detail snapshot in a responsive two-column composition on wide layouts and stacked composition on portrait; pending, empty, error and expired states are mutually exclusive; footer action slot uses only an existing server-authorized action and request path.

## Candidate source and approval

No local old candidate PNG is a new visual reference. New image prompts are saved before image generation. Approved shared design contracts and the official/store reference notes in `visual-research.md` constrain style and information hierarchy. Every new PNG, contract, manifest row and page mock remains `approved:false` for user visual review.
