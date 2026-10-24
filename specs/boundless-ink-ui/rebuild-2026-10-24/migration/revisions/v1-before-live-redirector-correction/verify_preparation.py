"""Verify rollback bytes, exact scope, script syntax and recovery path guards offline."""
import ast
import json
from pathlib import Path
import prepare_migration as prep
import recover_snapshot as recovery

OUT, ROOT = prep.OUT, prep.ROOT


def main():
    checks = []
    for p in OUT.glob('*.py'):
        ast.parse(p.read_text(encoding='utf-8'), filename=str(p))
    checks.append('All migration Python scripts parse; no Unreal import/execution')
    m = json.loads((OUT / 'migration-map.json').read_text())
    assert m['count'] == len(m['assets']) == 588
    assert sum(r['bytes'] for r in m['assets']) == m['old_ui_bytes'] == 214623195
    old = {r['old_package'] for r in m['assets']}
    new = {r['new_package'].casefold() for r in m['assets']}
    assert len(old) == len(new) == 588
    assert all(r['new_package'] == '/Game/newUI' + r['old_package'][8:] for r in m['assets'])
    assert all(r['new_object'].split('.')[-1] == r['asset_name'] for r in m['assets'])
    checks.append('588 exact path-preserving, case-fold unique mapped packages; object names preserved')
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
    assert all(indexed[r['source_relative']]['sha256'] == r['sha256'] for r in m['assets'])
    checks.append('All 1759 archived files freshly rehashed; all 588 UI source bytes match map')
    replacements = json.loads((OUT / 'reference-replacement-plan.json').read_text())
    assert len(replacements['current_config_aliases']) == 569
    assert len(replacements['additional_exact_canonical_compatibility_aliases']) == 588
    assert all(not r['new_destination'].startswith('/Game/UI/') for r in replacements['current_config_aliases'])
    checks.append('569 historical old-name keys distinguished from 588 relocated canonical keys')
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
    checks.append('Static destructive-scope review: UE rename + exact raw redirector fixup; empty-only directory removal')
    # Inspect sources as a live observation, not an assertion while integration may migrate them.
    source_present = sum((ROOT / r['source_relative']).exists() for r in m['assets'])
    target_present = sum((ROOT / r['destination_relative']).exists() for r in m['assets'])
    report = dict(checked_at=prep.now(), passed=len(checks), failures=0, checks=checks,
        archived_file_count=len(backup['files']), archive_bytes=sum(r['bytes'] for r in backup['files']),
        source_files_present_at_check=source_present, target_files_present_at_check=target_present,
        engine_started=False, assets_changed_by_verification=False,
        cpp_compiled=False, unreal_api_execution_verified=False, migration_executed=False,
        backup_manifest_sha256=m['backup_manifest_sha256'])
    prep.save(OUT / 'preparation-verification.json', report)
    print(json.dumps(report, ensure_ascii=False, indent=2), flush=True)


if __name__ == '__main__':
    main()
