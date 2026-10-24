# Panel Player batch 2 handoff

## Delivered in the isolated batch directory

- Saves and Settings contracts, integration map, promotion order and `host-integration-proposal.md`.
- Prompted page concepts at `assets/pages/saves-v1.png` and `assets/pages/settings-v1.png`; both are RGB PNGs, 941×1672, and remain `approved:false`.
- Composable C++ body builders under `staged-source/Frontend/UiRollout20261024/Player/`. These use real values passed by the host and dispatch current action IDs through `UInkFrontendActionButton`.
- `coverage-matrix.md` records the current scope and open backlog.

## Required next steps after the active compile lease

1. Have the source owner release the freeze and reconcile the proposal with the then-current `InkFrontendWidget.cpp`.
2. Promote only the two staged C++ files, and integrate only the Saves/Settings refresh methods.
3. Compile under a new bounded lease; after a successful compile, verify portrait and landscape reflow, save slot states and confirmation, settings selection/persistence, focus, touch sizes and Back behavior.
4. Ask the user to review visual candidates before any visual approval or page-art promotion. Do not import these full-page concepts as runtime backgrounds.

## Evidence and blockers

- Candidate PNG files were inspected visually and checked for PNG dimensions, RGB color type, file size and SHA-256.
- No source outside `batch-2/staged-source/` was changed. No asset import, source promotion, compile, editor interaction, PIE, package or device run occurred.
- `source_ready` and `assets_ready` are true for this isolated handoff. `import_verified`, `compile_verified`, and `runtime_verified` are false. Handoff is ready for source-owner review but is blocked from integration while the first-batch compile/freeze remains active.
- Visual approval is pending the user. The existing `screen-coverage.json` remains unchanged; its integration evidence must be updated by the source owner after promotion and verification.

## Preserved backlog

Methods, equipment, appearance, crafting, item sources, breakthrough and tribulation remain uncovered. Main, home, character and inventory were not redesigned or replaced.
