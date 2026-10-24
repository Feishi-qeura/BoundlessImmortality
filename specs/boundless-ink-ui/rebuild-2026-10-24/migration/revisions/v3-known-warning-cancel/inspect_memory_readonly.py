"""Integration may run this in its owned Editor; no asset load/save/rename or shutdown.

Writes only a timestamped report next to this independent revision. Does not alter
the active migration script, controller, journals or retry state.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import hashlib
import json
import os

HERE = Path(__file__).resolve().parent
TEAM = HERE.parents[1]
ROOT = TEAM.parents[3]


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    import unreal
    if Path(unreal.Paths.project_dir()).resolve() != ROOT:
        raise RuntimeError('Wrong project')
    m = json.loads((TEAM / 'migration-map.json').read_text(encoding='utf-8-sig'))
    if m['schema_version'] != 2:
        raise RuntimeError('Exact schema2 map required')
    dirty = [str(p.get_name()) for p in list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
             + list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())]
    rows, errors = [], []
    for r in m['assets'] + m['legacy_redirectors']:
        row = dict(source_kind=r['source_kind'], expected_old=r['old_object'], expected_new=r['new_object'])
        for label, path in (('old_loaded_object', r['old_object']), ('new_loaded_object', r['new_object'])):
            try:
                obj = unreal.find_object(None, path)  # Find only, never load.
                row[label] = None if obj is None else dict(path=obj.get_path_name(), asset_class=obj.get_class().get_path_name())
            except Exception as exc:
                errors.append(dict(path=path, error=str(exc)))
        if r['live_class'] == '/Script/UMGEditor.WidgetBlueprint':
            obj = unreal.find_object(None, r['new_object'] + '_C')
            row['new_loaded_generated_class'] = None if obj is None else obj.get_path_name()
        original = ROOT / r['source_relative']
        row['disk_source_exists'] = original.exists()
        row['disk_source_hash_matches_snapshot'] = original.exists() and sha(original) == r['sha256']
        row['disk_real_canonical_exists'] = (ROOT / r['destination_relative']).exists()
        rows.append(row)
    report = dict(checked_at=datetime.now(timezone(timedelta(hours=8))).isoformat(), engine_pid=os.getpid(),
        read_only=True, asset_loads_performed=False, dirty_packages=dirty, diagnostic_errors=errors,
        map_sha256=sha(TEAM / 'migration-map.json'), source_rows=rows,
        new_loaded_objects=sum(bool(r.get('new_loaded_object')) for r in rows),
        new_loaded_generated_classes=sum(bool(r.get('new_loaded_generated_class')) for r in rows),
        old_loaded_objects=sum(bool(r.get('old_loaded_object')) for r in rows),
        source_disk_drift=sum(not r['disk_source_hash_matches_snapshot'] for r in rows),
        note='Facts only. Unloaded objects are not proof of original memory identity; no automatic retry/close approval.')
    target = HERE / ('memory-readonly-' + str(os.getpid()) + '-' + datetime.now().strftime('%Y%m%dT%H%M%S%f') + '.json')
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('READONLY_RENAME_MEMORY_DIAG ' + str(target))


if __name__ == '__main__':
    main()
