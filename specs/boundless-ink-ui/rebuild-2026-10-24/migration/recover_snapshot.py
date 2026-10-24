"""Plan or restore ORIGINAL authored package bytes from the migration's write journal.

Recovery is distinct from migration: it restores exact original paths/bytes, never
pretends an OS move has renamed a UE package. All Unreal/LiveCoding must be stopped.
No Source/Config restoration is automatic; integration reviews those archived diffs.
"""
import argparse
import csv
import json
import os
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path
import prepare_migration as prep

OUT, ROOT = prep.OUT, prep.ROOT


def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def checked_original(relative):
    p = (ROOT / relative).resolve()
    if not (p.is_relative_to((ROOT / 'Content').resolve()) or
            p.is_relative_to((ROOT / 'Plugins').resolve()) and 'Content' in p.relative_to(ROOT / 'Plugins').parts):
        raise RuntimeError('Recovery original outside Content/project plugin Content.')
    if p.suffix not in {'.uasset', '.umap', '.uexp', '.ubulk', '.uptnl'}:
        raise RuntimeError('Recovery only supports exact asset package files.')
    return p


def make_plan():
    m = read(OUT / 'migration-map.json')
    journal = read(OUT / 'asset-write-journal.json')
    if journal['snapshot'] != m['backup_manifest']:
        raise RuntimeError('Write journal is for a different snapshot.')
    manifest = read(Path(m['backup_manifest']))
    indexed = {r['relative_path']: r for r in manifest['files']}
    destinations = {r['destination_relative'] for r in m['assets']}
    actions = []
    for row in journal['changed_or_missing_original_files']:
        p = checked_original(row['relative_path'])
        original = indexed[row['relative_path']]
        if original['sha256'] != row['before_sha256'] or prep.sha(Path(original['backup'])) != original['sha256']:
            raise RuntimeError('Recovery backup hash mismatch.')
        current = prep.sha(p) if p.exists() else None
        if current != row['after_sha256']:
            raise RuntimeError('Original changed since migration journal; stop and review: ' + str(p))
        actions.append(dict(action='restore_original_bytes', relative_path=row['relative_path'],
            archive=original['backup'], original_sha256=original['sha256'], current_sha256=current))
    for row in journal['created_canonical_files']:
        p = checked_original(row['relative_path'])
        canonical_uasset = p.with_suffix('.uasset').relative_to(ROOT).as_posix()
        if canonical_uasset not in destinations:
            raise RuntimeError('Journal destination outside exact mapping.')
        if not p.exists() or prep.sha(p) != row['sha256']:
            raise RuntimeError('New canonical changed since migration journal; preserve it and review.')
        actions.append(dict(action='archive_then_remove_exact_new_file', relative_path=row['relative_path'],
                            current_sha256=row['sha256']))
    if not actions:
        raise RuntimeError('No journaled changes to recover.')
    plan = dict(created_at=prep.now(), migration_map_sha256=prep.sha(OUT / 'migration-map.json'),
        write_journal_sha256=prep.sha(OUT / 'asset-write-journal.json'),
        backup_manifest_sha256=prep.sha(Path(m['backup_manifest'])), actions=actions,
        source_config_restore='Manual reviewed integration-only operation using archived originals',
        engine_must_be_stopped=True)
    prep.save(OUT / 'recovery-plan.json', plan)
    print('RECOVERY_PLAN', prep.sha(OUT / 'recovery-plan.json'), len(actions), 'actions; no assets changed')


def execute(expected_hash):
    if prep.sha(OUT / 'recovery-plan.json') != expected_hash:
        raise RuntimeError('Exact recovery-plan fingerprint required.')
    lease = read(OUT.parent / 'engine-lease.json')
    if (lease.get('owner_team') != 'logic-integration' or lease.get('state') not in {'granted', 'active'}
        or 'asset_root_migration' not in lease.get('allowed_operations', [])
        or datetime.fromisoformat(lease['expires_at']) <= datetime.now(timezone.utc)):
        raise RuntimeError('Recovery requires valid integration migration coordination lease.')
    # Read-only process inspection, never kills or closes user work.
    result = subprocess.run(['tasklist', '/fo', 'csv', '/nh'], capture_output=True, text=True, check=True)
    blocked = [r[0] for r in csv.reader(result.stdout.splitlines()) if r and
               (r[0].lower().startswith('unrealeditor') or r[0].lower() == 'livecodingconsole.exe')]
    if blocked:
        raise RuntimeError('Preserve user work; stop owned engine normally before recovery: ' + repr(blocked))
    plan = read(OUT / 'recovery-plan.json')
    if prep.sha(OUT / 'asset-write-journal.json') != plan['write_journal_sha256']:
        raise RuntimeError('Write journal drift.')
    # Validate every path and current byte state BEFORE the first restoration.
    for a in plan['actions']:
        p = checked_original(a['relative_path'])
        current = prep.sha(p) if p.exists() else None
        if current != a['current_sha256']:
            raise RuntimeError('Recovery state drift: ' + str(p))
        if a['action'] == 'restore_original_bytes' and prep.sha(Path(a['archive'])) != a['original_sha256']:
            raise RuntimeError('Archive drift.')
    recovery_archive = ROOT / 'Saved/Codex/UIRebuild20261024/Backups' / ('RecoveryBefore-' + datetime.now().strftime('%Y%m%dT%H%M%S%f'))
    for a in plan['actions']:
        p = checked_original(a['relative_path'])
        if p.exists():
            archived = recovery_archive / a['relative_path']
            archived.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(p, archived)
            if prep.sha(archived) != a['current_sha256']:
                raise RuntimeError('Could not preserve post-migration bytes before recovery.')
    for a in plan['actions']:
        p = checked_original(a['relative_path'])
        if a['action'] == 'restore_original_bytes':
            p.parent.mkdir(parents=True, exist_ok=True)
            temp = p.with_suffix(p.suffix + '.recovery-tmp')
            shutil.copy2(a['archive'], temp)
            if prep.sha(temp) != a['original_sha256']:
                raise RuntimeError('Recovery temp hash mismatch.')
            os.replace(temp, p)
        elif a['action'] == 'archive_then_remove_exact_new_file':
            # Single mapped file only, already hash-checked and archived; never recursive delete.
            p.unlink()
        else:
            raise RuntimeError('Unknown action.')
    prep.save(OUT / 'recovery-result.json', dict(checked_at=prep.now(), passed=True,
        exact_actions=len(plan['actions']), recovery_before_archive=str(recovery_archive),
        source_config_restored=False, requires_fresh_registry_and_reference_revalidation=True))
    print('EXACT_ASSET_RECOVERY_COMPLETED; integration must review source/config rollback and fresh registry')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode', choices=['plan', 'execute'])
    p.add_argument('--confirm-plan-sha256')
    a = p.parse_args()
    if a.mode == 'plan':
        make_plan()
    elif not a.confirm_plan_sha256:
        p.error('--confirm-plan-sha256 required for exact operator-reviewed recovery plan')
    else:
        execute(a.confirm_plan_sha256)
