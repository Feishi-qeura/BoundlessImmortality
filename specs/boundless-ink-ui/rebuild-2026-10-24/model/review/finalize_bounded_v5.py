"""Finalize bounded review of selected hashes and pre-existing evidence only."""
from pathlib import Path
import hashlib
import json
import csv
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent;BASE=HERE.parents[1];ROOT=HERE.parents[4]
manifest=json.loads((HERE/'selected-inputs-v5.json').read_text(encoding='utf-8'));src={r['path']:r for r in manifest['sources']}
late=json.loads((HERE/'late-source-v5.json').read_text(encoding='utf-8'))
src.update({r['path']:r for r in late['sources']})
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/'
def frozen(rel):return json.loads((HERE/src[prefix+rel]['snapshot']).read_text(encoding='utf-8-sig'))
prior=json.loads((HERE/'source-delta-v4.json').read_text(encoding='utf-8'));old={r['path']:r for r in prior['sources']}
checks=[]
for row in frozen('panel-activities/review-v5/source-delta-v5.json')['files']:
    path=prefix+row['replacement'];base=prefix+row['frozen_input']
    checks.append(dict(owner='Activities-v5',path=path,replacement_hash_matches=src[path]['sha256']==row['replacement_sha256'].lower(),
        frozen_hash_matches_inherited_review=base in old and old[base]['sha256']==row['frozen_sha256'].lower(),unchanged_frozen_file_rehashed=False))
for row in frozen('panel-player/review-v5/manifest.json')['files']:
    path=prefix+'panel-player/review-v5/'+row['path']
    checks.append(dict(owner='Player-v5',path=path,replacement_hash_matches=src[path]['sha256']==row['sha256'].lower()))
act=prefix+'panel-activities/review-v5/staged-source/RebuildActivitySecondaryWidgets.cpp'
eco=prefix+'panel-activities/review-v5/staged-source/RebuildEconomyWidgets.cpp'
pd=prefix+'panel-player/review-v5/contracts/player-d-adapter-namespace-overlay.yaml'
pe=prefix+'panel-player/review-v5/diff/player-e-breakthrough-confirmation.diff'
root='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp'
ext='Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp'
tests='Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildInventoryIdentityTests.cpp'
busy='Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildCanonicalAndBusyTests.cpp'
report=prefix+'logic-integration/default-root-pie-02/report.json'
rows=[]
def add(id,owner,state,path,note):
    rows.append(dict(id=id,owner=owner,disposition=state,path=path,sha256=src[path]['sha256'],note=note,overall_runtime_accepted=False))
add('V4-API-01','Activities + integration','repaired-in-paired-v5-source-compile-pending',act,'Defining HorizontalBoxSlot headers added to both secondary/economy translation units')
add('V4-ACT-01','Activities + integration','repaired-in-v5-source-host-pending',act,'No mandatory SourceId on native current-user offline acknowledgement')
add('V4-ACT-02','Activities + integration','repaired-in-v5-source-host-pending',act,'Captured OfferId/SKU/quote version/price/confirmation preserved in paired request schema')
add('V4-ECO-01','Activities + integration','repaired-in-v5-source-host-pending',eco,'Local subject switches clear corresponding confirmation; refreshed data compared to captured tuple')
add('V4-PD-01','Player + integration','namespace-overlay-ready-not-consumed',pd,'Correct actual namespace; frozen original contract left intact')
add('V4-PE-01','Player + integration','stage-subject-patch-apply-checked-unapplied',pe,'Host revision tied to actual stage tuple; zero valid; unknown display fields are not permissions')
add('I3-01','logic-integration','source-defect-repaired-live-busy-evidence-open',root,'Confirmed data retained; mutation Busy separate. Native fixture source/compile is not a default-route transport Busy test')
add('N03','logic-integration','source-correct-current-execution-record-open',tests,'Identity regression fixture exists; no execution pass record in selected evidence; search/clear is not identity regression')
add('BUSY-EXECUTION','logic-integration','current-test-execution-and-live-transition-open',busy,'Build07 compilation line exists; actual result and real service Busy flow absent in selected records')
rank='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp'
editor='Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp'
verify=prefix+'logic-integration/verify_default_root_pie.py'
add('N05','logic-integration','partial-real-default-route-observation-new-rank-source-wired',rank,'Saved new WBP and 6 prior actual route observations; late Source wiring is 7/0/37 including lazy Rank, not a new runtime pass')
add('JSON-PROTOCOL','logic-integration','PIE01-failed-new-PIE02-strict-json-witness',report,'Raw02 landscape parses strictly; null diagnostics are not finite geometry; neither attempt passed')
add('LANDSCAPE-GEOMETRY','logic-integration','open-failed-visible-finite-metrics-assertion',report,'40 records flagged visible have nonfinite geometry diagnostics; pending Source body-scroll correction not paired with this runtime epoch')
add('OLD-HUD-OWNERSHIP','logic-integration','source-build-fix-present-explicit-runtime-assertion-open',root,'Owner predicates and build09 exist; current smoke assertions do not explicitly prove old HUD/GM suppressed')
add('V5-VERIFY-01','logic-integration','open-P1-validation-filter-blind-spot',verify,'geometry_arranged already requires finite tested coordinates; filtering invalid records with it can hide the prior nonfinite failure')
add('V5-AVATAR-01','logic-integration','open-P2-backoff-request-latch',rank,'Page sets in-flight before broadcast; host backoff rejection leaves it set; post-backoff retry API then refuses forever')
matrix=dict(captured_utc=manifest['captured_utc'],parent='resolved-open-matrix-v4.json',report='bounded-v5-review.md',rows=rows)
(HERE/'resolved-open-matrix-v5.json').write_text(json.dumps(matrix,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'resolved-open-matrix-v5.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
# Only the explicitly selected input set is rechecked for concurrent drift.
drift=[]
for r in src.values():
    h=hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()
    if h!=r['sha256']:drift.append(dict(path=r['path'],reviewed_sha256=r['sha256'],now_sha256=h,state='later-input-not-covered-by-this-bounded-snapshot'))
runtime=json.loads((HERE/'runtime-evidence-v5.json').read_text(encoding='utf-8'));coverage=json.loads((HERE/'stage-to-runtime-v5.json').read_text(encoding='utf-8'))
ready=dict(team_id='logic-core',updated_utc=datetime.now(timezone.utc).isoformat(),captured_utc=manifest['captured_utc'],late_source_captured_utc=late['captured_utc'],phase='bounded-v5-evidence-review-handoff-ready',
    report='bounded-v5-review.md',matrix='resolved-open-matrix-v5.json',mapping='stage-to-runtime-v5.csv',runtime_evidence='runtime-evidence-v5.json',
    selected_inputs='selected-inputs-v5.json',late_source='late-source-v5.json',coverage=coverage['counts'],patch_owner_hash_checks=checks,
    pending_current_source_epoch_mismatches=[r for r in runtime['epoch']['selected_source_matches'] if not r['matches_recorded_epoch']],
    source_hashes_full_scan=False,unchanged_stage_rehash=False,engine_started=False,build_run=False,old_tests_rerun=False,images_inspected=False,visual_approval=False,
    native_execution_pass_record_in_reviewed_artifacts=False,failed_runtime_attempts=['01','02'],visible_nonfinite_records=runtime['json_and_geometry']['visible_nonfinite_widgets'],
    priority_new_findings=['V5-VERIFY-01','V5-AVATAR-01'],ready_v5_previous_finding_groups_reconciled=6,
    post_capture_changes=drift,production_edits_by_this_worker=False,handoff_ready=True)
(HERE/'readiness-v5.json').write_text(json.dumps(ready,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(matrix_rows=len(rows),owner_hash_mismatches=sum(not r['replacement_hash_matches'] for r in checks),frozen_hash_mismatches=sum(r.get('frozen_hash_matches_inherited_review') is False for r in checks),
    source_epoch_mismatches=len(ready['pending_current_source_epoch_mismatches']),post_capture_changes=len(drift),default_routes_observed=6,accepted=0)))
