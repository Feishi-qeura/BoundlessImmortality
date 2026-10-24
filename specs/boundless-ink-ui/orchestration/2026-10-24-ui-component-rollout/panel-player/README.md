# Player UI rollout package

This team package covers the complete Player group: main menu, HUD, character profile, inventory, breakthrough preparation, and tribulation live/result states. Existing native pages and the approved home-component set are reused where documented. Source candidate pages remain `approved:false` and are layout references only.

The live Player panel routes remain owned by existing code. The new source atom is `UInkPlayerMaterialSlotWidget` in `Source/BoundlessImmortality/Frontend/UiRollout20261024/Player/InkPlayerMaterialSlotWidget.h`; it accepts values from an authority-backed adapter and emits a selection request. It does not execute player actions.

## Build the review package

```powershell
$gameUiSkillRoot = 'C:\Users\zxxuh\.codex\skills\gameui'
$gameUiPython = Join-Path $gameUiSkillRoot '.venv\Scripts\python.exe'
& $gameUiPython 'specs\boundless-ink-ui\orchestration\2026-10-24-ui-component-rollout\panel-player\build_player_component_package.py' --project-root 'D:\demo\BoundlessImmortality'
```

Outputs are written only into this team directory: `asset-manifest.json`, `player-components-candidate.zip`, and `pixel-validation.json`. The copied components preserve their source-approved status and hashes; candidate page visuals remain unapproved.

## Unreal import handoff

`import_player_components_unreal.py` is intended for the single integration owner inside Unreal Editor Python. It targets `/Game/UI/Ink/ComponentRollout20261024/Player`. It has not been run; this package contains no imported `.uasset` and does not claim route integration or runtime acceptance.

## Candidate art

`prompts/components/breakthrough-material-slot-frame-default-v1.md` is the saved prompt for a new transparent frame. The image tool returned inline data but the local export failed because the Windows process command-length limit rejected the payload. The frame is therefore `pending-export`; it is not included in the ZIP and does not affect the usable existing components.
