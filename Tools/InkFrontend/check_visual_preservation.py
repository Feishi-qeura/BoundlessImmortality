"""Compare this task's immutable baseline; never restore or change protected files."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Saved/Codex/VisualRefactor20261022'
baseline=json.loads((OUT/'baseline.json').read_text(encoding='utf-8'))
result=dict(unchanged=[],changed=[],missing=[],added_ui=[])
for old in baseline:
    p=ROOT/old['path']
    if not p.is_file():result['missing'].append(old['path']);continue
    new=hashlib.sha256(p.read_bytes()).hexdigest()
    result['unchanged' if new==old['sha256'] else 'changed'].append(old['path'])
known={r['path'] for r in baseline}
for p in (ROOT/'Content/UI/Ink').rglob('*.uasset'):
    rel=p.relative_to(ROOT).as_posix()
    if rel not in known:result['added_ui'].append(dict(path=rel,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
result['protected_world_changes']=[p for p in result['changed'] if p.startswith('Content/') and not p.startswith('Content/UI/Ink/')]
result['passed']=not result['missing'] and not result['protected_world_changes']
(OUT/'preservation.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:len(v) if isinstance(v,list) else v for k,v in result.items()},ensure_ascii=False))
