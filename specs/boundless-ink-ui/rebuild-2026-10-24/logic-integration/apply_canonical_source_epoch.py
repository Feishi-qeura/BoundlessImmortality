"""Apply reviewed staged33 paths sequentially through the exact source ledger."""
from pathlib import Path
import json,hashlib,runpy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
final=json.loads((HERE.parent/'migration/engine-final-verification.json').read_text(encoding='utf-8-sig'))
assert final['passed'] and not (ROOT/'Content/UI').exists()
review=json.loads((HERE/'canonical-source-stage-review.json').read_text(encoding='utf-8'))
assert not review['unresolved'] and len(review['files'])==33
ledger=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='integration_recorder')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for row in review['files']:
    target=ROOT/row['path'];stage=Path(row['staged'])
    assert sha(target)==row['before_sha256'] and sha(stage)==row['staged_sha256']
for row in review['files']:
    ledger['begin'](row['path'],'Canonical UI references after verified582 migration; finite dynamic asset names and exact CoreRedirects',{'stage_review_sha256':sha(HERE/'canonical-source-stage-review.json'),'staged_sha256':row['staged_sha256']})
    (ROOT/row['path']).write_bytes(Path(row['staged']).read_bytes())
    ledger['finish'](row['path'])
print(json.dumps({'applied':len(review['files']),'old_directory_exists':(ROOT/'Content/UI').exists()}))
