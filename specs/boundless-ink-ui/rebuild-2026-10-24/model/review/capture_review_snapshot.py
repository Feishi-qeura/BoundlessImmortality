"""Capture read-only inputs under the logic-core owned review directory.

This records bytes/hashes for source review, not compile/runtime evidence.
"""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[5]
BASE = ROOT / 'specs/boundless-ink-ui/rebuild-2026-10-24'
OUT = BASE / 'model/review'
paths = set((ROOT / 'Source/BoundlessImmortality/Frontend/UIRebuild20261024').rglob('*.cpp'))
paths.update((ROOT / 'Source/BoundlessImmortality/Frontend/UIRebuild20261024').rglob('*.h'))
for name in ('InkMenuDirector.cpp', 'InkFrontendWidget.cpp', 'InkFrontendWidget.h',
             'InkFrontendExtensions.cpp', 'InkInventoryWidget.cpp', 'InkInventoryModel.cpp'):
    paths.add(ROOT / 'Source/BoundlessImmortality/Frontend' / name)
for team in ('panel-rank', 'panel-journey', 'panel-activities'):
    for folder in ('staged-source', 'review-v2/staged-source'):
        for suffix in ('*.h', '*.cpp'):
            paths.update((BASE / team / folder).rglob(suffix))
old = json.loads((OUT / 'readiness.json').read_text(encoding='utf-8-sig'))
old_hashes = {}
for row in old['sources']:
    key = row['path'] if row['path'].startswith('Source/') else str((BASE / row['path']).relative_to(ROOT)).replace('\\', '/')
    old_hashes[key] = row['sha256']
rows = []
for path in sorted(paths):
    data = path.read_bytes()
    relative = path.relative_to(ROOT)
    target = OUT / 'source-snapshot-v2' / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    key = relative.as_posix()
    digest = hashlib.sha256(data).hexdigest()
    rows.append(dict(path=key, sha256=digest, previous_sha256=old_hashes.get(key),
                     delta='new-review-input' if key not in old_hashes else 'unchanged' if digest == old_hashes[key] else 'changed',
                     snapshot=target.relative_to(OUT).as_posix()))
result = dict(captured_utc=datetime.now(timezone.utc).isoformat(), runtime_verified=False,
              note='Per-file byte snapshot; not a transactional snapshot of concurrent owner edits.', sources=rows)
(OUT / 'source-delta-v2.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print(json.dumps(dict(files=len(rows), unchanged=sum(r['delta']=='unchanged' for r in rows))))
