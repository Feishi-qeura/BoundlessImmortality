"""Final offline owner audit of completed directory migration; never starts Unreal.

Hashes current582 outputs, small evidence manifests, and selected recovery evidence;
does not repeat original1759 or partial612 full archive hashing. UI acceptance separate.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
from collections import Counter
import hashlib
import json
import re
from PIL import Image

HERE = Path(__file__).resolve().parent
TEAM = HERE.parents[1]
ROOT = TEAM.parents[3]
INTEGRATION = TEAM.parent / 'logic-integration'


def read(p): return json.loads(p.read_text(encoding='utf-8-sig'))


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda: f.read(8 * 1024 * 1024), b''): h.update(b)
    return h.hexdigest()


def static_paths(resolved):
    problems, exceptions, new_missing, evidence = [], [], [], []
    compat = ROOT / 'Source/BoundlessImmortality/Frontend/UI/InkUiAssetPaths.cpp'
    pair_re = re.compile(r'TEXT\("([^"]+)"\)\s*,\s*TEXT\("([^"]+)"\)')
    pairs = [(a, b) for a, b in pair_re.findall(compat.read_text(encoding='utf-8-sig')) if a.startswith('/Game/UI/')]
    if dict(pairs) != resolved or len(pairs) != len(resolved): problems.append('C++ compatibility table differs from exact1151 resolved map')
    config = ROOT / 'Config/DefaultEngine.ini'
    redirects = re.findall(r'\+PackageRedirects=\(OldName="([^"]+)",NewName="([^"]+)"\)', config.read_text(encoding='utf-8-sig'))
    redirects = [(a, b) for a, b in redirects if a.startswith('/Game/UI/')]
    if dict(redirects) != resolved or len(redirects) != len(resolved): problems.append('Config OldName whitelist/target/uniqueness differs from1151 map')
    helpers = {'LegacyUiRedirectorFixupCommandlet.cpp', 'NewUiMigrationExactFixup.cpp', 'ScopedKnownRenameWarning.cpp'}
    files = [p for base in ('Source', 'Config') for p in (ROOT / base).rglob('*')
             if p.is_file() and p.suffix in {'.h', '.cpp', '.ini'}]
    for p in sorted(files):
        raw = p.read_bytes(); text = raw.decode('utf-8-sig'); relative = p.relative_to(ROOT).as_posix()
        evidence.append(dict(file=relative, sha256=hashlib.sha256(raw).hexdigest()))
        for n, line in enumerate(text.splitlines(), 1):
            if line.lstrip().startswith(('//', '*', ';', '#')): continue
            if '/Game/UI' in line:
                kind = None
                if p == compat:
                    matches = pair_re.findall(line)
                    if matches and all(resolved.get(a) == b for a, b in matches if a.startswith('/Game/UI/')):
                        kind = 'exact_compatibility_old_key'
                    elif 'Value.StartsWith(TEXT("/Game/UI/"))' in line:
                        kind = 'explicit_legacy_resolver_guard'
                elif p == config:
                    match = re.search(r'OldName="([^"]+)",NewName="([^"]+)"', line)
                    if match and resolved.get(match[1]) == match[2]: kind = 'exact_CoreRedirect_OldName'
                elif 'Tests' in p.parts:
                    kind = 'negative_or_compatibility_test_fixture'
                elif p.name in helpers and relative.startswith('Source/BoundlessImmortalityEditor/Private/'):
                    kind = 'historical_exact_migration_helper'
                if kind: exceptions.append(dict(file=relative, line=n, kind=kind))
                else: problems.append(dict(file=relative, line=n, text=line.strip(), reason='ordinary old-root literal outside explicit whitelist'))
            if p.is_relative_to(ROOT / 'Source/BoundlessImmortality') and 'Tests' not in p.parts:
                for literal in re.findall(r'"(/Game/newUI/[^"\r\n]+)"', line):
                    if '%' in literal or literal.endswith('/'): continue
                    package = literal.split('.', 1)[0]
                    if not (ROOT / 'Content' / (package[len('/Game/'):] + '.uasset')).is_file():
                        new_missing.append(dict(file=relative, line=n, package=package))
        if hashlib.sha256(p.read_bytes()).hexdigest() != evidence[-1]['sha256']:
            problems.append(dict(file=relative, reason='file changed while audit read; refresh after writer settles'))
    if new_missing: problems.append(dict(reason='new canonical runtime literal file absent', paths=new_missing))
    return dict(checked_files=len(files), problems=problems, whitelist_counts=dict(Counter(x['kind'] for x in exceptions)),
                whitelist_evidence=exceptions, file_hashes=evidence, exact_cpp_aliases=len(pairs), exact_ini_aliases=len(redirects), passed=not problems)


def payload_eight(m, partial):
    proof_path = INTEGRATION / 'eight-source-copies-proof.json'
    proof = read(proof_path); recovery = read(INTEGRATION / 'eight-source-reconciliation-result.json')
    assert proof['all_proven'] and recovery['passed'] and len(proof['rows']) == len(recovery['operations']) == 8
    assert sha(proof_path) == recovery['proof_sha256']
    assert recovery['dirty_remaining'] == recovery['dirty_maps'] == []
    by_old = {r['old_object']: r for r in m['assets']}
    ops = {r['old']: r for r in recovery['operations']}
    snapshots = {Path(r['relative']).as_posix(): r for r in partial['files']}
    details = []
    for r in proof['rows']:
        row = by_old[r['old']]; op = ops[r['old']]
        assert r['new'] == op['retained'] == row['new_object'] and r['class'] == row['live_class']
        old, new, after = [Path(x) for x in (r['exports']['old']['file'], r['exports']['new']['file'], op['after_export'])]
        assert sha(old) == r['exports']['old']['sha256'] and sha(new) == r['exports']['new']['sha256']
        item = dict(old=r['old'], retained=r['new'], asset_class=r['class'], passed=True)
        if r['class'] == '/Script/Engine.Texture2D':
            hashes = []
            for file in (old, new, after):
                with Image.open(file) as image:
                    rgba = image.convert('RGBA'); hashes.append(hashlib.sha256(rgba.tobytes()).hexdigest())
            assert sha(old) == sha(new) == sha(after) and len(set(hashes)) == 1
            item.update(png_bytes_and_rgba_equal=True, rgba_sha256=hashes[0])
        else:
            assert old.read_text(encoding='utf-8-sig').replace('/Game/UI/', '/Game/newUI/') == new.read_text(encoding='utf-8-sig') == after.read_text(encoding='utf-8-sig')
            item['normalized_t3d_equal_and_new_unchanged'] = True
        if r['class'] == '/Script/Engine.FontFace':
            payload = Path(r['font_source']).read_bytes()
            assert hashlib.sha256(payload).hexdigest() == r['font_source_sha256']
            assert len(payload) == r['font_source_bytes']
            old_archive = Path(snapshots[row['source_relative']]['backup'])
            assert payload in old_archive.read_bytes() and payload in (ROOT / row['destination_relative']).read_bytes()
            item.update(full_native_font_payload_in_archived_old_and_current_new=True, font_sha256=r['font_source_sha256'])
        details.append(item)
    return dict(passed=True, count=8, rows=details, whole_archives_rehashed=False)


def main():
    started = datetime.now(timezone(timedelta(hours=8))).isoformat()
    m = read(TEAM / 'migration-map.json'); map_hash = sha(TEAM / 'migration-map.json')
    names = ['engine-preflight.json', 'engine-legacy-pin-result.json', 'engine-rename-result.json', 'engine-migration-journal.json',
             'engine-fixup-result.json', 'asset-write-journal.json', 'engine-final-verification.json']
    pf, pin, rename, journal, fixup, delta, final = [read(TEAM / n) for n in names]
    assert pf['passed'] and pin['passed'] and rename['passed'] and fixup['passed'] and final['passed']
    assert all(r['engine_pid'] == 47604 for r in (pf, pin, rename, journal, fixup, final))
    assert (pf['source_package_count'], pf['canonical_rename_count'], pf['preexisting_redirector_count'], pin['pinned_count'], rename['exact_count']) == (588, 582, 6, 6, 582)
    assert rename['native_rename_succeeded'] and rename['rejected_unknown_dialog_count'] == 0
    assert journal['state'] == 'renamed_complete' and journal['exact_count'] == 582
    assert pf['migration_map_sha256'] == pin['map_sha256'] == journal['map_sha256'] == fixup['map_sha256'] == final['migration_map_sha256'] == map_hash
    assert (fixup['exact_redirector_count'], fixup['source_scope_count'], fixup['canonical_destination_count']) == (29, 588, 582)
    assert final['old_registry_package_count'] == fixup['old_registry_package_count'] == 0 and final['unexpected_modified_packages'] == []
    assert final['preexisting_redirectors_removed'] == 6 and not final['old_directory_exists'] and not (ROOT / 'Content/UI').exists()
    expected = {r['old_object']: r['new_object'] for r in m['assets'] + m['legacy_redirectors']}
    inspected = fixup['inspected_exact_redirectors']; assert len(inspected) == len({r['old_object'] for r in inspected}) == 29
    assert all(expected[r['old_object']] == r['destination_object'] and not r['incoming_before_fixup'] for r in inspected)
    classes = {r['old_package']: r['asset_class'] for r in pf['assets']}
    targets = {r['package']: r for r in final['destination_assets']}; assert len(targets) == 582
    outputs = []
    for r in m['assets']:
        proof = targets[r['new_package']]; path = ROOT / r['destination_relative']
        assert path.is_file() and sha(path) == proof['sha256']
        assert proof['asset_class'] == proof['source_class'] == r['live_class'] == classes[r['old_package']]
        assert proof['asset_class'] != '/Script/CoreUObject.ObjectRedirector'
        outputs.append(dict(package=r['new_package'], sha256=proof['sha256'], asset_class=proof['asset_class']))
    for r in m['legacy_redirectors']:
        pkg = r['forbidden_projected_new_package']
        assert pkg in final['forbidden_projected_alias_packages_absent']
        assert not (ROOT / 'Content' / (pkg[len('/Game/'):] + '.uasset')).exists()
    archive = Path(m['backup_manifest']); assert sha(archive) == m['backup_manifest_sha256']
    archived = {r['relative_path']: r for r in read(archive)['files']}
    allowed = set(pf['touched_package_names']); assert len(allowed) == 589
    for r in delta['changed_or_missing_original_files']:
        backup = archived[r['relative_path']]
        assert r['package'] in allowed and r['before_sha256'] == backup['sha256'] and Path(backup['backup']).is_file()
        path = ROOT / r['relative_path']
        assert (sha(path) if path.exists() else None) == r['after_sha256']
    assert len(delta['changed_or_missing_original_files']) == 589
    assert len(delta['created_canonical_files']) == 582
    partial_info = read(INTEGRATION / 'partial-recovery-backup.json'); partial_path = Path(partial_info['manifest'])
    assert sha(partial_path) == partial_info['sha256'] and len(read(partial_path)['files']) == partial_info['count'] == 612
    eight = payload_eight(m, read(partial_path))
    four = read(TEAM / 'revisions/v4-partial-importdata-review/four-texture-independent-audit.json')
    assert four['independent_pixel_checks_passed'] == 4 and all(r['rgba_pixels_equal'] and r['alpha_equal'] for r in four['findings'])
    recovery = read(INTEGRATION / 'migration-recovery-continuation-result.json')
    assert recovery['passed'] and recovery['engine_pid'] == 47604 and recovery['dirty_packages'] == [] and recovery['native_rename_not_repeated']
    old_intent = Path(journal['recovery_evidence']) / 'engine-migration-journal.json'
    assert read(old_intent)['state'] == 'before_rename'
    receipt = read(INTEGRATION / 'live-diagnostic-bridge/receipt.json')
    assert not receipt['restore_required'] and sha(TEAM / 'migrate_ui_ue.py') == receipt['owner_before_sha256']
    adaptation = read(INTEGRATION / 'migration-final-verification-api-adaptation.json')
    original = (TEAM / 'migrate_ui_ue.py').read_text(encoding='utf-8-sig')
    adapted = (INTEGRATION / 'migration-final-verification-execution.py').read_text(encoding='utf-8-sig')
    assert original.count(adaptation['before']) == 1 and original.replace(adaptation['before'], adaptation['after']) == adapted
    resolved = read(TEAM / 'reference-replacement-plan.json')['resolved_package_replacements']
    static = static_paths(resolved)
    (HERE / 'canonical-source-config-audit.json').write_text(json.dumps(static, ensure_ascii=False, indent=2), encoding='utf-8')
    new_paths = sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / 'Content/newUI').rglob('*.uasset'))
    mapped_paths = {r['destination_relative'] for r in m['assets']}
    report = dict(started_at=started, checked_at=datetime.now(timezone(timedelta(hours=8))).isoformat(),
        directory_migration_complete=True, migration_complete=True, old_directory_exists=False,
        engine_report_old_registry_count=0, migrated_current_files_hash_verified=582,
        nonredirector_classes_verified_via_engine_report_and_current_byte_identity=582,
        exact_native_fixup_count=29, preexisting_redirectors_resolved=6,
        changed_original_packages_covered_by_verified_archive=589, original_archive_manifest_sha256=m['backup_manifest_sha256'],
        partial_archive_manifest_sha256=partial_info['sha256'], full1759_or612_archive_rehashed=False,
        original_full_archive_count=len(archived), partial_archive_count=612,
        current_newUI_uasset_count=len(new_paths), post_migration_extra_files=sorted(set(new_paths) - mapped_paths),
        four_texture_pixel_proof_reused=True, eight_copy_payload_proof=eight,
        source_config_canonical_static_passed=static['passed'], source_config_findings=static['problems'],
        original_owner_script_restored=True, original_failed_intent_preserved=True,
        final_execution_adapter_exact_single_api_replacement=True,
        source_config_and_new_assets_rollback_scope='Original archive/journal covers588old packages and589rewritten originals/582newmigration targets. Later4newPlayer textures+RuntimeWBP and laterSourceConfig epochs are outside original recovery plan; checkpoint them before any future rollback. No rollback performed.',
        full_mounted_dependency_negative_readback_not_supplied=True,
        saved_reference_evidence_scope='Same-PID native rename/continuation dirty0, exact29 redirectors incoming0,589actual rewrite whitelist/archive coverage,current file hashes. No separate exhaustive mounted dependency graph report invented.',
        new_default_ui_full_pie_acceptance=False, ui_visual_approval='pending_user',
        evidence=[dict(file=n, sha256=sha(TEAM / n)) for n in names], current_outputs=outputs)
    (HERE / 'final-owner-audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k not in {'evidence','current_outputs','eight_copy_payload_proof'}}, ensure_ascii=False, indent=2))


if __name__ == '__main__': main()
