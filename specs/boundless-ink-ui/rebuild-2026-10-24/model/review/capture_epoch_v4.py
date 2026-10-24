"""Capture new ready inputs and source deltas for bounded read-only review."""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent
BASE=HERE.parents[1]
ROOT=HERE.parents[4]
prior=json.loads((HERE/'source-delta-v3.json').read_text(encoding='utf-8'))
old={r['path']:r for r in prior['sources']}
paths={ROOT/p for p in old}
for folder in ('panel-activities-02','panel-economy-03','panel-player/player-d','panel-player/player-e','panel-player/review-v4','panel-journey/review-v4'):
    for ext in ('*.cpp','*.h','*.diff','*.patch','handoff.md','status.json','receipt.md','action-adapter.yaml'):
        paths.update((BASE/folder).rglob(ext))
for ext in ('*.cpp','*.h'):
    paths.update((ROOT/'Source/BoundlessImmortality/Frontend/UIRebuild20261024').rglob(ext))
rows=[]
for p in sorted(paths):
    data=p.read_bytes();key=p.relative_to(ROOT).as_posix();digest=hashlib.sha256(data).hexdigest()
    same=key in old and old[key]['sha256']==digest
    if not same:
        target=HERE/'source-snapshot-v4'/key;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    rows.append(dict(path=key,sha256=digest,previous_sha256=old[key]['sha256'] if key in old else None,
        delta='unchanged' if same else 'changed' if key in old else 'new-input',
        snapshot=old[key]['snapshot'] if same else target.relative_to(HERE).as_posix()))
(HERE/'source-delta-v4.json').write_text(json.dumps(dict(captured_utc=datetime.now(timezone.utc).isoformat(),evidence='source-only',sources=rows),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(inputs=len(rows),changed=sum(r['delta']=='changed' for r in rows),new=sum(r['delta']=='new-input' for r in rows))))
