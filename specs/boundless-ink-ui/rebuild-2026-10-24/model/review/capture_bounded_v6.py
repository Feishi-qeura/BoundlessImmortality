"""Capture the new formal reports and selected source hashes, without rerunning work."""
from pathlib import Path
import json
import hashlib
from datetime import datetime, timezone
HERE=Path(__file__).resolve().parent;BASE=HERE.parents[1];ROOT=HERE.parents[4]
paths=[BASE/'logic-integration'/p for p in (
    'core-runtime-verification.json','default-root-pie-03/report.json',
    'native-tests-01-report/index.json','native-tests-01-process-result.json',
    'build-11-result.json','source-epoch.json','verify_default_root_pie.py')]
paths += [ROOT/p for p in (
    'Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Rank/InkRankWulingPages.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildInventoryIdentityTests.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildCanonicalAndBusyTests.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp',
    'Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp')]
rows=[]
for p in paths:
    data=p.read_bytes();rel=p.relative_to(ROOT);target=HERE/'selected-snapshot-v6'/rel
    target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    rows.append(dict(path=rel.as_posix(),bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),snapshot=target.relative_to(HERE).as_posix()))
(HERE/'selected-inputs-v6.json').write_text(json.dumps(dict(captured_utc=datetime.now(timezone.utc).isoformat(),
    scope='new formal reports + selected issue/test/host sources; no full source/asset rehash',sources=rows),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(selected_files=len(rows))))
