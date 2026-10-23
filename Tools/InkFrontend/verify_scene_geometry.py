"""Validate captured rendered geometry; defer widgets which have no layout yet."""
from pathlib import Path
import json
OUT=Path(__file__).resolve().parents[2]/'Saved/Codex/VisualRefactor20261022'
source=json.loads((OUT/'RenderedScenes/report.json').read_text(encoding='utf-8'))
checks=[];deferred=[]
for capture in source['captures']:
    data=json.loads(Path(capture['path']).with_suffix('.json').read_text(encoding='utf-8'))
    art=[i for i in data['images'] if i['visible'] and 'MI_InkSceneSoft' in i.get('resource','')]
    laid_out=[i for i in art if i['painted_width']>0 and i['painted_height']>0]
    deferred.extend(dict(capture=capture['tag'],object=i['object'],reason='No painted geometry; outside current scroll layout') for i in art if i not in laid_out)
    checks.append(dict(name=capture['tag']+' laid out scene aspect',passed=bool(laid_out) and all(abs(i['painted_width']/i['painted_height']-3)<.02 for i in laid_out),ratios=[i['painted_width']/i['painted_height'] for i in laid_out]))
report=dict(passed=source.get('finished',False) and not source['errors'] and all(c['passed'] for c in checks),checks=checks,deferred=deferred,source='RenderedScenes/report.json',boundary='Captured painted geometry; deferred widgets are not claimed as rendered')
(OUT/'scene-geometry-review.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(passed=report['passed'],checks=len(checks),deferred=len(deferred))))
