"""Verify rollback bytes, exact scope, script syntax and recovery path guards offline."""
import ast
import json
from pathlib import Path
import prepare_migration as prep
import recover_snapshot as recovery
import migrate_ui_ue as migration

OUT, ROOT = prep.OUT, prep.ROOT


def main():
    checks = []
    for p in OUT.glob('*.py'):
        ast.parse(p.read_text(encoding='utf-8'), filename=str(p))
    checks.append('All migration Python scripts parse; no Unreal import/execution')
    m = json.loads((OUT / 'migration-map.json').read_text())
    assert m['schema_version'] == 2 and m['count'] == len(m['assets']) == 582
    all_source = m['assets'] + m['legacy_redirectors']
    assert len(all_source) == m['source_package_count'] == 588
    assert len(m['legacy_redirectors']) == m['preexisting_redirector_count'] == 6
    assert sum(r['bytes'] for r in all_source) == m['old_ui_bytes'] == 214623195
    old = {r['old_package'] for r in m['assets']}
    new = {r['new_package'].casefold() for r in m['assets']}
    assert len(old) == len(new) == 582
    assert all(r['new_package'] == '/Game/newUI' + r['old_package'][8:] for r in m['assets'])
    assert all(r['new_object'].split('.')[-1] == r['asset_name'] for r in m['assets'])
    assert len({r['old_package'] for r in all_source}) == 588
    assert not any(r['live_class'] == '/Script/CoreUObject.ObjectRedirector' for r in m['assets'])
    for r in m['legacy_redirectors']:
        target = next(c for c in m['assets'] if c['old_package'] == r['canonical_old_package'])
        assert r['new_object'] == target['new_object'] and r['new_package'] == target['new_package']
        assert r['forbidden_projected_new_package'].casefold() not in new
    checks.append('588 physical sources split into 582 unique canonical renames and 6 fixup-only redirectors; no phantom canonical')
    backup_path = Path(m['backup_manifest'])
    assert backup_path.is_relative_to((ROOT / 'Saved/Codex/UIRebuild20261024/Backups').resolve())
    assert prep.sha(backup_path) == m['backup_manifest_sha256']
    backup = json.loads(backup_path.read_text())
    indexed = {r['relative_path']: r for r in backup['files']}
    for count, r in enumerate(backup['files'], 1):
        assert Path(r['backup']).is_relative_to(backup_path.parent)
        assert prep.sha(Path(r['backup'])) == r['sha256'], r['relative_path']
        if count % 400 == 0:
            print('ARCHIVE_REVERIFIED', count, flush=True)
    assert all(indexed[r['source_relative']]['sha256'] == r['sha256'] for r in all_source)
    checks.append('All 1759 archived files freshly rehashed; all 588 UI source bytes match map')
    replacements = json.loads((OUT / 'reference-replacement-plan.json').read_text())
    assert len(replacements['current_config_aliases']) == 569
    assert len(replacements['additional_exact_canonical_compatibility_aliases']) == 588
    assert all(not r['new_destination'].startswith('/Game/UI/') for r in replacements['current_config_aliases'])
    for r in m['legacy_redirectors']:
        assert replacements['resolved_package_replacements'][r['old_package']] == r['new_package']
    assert set(replacements['resolved_package_replacements'].values()) <= {r['new_package'] for r in m['assets']}
    checks.append('569 historical aliases plus 588 physical old keys all resolve to the 582 real canonical targets')
    for invalid in ('Source/BoundlessImmortality/Game.cpp', '../Elsewhere.uasset', 'Saved/data.uasset'):
        try:
            recovery.checked_original(invalid)
        except RuntimeError:
            pass
        else:
            raise AssertionError('Recovery accepted scope escape: ' + invalid)
    checks.append('Recovery rejects all tested non-Content paths without modifying files')
    helper = (OUT / 'staged-source/NewUiMigrationExactFixup.cpp').read_text()
    script = (OUT / 'migrate_ui_ue.py').read_text()
    assert 'LOAD_NoRedirects' in helper and 'ERedirectFixupMode::DeleteFixedUpRedirectors' in helper
    assert 'delete_asset(' not in helper + script and 'rename_assets(rename)' in script
    assert '.rmdir()' in script and 'shutil.rmtree' not in script
    assert 'execute_console_command(None' not in script
    assert 'execute_console_command(unreal.EditorLevelLibrary.get_editor_world()' in script
    assert script.index('PinExactLegacyRedirectorsForMigration') < script.index('rename_assets(rename)')
    assert 'DestinationObjectName' in helper and 'Canonical != CanonicalCount' in helper
    checks.append('Static scope review: real world context; pin raw six before582 rename; exact588 fixup/empty-only directory removal')
    class StructPath:
        package_name = '/Script/Engine'
        asset_name = 'Texture2D'
        def __init__(self, address): self.address = address
        def __str__(self): return '<Struct TopLevelAssetPath (' + self.address + ')>'
    class Asset:
        def __init__(self, address): self.asset_class_path = StructPath(address)
    a, b = Asset('0x11111111'), Asset('0x22222222')
    assert str(a.asset_class_path) != str(b.asset_class_path)
    assert migration.class_path(a) == migration.class_path(b) == '/Script/Engine.Texture2D'
    b.asset_class_path.asset_name = 'Material'
    assert migration.class_path(a) != migration.class_path(b)
    checks.append('Regression: changing TopLevelAssetPath repr pointers does not change normalized class; real type differences still detected')
    # Inspect sources as a live observation, not an assertion while integration may migrate them.
    source_present = sum((ROOT / r['source_relative']).exists() for r in all_source)
    target_present = sum((ROOT / r['destination_relative']).exists() for r in m['assets'])
    report = dict(checked_at=prep.now(), passed=len(checks), failures=0, checks=checks,
        archived_file_count=len(backup['files']), archive_bytes=sum(r['bytes'] for r in backup['files']),
        source_files_present_at_check=source_present, target_files_present_at_check=target_present,
        engine_started=False, assets_changed_by_verification=False,
        cpp_compiled=False, unreal_api_execution_verified=False, migration_executed=False,
        backup_manifest_sha256=m['backup_manifest_sha256'])
    report.update(revision='v2', canonical_rename_count=582, preexisting_redirector_fixup_count=6,
                  migration_map_sha256=prep.sha(OUT / 'migration-map.json'))
    prep.save(OUT / 'preparation-verification.json', report)
    print(json.dumps(report, ensure_ascii=False, indent=2), flush=True)


if __name__ == '__main__':
    main()
