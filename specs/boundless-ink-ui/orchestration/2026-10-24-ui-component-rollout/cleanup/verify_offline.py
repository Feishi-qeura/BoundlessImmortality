"""Verify offline delivery and safety gates without importing or launching Unreal."""
import ast
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

OUT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('cleanup_audit', OUT / 'audit_legacy.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def main():
    checks = []
    for p in sorted(OUT.glob('*.py')):
        ast.parse(p.read_text(encoding='utf-8'), filename=str(p))
    checks.append('All preparation Python scripts parse')
    base = audit.read('baseline.json')
    decisions = audit.read('asset-decisions.json')['assets']
    summary = audit.read('audit-summary.json')
    assert base['asset_count'] == len(decisions) == 583
    assert base['before_new_component_assets'] and not base['new_component_files_present_at_capture']
    assert len({r['package'] for r in decisions}) == len(decisions)
    assert not any(r['zero_use_proven'] or r['deletion_authorized'] for r in decisions)
    assert all(r['decision'].startswith(('Retain', 'UnknownRetain')) for r in decisions)
    assert not summary['scan_errors'] and not summary['baseline_drift']
    assert summary['exact_six_current'] == dict(legacy_absent=6, canonical_matches=6, backup_matches=12)
    checks.append('583 baseline packages retained; six removed redirectors not reintroduced; 12 prior backups match')
    # Check every source file again at delivery, not just the earlier scan result.
    assert all(audit.digest(audit.ROOT / r['relative_path']) == r['sha256'] for r in base['files'])
    checks.append('Fresh SHA256 check: all 583 old assets still match immutable baseline')
    before = audit.digest(OUT / 'baseline.json')
    repeat = subprocess.run([sys.executable, str(OUT / 'audit_legacy.py'), 'baseline'], capture_output=True, text=True)
    assert repeat.returncode != 0 and 'Baseline already exists' in repeat.stderr
    assert audit.digest(OUT / 'baseline.json') == before
    checks.append('Repeated baseline capture refuses overwrite')
    registry = subprocess.run([sys.executable, str(OUT / 'registry_audit_ue.py')], capture_output=True, text=True)
    assert registry.returncode != 0 and 'Requires coordinator-issued' in registry.stderr
    assert 'ModuleNotFoundError' not in registry.stderr
    checks.append('Registry script rejects current ungranted lease before Unreal import')
    invalid = OUT / 'verification-invalid-packages.json'
    invalid.write_text(json.dumps(['/Game/UI/Ink/ComponentRollout20261024/Forbidden']), encoding='utf-8')
    bad_backup = subprocess.run([sys.executable, str(OUT / 'audit_legacy.py'), 'backup', '--packages', str(invalid)],
                                capture_output=True, text=True)
    assert bad_backup.returncode != 0 and 'outside immutable baseline' in bad_backup.stderr
    checks.append('Backup refuses any package outside immutable old scope')
    selection = OUT / 'verification-backup-packages.json'
    selection.write_text(json.dumps(['/Game/UI/Ink/Materials/Leaderboard/M_InkLeaderboard']), encoding='utf-8')
    copied = subprocess.run([sys.executable, str(OUT / 'audit_legacy.py'), 'backup', '--packages', str(selection)],
                            capture_output=True, text=True, check=True)
    manifest_path = Path(copied.stdout.strip())
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    assert manifest['verified'] and not manifest['deletion_authorized']
    assert len(manifest['files']) >= 3  # Asset plus config and resolver contracts.
    assert all(audit.digest(Path(r['source'])) == audit.digest(Path(r['backup'])) == r['sha256']
               for r in manifest['files'])
    checks.append('Real copy-and-hash backup smoke check passes; no source asset changes')
    result = dict(checked_at=audit.now(), checks=checks, passed=len(checks), failures=0,
                  engine_started=False, engine_registry_verified=False, assets_mutated=False,
                  backup_smoke_manifest=str(manifest_path),
                  registry_guard_expected_error=registry.stderr.strip().splitlines()[-1])
    audit.save('offline-verification.json', result)
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
