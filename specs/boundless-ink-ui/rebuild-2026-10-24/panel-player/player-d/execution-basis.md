# Player D execution basis

Updated: 2026-10-24 13:16 Asia/Hong_Kong

## Authority and scope

This isolated slice implements staged native widget composers for `craft` and `item-sources`. It does not edit production `Source/` or `Content/`, shared canonical contracts, or the frozen Player A/B handoff files. The player's continuous design/component authorization and user-owned final visual approval are recorded in the rebuild `quickstart.md`; this slice keeps every candidate `approved:false`.

## Canonical inputs

Use `contracts/style-contract.yaml`, `contracts/screen-contract.yaml`, the `craft` and `item-sources` briefs, `model/vm-contract.yaml`, `model/action-contract.yaml`, and `model/screen-bindings.json` as the source of truth. This directory contains only an implementation overlay; it does not alter those shared documents.

## Composition

- Craft: live recipe catalog, stable recipe selection, true ingredients/output detail, optional read-only inventory cross-check, provider/action states, query controls, and a host-owned confirmation stage before `craft` dispatch.
- Item Sources: live source rows with `source_id`, `item_id`, `method`, and `source`; selecting a row retains both IDs. `choose-focus` only enables for the exact `fixed_selected_reward` method and host-provided action availability.
- Both pages use independent trees, style-token/font injection, 720×1280 portrait and 1280×720 two-column reflow, 24 px safe inset, and 88×88 hit targets.
- No new image is necessary: dynamic catalog/source entries use actual provider icons where supplied, and the existing component atlases do not describe recipes or source methods. No decorative data surrogate is added.

## Review focus

- Never render missing recipes, ingredient costs, outputs, source locations, or rewards as invented values.
- Provider `Empty` must be explicit; unknown/loading/offline/error/pending stay distinct.
- Query offset and ownership filter remain host-owned and are sent unchanged.
- Craft confirmation precedes the canonical `craft(recipe_id)` command; the client does not consume inventory optimistically.
- Only `fixed_selected_reward` supports ChooseFocus; viewing other source methods remains available.
- Missing descriptive fields do not block unrelated navigation or valid server commands.

## Verification boundary

This slice uses file, contract, action-payload, and static-source checks only. It does not claim an Unreal build, runtime route, PIE, cook, package, device, or platform result.
