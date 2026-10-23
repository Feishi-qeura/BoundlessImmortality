# Portrait UI and Windows Distribution Implementation Plan

Goal: Implement approved portrait UI redesign, preserve Character/Inventory/Leaderboard behavior, organize all /Game/UI assets, validate and produce Windows distributable ZIP.

Architecture: Existing UE C++/UMG routes and service ViewModels remain authoritative. Reuse current ink assets, centralize background ownership and responsive shell, implement page-specific arrangements; migrate via editor asset APIs with compatibility redirects until references verified.

Spec: design.md and screen-layouts.md in this directory. User approval: “确认，实现全部，直接打包发布”; clarified output: “生成 Windows 可分发 ZIP”. These authorize continuous implementation, validation and local release packaging.

Execution: subagent-driven-development for independent implementation scopes; parent integrates and runs serialized Unreal builds/editor automation. This workspace has no Git checkout; use hash-verified backups and file diff ledger instead of commits/worktrees.

## Constraints
- Preserve Character/CharacterProfile, Inventory and Leaderboard content/control layouts and behavior; only background and asset-reference changes permitted there.
- Preserve world, camera, maps, weapon and gameplay rules. New UI reads existing state; do not invent state or enable gated features.
- MaShanZheng titles, LXGWWenKai other text and digits; visible health label 血量.
- Portrait reference 720x1280, 24 logical margin, 88 hit areas; body scrolls, headers/actions remain visible; landscape reflows.
- Retain all Texture, Material, MaterialInstance, Niagara, Widget, Font and Data assets. Rename via Unreal APIs, never OS move/delete uasset.
- Windows Development/prototype_full local package, existing portable authority launcher. No platform publication requested.

## Review focus
- Small portrait and wide layout, long labels, repeated size changes retain working controls.
- Preserved pages retain bindings, geometry and inventories; no paper duplicates on repeated open/close.
- Dynamically assembled asset paths and generated blueprint classes survive relocation in cooked build.
- UI navigation route aliases remain reachable with correct feature gate and back stack.
- Packaged launcher and local authority work after extraction with no developer path dependency.

## Tasks
- [x] T1 Parent: current-source and asset hash backup; approval record; disk/process preflight.
- [x] T2 Layout implementer: feature page families in InkFeaturePanels.cpp, InkRewardsPanels.cpp, InkCharacterPanels.cpp excluding CharacterProfile implementation, InkWulingPanel.cpp, UI/InkWulingToolsPanel.cpp and InkWulingRecordsPanel.cpp; retain data/actions, implement approved content arrangements and identify remaining gaps.
- [x] T3 HUD implementer: InkHudLayout.cpp, InkFrontendWidget.cpp main/settings/saves and InkAnchorStatusWidget.cpp; reuse existing artwork; preserve designated panels and bindings.
- [x] T4 Parent: shared shell/theme responsive background ownership, deterministic layout tests; migration script and exact/dynamic asset path handling; editor audit.
- [x] T5 Parent: integrate, build Editor, native tests, route traversal and portrait/landscape runtime captures, preservation checks; fresh reviewer evaluates actual patch and findings fixed.
- [x] T6 Parent: BuildCookRun new Win64 archive, portable runtime assembly, packaged portrait/landscape checks, ZIP + CRC + SHA256 + extraction verification and release notes.

## Evidence commands
Build: absolute F:/UNREAL/UE_5.8/UE_5.8/Engine/Build/BatchFiles/Build.bat BoundlessImmortalityEditor Win64 Development -Project=D:/demo/BoundlessImmortality/BoundlessImmortality.uproject -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=2.
Cook/package: Tools/WindowsRelease/package.py with new Saved/Packages/PortraitUI20261023 output and explicit engine/Python runtime.
Runtime: Tools/WindowsRelease/verify_package.py with distinct fresh profiles, 720x1280 and 1280x720; extend traversal evidence for newly redesigned routes.
Final assertions require fresh artifacts; documentation validator alone does not prove UE functionality.
