"""Record exact bounded-review dispositions and input drift; not a test runner."""
from pathlib import Path
import hashlib
import json
import csv
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent;BASE=HERE.parents[1];ROOT=HERE.parents[4]
manifest=json.loads((HERE/'source-delta-v4.json').read_text(encoding='utf-8'))
sources={r['path']:r for r in manifest['sources']}
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/'
act=prefix+'panel-activities-02/staged-source/RebuildActivitySecondaryWidgets.cpp'
eco=prefix+'panel-economy-03/staged-source/RebuildEconomyWidgets.cpp'
pd=prefix+'panel-player/player-d/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerCraftSourcesWidgets.h'
pdcontract=prefix+'panel-player/player-d/contracts/action-adapter.yaml'
pe=prefix+'panel-player/player-e/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerBreakthroughTribulationWidgets.cpp'
pc=prefix+'panel-player/player-c/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerMethodsEquipmentAppearanceWidgets.cpp'
pcpatch=prefix+'panel-player/review-v4/diff/player-c-pc3-02-03.diff'
j=prefix+'panel-journey/review-v4/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.cpp'
root='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp'
ext='Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp'
rows=[]
def add(id,owner,disposition,path,note):
    rows.append(dict(id=id,owner=owner,disposition=disposition,path=path,sha256=sources[path]['sha256'],note=note,runtime_verified=False))
add('PC3-01','panel-player','satisfied-at-current-exact-frozen-hash',pc,'Retained typed UInkPlayerCActionButton uses AddDynamic. Prior AddLambda report text does not describe this current hash; no redundant change needed.')
add('PC3-02','panel-player + integration','minimal-diff-reviewed-apply-check-passed-not-applied',pcpatch,'Known selected ID and host learn eligibility survive missing catalog display fields')
add('PC3-03','panel-player + integration','minimal-diff-reviewed-apply-check-passed-not-applied',pcpatch,'Actual equipped entry with missing name stays unknown-name rather than empty slot')
add('J3-01','panel-journey + integration','source-resolved-review-v4-host-pending',j,'Only nonempty-month permission removed; optional captured fingerprint and native day/milestone checks retained')
add('I3-01','logic-integration','open-next-source-epoch',root,'Current production hash unchanged; confirmed content disappears under Pending')
add('N05','logic-integration','open-production-mounting',ext,'Stage inventory now 44/44; production mount/proof remains 6/1/37 and runtime zero')
add('V4-API-01-secondary','Activities owner','open-P1-independent-TU-include',act,'Uses UHorizontalBoxSlot.SetPadding without Components/HorizontalBoxSlot.h')
add('V4-API-01-economy','Activities owner','open-P1-independent-TU-include',eco,'Same missing complete slot type in separate translation unit')
add('V4-ACT-01','Activities owner','open-P1-native-legal-request-blocked',act,'Offline ack adds mandatory SourceId absent from native offline_reward schema')
add('V4-ACT-02','Activities owner','open-P2-confirmed-quote-payload',act,'Captured offer/quote/price not preserved in final purchase intent')
add('V4-ECO-01','Activities owner','open-P2-local-confirmation-transition',eco,'Local selected subject changes retain stale confirm state, blocking new subject')
add('V4-PD-01','panel-player','open-P2-handoff-type-name',pdcontract,'Documented InkPlayerProcessPageWidgets namespace does not match actual InkPlayerCraftSourcesWidgets header')
add('V4-PE-01','panel-player + integration','open-P2-host-subject-contract',pe,'Need captured stage fingerprint or explicit revision semantics covering stage changes, not selection/mode only')
matrix=dict(captured_utc=manifest['captured_utc'],parent='resolved-open-matrix-v3.json',report='bounded-v4-review.md',rows=rows)
(HERE/'resolved-open-matrix-v4.json').write_text(json.dumps(matrix,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'resolved-open-matrix-v4.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
drift=[]
for r in manifest['sources']:
    digest=hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()
    if digest!=r['sha256']:drift.append(dict(path=r['path'],reviewed_sha256=r['sha256'],current_sha256=digest,state='changed-after-capture-needs-delta-review'))
owner_checks=[]
status=json.loads((BASE/'panel-journey/review-v4/status.json').read_text(encoding='utf-8-sig'))
for key,digest in status['source_sha256'].items():
    path=prefix+'panel-journey/review-v4/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets'+('.cpp' if key.endswith('_cpp') else '.h')
    owner_checks.append(dict(owner='Journey-v4',path=path,matches_reviewed=sources[path]['sha256']==digest.lower()))
status=json.loads((BASE/'panel-economy-03/status.json').read_text(encoding='utf-8-sig'))
for rel,digest in status['source_sha256'].items():
    p=(BASE/'panel-economy-03'/rel).resolve();path=p.relative_to(ROOT).as_posix()
    owner_checks.append(dict(owner='Economy-03',path=path,matches_reviewed=sources[path]['sha256']==digest.lower()))
coverage=json.loads((HERE/'stage-to-runtime-v4.json').read_text(encoding='utf-8'))
ready=dict(team_id='logic-core',updated_utc=datetime.now(timezone.utc).isoformat(),phase='bounded-v4-review-handoff-ready',
    report='bounded-v4-review.md',matrix='resolved-open-matrix-v4.json',mapping='stage-to-runtime-v4.csv',sources='source-delta-v4.json',
    coverage=coverage['counts'],new_finding_groups=6,new_matrix_rows=7,
    priority_dispatch=['V4-API-01: HorizontalBoxSlot definition include in both new Activities translation units','V4-ACT-01: remove nonexistent offline SourceId permission'],
    ready_patch_check='Player-v4 repository-root git apply --check succeeded; patch not applied',
    current_PC3_01='satisfied at current exact frozen hash; prior text finding stale',
    source_epoch='unchanged against v3; no repeated full production review',
    owner_hash_checks=owner_checks,changes_after_capture=drift,
    engine_started=False,build_run=False,service_tests_run=False,runtime_verified=False,player_entry_verified=False,
    production_edits_by_this_worker=False,visual_approval=False)
(HERE/'readiness-v4.json').write_text(json.dumps(ready,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(matrix_rows=len(rows),screen_rows=len(coverage['screens']),owner_hash_mismatches=sum(not r['matches_reviewed'] for r in owner_checks),drift_files=len(drift),coverage=coverage['counts'])))
