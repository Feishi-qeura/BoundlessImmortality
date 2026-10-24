# Player B handoff — Character and Inventory

Updated: 2026-10-24 12:53:06 Asia/Hong_Kong
Slice: `player-b`; pages: `character`, `inventory`; `approved:false`.

## Staged page candidates

- `InkPlayerCharacterInventoryWidgets::BuildCharacterPage` composes live name/realm, typed attribute rows, actual discipline data, provider state, and four real route actions. It uses the generated 3×3 attribute atlas by `AttributeId`.
- `InkPlayerCharacterInventoryWidgets::BuildInventoryPage` composes filters, live capacity, 40-slot pagination, item slots/details, actual quantity controls, and existing use/equip/sell handlers. It retains stable selection in the host; visible slot index is only the current host input bridge.
- Both pages include 88×88 minimum action targets and use the new style-contract palette.

## Data and action seam

Character data comes from `GetPresentedCharacterName`, the anchor status realm, `GetPresentedCharacterAttributes`, and `GetPresentedCharacterDisciplines`. Route controls dispatch `OpenEquipmentFabao`, `OpenCultivationMethods`, `OpenBreakthroughPrepare`, and `OpenCharacterAppearance`.

Inventory state comes from `UInkInventoryModel`; the page receives `EntryId`, capacity-known state, unlocked count, host filter/search/page state, and action quantity. Slot selection dispatches the existing `InventorySelect_<SlotIndex>` bridge, which resolves the current stable ID in the host. Use/equip/recycle dispatch through `InventoryUse`, `InventoryEquip`, and `InventorySell`; the host/authority adapter retains request identity, validates the current subject, and applies recycle confirmation. No item grants or costs are invented.

## Candidate art

`visible-entry/assets/components/asset-manifest.json` records measured RGBA dimensions, alpha range, SHA-256, and atlas cell mapping for HUD route glyphs, Character attribute glyphs, Inventory category/empty/locked glyphs, and a Main action-corner ornament. Images have not been imported into Unreal.

## Remaining for integration

The one runtime root must bind distinct `character` and `inventory` page IDs, provide the inventory search callback and state messages, reflow the pages for portrait and landscape safe areas, import `/Game/newUI` assets, and record actual entry screenshots and functional validation. This is a staged two-page handoff; it does not mark the Player group complete or any page runtime-verified.
