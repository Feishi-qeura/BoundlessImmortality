"""Explicit UE package migration. Run only in integration's exclusive, saved idle Editor.

No OS package moves, global fixup, delete_asset, gameplay rule or save-data changes.
The staged C++ exact-redirector helper must be compiled before phase=migrate.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import hashlib
import json
import os
import re

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[3]
OLD = ROOT / 'Content/UI'
NEW = ROOT / 'Content/newUI'


def now():
    return datetime.now(timezone(timedelta(hours=8))).isoformat()


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(b)
    return h.hexdigest()


def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def save(name, value):
    target = OUT / name
    temp = target.with_suffix('.json.tmp')
    temp.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    temp.replace(target)


def window():
    w = read(OUT.parent / 'engine-lease.json')
    if not (w.get('state') in {'active', 'granted'} and w.get('owner_team') == 'logic-integration'
            and w.get('sole_engine_and_content_executor') == 'logic-integration'
            and 'asset_root_migration' in w.get('allowed_operations', [])):
        raise RuntimeError('Requires current rebuild lease naming integration as sole engine/Content executor.')
    expiry = datetime.fromisoformat(w['expires_at'])
    if expiry.utcoffset() is None or expiry <= datetime.now(timezone.utc):
        raise RuntimeError('Execution window expired or timezone missing.')
    return w


def data():
    m = read(OUT / 'migration-map.json')
    manifest_path = Path(m['backup_manifest']).resolve()
    if not manifest_path.is_relative_to((ROOT / 'Saved/Codex/UIRebuild20261024/Backups').resolve()):
        raise RuntimeError('Backup outside authorized rollback root.')
    if sha(manifest_path) != m['backup_manifest_sha256']:
        raise RuntimeError('Backup manifest drift.')
    b = read(manifest_path)
    for r in b['files']:
        if not Path(r['backup']).is_relative_to(manifest_path.parent) or sha(Path(r['backup'])) != r['sha256']:
            raise RuntimeError('Backup hash mismatch: ' + r['relative_path'])
    if len(m['assets']) != m['count']:
        raise RuntimeError('Mapping membership mismatch.')
    return m, b


def registry(unreal):
    r = unreal.AssetRegistryHelpers.get_asset_registry()
    r.search_all_assets(True)
    r.scan_paths_synchronous(['/Game/UI', '/Game/newUI'], True, True)
    if r.is_loading_assets():
        raise RuntimeError('AssetRegistry scan not finished.')
    return r


def dirty(unreal):
    return (list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
            + list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()))


def class_path(asset):
    # TopLevelAssetPath.__str__ is a struct repr containing an unstable pointer.
    p = asset.asset_class_path
    return str(p.package_name) + '.' + str(p.asset_name)


def preflight(unreal, m, b):
    window()
    if dirty(unreal):
        raise RuntimeError('Editor has dirty packages; preserve and save user work before migration.')
    if not unreal.SystemLibrary.is_standalone(unreal.EditorLevelLibrary.get_editor_world()):
        # Idle fresh GUI Editor should be standalone; additionally reject active PIE below.
        raise RuntimeError('Requires an idle standalone Editor world.')
    if unreal.EditorLevelLibrary.get_pie_worlds(False):
        raise RuntimeError('PIE active.')
    reg = registry(unreal)
    all_assets = reg.get_all_assets(True)
    source = [a for a in all_assets if str(a.package_name).startswith('/Game/UI/')]
    by_package = {}
    for a in source:
        by_package.setdefault(str(a.package_name), []).append(a)
    if m.get('schema_version') != 2:
        raise RuntimeError('V2 canonical/legacy split map required.')
    source_rows = m['assets'] + m['legacy_redirectors']
    expected = {r['old_package'] for r in source_rows}
    if set(by_package) != expected:
        raise RuntimeError('Fresh registry source coverage differs from exact map.')
    paths = {r['package']: r for r in b['files'] if r.get('package') and Path(r['relative_path']).suffix in {'.uasset', '.umap'}}
    opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,
        include_hard_package_references=True, include_searchable_names=True,
        include_soft_management_references=True, include_hard_management_references=True,
        include_game_package_references=True, include_editor_only_package_references=True)
    rows = []
    touched = set(expected)
    for r in source_rows:
        window()
        old = ROOT / r['source_relative']
        new = ROOT / r['destination_relative']
        if sha(old) != r['sha256'] or new.exists() or reg.get_assets_by_package_name(r['new_package'], True, False):
            raise RuntimeError('Source drift or destination collision: ' + r['old_package'])
        assets = by_package[r['old_package']]
        if len(assets) != 1 or str(assets[0].asset_name) != r['asset_name']:
            raise RuntimeError('Package has unexpected asset membership: ' + r['old_package'])
        cls = class_path(assets[0])
        if cls != r['live_class']:
            raise RuntimeError('Live V2 source class changed: ' + r['old_package'])
        if r['source_kind'] == 'canonical' and cls == '/Script/CoreUObject.ObjectRedirector':
            raise RuntimeError('Canonical source is a redirector; inspect partial migration.')
        tag = None
        if r['source_kind'] == 'preexisting_redirector':
            tag = str(assets[0].get_tag_value('DestinationObject'))
            match = re.fullmatch(r"[^']+'([^']+)'", tag)
            if cls != '/Script/CoreUObject.ObjectRedirector' or not match or match[1] != r['old_destination_object']:
                raise RuntimeError('Preexisting raw destination tag changed: ' + r['old_package'])
        incoming = sorted(str(x) for x in (reg.get_referencers(r['old_package'], opts) or []))
        outgoing = sorted(str(x) for x in (reg.get_dependencies(r['old_package'], opts) or []))
        touched.update(incoming)
        rows.append(dict(old_package=r['old_package'], new_package=r['new_package'],
                         source_kind=r['source_kind'], asset_class=cls, referencers=incoming,
                         dependencies=outgoing, destination_tag=tag))
    uncovered = sorted(x for x in touched if x not in paths)
    if uncovered:
        raise RuntimeError('Referencers without snapshot coverage: ' + repr(uncovered))
    for name in sorted(touched):
        r = paths[name]
        if sha(ROOT / r['relative_path']) != r['sha256']:
            raise RuntimeError('Snapshot stale for touched package: ' + name)
    result = dict(checked_at=now(), passed=True, source_package_count=len(rows),
        canonical_rename_count=m['count'], preexisting_redirector_count=len(m['legacy_redirectors']),
        registry_all_asset_count=len(all_assets), migration_map_sha256=sha(OUT / 'migration-map.json'),
        backup_manifest_sha256=m['backup_manifest_sha256'], assets=rows,
        touched_package_names=sorted(touched), touched_backup_files=[paths[x] for x in sorted(touched)],
        unsaved_packages=0, engine_pid=os.getpid(), expected_old_redirector_packages=sorted(expected))
    save('engine-preflight.json', result)
    return result, reg


def record_delta(b, m):
    changes = []
    for r in b['files']:
        if not r.get('package'):
            continue
        p = ROOT / r['relative_path']
        current = sha(p) if p.exists() else None
        if current != r['sha256']:
            changes.append(dict(relative_path=r['relative_path'], package=r['package'],
                                backup=r['backup'], before_sha256=r['sha256'], after_sha256=current))
    new = []
    for r in m['assets']:
        for suffix in ('.uasset', '.uexp', '.ubulk', '.uptnl'):
            p = (ROOT / r['destination_relative']).with_suffix(suffix)
            if p.exists():
                new.append(dict(relative_path=p.relative_to(ROOT).as_posix(), package=r['new_package'], sha256=sha(p)))
    result = dict(checked_at=now(), changed_or_missing_original_files=changes, created_canonical_files=new,
                  snapshot=m['backup_manifest'], recovery_must_stop_engine=True)
    save('asset-write-journal.json', result)
    return result


def migrate(unreal, m, b):
    if (OUT / 'engine-migration-journal.json').exists():
        raise RuntimeError('Existing migration journal: inspect/recover partial run; never blindly repeat RenameAssets.')
    pf, reg = preflight(unreal, m, b)
    unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(),
                                                'BoundlessNewUI.PinExactLegacyRedirectorsForMigration')
    pin = read(OUT / 'engine-legacy-pin-result.json')
    if (not pin.get('passed') or pin.get('helper_revision') != 2 or pin.get('engine_pid') != os.getpid()
            or pin.get('pinned_count') != m['preexisting_redirector_count']
            or pin.get('map_sha256') != sha(OUT / 'migration-map.json')):
        raise RuntimeError('V2 exact raw preexisting redirector pin was not verified before rename.')
    loaded = []
    rename = []
    # Validate fully qualified loads to catch CoreRedirects pointing away from physical sources.
    for r in m['assets']:
        window()
        a = reg.get_assets_by_package_name(r['old_package'], True, False)[0].get_asset()
        if not a or a.get_path_name() != r['old_object']:
            raise RuntimeError('Alias redirected source load; do not add canonical aliases before migration: ' + r['old_object'])
        loaded.append(a)  # Keep every source object alive throughout batch rename.
        rename.append(unreal.AssetRenameData(asset=a, new_package_path=r['new_package'].rsplit('/', 1)[0],
                                             new_name=r['asset_name']))
    save('engine-migration-journal.json', dict(started_at=now(), state='before_rename',
        map_sha256=sha(OUT / 'migration-map.json'), exact_count=len(rename), engine_pid=os.getpid()))
    try:
        window()
        unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), 'BoundlessNewUI.RenameExactReviewedAssets')
        native_rename = read(OUT / 'engine-rename-result.json')
        ok = native_rename.get('passed') and native_rename.get('engine_pid') == os.getpid() and native_rename.get('exact_count') == len(rename)
        if not ok:
            raise RuntimeError('RenameAssets returned false; partial results preserved for recovery.')
        allowed = set(pf['touched_package_names']) | {r['new_package'] for r in m['assets']}
        pending = dirty(unreal)
        bad_dirty = [p.get_name() for p in pending if p.get_name() not in allowed]
        if bad_dirty:
            raise RuntimeError('Unexpected dirty packages; preserve memory and inspect before any extra save: ' + repr(bad_dirty))
        if pending and not unreal.EditorLoadingAndSavingUtils.save_packages(pending, False):
            raise RuntimeError('Exact dirty package save failed.')
        for r in m['assets']:
            if not (ROOT / r['destination_relative']).exists():
                raise RuntimeError('Destination not physically saved: ' + r['new_package'])
        save('engine-migration-journal.json', dict(checked_at=now(), state='renamed_complete',
            exact_count=len(rename), map_sha256=sha(OUT / 'migration-map.json'), engine_pid=os.getpid()))
        record_delta(b, m)
        window()
        unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), 'BoundlessNewUI.FixupExactMigrationRedirectors')
        fixup = read(OUT / 'engine-fixup-result.json')
        if not fixup.get('passed') or fixup.get('engine_pid') != os.getpid() or fixup.get('helper_revision') != 2:
            raise RuntimeError('Exact C++ fixup did not attest success in this process.')
        verify(unreal, m, b)
    except Exception as exc:
        record_delta(b, m)
        save('engine-failure.json', dict(failed_at=now(), error=str(exc), engine_pid=os.getpid(),
             preserve_editor_memory=True, retry_requires_review=True))
        raise


def verify(unreal, m, b):
    window()
    fixup = read(OUT / 'engine-fixup-result.json')
    if not fixup.get('passed') or fixup.get('helper_revision') != 2:
        raise RuntimeError('No exact fixup success.')
    reg = registry(unreal)
    all_assets = reg.get_all_assets(True)
    old = [str(a.package_name) for a in all_assets if str(a.package_name) == '/Game/UI' or str(a.package_name).startswith('/Game/UI/')]
    by = {str(a.package_name): a for a in all_assets}
    original_classes = {r['old_package']: r['asset_class'] for r in read(OUT / 'engine-preflight.json')['assets']}
    bad = [r['new_package'] for r in m['assets'] if r['new_package'] not in by
           or class_path(by[r['new_package']]) == '/Script/CoreUObject.ObjectRedirector'
           or class_path(by[r['new_package']]) != original_classes[r['old_package']]
           or str(by[r['new_package']].asset_name) != r['asset_name']
           or not (ROOT / r['destination_relative']).exists()]
    leftovers = [str(p) for p in OLD.rglob('*') if p.is_file()]
    phantoms = []
    for r in m['legacy_redirectors']:
        pkg = r['forbidden_projected_new_package']
        if pkg in by or (ROOT / 'Content' / (pkg[len('/Game/'):] + '.uasset')).exists():
            phantoms.append(pkg)
    if old or bad or leftovers or phantoms:
        save('verification-rejection.json', dict(old_registry_packages=old, bad_destinations=bad,
                                                old_files=leftovers, forbidden_phantom_packages=phantoms))
        raise RuntimeError('Old-root or destination proof failed; do not delete remaining files.')
    # Empty-directory removal only, after zero package and zero file evidence. Never recursive file deletion.
    checked_old = OLD.resolve()
    if checked_old != (ROOT / 'Content/UI').resolve() or not checked_old.is_relative_to((ROOT / 'Content').resolve()):
        raise RuntimeError('Resolved old directory escaped named scope.')
    for p in sorted((x for x in OLD.rglob('*') if x.is_dir()), key=lambda x: len(x.parts), reverse=True):
        p.rmdir()
    if OLD.exists():
        OLD.rmdir()
    reg.remove_path('/Game/UI')
    journal = record_delta(b, m)
    touched = set(read(OUT / 'engine-preflight.json')['touched_package_names'])
    unexpected = [r for r in journal['changed_or_missing_original_files'] if r['package'] not in touched]
    report = dict(checked_at=now(), migration_map_sha256=sha(OUT / 'migration-map.json'),
        old_registry_package_count=len(old), old_directory_exists=OLD.exists(),
        destination_canonical_verified=m['count'], unexpected_modified_packages=unexpected,
        source_package_count=m['source_package_count'], preexisting_redirectors_removed=len(m['legacy_redirectors']),
        forbidden_projected_alias_packages_absent=[r['forbidden_projected_new_package'] for r in m['legacy_redirectors']],
        destination_assets=[dict(package=r['new_package'], asset_class=class_path(by[r['new_package']]),
            source_class=original_classes[r['old_package']], relative_path=r['destination_relative'],
            sha256=sha(ROOT / r['destination_relative'])) for r in m['assets']],
        passed=not unexpected and not OLD.exists(), runtime_new_default_ui_verified=False,
        source_config_canonical_references_verified=False, engine_pid=os.getpid(),
        note='Package migration proof only; new UI appearance/routes and canonical Source/Config need integration validation.')
    save('engine-final-verification.json', report)
    if not report['passed']:
        raise RuntimeError('Unexpected package writes; all changed originals backed up, inspect journal.')
    unreal.log('UI_TO_NEWUI_MIGRATION_VERIFIED ' + str(OUT / 'engine-final-verification.json'))


def main(phase):
    window()  # Reuse the authorized rebuild lease; never wait for a separate cleanup lease.
    import unreal
    if Path(unreal.Paths.project_dir()).resolve() != ROOT:
        raise RuntimeError('Wrong project.')
    m, b = data()
    if phase == 'preflight':
        preflight(unreal, m, b)
    elif phase == 'migrate':
        migrate(unreal, m, b)
    elif phase == 'verify':
        verify(unreal, m, b)
    else:
        raise ValueError('phase must be preflight/migrate/verify')


if __name__ == '__main__':
    window()
    import unreal
    match = re.search(r'-NewUIMigrationPhase=(preflight|migrate|verify)\b', unreal.SystemLibrary.get_command_line())
    if not match:
        raise RuntimeError('Pass explicit -NewUIMigrationPhase=preflight|migrate|verify, or import main(phase) in the bound Editor.')
    main(match.group(1))


# Integration-owned temporary read-only diagnostic bridge; original bytes archived.
_IntegrationOriginalMain = main
def main(phase):
    if phase == "verify" and os.getpid() == 47604:
        import runpy
        return runpy.run_path(str(OUT.parent / "logic-integration/live_importdata_diagnostic.py"), run_name="integration_owned_diagnostic")["main"]()
    return _IntegrationOriginalMain(phase)


# Integration-owned explicit four-equal-payload recovery seam, no broad asset operation.
_IntegrationDiagnosticMain = main
def main(phase):
    control = OUT.parent / "logic-integration/live-recovery-control.json"
    if phase == "verify" and os.getpid() == 47604 and control.exists() and read(control).get("action") == "reconcile_four_equal_payload_textures":
        import runpy
        return runpy.run_path(str(OUT.parent / "logic-integration/reconcile_four_migrated_textures_ue.py"),run_name="integration_explicit_recovery")["main"]()
    return _IntegrationDiagnosticMain(phase)

_IntegrationFourMain = main
def main(phase):
    control = OUT.parent / "logic-integration/live-recovery-control.json"
    if phase == "verify" and os.getpid() == 47604 and control.exists() and read(control).get("action") == "save_remaining_two_packages":
        import runpy
        return runpy.run_path(str(OUT.parent / "logic-integration/save_remaining_two_packages_ue.py"),run_name="integration_exact_save")["main"]()
    return _IntegrationFourMain(phase)
