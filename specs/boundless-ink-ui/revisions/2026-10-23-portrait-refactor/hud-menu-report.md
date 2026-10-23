# HUD / main menu / settings / saves implementation

Implementation complete in the delegated source scope; native build and runtime verification are pending parent integration.

## Files changed
- `Source/BoundlessImmortality/Frontend/InkHudLayout.cpp`
- `Source/BoundlessImmortality/Frontend/InkFrontendWidget.cpp`
- `Source/BoundlessImmortality/Frontend/InkAnchorStatusWidget.cpp`

The existing hash backup is under `Saved/Codex/PortraitUI20261023/Backup/Source/BoundlessImmortality/Frontend`. Per-file original/current SHA256 ledger: `Saved/Codex/PortraitUI20261023/hud-menu-ledger.json`. Review diffs: `Saved/Codex/PortraitUI20261023/HudDiffs/`.

## Implemented behavior
- Inspected `home-v4-review.png` as a layout reference. No world scene, camera, character, actor, map, or concept screenshot asset replacement.
- HUD retains the original five actions (character, inventory, cultivation center, rank, menu), three upper shortcuts, identity click and right ranking entry. Shallow feathered dark ink now supports the navigation; the light navigation fade is removed and the existing bottom cloud receives a dark decorative tint. Labels use pale gold with an ink outline. The existing image art remains loaded from its original paths until parent migration.
- Navigation icon bounds are fitted to actual column widths, avoiding fixed-width children spilling across neighboring hit areas. Preview caption is positioned immediately above the bottom navigation and still only exists under the existing local-test flag.
- Identity ink is limited to the name/meter area and the realm retains its separate plaque. Shield inactive state displays text without an empty bar. No anchor snapshot displays `主播状态 · 待同步` rather than invented numbers. Existing live health, cultivation, avatar, progression, animation and damage handling remain unchanged.
- Main menu reuses existing generic ink button artwork with live text, places the start/continue primary actions at the bottom in portrait, and reparents settings/exit into a secondary row. Wide layout places the menu on the right with title on the left. Continue still opens save selection, and its real empty/available storage status is shown. Main layout does not issue filesystem queries every tick and no longer resets scrolling every tick.
- Saves has three selectable slots and a selected detail view. Portrait stacks list and details; wide uses two columns. Timestamp/playtime come from the original save metadata. Load/save/overwrite/delete enable conditions and action handlers are unchanged; delete is separated by extra space and danger text.
- Settings reparents the original quality/framerate controls into named groups, adds a motion group, and presents category guidance on the left in wide layouts. Existing immediate apply/save behavior remains; the fixed apply action can reapply/save the current settings. The original reduced-motion action remains unchanged.
- New settings/saves layout uses a single device SafeZone, a 24-unit content margin, full-width controls with an 88-unit minimum, a scrolling body and fixed footer. It reflows existing widgets without resetting selection or scroll on size changes. New main/settings/saves landscape decisions use `InkUiLayout::IsLandscape`; protected legacy branches remain unchanged.
- Runtime upgrades are applied to existing serialized WBP trees as well as freshly generated trees. The default tree itself was deliberately preserved.

## Honest data boundaries
The save model contains timestamp, map, pawn position/orientation and elapsed gameplay, but no realm snapshot. No realm is fabricated in the save details. There is no existing independent audio volume action or setting, so the settings page explicitly says that independent audio controls are unavailable in the current version. A project version string was not invented. These limitations remain visible instead of presenting inert controls.

## Verification completed
Ran `F:/UNREAL/UE_5.8/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe Saved/Codex/PortraitUI20261023/verify_hud_menu_static.py`.

Result: **40/40 static checks pass**. Evidence: `Saved/Codex/PortraitUI20261023/hud-menu-static-checks.json`.

The checks compare all non-owned frontend functions with the backup, confirm the entire existing action-handler block is unchanged after removing the two additive UI actions, preserve existing asset-path literals, preserve default tree construction and legacy breakpoints, and verify the new pending-state/navigation/breakpoint hooks. These are source-preservation checks, not a native compilation or UMG runtime test. The asset-path comparison is intended to run before parent migration.

## Parent integration checks still required
- Native Editor build and automation; no native build or UE launch was performed by this agent.
- PIE/runtime screenshots at 540x960, 720x1280, 1080x1920, 1080x2400, 960x720 and 1280x720; verify labels, single-line primary actions, icon fitting, pending/live identity and preview badge.
- Check saved WBP upgrade, settings scrolling/apply/reduced motion, save selection/overwrite confirmation/delete confirmation/read error, main scroll on resizing, and selection retention after orientation change.
- Verify protected Character, Inventory and Leaderboard live geometry and actions after shared integration; this change does not claim screenshot-level preservation.
- Resource migration, cooking and Windows ZIP packaging remain parent-owned.


## Latest correction: portrait-first HUD, 2026-10-23

The user explicitly corrected the earlier result: “主HUD你还没有修改，目前来说以竖屏为主”. The actual `PIE/hud-portrait.png` was inspected. The earlier output still used a tall identity card, horizontal top shortcuts, a wide middle ranking plaque and oversized bottom icons. The earlier styling-only changes are superseded by this structural HUD revision.

Decision record: `hud-portrait-correction.md` in this revision directory.

Implemented:
- Identity reference changed from 368×184 to 440×128. Avatar/realm occupy the left; live name, health and cultivation occupy the right. Health/cultivation values now sit beside thin meters instead of over the ornate bar frames. Shield remains the existing real value/inactive text.
- Portrait activities/sign-in/journey now form a right-edge vertical rail, three 88×88 targets with eight-unit gaps. Landscape retains the top horizontal arrangement. This rail stays outside the central x=.22W..78W character corridor for the requested portrait sizes.
- Bottom dock is now 104 high, with a stable translucent dark ink backing and a fine gold edge. The former bottom cloud is collapsed. Regular icons are approximately 48 and cultivation 64; cultivation is distinguished by its local green/ink fill and gold outline. The existing five actions and cultivation text update remain unchanged.
- The collapsed ranking entrance changes from the wide paper plaque to an 88×88 dark tag below the rail. Its original action remains `HUDRankView`. The rail yields its space while the existing expanded rank overlay is visible and restores after fade-out. The expanded rows, geometry, header actions, data, ordering and fade state logic are unchanged.
- Existing serialized HUD rank-entry content is upgraded in place; the full leaderboard widget is not regenerated just to change this entry.

New/updated files for this correction: `InkHudLayout.cpp`, `InkAnchorStatusWidget.cpp/.h`, `UI/InkUiResponsiveLayout.cpp` (ResolveHud only), `Tests/InkUiLayoutTests.cpp`, `Tests/InkAnchorStatusTests.cpp`, and HUD-only portions of `InkLeaderboardWidget.cpp`.

Exact leaderboard scope: `RebuildLeaderboardSurfaces` from creation of `HUDRankView` through the entry slot creation, plus `UpdateGameplayLeaderboard`'s collapsed-entry placement/serialized-content upgrade and shortcut-rail visibility. Full/expanded panel construction before the entry and the subsequent row/data/avatar functions were compared exactly against the correction backup and pass. Do not claim the entire LeaderboardWidget source remains byte-identical.

Correction backup, source diffs and SHA256 ledger: `Saved/Codex/PortraitUI20261023/HudPortraitCorrection/`. Static preservation and layout-contract checks: **28/28 pass**, evidence `static-checks.json`. Native automation coverage now checks the vertical rail, compact dock, protected central corridor, identity dimensions and retained avatar/frame alignment. These native tests are written but have not yet been executed by this agent.

Parent must build and capture fresh actual portrait/wide output. Earlier screenshots are explicitly the rejected baseline, not evidence for this new revision. No game scene/camera/character edits or non-HUD page edits were made in this correction.
