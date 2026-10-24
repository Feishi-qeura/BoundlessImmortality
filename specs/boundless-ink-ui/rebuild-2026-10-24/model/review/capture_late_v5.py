"""Append one bounded late Source epoch without replacing prior runtime witnesses."""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[4]
paths=[
 'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp',
 'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h',
 'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp',
 'Source/BoundlessImmortality/Frontend/InkFrontendWidget.h',
 'Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',
 'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration/verify_default_root_pie.py',
]
paths += ['Source/BoundlessImmortality/Frontend/UIRebuild20261024/Rank/'+n+ext for n in ('InkRankWulingPages','InkRankWulingPageBase','InkRankWulingExtendedPages') for ext in ('.h','.cpp')]
rows=[]
for rel in paths:
    p=ROOT/rel;data=p.read_bytes();target=HERE/'late-source-snapshot-v5'/rel
    target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    rows.append(dict(path=rel,sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),snapshot=target.relative_to(HERE).as_posix()))
(HERE/'late-source-v5.json').write_text(json.dumps(dict(captured_utc=datetime.now(timezone.utc).isoformat(),scope='one bounded late source delta; runtime reports remain original failed epochs',sources=rows),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(late_selected_source_files=len(rows))))
