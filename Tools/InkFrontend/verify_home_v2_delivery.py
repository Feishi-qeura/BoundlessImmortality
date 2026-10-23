"""Post-integration evidence and preservation checks; never substitutes for PIE."""
from pathlib import Path
import csv,hashlib,json,subprocess,sys,zipfile
import yaml
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Saved/Codex/HomeV2Integration20261022';SPEC=ROOT/'specs/boundless-ink-ui'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text('utf-8-sig'))
R=dict(checks=[],passed=False,boundaries={'design_package':'checked separately','build':'read build-final.log','pie':'read actual PIE/report.json','cook':'not-run','device':'not-run','production_provider':'not-run'})
def ck(n,v,a=None):R['checks'].append(dict(name=n,passed=bool(v),actual=a))
rows=read(ROOT/'Tools/InkFrontend/home_v2_imports.json')
for r in rows:
 ck('source bytes '+r['id'],sha(Path(r['source']))==r['sha256'])
 ck('uasset exists '+r['id'],(ROOT/'Content'/r['asset'].removeprefix('/Game/')).with_suffix('.uasset').is_file())
imported=read(OUT/'import-report.json');ck('UE import completed',imported['passed'] and len(imported['assets'])==18)
ck('final build success','Result: Succeeded' in (OUT/'build-final.log').read_text('utf-8-sig'))
pie=read(OUT/'PIE/report.json');ck('actual PIE completed',pie['finished'] and pie['passed'])
post=read(OUT/'PostCleanupPIE/report.json');ck('post-cleanup PIE completed',post['finished'] and post['passed'])
ck('post-cleanup PIE matches source',all(sha(ROOT/p)==h for p,h in post['source_hashes'].items()))
native=read(OUT/'NativeRegressionFinal/index.json');ck('post-cleanup native regression',native['failed']==0 and native['succeeded']>0,dict(passed=native['succeeded'],warnings=native['succeededWithWarnings'],failed=native['failed']))
ck('PIE matches final source',all(sha(ROOT/p)==h for p,h in pie['source_hashes'].items()))
for cap in pie['captures']:ck('actual screenshot '+Path(cap).name,Path(cap).is_file())
rank=[x for x in pie.get('art_geometry',[]) if x['component']=='home-rank-paper']
ck('both rank rendering branches observed',{x['mode'] for x in rank}=={'full-Fit','height-locked-X-nine-slice'})
ck('rank actual text and hit geometry',all(x['caption_inside_art'] and x['hit_inside_art'] for x in rank))
baseline=read(OUT/'baseline.json')
allowed={f'Source/BoundlessImmortality/Frontend/{n}' for n in ['InkFrontendWidget.cpp','InkHudLayout.cpp','InkAnchorStatusWidget.cpp','InkAnchorStatusWidget.h','InkLeaderboardWidget.cpp','InkNiagaraTitleWidget.cpp']}
allowed.add('Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp')
preserve=[];changed=[]
for x in baseline:
 rel=x['path'];p=ROOT/rel
 if rel.startswith(('Source/','Config/','Content/UI/')) or rel=='BoundlessImmortality.uproject':
  ok=p.is_file() and sha(p)==x['sha256']
  if rel in allowed:changed.append(rel)
  else:preserve.append(dict(path=rel,sha256=x['sha256'],unchanged=ok))
ck('unrelated source config and UI assets preserved',all(x['unchanged'] for x in preserve),len(preserve))
R['authorized_existing_source_changes']=changed;R['preserved']=preserve
visual=read(OUT/'visual-review.json');ck('observed pixel review',visual['passed'] and all(sha(Path(x['path']))==x['sha256'] for x in visual['images']))
cleanup=read(OUT/'cleanup-result.json')
for x in cleanup['items']:
 ck('archive and removal '+x['design_relative'],not Path(x['original']).exists() and sha(Path(x['archive']))==x['sha256'])
 if x['archive'].endswith('.zip'):
  with zipfile.ZipFile(x['archive']) as z:ck('archived ZIP CRC '+Path(x['archive']).name,z.testzip() is None)
external=Path(cleanup['archive'])
ck('external baseline remains fully recoverable',all(sha(external/'baseline'/x['path'])==x['sha256'] for x in baseline),len(baseline))
restore=read(OUT/'restore-probe-report.json');ck('actual restore drill',restore['passed'] and restore['verified_files']==8)
audit=read(OUT/'ue-reference-audit.json');ck('UE hard soft manage and redirector audit',audit['passed'],audit['assets_scanned'])
ck('no redirectors',not audit['redirectors'])
# Explicit pending list; zero registry references never authorizes uasset deletion.
pending=[]
for x in audit['assets']:
 if '/HomeV2/' in x['asset']:continue
 reason='Shared runtime/dynamic source reference or packaging rule requires preservation' if x['dynamic_or_parent_path_sources'] or x['hard_soft_manage_referencers'] else 'Purpose/dynamic Blueprint string loading not exhaustively proven; also covered by UI cook rule'
 pending.append(dict(asset=x['asset'],asset_class=x['asset_class'],referencers=len(x['hard_soft_manage_referencers']),source_references=len(x['dynamic_or_parent_path_sources']),decision='retain',reason=reason))
with (OUT/'pending-cleanup.csv').open('w',encoding='utf-8-sig',newline='') as f:
 writer=csv.DictWriter(f,fieldnames=['asset','asset_class','referencers','source_references','decision','reason']);writer.writeheader();writer.writerows(pending)
screens=yaml.safe_load((SPEC/'contracts/screen-contract.yaml').read_text('utf-8'))['screens']
(OUT/'other-pages-pending.json').write_text(json.dumps([dict(id=s['id'],name=s.get('name'),new_home_pack_assets_delivered=False,scope='Existing implementation preserved; no new page artwork delivered in this batch') for s in screens if s['id']!='home'],ensure_ascii=False,indent=2),'utf-8')
active=yaml.safe_load((SPEC/'contracts/asset-manifest.yaml').read_text('utf-8'))
for a in active['assets']:
 if a.get('status') in ['generated','approved']:ck('active asset path resolves '+a['id'],(SPEC/a['path']).is_file())
with zipfile.ZipFile(SPEC/'packages/home-components-trim-v2-png.zip') as z:
 ck('immutable approved v2 ZIP integrity',z.testzip() is None)
 ck('immutable v2 package bytes retained',sha(SPEC/'packages/home-components-trim-v2-png.zip')=='178851e9e1e935243216b57509af5d19c30863593c60a2c3964263f7642cafcb')
R['passed']=all(x['passed'] for x in R['checks']);R['cleanup_files']=len(cleanup['items']);R['cleanup_bytes']=sum(x['bytes'] for x in cleanup['items']);R['uasset_removed']=0;R['pending_assets']=len(pending)
(OUT/'delivery-validation.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),'utf-8')
print(json.dumps({k:R[k] for k in ['passed','cleanup_files','cleanup_bytes','uasset_removed','pending_assets']}));print('checks',len(R['checks']),'failed',[x['name'] for x in R['checks'] if not x['passed']]);sys.exit(0 if R['passed'] else 1)
