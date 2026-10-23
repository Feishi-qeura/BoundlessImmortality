# Integration code review

**Latest code disposition (2026-10-23, third-pass source): all three original P2 code findings are addressed.** The reviewer re-read the actual current source, not only the implementation summary. The closure details at the end distinguish static confirmation from runtime evidence. Original findings below are retained for traceability.

Reviewed 2026-10-23 against `Saved/Codex/PortraitUI20261023/Backup/Source`. Read design, implementation plan, screen layouts and all three implementer reports. This review is static: no UE process, build, PIE or asset migration was run. Asset migration is in progress; old paths and uncompleted migration evidence are not findings here. Parent-known C4458 shadow errors are excluded.

## Findings

### P2 — Preserve the new appearance-options scroll when applying a preview

Location: `Source/BoundlessImmortality/Frontend/InkCharacterPanels.cpp:287-290`; triggers at lines 313-314.

Every `Frame_` / `Style_` click rebuilds the shell and constructs a fresh `AppearanceOptionsScroll`, without saving or restoring its offset. The shell only preserves `ResponsiveScroll`, which is a different outer scroll. At the accepted 960×720 and 1280×720 sizes, the options viewport is about 424 logical units but the bar controls begin at 524 and the progress preview at 736. Scroll down, select a bar style: the options jump to the first row and the changed preview disappears. A size change does the same. Persist this inner scroll offset (or retain/reflow the widget) before clearing the old shell, restoring it on rebuild. Verify selecting all four bar styles and changing orientation while viewing the lower previews.

### P2 — Keep the world-options position when changing route or encounter size

Location: `Source/BoundlessImmortality/Frontend/InkWulingPanel.cpp:50-53`; triggers at lines 122-123.

The new `WulingWorldScroll` likewise starts at zero on every rebuild. Its content is 1,040 units tall; the accepted landscape sizes provide only about 324 units for it. Both encounter presets (Y=388) and route selection (Y=488) require scrolling, and selecting either immediately reconstructs the scroll at the top. This makes repeated configuration and reaching the next controls unnecessarily jump away from the current action, and also loses position on repeated size changes. Capture and restore the world/roster offsets before shell teardown; preserve them separately from the shell scroll. Reproduce in landscape by scrolling to route selection and toggling it twice.

### P2 — Draw primary and skip actions still scroll with the result body

Location: `Source/BoundlessImmortality/Frontend/InkDrawPanel.cpp:36-40`, integration at `InkSignInShell.cpp:80-89`.

The approved design requires primary actions to remain visible and places Draw count/cost actions at the bottom. Single draw, ten draw and skip are still children of `BodyCanvas`; the shared shell footer extraction handles world/detail actions only. Scrolling to ten-draw results or the lower rules hides these controls while the shell retains a large footer containing only Back. On short portrait windows, the width-sized disc pushes them below the initial viewport as well. Give Draw its own fixed action strip (including the animation-dependent skip action) and let the wheel/rules/results consume the scrolling body. Verify both results and rules with the body scrolled to its end, including a resize during animation.

## Explicit review coverage

- **Small portrait and landscape:** accounted for the existing shortest-side DPI curve: 540×960 maps to approximately 720×1280 logical units; 1080×1920 does likewise, and 1080×2400 maps to 720×1600. 960×720 / 1280×720 remain logical 960×720 / 1280×720. Default feature body heights are approximately 984 / 1304 / 424 respectively. No additional deterministic overlap was found in the default Wuling tools/records footer arithmetic at these acceptance sizes. Wrapped authoritative text and image/material appearance still require actual PIE.
- **Repeated size changes:** the findings above cover newly introduced inner scrolls that are recreated without restoration. Tools/records have transient `UPROPERTY` scroll pointers and explicit offset restoration; no new dangling raw-pointer defect was identified there. Their offsets are read after the shared shell teardown, so the actual Slate retention should still be observed during runtime verification.
- **Fixed actions:** shell-owned sign-in claim and Back are fixed; world entry and generic detail participation are reparented into the footer. Wuling tools/records and character subpage management actions use their own body-height-relative action rows. Draw is the concrete remaining contract failure above. The shared extent calculation adds 16 units beyond body-bottom actions, so these pages also have a small redundant outer scroll range; this is a minor polish issue, not a separate blocking finding.
- **Preserved pages:** CharacterProfile implementation from the namespace through the end of its methods is unchanged; the only preceding diff is two added includes for redesigned subpages. `InkInventoryWidget.cpp`, `InkInventoryModel.cpp` and `InkLeaderboardWidget.cpp` are byte-identical to the backup. Its default shell header 104 + footer 144 equals the original 248-unit subtraction, and its dense frame width remains unchanged. Frontend Character/Leaderboard legacy geometry branches remain separate from the new Saves/Settings branches. This is source evidence only: compare actual CharacterProfile, Inventory, full Leaderboard and HUD-expanded leaderboard screenshots before claiming preservation.
- **Back stack and data:** `InkFrontendExtensions.cpp` is byte-identical to the backup. Existing route aliases and ancestor-return/retirement behavior remain. The added launch link uses the existing service-enabled route. Inspected sign-in, draw, world and authority command handlers preserve their existing economic semantics; no new grant, price, gate bypass or repeated command submission was identified. Catalog refresh still goes through the existing kind/offset check.
- **Generated WBP upgrade:** main secondary actions and settings groups upgrade existing named widget trees at runtime, while default-tree construction is retained. The guards prevent repeated secondary/settings creation. Save refresh reuses its named grid/switcher, clears their children and binds the reconstructed action tree. No definite upgrade failure was identified statically; test an existing serialized WBP as well as a freshly generated one.
- **Background/repetition:** responsive shell rebuild clears the old child tree before creating the replacement paper. Embedded content creates no full paper. No new duplicate-paper creation path was identified in the inspected code. Runtime instance counts and 20 open/close cycles remain required.

## Disposition

The three original P2 findings are addressed in the source closure below. Parent-owned runtime/Cook verification remains separate. This document does not approve the final visual result, migrated asset loads or Windows package.

## Third-pass code closure

Re-read `InkSignInShell.cpp`, `InkFeaturePanel.cpp`, `InkFeatureGroupPanel.cpp`, `InkRewardsPanels.cpp`, and the relevant appearance/world builders after the final native integration. No source was edited by the reviewer.

1. **Appearance inner-scroll reset — addressed in source.** `InkSignInShell.cpp:28-38` walks the old shell panel hierarchy and captures every named `UScrollBox` offset before clearing/renaming any descendants. `:93-94` restores matching offsets after `BuildPanelContent`. `AppearanceOptionsScroll` is a named descendant reached by that traversal, so the initial finding is corrected centrally without changing appearance actions. Actual offset retention across a preview click/orientation change remains a runtime-specific assertion.
2. **Wuling world-options reset — addressed in source.** The same capture/restore mechanism reaches `WulingWorldScroll` and `WulingRosterScroll`; their names are stable across reconstruction. The capture is before the old Slate subtree is detached, resolving the earlier timing concern. Switching to a different tab that removes a scroll entirely is not claimed to retain its old offset indefinitely; the original issue concerned rebuilds of the active page when changing a route/preset or size.
3. **Draw controls scrolling away — addressed in source.** `InkSignInShell.cpp:95-104` extracts the three existing Once/Ten/Skip buttons from `BodyCanvas`, retaining their instances, state and handlers, and places them in an 88-unit fixed shell footer beside Back. The previously inspected second-pass `draw-fixed.png` at 360×640 also showed all four footer targets visibly separated. Final-build screenshots remain the parent's runtime evidence.

The visual follow-up layering is also present in actual source: non-protected Scene Z=1, Action Z=2, AtlasIcon Z=3, Label Z=4; FortuneVerseSlip explicitly Z=2 above its scene. CharacterProfile retains its previous helper Z values, and its BodyWidth bypasses the new 16-unit inset. These static checks support the intended fixes but do not substitute for the final timed-card and verse images.

Parent reports the final native build and 93 automation checks passed. The reviewer did not launch/repeat them. No new actionable code defect was found in this narrow fix verification; final visual, asset and packaged checks retain their own evidence boundaries.

## Portrait HUD correction source review (final HUD integration)

Read the actual updated `InkHudLayout.cpp`, `InkAnchorStatusWidget.cpp`, `UI/InkUiResponsiveLayout.cpp`, HUD-only changes in `InkLeaderboardWidget.cpp`, correction diffs and `verify_preservation.py`. The correction is structural: `ResolveHud` now defines a 440×128 identity band, 104-unit dock, and portrait right-edge three-button rail. The 88-unit collapsed rank entry is placed below the last shortcut. The five dock actions and three shortcut action names remain the existing ones. The identity painter's two custom brush drawing paths now multiply `Brush.GetTint(Style)`; final visual confirmation awaits the rebuilt HUD capture.

The whole leaderboard source is no longer byte-identical and must not be described that way. The inspected exclusion is narrow: collapsed `HUDRankView` creation/content, its adaptive placement/serialized entry upgrade, and shortcut visibility while the existing expanded ranking overlay is active. The expanded scale placement, its rows/data/avatar paths and full leaderboard construction remain outside those exclusions. The current preservation report passes all five checks after asset-path normalization, including dedicated CharacterProfile implementation and Inventory source. Reading the correction diff supports that boundary; the updated validator does not merely omit the complete ranking file.

Repeated adaptive HUD layout repositions existing controls. `bRuntimeHudBuilt` prevents rebuilding the dock/identity/rail on each resize; the existing named ranking caption guards its serialized-content upgrade. No new stale-pointer or repeated-widget creation path was identified. The rail visibility follows the existing requested/fade state and restores when the expanded rank has fully faded out. The default logical portrait and landscape layouts leave the dock and collapsed rank entry separated; latest rendered small/wide output and opening/closing the rank overlay still require the parent-owned focused run.

No new actionable code defect found in this narrow HUD correction review. Parent reports native 94 checks passed (93 success plus one success-with-warning); reviewer did not launch or repeat the build/tests. The current full PIE images precede the brush-tint rebuild and are not final HUD color evidence.


## Packaged screenshot scheduling review

Read current `Wuling/WulingPackageAcceptance.cpp:122-153`. `Capture` at lines 52-54 uses `FScreenshotRequest::RequestScreenshot`, so capture requests are deferred to rendering rather than producing pixels synchronously. The revised state machine now keeps the screenshot-request tick free of page changes:

- Step 45 captures verse and returns; step 450 opens Activities two seconds later.
- Step 47 captures timed activities and returns; step 470 opens the launch page two seconds later.
- Inventory is opened in step 11, captured in step 39, then Leaderboard opens in step 40 and is captured in step 400.
- Final HUD is opened in step 13, captured in step 130 after three seconds, then Finish runs after another two seconds.

Each capture branch returns immediately and schedules a later tick. This addresses the identified same-frame page-transition race, including the added inventory/leaderboard/finalHUD coverage. The regular page loop already separated page opening (11) and capture (12). No production navigation, service command or gameplay behavior is changed by these state additions. The pending P3/L3 image runs, rather than the pre-fix screenshot names alone, must establish the corrected pixels. The reviewer independently read `Saved/Packages/PortraitUI20261023/BuildCookRun-capturefix.log`, which ends BUILD SUCCESSFUL and ExitCode=0; no build or game was launched by the reviewer.

## Announcement clearance source review

Read current `Frontend/UI/InkUiResponsiveLayout.cpp:30-35`, `LiveWeapons/LiveGMOverlay.cpp:297-316`, `LiveWeapons/LiveWeaponHUD.cpp:80-101`, and `Frontend/Tests/InkUiLayoutTests.cpp:64-88`.

`FInkHudLayoutRects::Announcement` supplies one logical rectangle to both render paths. In portrait its right edge is bounded 12 logical units before the shortcut rail, with a maximum width of 560 and a 16-unit gap below identity. Landscape places it below the identity/top-shortcut row; at the tested dimensions it is separate from the right collapsed rank entry and dock. Both painters normalize CR/LF to spaces and measure the rendered caption, removing complete UTF-16 surrogate pairs as needed before adding an ellipsis. The Slate path also clips the caption to its reserved text region.

The Slate overlay consumes `Geometry.GetLocalSize()` directly in logical units. The Canvas HUD divides its pixel canvas dimensions by viewport DPI scale before resolving the same layout, then multiplies position, extent, text scale and text inset once to draw in pixels. Width measurement uses the same font and scale as drawing. No double DPI scaling or new service/gameplay command was identified. Existing announcement events and lifetime remain unchanged. `AnnouncementClearance` checks non-intersection with identity, dock and every shortcut across six sizes, and checks the Canvas conversion for 0.75/1/1.5 scale. These are arithmetic/source tests; they do not replace the upcoming actual long-caption fixture screenshot.

No new actionable defect found in this narrow correction. Independently read `BuildCookRun-announcement.log`: BUILD SUCCESSFUL, AutomationTool ExitCode=0. The reviewer did not launch the build, automation or game. Final P4/L4 `hud-announcement-fixture.png` remains the rendered closure requirement.

## L4 blocker: Niagara UI lifetime review

Parent reports final landscape L4 crashed after leaving sign-in: `UNiagaraUIComponent::RequestDeactivateSystem` -> `UInkSignInChestWidget::Tick`. This supersedes any implication that P4 portrait success establishes final cross-orientation package acceptance. Reviewer did not run the failing program.

Read `NiagaraUIRenderer/Private/NiagaraSystemWidget.cpp` and header, `NiagaraUIActor.cpp`, `NiagaraUIComponent.cpp`, plus `InkSignInChestWidget` and `InkNiagaraTitleWidget`. The plugin already has UPROPERTY TObjectPtr actor/component fields and clears both during ReleaseSlateResources; these files predate this revision. Therefore an untracked raw pointer left uncleared is not an established root cause.

Concrete lifecycle gaps in the pre-fix code:

- Both host widgets inherit FTickableGameObject and their IsTickable checks only template/game-world state. NativeDestruct stops the effect but does not disable subsequent global ticks. Detached/deconstructed hosts can continue issuing StopGlow/StopEffect every tick until GC.
- Plugin GetNiagaraComponent and Activate/Deactivate/Update methods test only non-null, which does not exclude UObject destruction or an actor/component pending destruction before GC clears its tracked reference. Release destroys the actor before clearing the fields, leaving an avoidable reentrant-access interval.
- InitializeNiagaraUI assumes a usable Slate widget and actor and immediately dereferences the spawned actor. Reinitialization must fail closed when Slate was released or actor/world teardown has started.

Minimum correction recommended to parent: explicit Slate-ready lifetime flag on both hosts (NativeConstruct enables; NativeDestruct and ReleaseSlateResources disable before teardown), reject unreachable/begin-destroyed/world-teardown ticks, one validity-checked plugin component accessor used by every public operation, and clear actor/component/Slate references before destroying a valid actor. Initialize should require a valid Slate widget and live world/actor, checking spawn success before component creation. No change to economic or particle-style behavior is needed. These guards address the demonstrated lifetime exposure; the stack alone does not prove whether the original fault arose during GC, release reentry, actor destruction or Niagara's internal controller teardown.

Recommended executable regression: retain a live host UObject while releasing its Slate tree, assert non-tickable and null child component, drop retained Slate shared references, force full GC, then re-add/rebuild and confirm fresh valid components; repeat at least 20 times for both chest and title. Separately destroy the plugin actor while retaining the widget, exercise every public component operation before/after GC, and require null/no-op behavior. In packaged acceptance, cycle weekly/monthly/verse/activities and title-bearing pages with a full GC scheduled on a later step after each close, then reopen and render. Keep page mutation, GC and screenshot on separate frames, verify HUD/title rendering returns, and check retired Niagara actors/components do not accumulate. Arithmetic visibility tests alone do not exercise this failure mode.

## Implemented Niagara lifetime correction rereview

Read the actual post-fix plugin/host code and new `DestroyedComponentLifecycle` test. The plugin clears actor/component/Slate references before calling Super release and destroying the weakly retained valid actor. All component-facing public operations now use the same getter, which rejects invalid/unreachable/being-destroyed/begin-or-finish-destroyed components. Initialize guards the Slate pointer, world teardown, actor validity and spawn result. Both chest and title explicitly enable Slate-ready state in NativeConstruct and disable it before NativeDestruct/Release teardown; IsTickable and Tick reject unusable lifetime states. Stop now clears pending autoactivation and calls DeactivateImmediate only on a valid registered component instead of repeated layered Deactivate calls. Reconstructing a host re-enables its readiness. No new production-code blocker found in this limited rereview.

`DestroyedComponentLifecycle` uses a real Niagara UI component, invokes DestroyComponent, verifies the production getter rejects it, and exercises Activate/Deactivate/asset/tick-setting APIs without mutating or resurrecting the destroyed component. Repeated release and post-release API no-op checks are also present. Its reflected private-field setup is narrow and it does not pretend to test real Slate/GC painting.

Packaged steps 200–203 open monthly sign-in, switch to HUD (ShowPage removes/clears extension pages), force GC on a later step, and inspect survival on the next step for three repetitions. Capture steps remain isolated. One test-evidence limitation was reported to parent: the post-GC iterator may find zero released chest objects, making its non-tickable assertion vacuously true. A pre-close record of at least one constructed monthly chest, followed by a weak-reference check that it is collected or non-tickable, would establish the intended population was actually exercised. Surviving three real GC cycles is still useful runtime crash evidence; it is not equivalent to the proposed retained-host, released-Slate/rebuild test. Native compilation and package results were still pending at this rereview; no success is inferred solely from source.

### Lifecycle test population correction

Re-read packaged steps 201/203 after the final test refinement. Before closing, the harness now retains a weak reference to a real chest in the current world whose typed outer is `UInkMonthlySignInPanel`, with valid cached Slate and IsTickable=true, and requires that it exists. After GC it asserts that same weakly observed object is collected or non-tickable. The typed-outer selector matches monthly chest ownership (the widget tree belongs to the panel, not the frontend). This addresses the prior empty-iterator limitation. P5 runtime verification independently reports all three such cycles passed; L5 is still pending.
