# C05 / C06 source correction record

Source review: `specs/boundless-ink-ui/rebuild-2026-10-24/model/review/current-source-review.md`.

## C05 — UButton lifecycle and WidgetTree

In the immutable prior stage, `UInkJourneyMapPinButton` and `UInkJourneyMilestoneButton` inherited `UButton`, although the implementation used `NativeOnInitialized` and `WidgetTree`. Review-v2 changes both to `UUserWidget`; each owns a `UPROPERTY` child `UButton` and text controls built once as the widget-tree root in `NativeOnInitialized`. A new `UInkJourneyRaceActionButton` follows the same composition pattern. Payload-bearing clicks bind through reflected `UFUNCTION`s with `AddUniqueDynamic` and broadcast typed IDs/kinds/milestones.

## C06 — temporary FString label name

`InkJourneyNew::Button` now passes `*(FString(Name) + TEXT("Label"))` to the helper. This evaluates the string concatenation before taking its character pointer; the temporary survives through the full helper-call expression. It replaces the invalid `*FString(Name)+TEXT("Label")` pointer addition.

## Evidence boundary

The fixes are present only under `panel-journey/review-v2/staged-source/`. The originally handed-off staged files and production Source were not overwritten. This is static source correction, not compiler or UHT evidence. Integration still must reconcile/copy the corrected revision and run the approved engine verification.
