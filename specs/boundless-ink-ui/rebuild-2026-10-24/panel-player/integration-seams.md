# Panel Player integration seams

Date: 2026-10-24 (Asia/Hong_Kong)
Approval state: `approved:false`; all generated art remains a candidate for final user visual review.

## Staged source

Production candidates are isolated under `staged-source/Frontend/UIRebuild20261024/Player/`. Add the module root `Source/BoundlessImmortality` to the module include search path; retain the module's normal compile/response-file discovery. This panel has not edited production source or build configuration.

## Main route

Call `InkPlayerVisibleEntryWidgets::BuildMainMenuSurface` from the actual runtime Main route after the saved tree and action dispatcher are available. Place the returned surface inside the existing bottom menu viewport, replacing the visible legacy action stack rather than painting over it. Supply `bCanContinue` and `ContinueStatus` from the current real save-slot query, plus the imported transparent Main ornament texture. The widget actions remain the existing `StartGame`, `ContinueGame`, `OpenSaves`, `OpenSettings`, and `RequestQuit`; keep the real center camera/character region clear.

After the surface is mounted, let the host update `Rebuild_Main_ContinueGame` enabled state, `Rebuild_MainContinueStatus`, and `Rebuild_MainFeedback` through the same current save/action results used for the old buttons. Do not infer a slot or fabricate status text.

## HUD route

Call `InkPlayerVisibleEntryWidgets::BuildHudSurface(WidgetTree, this, HudCanvas, AnchorStatusWidget, ActivityStatus, RouteGlyphAtlas)` after the actual HUD Canvas and live status source are ready. `AnchorStatusWidget` is the existing authoritative presentation adapter. The source reads only its live name, realm, health, cultivation, and action state; the meters do not own gameplay values. The HUD glyph texture atlas cells are Character, Inventory, Cultivation, Leaderboard, Journey, and Menu in row-major order.

In the existing periodic `RefreshLiveStatus` path, refresh the live status source first and then call `RefreshHudStatus(WidgetTree, AnchorStatusWidget, ActivityStatus)`. Preserve the status adapter's update/tick path when replacing its previous visible painting; hide the old visible status presentation only after verifying its adapter still updates while hidden. Keep `HUDRuntimeMenu` as that exact widget name because the current `ToggleHUDMenu` handler targets it. Update the cultivation toggle label refresh to address `Rebuild_HudCultivationLabel`. Preserve the current five HUD routes (character, inventory, cultivation, leaderboard, menu), five existing shortcut routes, and saves/settings/return entries in the collapsible menu.

The composition uses proportional Canvas anchors: identity upper-left, shortcuts upper-right, action rail centered at bottom, and runtime menu above the rail. Integration must route it through the current adaptive safe-area handling and review portrait, landscape and stream bottom-inset cases before runtime acceptance. Avoid showing the legacy HUD navigation alongside these controls.

## Next slice

### Character and Inventory (player-b)

Stage source: `staged-source/Frontend/UIRebuild20261024/Player/InkPlayerCharacterInventoryWidgets.h/.cpp`.

Character composer inputs come from `GetPresentedCharacterName`, the live anchor realm, `GetPresentedCharacterAttributes`, and `GetPresentedCharacterDisciplines`. Preserve each source `Label`, `Value`, `Category`, `Name`, and `Detail`; do not synthesize attribute rows, grades, progress, or empty state. Provider state/message is passed explicitly. The route buttons use the existing action names `OpenEquipmentFabao`, `OpenCultivationMethods`, `OpenBreakthroughPrepare`, and `OpenCharacterAppearance`, mapped to the semantic contract routes EquipmentFabao, CultivationMethods, BreakthroughPrepare, and CharacterAppearance.

Inventory composer inputs come from `GetInventoryModel()->GetEntriesView()`, exact `EntryId`, actual `Item` fields, known capacity, provider filter/search/page state, and the host's action quantity. Provider state controls whether collection data is rendered. `InventorySelect_<SlotIndex>` is only the existing UInkFrontendWidget input bridge; the host resolves that slot to its stable `EntryId` immediately. The authority action adapter must send that stable ID and quantity, keep request/version/receipt identity, avoid optimistic mutation, and apply the contract's confirmation for `InventorySell`/semantic `inventory-recycle`. Buttons use the actual `InventoryUse`, `InventoryEquip`, `InventorySell`, `InventoryPrevious`, `InventoryNext`, `FilterAll`, `FilterEquipment`, `FilterMaterial`, `FilterConsumable`, and `FilterTreasure` handlers.

Search is passed as host state and locally filters only actual item name/item ID. The source has no editable-text callback to a new root yet; integration must bind its existing inventory search input state to `SearchQuery`. Likewise the builder receives the existing page index and selected ID as inputs; it does not shadow host selection or paging state.

Generated transparent atlases are documented in `visible-entry/assets/components/asset-manifest.json`. Import each to the agreed `/Game/newUI/Components/Player` path, then prove AssetRegistry/Texture2D references separately. Character and Inventory page composition remains staged until the single root selects the distinct `character` and `inventory` page IDs and adds the required portrait/landscape responsive host layout.
