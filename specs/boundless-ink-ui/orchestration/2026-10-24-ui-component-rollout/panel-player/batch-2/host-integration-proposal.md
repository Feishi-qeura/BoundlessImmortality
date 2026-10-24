# Host integration proposal (not applied)

This proposal assumes the first compile has ended, `Source` is released by its owner, and the current `InkFrontendWidget.cpp` is re-read. It should be reconciled against the then-current file; no source mutation is authorized in this batch.

## Include

```cpp
#include "Frontend/UiRollout20261024/Player/InkPlayerMenuPageWidgets.h"
```

Promote the staged pair into `Source/BoundlessImmortality/Frontend/UiRollout20261024/Player/` first.

## Saves refresh body

Keep the existing page frame and its `SaveSlotList`. Replace only the list contents after retrieving the authoritative snapshot:

```cpp
void UInkFrontendWidget::RefreshSaves()
{
    auto* List = InkFrontend::Find<UVerticalBox>(this, TEXT("SaveSlotList"));
    if (!List || !WidgetTree) return;
    List->ClearChildren();

    auto* Save = InkFrontend::Saves(this);
    TArray<FInkSaveSlotInfo> Slots;
    if (Save) Slots = Save->GetSlots();
    else for (int32 I = 0; I < UInkSaveSubsystem::ManualSlotCount; ++I)
    {
        FInkSaveSlotInfo Info;
        Info.SlotIndex = I;
        Slots.Add(Info);
    }

    const bool bCanSave = Save && Director && !Director->bMenuActive && GetOwningPlayerPawn();
    const FText SaveDisabledReason = !Save
        ? LOCTEXT("SaveUnavailable", "本地存档尚未就绪。")
        : !bCanSave ? LOCTEXT("SaveGameplayOnly", "进入游戏后可保存当前行程。")
        : FText::GetEmpty();
    List->AddChildToVerticalBox(
        InkPlayerMenuPageWidgets::BuildSavesBody(WidgetTree, this, Slots, bCanSave, SaveDisabledReason));
}
```

The existing `ExecuteAction()` save/load/delete branches remain unchanged. They keep index validation, overwrite/delete confirmation, actual sync calls, `LastError` and refresh. The page frame remains responsible for the route title, Back, feedback and scrolling.

## Settings refresh composition

In the current `RefreshSettings()`, keep its live-value reads and post-build style/value updates. Replace only the original `if (!GetWidgetFromName("SettingsPresentationGrid"))` body-construction block with a named wrapper so later refreshes do not rebuild the old controls:

```cpp
if (!GetWidgetFromName(TEXT("SettingsPresentationGrid")))
{
    auto* Scroll = InkFrontend::Find<UScrollBox>(this, TEXT("SettingsScroll"));
    auto* Body = Scroll ? Cast<UVerticalBox>(Scroll->GetChildAt(0)) : nullptr;
    if (Body)
    {
        Body->ClearChildren();
        auto* Grid = WidgetTree->ConstructWidget<UGridPanel>(
            UGridPanel::StaticClass(), TEXT("SettingsPresentationGrid"));
        Body->AddChildToVerticalBox(Grid);
        Grid->AddChildToGrid(InkPlayerMenuPageWidgets::BuildSettingsBody(
            WidgetTree, this, CurrentQuality, EffectiveFrameLimit, bReducedMotion), 0, 0);
    }
}
```

The option buttons carry the existing action IDs in `UInkFrontendActionButton::Action` and bind through the host. Update the current post-build style pass lookups to the prefixed widget names listed below so it selects supported options; unsupported/custom quality or frame caps leave all corresponding choices unselected. Keep the existing Back control in the page frame and keep `ExecuteAction()` settings branches unchanged.

Because the original detached WBP controls retain their names in `WidgetTree`, the new buttons use `Batch2_<ActionId>` object names while their `Action` property remains the exact existing action ID. Update the `RefreshSettings()` selection/style lookup names to `Batch2_QualityLow`, `Batch2_QualityMedium`, `Batch2_QualityHigh`, `Batch2_Frame30`, `Batch2_Frame60`, `Batch2_ToggleReducedMotion`, and `Batch2_ApplySettings`. Update the visible value lookups to `Batch2_QualityValue` and `Batch2_FrameValue` too; the helper creates those as actual visible TextBlocks. Preserve the existing format logic for current values (including custom quality, any positive frame cap, and unlimited). Keep the selected-state and value text sourced from the same live host values; the motion action caption is static and the selected style shows the current toggle state.

## Integration review checklist

- Confirm no other change has altered the `SaveSlotList`, `SettingsScroll`, or page-frame names.
- Check that a missing save subsystem displays three unavailable/empty records without enabling destructive actions.
- Confirm that the selected-slot legacy path is not visible in the new body, while existing save/load/delete actions still refresh it.
- Confirm the settings helper's named buttons are found by the existing style pass and that the original Back remains in the frame.
- Verify portrait and landscape layout, focus, touch targets, disabled states, confirmation ownership, and feedback after the next bounded compile.

This is a reviewable integration suggestion, not an applied or compile-checked patch.
