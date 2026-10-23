# Packaged Niagara lifecycle regression

The final landscape P4 run reached all feature pages but crashed after returning to HUD. The stack was `UNiagaraUIComponent::RequestDeactivateSystem` → `UInkSignInChestWidget::Tick`; its report was not completed. P4 therefore does not establish release readiness.

The plugin already used reflected TObjectPtr references and cleared them on Slate release. The evidence does not prove a raw-pointer ownership defect. Two verified lifetime gaps were corrected: chest/title FTickableGameObject instances remained tickable after Slate destruction, and plugin methods checked only non-null component references, allowing already-destroying components to escape.

Chest and title hosts now gate tick on native Slate construction, clear that state before destruct/release, and exclude unreachable, destroying and tearing-down objects. Stop functions cancel pending autoactivation and immediately stop only valid registered components. The plugin exposes only a valid non-destroying component; release invalidates references before destroying its actor, and initialization checks Slate/world/actor availability.

The native `DestroyedComponentLifecycle` test uses an actual UObject component and DestroyComponent, verifies that the getter rejects it and public activation/deactivation/update calls do not mutate it, then verifies repeated release. It passed with 95 successes, one existing unrelated warning, zero failures.

Final packaged verification uses three reopen-monthly/close/force-GC rounds in each orientation. It observes a real constructed chest before each close and requires that same weak reference to be collected or no longer tickable afterward. GC is requested in a separate frame; the next step checks survival. Final P5/L5 runtime results are required for delivery.
