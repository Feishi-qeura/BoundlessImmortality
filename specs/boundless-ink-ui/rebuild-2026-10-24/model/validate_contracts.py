"""Cross-contract consistency and isolated GameUI draft schema validation.
These checks verify authored contracts; they are not gameplay/UE tests.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys
import yaml

MODEL=Path(__file__).resolve().parent
NEW=MODEL.parent
CONTRACTS=NEW/'contracts'
SKILL=Path('C:/Users/zxxuh/.codex/skills/gameui')
checks=[]


def check(condition,label):
    checks.append(dict(name=label,passed=bool(condition)))
    if not condition: raise AssertionError(label)


def load_yaml(path): return yaml.safe_load(path.read_text(encoding='utf-8'))


screens=load_yaml(CONTRACTS/'screen-contract.yaml')['screens']
components=load_yaml(CONTRACTS/'component-contract.yaml')['components']
actions=load_yaml(MODEL/'action-contract.yaml')['actions']
models=load_yaml(MODEL/'vm-contract.yaml')['models']
ids={s['id'] for s in screens};cids={c['id'] for c in components};aids={a['id'] for a in actions};mids={m['id'] for m in models}
check(len(screens)==len(ids)==44,'44 unique screens')
check(len(cids)==len(components),'unique semantic components')
check(len(aids)==len(actions),'unique action mappings')
check(len(mids)==len(models),'unique VM contracts')
check(all(set(s['components'])<=cids for s in screens),'every page component resolves')
check(all(set(s['all_actions'])<=aids for s in screens),'every page action resolves to real existing handler mapping')
check(all(set(s['data_needs'])<=mids for s in screens),'every page VM resolves')
check(all(s['new_root']=='UInkRebuildFrontendWidget' for s in screens),'owner architecture root consistent')
check(all(s['new_root_package']=='/Game/newUI/Runtime/WBP_NewUIFrontend' for s in screens),'owner root package consistent')
check(all(s['new_runtime_binding']['actual_class'] is None and not s['delivery']['player_entry_verified'] for s in screens),'no contract-only page claims actual runtime completion')
check(all(not s['approved'] and s['delivery']['visual_pending_user'] for s in screens),'visual review remains pending')
check(all(s['existing_route']['entry'] for s in screens),'all screens identify current entry routes')
check(all((CONTRACTS/'screen-briefs'/f'{sid}.md').is_file() for sid in ids),'44 actionable briefs exist')
check(all(c['category']=='background' or c['transparent_background'] for c in components),'nonbackground components require transparent exterior')
check(all(not c['slice']['measured'] and not c['slice']['usable_for_import'] for c in components),'proposed slice margins never called measured/usable')
sources=json.loads((CONTRACTS/'source-registry.json').read_text(encoding='utf-8'))['sources']
sourceids={s['id'] for s in sources}
check(all(set(s['visual_source']['source_ids'])<=sourceids for s in screens),'every visual provenance resolves')
check(all(s['visually_inspected_in_this_run'] for s in sources),'all 13 registered visual inputs inspected')
check(all(hashlib.sha256(Path(s['path']).read_bytes()).hexdigest()==s['sha256'] for s in sources),'source PNG hash readback matches registry')
check(all(not s['approved'] for s in sources),'candidate inputs not falsely approved')
schemas=[json.loads((MODEL/'view-model.schema.json').read_text(encoding='utf-8')),json.loads((CONTRACTS/'runtime-evidence.schema.json').read_text(encoding='utf-8'))]
check(all(s.get('$schema')=='https://json-schema.org/draft/2020-12/schema' and s.get('required') for s in schemas),'JSON schema documents parse and declare required envelopes (not full JSON Schema metavalidation)')
binding=json.loads((MODEL/'architecture-binding.json').read_text(encoding='utf-8'))
check(binding['sha256']==hashlib.sha256((NEW/'architecture-and-interfaces.md').read_bytes()).hexdigest(),'architecture binding matches latest consumed owner document')

def runtime_paths(value,key=''):
    if isinstance(value,dict):
        for k,v in value.items(): yield from runtime_paths(v,k)
    elif isinstance(value,list):
        for v in value: yield from runtime_paths(v,key)
    elif isinstance(value,str) and value.startswith('/Game/'):
        yield value


for name in ['style-contract.yaml','screen-contract.yaml','component-contract.yaml','asset-manifest.yaml']:
    check(all(p.startswith('/Game/newUI/') or p=='/Game/newUI' for p in runtime_paths(load_yaml(CONTRACTS/name))),name+' has canonical new runtime paths only')

# Check stable design contrast pairs from authored tokens; raster composition remains separately unverified.
style=load_yaml(CONTRACTS/'style-contract.yaml')


def luminance(rgb):
    v=[int(rgb[i:i+2],16)/255 for i in [1,3,5]]
    v=[x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4 for x in v]
    return sum(a*b for a,b in zip(v,[.2126,.7152,.0722]))


contrast={}
for foreground,background in [('ink','paper'),('ink-muted','paper'),('on-primary','primary'),('warning','paper')]:
    a,b=sorted([luminance(style['colors'][foreground]),luminance(style['colors'][background])])
    ratio=(b+.05)/(a+.05);contrast[f'{foreground}/{background}']=round(ratio,3)
    check(ratio>=4.5,'token contrast '+foreground+'/'+background+' >=4.5')

# Validate the actual four contracts with the real GameUI validator in an owned snapshot.
# The shared rebuild root's other documentation is not this team's write ownership.
SNAPSHOT=MODEL/'gameui-validation'/'rebuild-2026-10-24'
SNAPSHOT.mkdir(parents=True,exist_ok=True)
shutil.copytree(CONTRACTS,SNAPSHOT/'contracts',dirs_exist_ok=True)
doc_content={
 'spec.md':'Scope: all 44 screens; UE5.8; real business preserved; all visible layouts rebuilt; canonical newUI only.',
 'research.md':'13 authorized current candidates inspected/measured. No unrelated old local image research. See copied source-registry.json.',
 'plan.md':'Merge in independent 2-3 page slices; core Main/HUD then Character/Inventory, rank, maps/sign-in and activities. See actual coverage-matrix.csv.',
 'tasks.md':'Contracts complete; PNG generation, source composition, import, routes, native/PIE, migration and user visual review remain distinct pending stages.',
 'quickstart.md':(NEW/'quickstart.md').read_text(encoding='utf-8'),
 'gdd.md':'Core loop uses existing authoritative cultivation, inventory, world, race, guild and activity systems. No economy/progression changes authorized by these design contracts.',
 'prd.md':'44 page IDs, per-page states and actions are in screen-contract.yaml. No partial component equals a whole new page. All new assets use /Game/newUI.',
 'interaction.md':'Existing actions/outbox/role/version/confirm/date checks preserved; unknown is not zero; missing display-only quote fields never prohibit native-valid requests. Back retains parent state.',
}
for name,text in doc_content.items():
    (SNAPSHOT/name).write_text('# Contract-validation scope snapshot\n\n- status: draft\n\n'+text+'\n\nThis is a validation copy, not a second design authority or runtime delivery claim.\n',encoding='utf-8')
result=subprocess.run([sys.executable,str(SKILL/'scripts/validate_project.py'),str(SNAPSHOT),'--strict'],capture_output=True,text=True,encoding='utf-8')
(MODEL/'gameui-validation.log').write_text(result.stdout+result.stderr,encoding='utf-8')
check(result.returncode==0,'real GameUI validator --strict on owned contract/planning snapshot')
report=dict(scope='contract consistency + design token math + GameUI draft schemas; not runtime',checks=checks,passed=all(c['passed'] for c in checks),
    screens=44,components=len(components),models=len(models),actions=len(actions),source_images=len(sources),token_contrast=contrast,
    gameui_snapshot=str(SNAPSHOT),gameui_exit_code=result.returncode,root_production_validation_run=False,ue_build_run=False,native_tests_run=False,pie_run=False,
    pending_runtime_assets=4,approved=False,visual_pending_user=True,
    json_schema_metavalidator_used=False,
    contract_sha256={name:hashlib.sha256((CONTRACTS/name).read_bytes()).hexdigest() for name in ['style-contract.yaml','screen-contract.yaml','component-contract.yaml','asset-manifest.yaml']})
(MODEL/'validation-result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(passed=report['passed'],checks=len(checks),gameui_strict_exit_code=result.returncode,screens=44,components=len(components),models=len(models),actions=len(actions))))
