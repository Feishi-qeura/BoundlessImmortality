# Player D handoff — Craft and Item Sources

Status: staged candidate, `approved:false`. Production `Source/` and `Content/` were not modified. No Unreal build, automation, Editor, PIE, cook, package, device, or platform verification was run.

## Delivered

- Independent `BuildCraftPage` and `BuildItemSourcesPage` composers under `staged-source/Frontend/UIRebuild20261024/Player/`.
- Typed RecipeVm, read-only InventoryVm, and ItemSourceVm view data; provider state and styles are supplied by the host.
- Craft uses real recipe IDs, ingredients, and output fields. It opens a confirmation stage first and only then emits canonical `craft(recipe_id)`. Inventory is displayed as a read-only cross-check; this page does not calculate readiness or mutate inventory.
- Item Sources retains the selected `source_id` and `item_id`. `choose-focus` is offered only for the selected row whose method is exactly `fixed_selected_reward`; other sources remain viewable.
- Portrait and landscape composer paths are separate. Landscape lays catalog/source and detail content into columns; portrait keeps one scrolling column. Shared safe inset, palette, typography, and hit-target tokens come from the host.
- Canonical route/action overlays and per-file hashes are recorded in this slice's contracts and manifest.

## Integration boundary

Bind the typed provider data to the existing RecipeVm/InventoryVm/ItemSourceVm producers and wire the emitted action IDs through the logic-integration adapter. Keep offset and OwnedOnly state host-owned. Revalidate route, role, connection, busy state, and current subject before command dispatch. The existing route names are represented in the overlay; this staged code does not prove a page class, WBP, root registration, or runtime route exists.

## Review boundary

Candidates remain unapproved; final visual acceptance belongs to the user. Missing provider fields remain visibly unknown, and no recipe, cost, output, source reward, activity, or entitlement is invented. This slice is a static handoff only; do not treat it as production integration, build, or runtime evidence.
