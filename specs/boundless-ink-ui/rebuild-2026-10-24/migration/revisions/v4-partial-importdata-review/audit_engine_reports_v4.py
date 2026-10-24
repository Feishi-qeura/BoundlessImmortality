"""Offline actual-attempt auditor; never treats a passed live-bridge verify as final.

Requires original five engine reports PLUS raw-pin and actual native-rename result.
Uses existing verified archive manifest identities; does not repeat whole archive hashes.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import argparse
import hashlib
import importlib.util
import json
import sys

HERE = Path(__file__).resolve().parent
TEAM = HERE.parents[1]
ROOT = TEAM.parents[3]
INTEGRATION = TEAM.parent / 'logic-integration'


def read(path): return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(8 * 1024 * 1024), b''): h.update(b)
    return h.hexdigest()


def main(allow_pending):
    required = ['engine-preflight.json', 'engine-legacy-pin-result.json', 'engine-rename-result.json',
                'engine-migration-journal.json', 'engine-fixup-result.json', 'asset-write-journal.json',
                'engine-final-verification.json']
    present = {name: read(TEAM / name) for name in required if (TEAM / name).exists()}
    missing = [name for name in required if name not in present]
    actual_rename = present.get('engine-rename-result.json', {})
    now = datetime.now(timezone(timedelta(hours=8))).isoformat()
    common = dict(checked_at=now, reports_present=list(present), missing_reports=missing,
        actual_rename582_verified=actual_rename.get('passed') is True and actual_rename.get('native_rename_succeeded') is True and actual_rename.get('exact_count') == 582,
        old_directory_exists=(ROOT / 'Content/UI').exists(), engine_actions=False, content_writes=False,
        original1759_or_partial612_archive_rehashed=False)
    if missing:
        common.update(status='PARTIAL_MIGRATION_FINAL_PROOF_PENDING', migration_complete=False,
                      warning='Rename already succeeded. Do not retry migrate or treat controller verify passed as machine final proof.')
        (HERE / 'actual-engine-report-audit.json').write_text(json.dumps(common, ensure_ascii=False, indent=2), encoding='utf-8')
        print(json.dumps(common, ensure_ascii=False, indent=2))
        return 0 if allow_pending else 2
    m = read(TEAM / 'migration-map.json')
    pf, pin, ren, intent, fx, delta, final = [present[x] for x in required]
    failures = []
    expected_pid = ren.get('engine_pid')
    if not (pf.get('passed') and pin.get('passed') and ren.get('passed') and fx.get('passed') and final.get('passed')):
        failures.append('One or more actual engine phases did not pass')
    if not common['actual_rename582_verified'] or ren.get('rejected_unknown_dialog_count') != 0:
        failures.append('Missing actual exact native rename or unknown modal was accepted')
    if (pf.get('source_package_count'), pf.get('canonical_rename_count'), pf.get('preexisting_redirector_count'), pin.get('pinned_count')) != (588, 582, 6, 6):
        failures.append('Preflight or raw-pin scope differs from588/582/6')
    if any(r.get('engine_pid') != expected_pid for r in (pf, pin, intent, fx, final)):
        failures.append('Phase PID mismatch requires explicit continuation provenance, not inferred success')
    if intent.get('state') != 'renamed_complete':
        failures.append('Saved/recovered rename boundary has not been truthfully advanced from preserved before_rename intent')
    if (fx.get('source_scope_count'), fx.get('canonical_destination_count'), final.get('destination_canonical_verified'), final.get('preexisting_redirectors_removed')) != (588, 582, 582, 6):
        failures.append('Exact fixup/final582+6 source coverage mismatch')
    if final.get('old_registry_package_count') != 0 or common['old_directory_exists']:
        failures.append('Old root still exists on disk or registry final proof is nonzero')
    if final.get('migration_map_sha256') != sha(TEAM / 'migration-map.json'):
        failures.append('Exact map fingerprint mismatch')
    classes = {r['old_package']: r['asset_class'] for r in pf['assets']}
    destinations = {r['package']: r for r in final.get('destination_assets', [])}
    for r in m['assets']:
        p = ROOT / r['destination_relative']
        proof = destinations.get(r['new_package'])
        if not proof or not p.exists() or sha(p) != proof['sha256'] or proof['asset_class'] != classes[r['old_package']]:
            failures.append('Saved new canonical hash/class mismatch: ' + r['new_package'])
    for r in m['legacy_redirectors']:
        pkg = r['forbidden_projected_new_package']
        if (ROOT / 'Content' / (pkg[len('/Game/'):] + '.uasset')).exists() or pkg not in final.get('forbidden_projected_alias_packages_absent', []):
            failures.append('A preexisting redirector has a forbidden projected new package: ' + pkg)
    archive = Path(m['backup_manifest'])
    if sha(archive) != m['backup_manifest_sha256']:
        failures.append('Verified original backup manifest changed')
    index = {r['relative_path']: r for r in read(archive)['files']}
    allowed = set(pf['touched_package_names'])
    for r in delta['changed_or_missing_original_files']:
        b = index.get(r['relative_path'])
        if not b or r['package'] not in allowed or b['sha256'] != r['before_sha256'] or not Path(b['backup']).exists():
            failures.append('Actual original-package rewrite lacks exact backup/whitelist provenance: ' + r['relative_path'])
    sys.path.insert(0, str(TEAM))
    spec = importlib.util.spec_from_file_location('existing_source_path_audit', TEAM / 'audit_migration_results.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    residuals = module.source_residuals()
    if residuals: failures.append('Production Source/Config old canonical references remain outside explicit compatibility tables')
    graph_file = HERE / 'post-save-reference-readback.json'
    graph = read(graph_file) if graph_file.exists() else {}
    if not graph.get('passed') or graph.get('migration_map_sha256') != sha(TEAM / 'migration-map.json'):
        failures.append('Post-save mounted reference graph/current canonical class readback missing or failed')
    common.update(status='VERIFIED' if not failures else 'FAILED_OR_INCOMPLETE', migration_complete=not failures,
        failures=failures, production_reference_residuals=residuals, retained_canonical_count=582,
        preexisting_redirector_count=6, rewritten_original_files=len(delta['changed_or_missing_original_files']),
        saved_reference_graph_verified=graph.get('passed', False),
        runtime_default_ui_verified=final.get('runtime_new_default_ui_verified', False),
        new_ui_visual_approval='pending_user',
        note='Archive manifest provenance reused; fresh hashes only for changed current582 canonical outputs. Source pixel/cooked/runtime evidence remains distinct.')
    (HERE / 'actual-engine-report-audit.json').write_text(json.dumps(common, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(common, ensure_ascii=False, indent=2))
    return 0 if not failures else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--allow-pending', action='store_true')
    raise SystemExit(main(p.parse_args().allow_pending))
