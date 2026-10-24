"""Finalize the bounded source-review metadata, without changing owner inputs."""
from pathlib import Path
import csv
import hashlib
import json
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent
BASE=HERE.parents[1]
ROOT=HERE.parents[4]
manifest=json.loads((HERE/'source-delta-v3.json').read_text(encoding='utf-8'))
src={r['path']:r for r in manifest['sources']}
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/'
rank=prefix+'panel-rank/review-v3/staged-source/InkRankWulingPages.cpp'
rankbase=prefix+'panel-rank/review-v3/staged-source/InkRankWulingPageBase.cpp'
act=prefix+'panel-activities/review-v3/staged-source/RebuildActivityWidgets.cpp'
journey=prefix+'panel-journey/review-v3/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.cpp'
pc=prefix+'panel-player/player-c/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerMethodsEquipmentAppearanceWidgets.cpp'
root='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp'
inventory='Source/BoundlessImmortality/Frontend/InkInventoryWidget.cpp'
ext='Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp'
rows=[]
def add(id,owner,state,path,note):
    rows.append(dict(id=id,owner=owner,disposition=state,path=path,sha256=src[path]['sha256'],note=note,runtime_verified=False))
add('N01','panel-activities + integration','source-resolved-stage-host-pending',act,'Native known-absent start subject captured; no mandatory future EventId or nonempty day')
add('N02','panel-journey + integration','identity-defect-resolved-new-J3-01-open',journey,'Captured day/month/milestone emitted; new optional month-string gate remains')
add('N03','logic-integration','source-resolved-production-runtime-pending',inventory,'Visible GUID selection and separate revalidated empty-slot intent')
add('N04','panel-journey + integration/cleanup','source-path-resolved-import-pending',journey,'Canonical mapped font path used; imported/runtime typography unverified')
add('N05','logic-integration','open-next-source-epoch',ext,'6 connected / 1 placeholder / 37 without observed new production component; legacy feature host remains')
add('N06','logic-integration','key-provider-defects-resolved-new-I3-01-open',root,'Complete entry export/provider key; Busy projection hides confirmed content')
add('N07','panel-rank + integration','source-resolved-async-host-pending',rank,'Null releases in-flight; explicit retry API; host must back off all request paths')
add('N08','panel-rank','source-resolved-compile-pending',rankbase,'Engine/Font.h explicitly included')
add('N09','panel-rank','source-resolved-render-pending',rank,'Inner button GetContent yields label')
add('PC3-01','panel-player','open-P1-API',pc,'Ordinary dynamic UButton.OnClicked still uses AddLambda')
add('PC3-02','panel-player','open-P2-action-display-coupling',pc,'Known selected item + host eligibility cannot render learn action without catalog display row')
add('PC3-03','panel-player','open-P2-unknown-field',pc,'Missing item name claims empty slot despite known actual entry identity')
add('J3-01','panel-journey','open-P2-display-gate',journey,'Nonempty ServerMonth newly required although native MonthlyChest does not require month string')
add('I3-01','logic-integration','open-P2-pending-content',root,'Busy switches confirmed Character/Inventory content to status-only presentation')
result=dict(captured_utc=manifest['captured_utc'],report='bounded-v3-review.md',parent_matrix='resolved-open-matrix-v2.json',rows=rows)
(HERE/'resolved-open-matrix-v3.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'resolved-open-matrix-v3.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
drift=[]
for r in manifest['sources']:
    current=hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()
    if current!=r['sha256']:
        drift.append(dict(path=r['path'],reviewed_sha256=r['sha256'],current_sha256=current,state='changed-after-capture-needs-delta-review'))
owner_checks=[]
rank_status=json.loads((BASE/'panel-rank/review-v3/status.json').read_text(encoding='utf-8-sig'))
for file,digest in rank_status['source_sha256'].items():
    path=prefix+'panel-rank/review-v3/staged-source/'+file
    owner_checks.append(dict(owner='panel-rank',path=path,status_hash_matches_reviewed=src[path]['sha256']==digest.lower()))
journey_status=json.loads((BASE/'panel-journey/review-v3/status.json').read_text(encoding='utf-8-sig'))
for key,digest in journey_status['source_sha256'].items():
    suffix='.cpp' if key.endswith('_cpp') else '.h'
    path=prefix+'panel-journey/review-v3/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets'+suffix
    owner_checks.append(dict(owner='panel-journey',path=path,status_hash_matches_reviewed=src[path]['sha256']==digest.lower()))
coverage=json.loads((HERE/'stage-to-runtime-v3.json').read_text(encoding='utf-8'))
ready=dict(team_id='logic-core',updated_utc=datetime.now(timezone.utc).isoformat(),phase='bounded-v3-review-handoff-ready',
    report='bounded-v3-review.md',matrix='resolved-open-matrix-v3.json',mapping='stage-to-runtime-v3.csv',
    historical_mapping_preserved='screen-coverage-v2.json',sources='source-delta-v3.json',
    priority_dispatch='PC3-01 P1: Player-c ordinary UButton dynamic OnClicked AddLambda API mismatch',
    new_findings=5,previous_N05_open=True,reviewed_production_delta_files=5,coverage=coverage['counts'],
    activities_follow_on='No ready paired source/handoff for rank/commerce/orders observed in this capture',
    activities_owner_hashes='Owner status lacks a source_sha256 map; independent exact input hashes retained here',
    owner_hash_checks=owner_checks,changed_after_capture=drift,
    production_edits_by_this_worker=False,new_test_source_observed_unrun=1,engine_started=False,build_run=False,runtime_verified=False,player_entry_verified=False,
    historical_tests_rerun=False,visual_approval=False)
(HERE/'readiness-v3.json').write_text(json.dumps(ready,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(rows=len(rows),screens=len(coverage['screens']),owner_hash_mismatches=sum(not r['status_hash_matches_reviewed'] for r in owner_checks),drift_files=len(drift),coverage=coverage['counts'])))
