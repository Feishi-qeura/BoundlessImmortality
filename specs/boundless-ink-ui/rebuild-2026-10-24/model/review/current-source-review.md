# New UI source review, 2026-10-24

Read-only review for the sole production writer. Paths below are relative to `specs/boundless-ink-ui/rebuild-2026-10-24/` unless prefixed `Source/`. Sources are being edited concurrently: compare the hashes in `readiness.json` before applying a finding. No source patch, engine launch, UBT invocation, PIE, or runtime acceptance was performed here.

## Confirmed API/type issues in staged sources

These are source/API findings checked against the installed UE 5.8 public headers, not compiler diagnostics from a new build.

| ID | File and lines | Problem | Minimal correction |
|---|---|---|---|
| C01 | `panel-rank/staged-source/InkRankWulingPages.cpp:71`; `panel-activities/staged-source/RebuildActivityWidgets.cpp:86,500,577` | `UButton::OnClicked` is `FOnButtonClickedEvent`, a dynamic multicast delegate; it does not expose the native multicast `AddLambda` API. | Bind a `UFUNCTION` through `AddUniqueDynamic`, using a button subclass or a retained UObject relay for row payloads. Retain relay objects through reflected ownership and avoid raw dangling delegate pointers. |
| C02 | `panel-rank/staged-source/InkRankWulingPages.cpp:63` | `UButton` has no `SetPadding` API. The nearby `UBorder::SetPadding` call is valid and should remain. | Set padding on its `UButtonSlot` after adding content, or use the button style's content padding. |
| C03 | `panel-rank/staged-source/InkRankWulingPages.cpp:121,196,201,243,384` | `FSlateChildSize(rule, weight)` has no matching constructor. | Construct with the rule, then assign `Value` before passing to `SetSize`. Single-argument calls are valid. |
| C04 | `panel-rank/staged-source/InkRankWulingPages.cpp:37,66,118,145,195,199` | `FSlateFontInfo(NAME_None, size, FName("Regular"))` has no matching overload. The FName filename constructor takes `EFontHinting` as argument 3. | Use the shared theme font helper or a real UFont/composite-font overload with its typeface name. |
| C05 | `panel-journey/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.h:113-139`; corresponding `.cpp:57,62` | Two classes derive from `UButton` but override/call `NativeOnInitialized` and use `WidgetTree`, which belong to `UUserWidget`, not this inheritance chain. | Use a composed UUserWidget with a real widget tree, or implement a UButton lifecycle/owned-content construction path appropriate to that class. Keep typed payload callbacks. |
| C06 | Journey `.cpp:45` | `*FString(Name)+TEXT("Label")` adds two TCHAR pointers after dereferencing the string. | Form the FString first: `*(FString(Name)+TEXT("Label"))`, consumed within the call. |
| C07 | `panel-activities/staged-source/RebuildActivityWidgets.cpp:159` | `UProgressBar` has no `SetBackgroundColor` setter. | Set the background brush through `FProgressBarStyle` and `SetWidgetStyle`; retain `SetFillColorAndOpacity` for fill. |
| C08 | Activities `.cpp:133,149`, helper `.cpp:74` | ActivityRow receives a const delegate reference but passes it to a helper requiring a nonconst reference. | Correct the callback ownership/interface together with C01; do not const_cast a borrowed delegate. |

Engine evidence: `F:/unreal/UE_5.8/UE_5.8/Engine/Source/Runtime/UMG/Public/Components/Button.h:18,76`, `SlateWrapperTypes.h:167-173`, `ProgressBar.h:75,114`; `Runtime/SlateCore/Public/Fonts/SlateFontInfo.h:227-276`. Journey lifecycle ownership was checked against the UButton/UContentWidget/UWidget and UUserWidget declarations.

## Confirmed content tint issue

**V01 — Activities `.cpp:85,149`:** `SetColorAndOpacity` is the button **content multiplier**. ActivityRow passes Transparent, so after the compile issues are repaired its card/text content receives zero alpha. Keep content tint white and apply background styling to the button brushes or child card. This conclusion follows the API semantics; no rendered screenshot was produced.

## Lifecycle risks requiring integration verification

- **L01 — Activities `.cpp:184-190,269,272-292,420,423-429,586`:** initial construction occurs in NativeConstruct, and ApplyViewModel rebuilds/replaces `WidgetTree->RootWidget`. Slate can already hold the earlier root. Build once before Slate in NativeOnInitialized/RebuildWidget, then update existing controls or children of stable containers. Verify first open, repeated refresh, scrolling and confirmation under real Slate; source inspection alone does not establish the visible result.
- **L02 — Rank `.cpp:329-335,435-440`:** Wuling overview/members refresh clears Content and calls BuildPage, which replaces the widget root. Keep a stable root and rebuild only its content subtree, preserving selection and paging state. Test refresh while displayed.
- **L03 — Rank `.cpp:230-256`:** board rows and avatar images are rebuilt wholesale. Integration must preserve the existing participant/avatar request mapping or supply a new persistent participant-keyed image map. Do not assume old `RankAnchorAvatar_<rank>` lookups resolve these unnamed new images. Confirm delayed avatar completion after refresh and page return.

## Production Root snapshot and positive checks

`Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp` now exists. BuildDefaultTree (49-85) creates the new independent overlay/safe root/switcher; it does not call the legacy tree builder. MountPageComponent (88-94) retains mounted components, and Saves/Settings use content keys (145-165). The base class `InkFrontendWidget.cpp:383-400` recursively collects nested UUserWidget action buttons, with a Seen set. These address the previous nested binding concern in source shape; behavior remains untested.

At this snapshot `Source/BoundlessImmortality/Frontend/InkMenuDirector.cpp:34` still defaults to `UInkFrontendWidget::StaticClass()`. Character/Inventory/Leaderboard in the new root still carry the explicit pending owner-page placeholder/version. These are **in-progress integration checks**, not a report that a completed rollout failed. Before acceptance, prove that the actual default WBP/native fallback selects the new root, every owner page is mounted, routes bind to the new tree, and no old visible fallback remains.

Root styles currently load migrated `/Game/newUI/Ink/Fonts/Shared/...`; Journey loads `/Game/newUI/Shared/Fonts/...`. Both use the authorized new root, but integration/cleanup must choose an actually existing canonical font mapping before import/runtime verification. Do not remove migrated fonts based only on the suggested folder layout.

## Action and authority acceptance to retain

The staged pages expose presentation requests; their existence does not prove host binding or server idempotency. Verify final host handlers with fresh state, Busy, exact selection/user id and captured confirmation subject/date. Same-subject refresh must preserve pending/confirmation; changed subjects must invalidate it. Launch's confirmation currently uses the refreshed ViewModel day at emission: host binding must ensure the confirmed event/day remains the requested event/day.

Unknown display fields must remain unknown and must not disable a native legal action by themselves. In particular, do not restore the superseded Quote Complete gate requiring display-only normalized values, ranges, slots or source descriptions. Keep real selection/kind/stage, explicit business readiness false, stale version, inventory, safety and Busy checks. Viewer assistance must remain actor-verified through its real route. Existing review/test results are historical evidence and are not new-tree runtime validation.

## Handoff boundary

Contracts remain 44 screens / 36 components / 25 model mappings / 76 actions, with the previous current-run contract validation result of 30 checks passed and owned GameUI snapshot strict validation at 0 errors / 0 warnings. Four optional raster assets remain pending generation. The eight API/type groups, V01 and lifecycle/integration checks above require owner remediation and then current-source build/Slate evidence. No production files were changed by this review.
