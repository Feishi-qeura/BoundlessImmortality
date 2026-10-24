"""Independent offline owner audit of integration's actual engine migration evidence."""
import argparse
import json
import re
from pathlib import Path
import prepare_migration as prep

OUT, ROOT = prep.OUT, prep.ROOT


def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def source_residuals():
    residuals = []
    for base in (ROOT / 'Source/BoundlessImmortality', ROOT / 'Config'):
        for p in base.rglob('*'):
            if not p.is_file() or p.suffix not in {'.cpp', '.h', '.ini'} or 'Tests' in p.parts:
                continue
            text = p.read_text(encoding='utf-8-sig')
            if p.name == 'InkUiAssetPaths.cpp':
                for old, target in re.findall(r'\{TEXT\("([^"]+)"\),TEXT\("([^"]+)"\)\}', text):
                    if target.startswith('/Game/UI/'):
                        residuals.append(dict(file=str(p.relative_to(ROOT)), kind='compatibility_destination_still_old', target=target))
                continue
            for n, line in enumerate(text.splitlines(), 1):
                if '/Game/UI' not in line or line.lstrip().startswith(('//', '*', ';', '#')):
                    continue
                if 'PackageRedirects=' in line:
                    match = re.search(r'OldName="([^"]+)",NewName="([^"]+)"', line)
                    if match and match[1].startswith('/Game/UI/') and not match[2].startswith('/Game/UI/'):
                        continue
                residuals.append(dict(file=str(p.relative_to(ROOT)), line=n, text=line.strip(), kind='runtime_or_config_old_path'))
    return residuals


def main(allow_pending):
    required = ['engine-preflight.json', 'engine-legacy-pin-result.json', 'engine-migration-journal.json', 'engine-fixup-result.json',
                'engine-final-verification.json', 'asset-write-journal.json']
    missing = [x for x in required if not (OUT / x).exists()]
    if missing:
        result = dict(checked_at=prep.now(), status='PENDING_INTEGRATION_EXECUTION', missing=missing,
                      migration_verified=False, old_directory_exists=(ROOT / 'Content/UI').exists())
        prep.save(OUT / 'migration-owner-audit.json', result)
        print(json.dumps(result, indent=2))
        return 0 if allow_pending else 2
    m = read(OUT / 'migration-map.json')
    pf = read(OUT / 'engine-preflight.json')
    fx = read(OUT / 'engine-fixup-result.json')
    final = read(OUT / 'engine-final-verification.json')
    journal = read(OUT / 'asset-write-journal.json')
    archived = read(Path(m['backup_manifest']))
    failures = []
    if (ROOT / 'Content/UI').exists():
        failures.append('Physical Content/UI still exists')
    if not final.get('passed') or final.get('old_registry_package_count') != 0 or not fx.get('passed'):
        failures.append('Engine old-root zero/fixup proof failed')
    if (pf.get('source_package_count') != m['source_package_count']
        or pf.get('canonical_rename_count') != m['count']
        or pf.get('preexisting_redirector_count') != m['preexisting_redirector_count']
        or final.get('destination_canonical_verified') != m['count']
        or final.get('preexisting_redirectors_removed') != m['preexisting_redirector_count']
        or fx.get('helper_revision') != 2 or fx.get('source_scope_count') != m['source_package_count']
        or fx.get('canonical_destination_count') != m['count']):
        failures.append('Exact mapping coverage differs from engine counts')
    if final.get('migration_map_sha256') != prep.sha(OUT / 'migration-map.json'):
        failures.append('Map fingerprint drift')
    classes = {r['old_package']: r['asset_class'] for r in pf['assets']}
    finals = {r['package']: r for r in final.get('destination_assets', [])}
    for r in m['assets']:
        dest = ROOT / r['destination_relative']
        proof = finals.get(r['new_package'])
        if not proof or not dest.exists() or proof['sha256'] != prep.sha(dest) or proof['asset_class'] != classes[r['old_package']]:
            failures.append('Destination current hash/class evidence mismatch: ' + r['new_package'])
    for r in m['legacy_redirectors']:
        pkg = r['forbidden_projected_new_package']
        p = ROOT / 'Content' / (pkg[len('/Game/'):] + '.uasset')
        if p.exists() or pkg not in final.get('forbidden_projected_alias_packages_absent', []):
            failures.append('Preexisting redirector was incorrectly projected as a new canonical: ' + pkg)
    touched = set(pf['touched_package_names'])
    archived_paths = {r['relative_path']: r for r in archived['files']}
    for r in journal['changed_or_missing_original_files']:
        backup = archived_paths.get(r['relative_path'])
        if not backup or r['package'] not in touched or prep.sha(Path(backup['backup'])) != r['before_sha256']:
            failures.append('Uncovered/unexpected changed referencer: ' + r['relative_path'])
    residual = source_residuals()
    if residual:
        failures.append('Production Source/Config still contains unclassified old canonical paths')
    result = dict(checked_at=prep.now(), status='VERIFIED' if not failures else 'FAILED',
        migration_verified=not failures, failures=failures, mapped_count=m['count'],
        source_package_count=m['source_package_count'], preexisting_redirector_count=m['preexisting_redirector_count'],
        old_directory_exists=(ROOT / 'Content/UI').exists(),
        old_registry_package_count=final.get('old_registry_package_count'),
        production_reference_residuals=residual, actual_changed_original_files=len(journal['changed_or_missing_original_files']),
        runtime_new_default_ui_verified=final.get('runtime_new_default_ui_verified', False),
        full_new_ui_visual_system_accepted=False, visual_approval='pending_user')
    prep.save(OUT / 'migration-owner-audit.json', result)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if not failures else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--allow-pending', action='store_true')
    a = p.parse_args()
    raise SystemExit(main(a.allow_pending))
