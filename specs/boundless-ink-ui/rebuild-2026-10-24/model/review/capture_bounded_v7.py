"""Read-only metadata capture; does not execute any production or validation code."""
from pathlib import Path
import hashlib,json
from datetime import datetime,timezone
HERE=Path(__file__).resolve().parent; BASE=HERE.parents[1]; ROOT=HERE.parents[4]
paths=list((ROOT/'Source/BoundlessImmortality/Frontend/UIRebuild20261024').glob('InkRebuildFeature*'))
paths += [ROOT/p for p in ['Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp','Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp','Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp']]
paths += [BASE/'logic-integration'/p for p in ['status.json','build-14-result.json','build-15-result.json','source-epoch.json','default-root-pie-04/report.json','verify_all44_default_pie.py','geometry_metric_validation.py','journey-import-result.json','player-import-result.json']]
paths += [p for p in BASE.glob('panel-*/**/*asset-manifest*') if p.is_file()]
rows=[]
for p in paths:
    if not p.is_file(): continue
    data=p.read_bytes(); rel=p.relative_to(ROOT); target=HERE/'selected-snapshot-v7'/rel
    target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    rows.append(dict(path=rel.as_posix(),bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),snapshot=target.relative_to(HERE).as_posix()))
(HERE/'selected-inputs-v7.json').write_text(json.dumps(dict(captured_utc=datetime.now(timezone.utc).isoformat(),scope='bounded static source/report/owner-manifest capture; no engine or tests executed',sources=rows),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(files=len(rows))))
