# Batch 2 integration map

| Page | Snapshot source | UI state / actions | Business owner | Staged composition helper |
|---|---|---|---|---|
| Saves | `UInkSaveSubsystem::GetSlots()` and `FInkSaveSlotInfo`; exactly 3 manual slots | Render actual slot metadata; enable Load only for `bValid`, Delete only for `bExists`, Save only when host passes `bCanSave`; host passes the real disabled reason; actions `LoadSlot_i`, `SaveSlot_i`, `DeleteSlot_i` | `UInkFrontendWidget::ExecuteAction`; overwrite/delete retain the existing confirmation layer; `SaveSlot`, `LoadSlot`, `DeleteSlot`, refresh and `LastError` stay authoritative | `InkPlayerMenuPageWidgets::BuildSavesBody(Tree, Frontend, Slots, bCanSave, SaveDisabledReason)` |
| Settings | `UGameUserSettings` overall quality, exact effective frame-rate limit, and host `bReducedMotion` | `QualityLow/Medium/High`, `Frame30/60`, `ToggleReducedMotion`, `ApplySettings`; visible current-value labels preserve custom quality, any positive FPS cap, and unlimited state | Existing host setters, `ApplyNonResolutionSettings`, `SaveSettings`, feedback and refresh | `InkPlayerMenuPageWidgets::BuildSettingsBody(Tree, Frontend, Quality, EffectiveFrameRateLimit, bReducedMotion)` |

## Behavior and constraints

- The save body displays current `FInkSaveSlotInfo` values only. Invalid reasons come from `ValidationError`; no synthetic error/status is added. The current actions are synchronous, so there is no fabricated pending state.
- The existing subsystem's three local slots remain the only save type. No cloud/account sync or extra fields are shown.
- Settings choices remain exactly the host's supported quality levels 0/1/2, frame caps 30/60, and reduced-motion toggle. Quality/frame actions currently persist immediately; the Apply button calls the existing apply/save route.
- The helper buttons bind through `UInkFrontendActionButton::BindFrontend`, then call `TriggerAction`. They do not call subsystem or settings APIs directly.
- `SelectSave_i` remains supported in the frozen host for its original details switcher. The new staged card body does not emit it.

## Component roster

- `InkPlayerMenuPageWidgets.h/.cpp`: C++ composition namespace producing save-slot cards and settings option groups from supplied runtime values.
- The host owns route frame, title, Back, feedback, action validation, confirmation, sync calls and refresh.
- PNGs are layout concepts only. Runtime text and values must remain `UTextBlock` content; the PNGs must not be mounted as page backgrounds.

## Promotion order

1. Review `staged-source/Frontend/UiRollout20261024/Player/InkPlayerMenuPageWidgets.h/.cpp` while `Source` remains frozen.
2. When the compile lease has ended and the source owner releases the freeze, promote the two staged files to `Source/BoundlessImmortality/Frontend/UiRollout20261024/Player/`.
3. Reconcile `host-integration-proposal.md` against the then-current `InkFrontendWidget.cpp`; integrate only in `RefreshSaves()` and `RefreshSettings()` with the shared owner.
4. Compile under the next bounded lease. Then verify both routes at portrait and landscape sizes and exercise the exact existing action IDs. No import is proposed for these full-page concept PNGs.

## Evidence boundary

No source was promoted, imported, compiled, or mounted. The C++ files are staged proposals, not compile-verified code. The candidate PNGs are verified as readable 941×1672 RGB PNGs and remain `approved:false`; no runtime or visual approval is claimed.
