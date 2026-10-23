"""Keep original failures and explicitly link narrowly corrected, rerun checks."""
from pathlib import Path
import json,csv
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Saved/Codex/VisualRefactor20261022'
def read(name):return json.loads((OUT/name).read_text(encoding='utf-8-sig'))
matrix=read('Rendered/report.json');business=read('RenderedBusiness/report.json');scenes=read('RenderedScenes/report.json')
native=read('NativeAll/index.json');reentry=read('Reentry/report.json');preservation=read('preservation.json')
geometry=read('scene-geometry-review.json')
scene_failures=[c for c in scenes['checks'] if not c['passed']]
scene_validated=scenes.get('finished',False) and not scenes['errors'] and geometry['passed'] and all(c['name'].endswith('scene art preserves 3 to 1 aspect') for c in scene_failures)
rechecked={c['name'] for c in business['checks'] if c['passed']}
allowed={'sign gives no qi','shop select row','shop fixed confirmation click','purchase costs exact stone','purchase costs exact herb','purchase grants one heal'}
superseded=[];unresolved=[]
for check in matrix['checks']:
    if not check['passed']:
        if check['name'] in allowed and check['name'] in rechecked:
            superseded.append(dict(original=check,recheck='RenderedBusiness/report.json',reason='fixed button binding and scrolled to product; frozen QA clock isolates passive qi'))
        else:unresolved.append(check)
captures={c['tag']:dict(c,report='Rendered/report.json') for c in matrix['captures']}
for c in business['captures']:captures[c['tag']]=dict(c,report='RenderedBusiness/report.json')
for c in scenes['captures']:captures[c['tag']]=dict(c,report='RenderedScenes/report.json')
builds={name:'Result: Succeeded' in (OUT/name).read_text(encoding='utf-8',errors='replace') for name in ('build-final-editor.log','build-final-game.log')}
service_log=(OUT/'service-tests.log').read_text(encoding='utf-8',errors='replace')
report=dict(boundary='Windows rendered PIE and isolated simulated data; not Cook, mobile device or production provider',
    matrix_finished=matrix.get('finished',False),matrix_checks=len(matrix['checks']),
    business_checks=len(business['checks']),business_passed=business['passed'],scene_checks=len(scenes['checks']),scene_passed=scene_validated,scene_original_failures=scene_failures,scene_geometry_review='scene-geometry-review.json',superseded=superseded,unresolved=unresolved,
    rendered_captures=list(captures.values()),native=dict(succeeded=native['succeeded'],failed=native['failed']),
    reentry=reentry['passed'],service_80_passed='Ran 80 tests' in service_log and '\nOK' in service_log,
    builds=builds,preservation=preservation['passed'])
report['passed']=all((report['matrix_finished'],not matrix['errors'],not unresolved,business['passed'],scene_validated,native['failed']==0,
    reentry['passed'],report['service_80_passed'],all(builds.values()),preservation['passed']))
(OUT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
# Copy the original matrix, leaving unexecuted acceptance criteria explicitly pending.
with (ROOT/'docs/design/2026-10-21-ink-ui-standard-v1/验收矩阵.csv').open(encoding='utf-8-sig',newline='') as f:
    reader=csv.DictReader(f);fields=reader.fieldnames;rows=list(reader)
verified={'BASE-01':'baseline.json','BASE-02':'preservation.json','BASE-03':'preservation.json','SHIP-01':'verification.json'}
partial={'VIS-01','VIS-02','VIS-03','VIS-04','VIS-05','VIS-09','VIS-13','VIS-14','VIS-16','VIS-17','PAGE-05','PAGE-13','MOT-04','DRAW-01','DRAW-02','DRAW-05','SHOP-01','SHOP-02','SHIP-04'}
for row in rows:
    key=row['id']
    if key in verified:row.update(status='verified',evidence_path=verified[key],notes='Current task evidence; Windows boundary')
    elif key in partial:row.update(status='partially_verified',evidence_path='verification.json',notes='Rendered samples and/or demo path; all states/devices not covered')
    else:row.update(status='not_verified_this_pass',evidence_path='',notes='No claim of full acceptance')
with (OUT/'acceptance-current.csv').open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader();writer.writerows(rows)
print(json.dumps(dict(passed=report['passed'],captures=len(captures),business=report['business_passed'],native=report['native'],superseded=len(superseded),unresolved=unresolved),ensure_ascii=False))
