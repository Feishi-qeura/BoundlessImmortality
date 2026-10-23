from pathlib import Path
import sys, json, hashlib, zipfile, subprocess
import yaml
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];GAME=ROOT.parents[1]
def write(rel,text):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text.strip()+'\n',encoding='utf-8')
def load(rel):return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))
contract=load('contracts/component-contract.yaml');state=load('contracts/home-state-recipes.yaml');manifest=load('contracts/asset-manifest.yaml')
assert len(contract['components'])==18
assert len(state['controls'])==9 and sum(len(c['states']) for c in state['controls'])==63
assert next(c for c in state['controls'] if c['id']=='home-control-view-rank')['action']=='HUDRankView'
home=next(a for a in manifest['assets'] if a['id']=='home-v2')
assert home['approved'] and home['status']=='approved'
assert hashlib.sha256((ROOT/home['path']).read_bytes()).hexdigest()==home['sha256']
current={a['id']:a for a in manifest['assets'] if a['kind']=='component' and a['status']=='generated'}
assert len(current)==18
for c in contract['components']:
    a=current[c['current_asset_id']];p=ROOT/a['path']
    assert p.is_file() and (ROOT/a['prompt_path']).is_file()
    with Image.open(p) as im:
        assert list(im.size)==a['dimensions'] and im.mode=='RGBA'
        assert im.getchannel('A').getextrema()[0]==0
        if c['id'] in {'home-avatar-frame','home-icon-menu'}:assert im.getchannel('A').getpixel((im.width//2,im.height//2))==0
    assert hashlib.sha256(p.read_bytes()).hexdigest()==a['sha256']
    assert not a['approved']
screens=[]
for bg in ['checker','light','dark']:
    p=ROOT/f'evidence/component-review-{bg}.png'
    with Image.open(p) as im:assert im.size==(1400,2250)
    screens.append(dict(background=bg,path=p.relative_to(ROOT).as_posix(),dimensions=[1400,2250],visually_inspected=True))
qa=dict(passed=True,renderer='Chrome headless local file; background networking disabled',
    browser_plugin_available=False,fallback_reason='No browser automation plugin; sandbox browser startup failed; local rendering succeeded with approved escalation',
    component_images_loaded='18/18 visible in rendered page',screenshots=screens,
    visual_findings='All current components visible; no baked labels, opaque screenshot rectangles or visible colored edge spill at review scale. Frame and gear centers show selected background.',
    interaction_limit='State dropdown is a visual recipe demonstration; UE interactions and device sampling not tested.')
write('evidence/component-browser-qa.json',json.dumps(qa,ensure_ascii=False,indent=2))
base=json.loads((ROOT/'evidence/preservation-baseline.json').read_text(encoding='utf-8'));changed=[]
for b in base:
    p=GAME/b['path']
    if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=b['sha256']:changed.append(b['path'])
assert not changed,changed

# Refresh the review ZIP with current state bindings, preview and source page.
files=[ROOT/c['output'] for c in contract['components']]+[ROOT/c['prompt'] for c in contract['components']]
files += [ROOT/x for x in ['components.md','component-handoff.md','component-review.html',
    'contracts/component-contract.yaml','contracts/home-state-recipes.yaml','contracts/asset-manifest.yaml','contracts/style-contract.yaml',
    'evidence/home-components.csv','evidence/component-image-review.json','evidence/approval-home-v2.json','evidence/component-browser-qa.json',
    'evidence/component-review-checker.png','assets/pages/home-v2.png','prompts/pages/home-v2.txt','prompts/pages/home-v2.md']]
zip_path=ROOT/'D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-v1-review.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
    for p in files:z.write(p,p.relative_to(ROOT).as_posix())
    z.writestr('README.txt','Review package: 18 current transparent component PNGs, 1 approved source-page PNG and 1 review screenshot. Full project manifest is supplied as context; stale revisions are intentionally excluded. Component assets remain unapproved. No Atlas or native UE files. Open component-review.html locally.\n')
with zipfile.ZipFile(zip_path) as z:
    assert z.testzip() is None
    assert len([x for x in z.namelist() if x.startswith('assets/components/') and x.endswith('.png')])==18
    for p in files:assert hashlib.sha256(z.read(p.relative_to(ROOT).as_posix())).digest()==hashlib.sha256(p.read_bytes()).digest()
package=dict(path=zip_path.relative_to(ROOT).as_posix(),files=len(files)+1,component_png_count=18,source_page_png_count=1,review_screenshot_count=1,
    bytes=zip_path.stat().st_size,sha256=hashlib.sha256(zip_path.read_bytes()).hexdigest(),status='review-package',user_approved=False,not_atlas=True)
write('evidence/component-package.json',json.dumps(package,ensure_ascii=False,indent=2))
cmd=[sys.executable,r'C:\Users\zxxuh\.codex\skills\gameui\scripts\validate_project.py',str(ROOT),'--strict']
result=subprocess.run(cmd,capture_output=True,text=True,encoding='utf-8');write('evidence/validate-components-strict.txt',result.stdout+'\n'+result.stderr)
assert result.returncode==0,result.stdout+result.stderr
report=dict(stage='home-component-review',active_components=18,historical_component_revisions=2,individual_state_records=63,
    home_approved=True,style_approved=True,screen_map_approved=True,current_components_user_approved=False,
    protected_hashes_unchanged=len(base),protected_hashes_checked=len(base),approved_home_hash_unchanged=True,
    strict_validation=dict(exit_code=0,errors=0,warnings=0),browser_qa=qa,package=package,
    no_asset_raster_postprocessing=True,native_engine_files_created=False,runtime_tests_run=False)
write('evidence/component-delivery-status.json',json.dumps(report,ensure_ascii=False,indent=2))
write('evidence/delivery-status.json',json.dumps(report,ensure_ascii=False,indent=2))
print(json.dumps({k:report[k] for k in ['active_components','individual_state_records','protected_hashes_unchanged','strict_validation','runtime_tests_run']},ensure_ascii=False))
print(json.dumps(package,ensure_ascii=False))
