"""Preserve current exact migration outputs before narrowly scoped recovery."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,shutil
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];TEAM=HERE.parent/'migration'
def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda:file.read(8*1024*1024),b''):h.update(chunk)
    return h.hexdigest()
mapping=json.loads((TEAM/'migration-map.json').read_text(encoding='utf-8-sig'))
pf=json.loads((TEAM/'engine-preflight.json').read_text(encoding='utf-8-sig'))
assert pf['engine_pid']==47604 and pf['passed'] and mapping['count']==582
paths={ROOT/row['destination_relative'] for row in mapping['assets']}
paths.update(path for path in (ROOT/'Content/UI').rglob('*') if path.is_file())
paths.update(ROOT/row['relative_path'] for row in pf['touched_backup_files'] if (ROOT/row['relative_path']).exists())
folder=ROOT/'Saved/Codex/UIRebuild20261024/PartialBeforeRecovery'/datetime.now().strftime('%Y%m%dT%H%M%S')
folder.mkdir(parents=True)
rows=[]
for path in sorted(paths):
    assert path.resolve().is_relative_to((ROOT/'Content').resolve()) and path.exists()
    relative=path.relative_to(ROOT);target=folder/relative;target.parent.mkdir(parents=True,exist_ok=True)
    before=sha(path);shutil.copy2(path,target)
    assert sha(target)==before and sha(path)==before
    rows.append(dict(relative=str(relative),source_sha256=before,backup=str(target)))
manifest=folder/'manifest.json'
manifest.write_text(json.dumps(dict(created_at=datetime.now(timezone.utc).isoformat(),engine_pid=47604,
    reason='Preserve partial outputs before four exact equal-payload old-to-migrated-target reference reconciliations',
    files=rows),indent=2),encoding='utf-8')
(HERE/'partial-recovery-backup.json').write_text(json.dumps(dict(manifest=str(manifest),sha256=sha(manifest),count=len(rows),verified=True),indent=2),encoding='utf-8')
print(json.dumps(dict(files=len(rows),manifest=str(manifest),verified=True)))
