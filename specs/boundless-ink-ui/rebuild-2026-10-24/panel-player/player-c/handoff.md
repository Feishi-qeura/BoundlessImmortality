# Player C handoff — Methods, Equipment, Appearance

Updated: 2026-10-24 13:15 Asia/Hong_Kong  
Slice: `player-c`; candidate approval: `approved:false`; production integration: not performed.

## Page composers

`InkPlayerMethodsEquipmentAppearanceWidgets.h/.cpp` adds three independent native widget-tree composers. They accept typed page view models, a responsive layout mode, optional atlases, style tokens, and an `FActionRequested` callback. Each page has its own root widget names and can compose vertically at 720×1280 or reflow list/detail into two columns at 1280×720.

- **Methods:** separate provider states for catalog, owned methods, main, supports, and inventory; only actual catalog and host draft IDs render. The inventory section reads real `UInkInventoryModel` entries and retains selected `EntryId` for local detail. Selection/draft events stay local. `learn-method` sends the chosen catalog `ItemId`; `configure-methods` sends the host-owned `main` and `supports` draft. No support-slot entitlement is inferred.
- **Equipment:** provider-backed slots, fabao, owned item instances, and catalog. Inventory selection carries stable `EntryId`; equip/unequip actions carry `EntryId` and quantity. Availability/reasons come from the authority adapter. `activate-fabao` carries no invented client parameters.
- **Appearance:** optional actual identity portrait, actual CharacterVm attribute summary, current preview fields, and host-supplied local style options. The Frame/Bar tab strip changes only the local host draft. Visible copy says the selection is local preview only. It makes no unlock, ownership, equip-persistence, or account-progress claim.

The typed seam and canonical handler mapping are in `contracts/action-adapter.yaml`; component and provider mapping is in `contracts/page-component-map.yaml`. The existing shared style, screen, brief, VM, action, and binding files remain untouched. `FStyleTokens` receives the canonical palette and the MaShanZheng/LXGWWenKai font objects from the newUI host; no legacy theme helper or font fallback is embedded.

Appearance reuses the already generated Character attribute atlas from the frozen player-a slice by reference; it does not duplicate or modify that file.

## Transparent component candidates

Three page-specific atlas PNGs are in `visible-entry/assets/components/` with saved prompts and SHA-256, dimensions, RGBA mode, alpha extrema, cell semantics, and `approved:false` in `asset-manifest.json`. Pixel metadata was read from the copied files. An initial generator pass baked checkerboard pixels into RGB; those files were replaced with true-alpha RGBA outputs, and the rejected pass is retained only as generation provenance. The assets are candidates, not imported Texture2D assets.

## Host-owned integration seams

- Resolve `ELayoutMode` from the live viewport ratio and preserve scroll/selection/draft state when rebuilding.
- Load canonical fonts and colors into `FStyleTokens`; the builder does not substitute default Slate typography.
- Bind `FActionRequested` to the per-page host, route `ui.*` events into page-local state, and map only the canonical command IDs to the handlers documented in `action-adapter.yaml`.
- Supply field-specific provider states and action availability from fresh authority/context data. Preserve stable IDs and pending state; do not optimistically mutate loadouts or inventory.
- Import the candidate atlases into `/Game/newUI/Components/Player`, assign references, then inspect the actual newUI route and portrait/landscape player-entry behavior.

## Validation boundary and next work

Manifest JSON and PNG metadata are checked; source received static review only. No Unreal build, Editor, PIE, cook, package, device, or platform validation was run. This is a staged page-composer handoff, not an integrated or visually approved deliverable.

Next page batch continues in brief order: `craft`, `item-sources`, then `breakthrough`, `tribulation`. The Player group remains active.


