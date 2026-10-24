# /Game/UI → /Game/newUI migration V2

> **当前已完成（2026-10-24 14:13:17 +08:00独立审计）**：Content/UI不存在、实际registry旧根0、582迁移目标当前hash/非redirector类证明一致；newUI共587含后续5新资产。见 final-independent-audit.md 与 revisions/v5-final-independent-audit/final-owner-audit.json。下文是已执行流程与历史工具契约，不是要求重新迁移。禁止重复Rename/精确Fixup或把原回退工具当覆盖后续新5资产/源码epoch的一键整体恢复。完整新版UI/PIE/视觉另验。

Authoritative revision: v2-live-canonical-redirector-split. V1 is withdrawn and preserved under revisions/v1-before-live-redirector-correction/. Full original 588-file/1759-backup snapshot remains immutable. The fresh UE inventory identified 582 real canonical assets and SIX OTHER preexisting ObjectRedirectors. They are distinct from the historical six already removed earlier.

## Exact scope

migration-map.json schema2:
- assets / count: 582 real canonical UE RenameAssets rows; each destination retains its canonical suffix and asset name.
- legacy_redirectors / preexisting_redirector_count: 6 fixup-only rows; each points to the TRUE canonical target after migration. Never RenameAssets these six, never create their former naive projected newUI packages.
- source_package_count / expected_old_packages: 588 original physical packages, all must disappear from the old root.
- old_ui_bytes: 214623195; backup manifest unchanged, SHA256 625eef06abf776f245406e0991bd8b563bfc6b5c51a206fa8bbb84826a0b0980. All 1759 files / 1538854456 bytes freshly rehashed in preparation-verification.json.

The exact preexisting sources, all under /Game/UI/Ink/, are:
1. Materials/M_InkTitleFeather → Materials/Shared/M_InkTitleFeather
2. Materials/Atlas/M_InkScrollFade → Materials/Shared/M_InkScrollFade
3. NiagaraRevision20261022/Textures/T_XianxiaMeters → Textures/HUD/T_XianxiaMeters
4. Provided/T_ProvidedButtons → Textures/Shared/T_ProvidedButtons
5. Provided/T_ProvidedInventory → Textures/Inventory/T_ProvidedInventory
6. Provided/T_ProvidedRank → Textures/Leaderboard/T_ProvidedRank

Destination root becomes /Game/newUI/Ink/, using the target suffix on the right, NOT the source suffix. v2-correction-evidence.json contains exact object names, tags, dependencies, hashes and inventory fingerprint. Live inventory came from integration's read-only Editor PID18992; it did not perform migration. New V2 preflight must reverify all588 membership/classes/hashes/tags/referencers.

## Sole executor and updated entry points

logic-integration alone writes production Source/Config/Content and starts Editor/UBT under the already granted rebuild engine-lease.json. Cleanup never requests a second engine lease or starts an engine. Preserve and normally close any owned session before rebuilding helper; never force-close user work.

1. Re-copy BOTH staged-source/NewUiMigrationExactFixup.cpp/.h to the Editor module and rebuild. V2 adds the Pin command and helper_revision=2. An old compiled helper is insufficient.
2. Refresh the integration controller source. Its initial block currently asserts SOURCE.count(OLD_DISPATCH)==1; V2 has ZERO old None-context dispatches. Remove the obsolete mandatory replacement/assert, load the corrected owner script verbatim, and record its hash. The existing later reload path already accepts absence of OLD_DISPATCH. A new controller using the old initial assert will fail before preflight.
3. In the sole fresh idle GUI Editor, import migrate_ui_ue.py as a module and main('preflight'). It uses the real Editor world context, requires no PIE/dirty user packages, validates all1759 archived hashes, all588 physical source hashes/classes, 582 absent collision-free targets, the six precise old DestinationObject tags, and every incoming referencer's backup coverage.
4. main('migrate') reruns preflight, then calls BoundlessNewUI.PinExactLegacyRedirectorsForMigration. This C++ command temporarily suspends only the six precise process aliases, raw-loads those exact redirector objects with LOAD_NoRedirects, verifies their original canonical target and holds strong references. No save/delete occurs. Pinning BEFORE canonical rename lets DestinationObject pointers follow the real renamed canonical object, avoiding stale redirector-chain loads later.
5. Python keeps 582 canonical objects alive, submits ONLY assets[582] to RenameAssets, saves only backed-up exact dirty packages, writes the complete rename journal and byte delta.
6. Same PID calls BoundlessNewUI.FixupExactMigrationRedirectors. V2 checks map/preflight/journal/schema/counts/pins, inspects at most 588 explicitly mapped old-path objects (582 rename-generated +6 preexisting), validates each exact new canonical object destination, and passes only these raw redirectors to FixupReferencers(DeleteFixedUpRedirectors). New canonical uniqueness/count is582, not588. Missing old paths are acceptable only if their physical old package is also absent. No global Fixup/AdvancedCopy/delete_asset.
7. Python verifies stable class IDs via package_name+'.'+asset_name, not str(struct repr) pointers. All582 canonical classes/names and physical files must match preflight. All588 old packages must be absent; all six forbidden projected new packages must also be absent. Only after zero old files and zero registry packages does it remove empty directories via rmdir.
8. audit_migration_results.py independently checks actual engine reports,582 destination classes/hashes,6 old redirector removals,forbidden new aliases absent,changed original referencer backup coverage and production Source/Config residuals. New UI default route/render/business evidence is separately integration-owned; moving directories does not prove the new visual system works.

Engine outputs now include engine-preflight.json, engine-legacy-pin-result.json, engine-migration-journal.json, engine-fixup-result.json, asset-write-journal.json, engine-final-verification.json. They do not yet exist for V2. Current tooling checks are offline, not C++ build or actual UE execution claims.

## Source/config canonical replacement

reference-replacement-plan.json resolves historical569 aliases AND physical588 old paths to the582 REAL canonical destinations. Preserve historical OldName keys only in explicit compatibility tables. Destinations and ordinary runtime defaults must newUI canonical. Do not enable canonical-old→newUI CoreRedirects before old source loads/RenameAssets; do not use substring/global redirects.

Do NOT blindly replace only '/Game/UI' in old legacy paths. For example old Provided/T_ProvidedInventory resolves to newUI/Ink/Textures/Inventory/T_ProvidedInventory, not newUI/Ink/Provided/T_ProvidedInventory. Finite dynamic arrays (Gifts/ratings) require exact per-value canonical mapping or explicit finite resolver aliases. The plan's resolved_package_replacements provides the exact table.

Perform migration/fixup before importing unrelated new newUI resources, so newly authored old-root references cannot enter without backup coverage. Any source membership/hash/type/tag drift, target collision, unknown outside referencer, incomplete helper or existing partial rename journal must stop and preserve the failure checkpoint; do not simply retry RenameAssets.

## Rollback and evidence

Original full snapshot remains under Saved/Codex/UIRebuild20261024/Backups/20261024T123117935118/, outside Content. The V1 map/scripts/reports are archived before correction; no original evidence was overwritten without that history.

recover_snapshot.py is journal-driven: plan is read-only; execute requires a reviewed plan fingerprint, valid integration migration coordination and no UnrealEditor/LiveCoding processes. It revalidates exact current before/after hashes, archives migration-time bytes again, restores original authored package bytes to their original paths, and removes only exact mapped newly-created canonical files already archived. It never recursively deletes directories, never automatically overwrites Source/Config or later changed files. Original preexisting six redirectors are restored from original snapshot on rollback; they were never renamed as canonicals.

Useful commands from project root:

```powershell
$py = 'C:/Users/zxxuh/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$team = 'specs/boundless-ink-ui/rebuild-2026-10-24/migration'
& $py "$team/verify_preparation.py"
# After sole-executor actual V2 engine run:
& $py "$team/audit_migration_results.py"
# Only for a real partial run needing a reviewed rollback:
# & $py "$team/recover_snapshot.py" plan
# & $py "$team/recover_snapshot.py" execute --confirm-plan-sha256 '<actual reviewed fingerprint>'
```

Status/ready-v2.json describe readiness only. Actual completion requires Content/UI absent, current registry /Game/UI zero,582 true new canonicals preserved,source/config reference validation and integration's new default UI evidence. Visual approval remains the user.
