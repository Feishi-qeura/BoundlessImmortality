# UI asset layout — 2026-10-23

The current `/Game/UI/Ink` assets are organized by type, then screen family:

`Textures/`, `Materials/`, `MaterialInstances/`, `Niagara/`, `Widgets/`, `Fonts/`, and `Data/`.

Use `ui_asset_paths.json` to resolve a historical package to its canonical package. Native dynamic UI loads use `InkUiAssets::Resolve`; `DefaultEngine.ini` retains exact legacy package aliases for saved references and downstream tools. Names of objects inside packages were preserved.

The dated folders in old asset-generator scripts describe historical inputs. Do not rerun those generators against the live project without routing their import/create destinations through `ui_asset_paths.json`; otherwise they can recreate the old folder structure. New resources belong in the typed folders above. Never move `.uasset` files with filesystem tools.

Migration evidence and backup: `Saved/Codex/PortraitUI20261023/`; the authoritative original-to-new manifest is `specs/boundless-ink-ui/revisions/2026-10-23-portrait-refactor/migration-manifest.json`. This preserves all 570 original UI resources, including the three protected panels.
