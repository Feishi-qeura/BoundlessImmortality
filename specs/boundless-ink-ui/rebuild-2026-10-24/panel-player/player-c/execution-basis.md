# Player C execution basis

Updated: 2026-10-24 13:15 Asia/Hong_Kong

## Scope and authority

This slice adds new staged page composers for `methods`, `equipment`, and `appearance` under `panel-player/player-c/`. It does not edit production `Source/` or `Content/`, and it does not modify the frozen `panel-player/handoff.md` or `player-b-handoff.md`.

The project [quickstart](../../quickstart.md) records the user's authorization for continuous design and component production and states that final visual review remains with the user while candidates stay `approved:false`. The user's latest instruction again authorized continuing the unfinished player queue and requested only genuinely missing transparent components. Accordingly, this slice may generate semantic transparent component art, measure the saved files, and register them as candidates; generation is not a visual approval.

## Contract alignment

Canonical inputs are `contracts/style-contract.yaml`, `contracts/screen-contract.yaml`, `contracts/screen-briefs/{methods,equipment,appearance}.md`, `model/vm-contract.yaml`, `model/action-contract.yaml`, and `model/screen-bindings.json`. They remain the source of truth. This directory records a local implementation overlay only; it does not silently change shared contracts.

All three pages use the 720×1280 portrait baseline, 24 px safe content inset, 88×88 minimum hit target, 1280×720 landscape reflow, shared palette, and injected MaShanZheng/LXGWWenKai font slots. Dynamic labels and values remain live widgets. Missing values stay unknown; only explicit provider-empty state renders an empty collection. Preview actions remain local and never claim entitlement or persistence.

## Component work plan

1. Define the page layout/style injection, provider states, page-specific typed view models, and typed action payloads in `staged-source/` and `contracts/`.
2. Compose each page as an independent widget tree: Methods loadout/catalog/detail/actions; Equipment actual slots/fabao/owned item selection/actions; Appearance identity/preview/options with explicit local-preview labeling.
3. Save prompts before image generation. Generate only a methods semantic atlas, generic equipment/fabao glyph atlas, and appearance preview ornament atlas if the staged trees need those visuals. Record actual dimensions, alpha extrema, byte counts, and SHA-256 with `approved:false`.
4. Add a short handoff with exact provider/action seams, host-owned fields, outstanding integration boundaries, and verification limits. Keep the root status active and queue Craft → ItemSources → Breakthrough → Tribulation.

## Review focus

- Unknown catalog or loadout data must not appear as empty, zero, locked, or ineligible.
- Method support rows must reflect only provider-supplied `supports`; do not invent slot entitlement.
- Equipment commands must carry stable `EntryId` and quantity; eligibility stays with authority action state.
- `activate-fabao` has no invented client parameters and must not infer a selected item.
- Appearance options are presentation-only; no unlock, purchase, equip, or persistence claim.
- Landscape reflows to list/detail columns instead of scaling the portrait page.

## Verification boundary

Only file/contract/static checks and PNG metadata checks are in scope for this slice. No UE build, Editor, PIE, cook, package, device, or platform acceptance is claimed.


