"""Archive a reviewed exact allowlist after PIE pass. Never operates on uasset."""
from pathlib import Path
import argparse,hashlib,json,re,shutil,zipfile

ROOT=Path(__file__).resolve().parents[2]
SPEC=ROOT/'specs/boundless-ink-ui'
OUT=ROOT/'Saved/Codex/HomeV2Integration20261022'
ARCHIVE=ROOT.parent/'BoundlessImmortality-UIArchive/20261022-home-v2'
FILES={
 'packages/home-components-v1-review.zip':'Superseded component review package; all 18 approved source PNGs and active v2 package retained.',
 'packages/home-components-trim-v1-png.zip':'Superseded metadata v1 package; current approved rendering policy is v4 in pack v2.',
 'evidence/9slice-review-v2/home-top-tile-wide-v2.png':'Recorded failed all-axis slice; replaced by approved whole-image Fit.',
 'evidence/9slice-review-v2/home-top-tile-narrow-v2.png':'Recorded failed all-axis slice; replaced by approved whole-image Fit.',
 'evidence/9slice-review-v2/home-top-tile-tall-v2.png':'Recorded failed all-axis slice; replaced by approved whole-image Fit.',
 'evidence/9slice-review-v2/home-rank-paper-wide-v2.png':'Recorded failed endcap margins; replaced by v3 reviewed wide branch carried into v4.',
 'evidence/9slice-review-v2/home-rank-paper-narrow-v2.png':'Recorded failed slice; replaced by approved v4 narrow Fit.',
 'evidence/slice-correction-v3/home-rank-paper-narrow-v3.png':'Recorded failed center compression; replaced by approved v4 narrow Fit.',
}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def under(p,r):return p.resolve().is_relative_to(r.resolve())
def plan():
 assert not under(ARCHIVE,ROOT)
 if (OUT/'cleanup-result.json').exists():
  return json.loads((OUT/'cleanup-result.json').read_text('utf-8'))
 items=[]
 for rel,reason in FILES.items():
  p=SPEC/rel
  assert under(p,SPEC) and p.suffix in ['.png','.zip']
  items.append(dict(original=str(p),original_relative=p.relative_to(ROOT).as_posix(),design_relative=rel,archive=str(ARCHIVE/'files'/p.relative_to(ROOT)),sha256=sha(p),bytes=p.stat().st_size,reason=reason))
 textrefs=[]
 for p in SPEC.rglob('*'):
  if p.suffix in ['.yaml','.json','.py','.md','.html','.txt']:
   try:t=p.read_text('utf-8-sig')
   except (UnicodeError,OSError):continue
   hits=[x['design_relative'] for x in items if x['design_relative'] in t]
   if hits:textrefs.append(dict(file=p.relative_to(ROOT).as_posix(),dependencies=hits,historical_snapshot='pre-finalization-snapshot' in p.parts))
 result=dict(archive=str(ARCHIVE),items=items,reference_updates=textrefs,approved='evidence/approval-ue-integration-v2-v4.json',uasset_deletions=[],preserve='All current sprite PNGs, source PNGs, effective contracts, current package, approvals and passing evidence; one verified full baseline.',restore='Copy archived files to original_relative under project after hash verification; restore.ps1 has exact paths.')
 (OUT/'cleanup-plan.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),'utf-8')
 return result
def copy_archive():
 if (OUT/'cleanup-result.json').exists():
  d=plan();assert all(sha(Path(x['archive']))==x['sha256'] for x in d['items']);print('Archive already verified and cleanup complete');return
 d=plan()
 pie=json.loads((OUT/'PIE/report.json').read_text('utf-8'))
 assert pie['passed'],'Actual PIE must pass first'
 assert pie.get('source_hashes') and all(sha(ROOT/p)==h for p,h in pie['source_hashes'].items()),'PIE evidence must match current sources'
 assert json.loads((OUT/'ue-reference-audit.json').read_text('utf-8'))['passed']
 ARCHIVE.mkdir(parents=True,exist_ok=True)
 for x in d['items']:
  p=Path(x['original']);dest=Path(x['archive']);dest.parent.mkdir(parents=True,exist_ok=True)
  if dest.exists():assert sha(dest)==x['sha256']
  else:shutil.copy2(p,dest)
  assert sha(dest)==x['sha256']
  if dest.suffix=='.zip':
   with zipfile.ZipFile(dest) as z:assert z.testzip() is None
 # An external verified baseline also retains all original metadata before reference migration.
 baseline=ARCHIVE/'baseline';baseline.mkdir(exist_ok=True)
 records=json.loads((OUT/'baseline.json').read_text('utf-8'))
 for x in records:
  src=OUT/'Backup'/x['path'];dest=baseline/x['path'];dest.parent.mkdir(parents=True,exist_ok=True)
  if not dest.exists():shutil.copy2(src,dest)
  assert sha(dest)==x['sha256']
 shutil.copy2(OUT/'baseline.json',ARCHIVE/'baseline-manifest.json')
 d['archive_verified']=True;d['baseline_verified_files']=len(records);d['working_files_removed']=False
 (ARCHIVE/'archive-manifest.json').write_text(json.dumps(d,ensure_ascii=False,indent=2),'utf-8')
 restore='''param([string]$ProjectRoot = 'D:\\demo\\BoundlessImmortality', [switch]$RestoreBaseline, [switch]$Overwrite)
$ErrorActionPreference='Stop'
$archiveRoot=$PSScriptRoot
$manifest=Get-Content -Raw -LiteralPath (Join-Path $archiveRoot 'archive-manifest.json') | ConvertFrom-Json
$resolvedProject=[IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\\')
$records=if($RestoreBaseline){Get-Content -Raw -LiteralPath (Join-Path $archiveRoot 'baseline-manifest.json') | ConvertFrom-Json}else{$manifest.items}
foreach($entry in $records){
 $relative=if($RestoreBaseline){$entry.path}else{$entry.original_relative}
 $target=[IO.Path]::GetFullPath((Join-Path $resolvedProject $relative))
 if(-not $target.StartsWith($resolvedProject+'\\',[StringComparison]::OrdinalIgnoreCase)){throw 'Target escapes project'}
 $source=if($RestoreBaseline){Join-Path (Join-Path $archiveRoot 'baseline') $relative}else{Join-Path (Join-Path $archiveRoot 'files') $relative}
 if((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLower() -ne $entry.sha256){throw "Archive hash mismatch: $relative"}
 if((Test-Path -LiteralPath $target) -and -not $Overwrite){Write-Host "Preserved existing: $relative";continue}
 New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
 Copy-Item -LiteralPath $source -Destination $target -Force
 if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLower() -ne $entry.sha256){throw "Restore hash mismatch: $relative"}
}
Write-Host 'Restore hashes verified. For baseline rollback, close UE and rebuild using absolute Engine/Build/BatchFiles/Build.bat.'
'''
 (ARCHIVE/'restore.ps1').write_text(restore,'utf-8-sig')
 (OUT/'archive-verification.json').write_text(json.dumps(d,ensure_ascii=False,indent=2),'utf-8')
 print('External archive verified',len(d['items']),'files; baseline',len(records),'files')
def remove_working():
 if (OUT/'cleanup-result.json').exists():
  d=plan();assert all(not Path(x['original']).exists() and sha(Path(x['archive']))==x['sha256'] for x in d['items']);print('Cleanup already complete and archive hashes valid');return
 d=json.loads((OUT/'archive-verification.json').read_text('utf-8'));assert d['archive_verified']
 migrations={x['design_relative']:Path(x['archive']).as_posix() for x in d['items']}
 # Original immutable snapshots and delivered ZIPs are forensic records. Their old
 # paths resolve via history-archive-index.json; do not silently rewrite their bytes.
 updates=[]
 for p in SPEC.rglob('*'):
  if p.suffix not in ['.yaml','.json','.py','.md','.html','.txt'] or 'pre-finalization-snapshot' in p.parts:continue
  if p.name in ['cleanup-plan.json','archive-verification.json','history-archive-index.json']:continue
  try:t=p.read_text('utf-8-sig')
  except (UnicodeError,OSError):continue
  changed=t
  for old,new in migrations.items():changed=re.sub(r'(?<![\w/\\:-])'+re.escape(old),lambda _:new,changed)
  if p.name in ['finalize_components.py','finalize_home_trim_delivery.py','build_home_slice_corrections.py','build_home_rank_narrow_fallback.py']:
   guard="\nif (Path(__file__).resolve().parents[1]/'evidence/history-archive-index.json').exists():\n    raise RuntimeError('Historical generator frozen after UE integration. Use the external baseline in a separate workspace to reproduce; never overwrite archived evidence.')\n"
   if 'Historical generator frozen after UE integration' not in changed:changed=changed.replace('from pathlib import Path','from pathlib import Path'+guard,1)
  if changed!=t:p.write_text(changed,'utf-8');updates.append(p.relative_to(ROOT).as_posix())
 for x in d['items']:
  src=Path(x['original']);dest=Path(x['archive']);assert under(src,SPEC)
  assert src.suffix in ['.png','.zip'] and sha(dest)==x['sha256']
  if src.exists():assert sha(src)==x['sha256']
 # All copy verification and reference writes complete before first removal.
 for x in d['items']:Path(x['original']).unlink(missing_ok=True)
 index=dict(archive=str(ARCHIVE),migrations=d['items'],updated_references=updates,historical_reference_policy='Immutable pre-finalization snapshots and ZIPs retain original paths; resolve these through migrations[].design_relative. All current loose metadata uses archive file paths.',restore=str(ARCHIVE/'restore.ps1'))
 (SPEC/'evidence/history-archive-index.json').write_text(json.dumps(index,ensure_ascii=False,indent=2),'utf-8')
 d['working_files_removed']=True;d['reference_updates_applied']=updates
 (OUT/'cleanup-result.json').write_text(json.dumps(d,ensure_ascii=False,indent=2),'utf-8')
 print('Removed verified archived working copies',len(d['items']),'updated references',len(updates))
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('mode',choices=['plan','archive','remove']);a=parser.parse_args()
 {'plan':plan,'archive':copy_archive,'remove':remove_working}[a.mode]()
