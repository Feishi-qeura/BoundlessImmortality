"""Read-only source capture for bounded incremental reviews; no engine operations."""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
BASE = HERE.parents[1]
old = json.loads((HERE / 'source-delta-v2.json').read_text(encoding='utf-8'))
previous = {row['path']: row for row in old['sources']}
paths = {ROOT / p for p in previous}
for team in ('panel-rank', 'panel-activities', 'panel-journey'):
    for folder in ('review-v3', 'staged-source'):
        paths.update((BASE / team / folder).rglob('*.cpp'))
        paths.update((BASE / team / folder).rglob('*.h'))
paths.update((BASE / 'panel-player').rglob('*.cpp'))
paths.update((BASE / 'panel-player').rglob('*.h'))
paths.update((ROOT / 'Source/BoundlessImmortality/Frontend/UIRebuild20261024').rglob('*.cpp'))
paths.update((ROOT / 'Source/BoundlessImmortality/Frontend/UIRebuild20261024').rglob('*.h'))
rows = []
for path in sorted(paths):
    data = path.read_bytes()
    key = path.relative_to(ROOT).as_posix()
    digest = hashlib.sha256(data).hexdigest()
    prior = previous.get(key)
    same = prior is not None and prior['sha256'] == digest
    target = HERE / 'source-snapshot-v3' / key
    if not same:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    rows.append(dict(path=key, sha256=digest,
                     previous_sha256=prior['sha256'] if prior else None,
                     delta='unchanged' if same else 'changed' if prior else 'new-input',
                     snapshot=prior['snapshot'] if same else target.relative_to(HERE).as_posix()))
result = dict(captured_utc=datetime.now(timezone.utc).isoformat(), evidence='source-only', sources=rows)
(HERE / 'source-delta-v3.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print(json.dumps(dict(files=len(rows), changed=sum(r['delta']=='changed' for r in rows), new=sum(r['delta']=='new-input' for r in rows))))
