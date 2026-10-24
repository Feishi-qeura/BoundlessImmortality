"""Capture selected new epoch inputs/evidence only; never rehash the whole tree."""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent;BASE=HERE.parents[1];ROOT=HERE.parents[4]
paths=set()
for directory in ('panel-activities/review-v5','panel-player/review-v5'):
    for p in (BASE/directory).rglob('*'):
        if p.is_file() and p.suffix in ('.cpp','.h','.json','.md','.patch','.diff','.yaml'):
            paths.add(p)
for rel in (
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Player/InkPlayerCharacterInventoryWidgets.h',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Player/InkPlayerCharacterInventoryWidgets.cpp',
    'Source/BoundlessImmortality/Frontend/InkMenuDirector.cpp',
    'Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp',
    'Source/BoundlessImmortality/Frontend/InkInventoryWidget.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildInventoryIdentityTests.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildCanonicalAndBusyTests.cpp',
    'Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',
    'Source/BoundlessImmortality/LiveWeapons/LiveWeaponHUD.cpp',
    'Source/BoundlessImmortality/LiveWeapons/LiveGMOverlay.cpp',
    'Config/DefaultGame.ini',
):paths.add(ROOT/rel)
for rel in ('build-07-result.json','build-08-result.json','build-09-result.json','source-epoch.json',
            'player-import-result.json','default-root-pie-01/report.json','default-root-pie-02/report.json',
            'default-root-pie-02/raw-snapshot-19.txt','verify_default_root_pie.py','import_player_components_ue.py'):
    p=BASE/'logic-integration'/rel
    if p.exists():paths.add(p)
paths.add(BASE/'migration/engine-final-verification.json')
rows=[]
for p in sorted(paths):
    data=p.read_bytes();rel=p.relative_to(ROOT);target=HERE/'selected-snapshot-v5'/rel
    target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    rows.append(dict(path=rel.as_posix(),sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),snapshot=target.relative_to(HERE).as_posix()))
result=dict(captured_utc=datetime.now(timezone.utc).isoformat(),scope='explicit selected inputs only; no full source or Content rehash',
            inherited_stage_inventory='stage-to-runtime-v4.json',inherited_source_manifest='source-delta-v4.json',sources=rows)
(HERE/'selected-inputs-v5.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(selected_files=len(rows))))
