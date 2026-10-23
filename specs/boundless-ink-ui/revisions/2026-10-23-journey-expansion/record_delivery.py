"""Record measured assets, source preservation and actual runtime evidence."""
import hashlib,json,platform
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[4];REV=Path(__file__).resolve().parent
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assets=[]
for stem,prompt,dest in [('world-map','world-map-prompt.md','T_WorldAtlas_v1'),('resource-map','resource-map-prompt.md','T_ResourceAtlas_v1')]:
 p=REV/'assets'/(stem+'-v1.png')
 with Image.open(p) as im:size=list(im.size);mode=im.mode
 native=ROOT/'Content/UI/Ink/Textures/Journey'/(dest+'.uasset')
 assets.append(dict(id=stem+'-v1',path=str(p.relative_to(REV)),dimensions=size,mode=mode,transparent_background='A' in mode,sha256=digest(p),prompt_path=prompt,source_tool='built-in-image_gen',unreal_asset='/Game/UI/Ink/Textures/Journey/'+dest,imported=native.is_file()))
(REV/'asset-manifest.json').write_text(json.dumps(dict(assets=assets,python=platform.python_version()),ensure_ascii=False,indent=2),encoding='utf-8')
preservation=[]
for previous in (REV/'backup/Frontend').rglob('*'):
 if not previous.is_file():continue
 relative=previous.relative_to(REV/'backup/Frontend');current=ROOT/'Source/BoundlessImmortality/Frontend'/relative
 preservation.append(dict(path=relative.as_posix(),present=current.is_file(),changed=current.is_file() and digest(previous)!=digest(current)))
(REV/'source-preservation.json').write_text(json.dumps(preservation,ensure_ascii=False,indent=2),encoding='utf-8')
native={}
for folder in ['automation','automation-layout']:
 p=REV/folder/'index.json'
 if p.exists():
  for test in json.loads(p.read_text(encoding='utf-8-sig'))['tests']:native[test['fullTestPath']]=dict(state=test['state'],errors=test['errors'],warnings=test['warnings'],source=str(p.relative_to(REV)))
pie=ROOT/'Saved/Codex/JourneyExpansion20261023/PIE/report.json';runtime=json.loads(pie.read_text(encoding='utf-8')) if pie.exists() else {}
result=dict(editor_assets_imported=all(a['imported'] for a in assets),native_count=len(native),native_failed=[k for k,v in native.items() if v['errors'] or v['state'] not in ('Success','SuccessWithWarnings')],native=native,pie_finished=runtime.get('finished',False),pie_passed=runtime.get('passed',False),pie_check_count=len(runtime.get('checks',[])),pie_failures=[c for c in runtime.get('checks',[]) if not c['passed']],pie_error=runtime.get('error'),screenshots=runtime.get('captures',[]),device_tests_run=False,new_distribution_packaged=False)
(REV/'delivery-qa.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k not in ('native','screenshots')},ensure_ascii=False,indent=2))
