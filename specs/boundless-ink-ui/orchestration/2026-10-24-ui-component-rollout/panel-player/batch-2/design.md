# Batch 2 design: saves and settings shells

## Intent

Deliver isolated, composable UMG shells for the two uncovered Player routes `Saves` and `Settings`. The shells separate layout and live text from operations. They consume the current host APIs and emit the host's existing action IDs, so local save logic and `UGameUserSettings` remain authoritative.

## Goals

- Saves: show the three actual manual slots with real existence/validity, saved time, map, elapsed play time, and available action state; expose read/load/save/delete via the existing `UInkFrontendActionButton` action pipeline.
- Use the host's existing confirmation layer for overwrite and delete. Show disabled, pending, unavailable and error states only when the actual adapter supplies them.
- Settings: show only capabilities present in the current host—overall quality levels 0/1/2, frame caps 30/60, and reduced motion—and the existing Apply and Back routes.
- Build concrete C++ widget trees and candidate page concepts under the isolated batch directory. Candidate art stays `approved:false`.
- Preserve main, home, character and inventory without redesign. Keep the remaining page backlog explicit and out of scope.

## Non-goals and anti-goals

- No sound, key-binding, text scale, resolution, cloud-save, account, game-mode, profile, or new save fields are added.
- No made-up save names, timestamps, maps, quality choices, errors, network state, or success results.
- No changes to existing source or Content during the active compile. Integration receives staged source and a small host-patch proposal only.
- No new confirmation business logic: the host already confirms overwrite/delete through `ShowConfirmation` and `ConfirmPending`/`CancelPending`.

## Approach

Use a small C++ composition helper that builds a scrollable body from actual snapshots, with the existing `UInkFrontendActionButton` for action routing. The helper is staged only; a short host integration proposal will replace children inside the current Saves/Settings page bodies after the engine lease ends. This keeps page routing, save operations, confirmation and settings persistence in `UInkFrontendWidget` and its existing subsystems.

The user explicitly authorized this remaining-page batch and provided the page boundaries. This batch follows the approved screen contract; visual approval remains pending.

## Saves behavior

`UInkSaveSubsystem::ManualSlotCount` is three. `GetSlots()` supplies `FInkSaveSlotInfo`; `bExists`, `bValid`, `SavedAt`, `MapName`, `PlayedSeconds`, and `ValidationError` are displayed without deriving other facts. Load is enabled only for valid slots, Delete only for existing slots, and Save only when the host passes the exact existing gameplay-save permission. Existing action IDs `LoadSlot_<index>`, `SaveSlot_<index>`, and `DeleteSlot_<index>` invoke the host, which owns confirmation, operation, and refresh. The host retains `SelectSave_<index>` for its current detail-switcher tree; the staged body does not depend on selected-slot state.

Current storage methods are synchronous. A pending display can be set only by an adapter that observes a real in-flight operation. Failed or unavailable text comes from the real subsystem/host. The shell does not simulate a spinner or fake a save receipt.

## Settings behavior

`UGameUserSettings::GetGameUserSettings()` is the source for quality and frame cap. The existing `UInkFrontendWidget::bReducedMotion` value is passed by the host. Action IDs are the existing `QualityLow/Medium/High`, `Frame30/60`, `ToggleReducedMotion`, `ApplySettings`, and `Back`. The current code applies and saves quality/frame choices immediately; the Apply button repeats the existing apply/save action. There is no unsaved dirty state in the current implementation. `ApplyNonResolutionSettings()` and `SaveSettings()` are void APIs; no component-level rollback is claimed without an actual host failure result.

## Layout decisions

- Review canvas 720×1280 with reflow to 1280×720. The staged helper supplies independently scrollable cards and grouped options; final fixed header/footer and landscape reflow remain owned by the host page frame and require verification at integration.
- Minimum 88×88 logical touch target for action buttons; focus style remains visible.
- Dynamic labels, dates, map names, counts and state text are UMG TextBlocks. Concept images contain no text and are not runtime page backgrounds.
- Inherit approved style contract tokens: ink `#202823`, paper `#F3EBDD`, light paper `#FBF7EE`, jade `#3A8D83`, antique gold `#B88B45`, warning cinnabar `#AA4937`; restrained 1px/2px linework and low contrast paper cards.

## External layout research

- Apple App Store listing for [少年西行记-文字修仙](https://apps.apple.com/cn/app/%E5%B0%91%E5%B9%B4%E8%A5%BF%E8%A1%8C%E8%AE%B0-%E6%96%87%E5%AD%97%E4%BF%AE%E4%BB%99/id1596074348?platform=watch) describes portrait single-hand play. It informs reachable action placement only.
- Apple App Store listing for [修真江湖2](https://apps.apple.com/cn/app/%E4%BF%AE%E7%9C%9F%E6%B1%9F%E6%B9%962-%E5%9B%9B%E5%B9%B4%E4%BF%AE%E4%BB%99%E6%A2%A6-%E4%B8%89%E5%91%A8%E5%B9%B4/id1662433851) identifies an ink-wash cultivation RPG. It informs genre context only.

These listings were not copied or used as image-generation inputs, and neither provides a saves/settings screen specification. Page structure and business states come from this project's approved interaction and screen contracts and current source.

## Decisions left for integration

- In `RefreshSaves()`, retain the existing `SaveSlotList`; after fetching slots and calculating `bCanSave`, clear that container and add `InkPlayerMenuPageWidgets::BuildSavesBody(WidgetTree, this, Slots, bCanSave)`. Keep the host confirmation and action handlers intact.
- In `RefreshSettings()`, replace only the scroll-body contents with `InkPlayerMenuPageWidgets::BuildSettingsBody(...)`, passing current quality, effective frame cap and `bReducedMotion`; preserve the page frame, existing Back action and host settings handlers.
- Keep all state/pending/error copy sourced from host APIs. If the existing synchronous operations remain synchronous, the pending state may remain unused.
- Do not mount concept PNGs as production backgrounds. Read component PNGs into real texture assets only after the bounded engine lease.
