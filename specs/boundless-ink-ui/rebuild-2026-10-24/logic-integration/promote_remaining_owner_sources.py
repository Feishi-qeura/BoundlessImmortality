from pathlib import Path
import json,hashlib,runpy,shutil
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
manifest=read(HERE/'remaining-source-stage-manifest.json');assert manifest['staged_files']==14
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='remaining_owner_recorder')
for row in manifest['files']:
    assert not (ROOT/row['path']).exists(),'Unknown existing Source target; preserve before promotion'
    for path,expected in ((row['source'],row['source_sha256']),(row['receipt'],row['receipt_sha256']),(row['staged'],row['staged_sha256'])):assert sha(Path(path))==expected
    for patch in row['patches']:assert sha(Path(patch['path']))==patch['sha256']
for row in manifest['files']:
    metadata={key:row[key] for key in ('owner','revision','source','source_sha256','receipt','receipt_sha256','staged_sha256','patches')}
    change=rec['begin'](row['path'],'Promote complete reviewed owner source with exact minimal Player confirmation/display patches',metadata)
    archive=rec['HERE']/change['backup'];shutil.copy2(row['source'],archive/'owner-stage');shutil.copy2(row['receipt'],archive/'owner-resolution')
    for i,patch in enumerate(row['patches']):shutil.copy2(patch['path'],archive/('owner-patch-'+str(i)))
    target=ROOT/row['path'];target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(Path(row['staged']).read_bytes());rec['finish'](row['path'])
print('Promoted14 reviewed Journey/Activity/Economy/Player source files; mounting and compilation still required')
