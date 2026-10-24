"""Publish qualified current statuses; preserve earlier evidence cutoffs."""
from pathlib import Path
import json
import csv
import hashlib
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[4]
inputs=json.loads((HERE/'selected-inputs-v6.json').read_text(encoding='utf-8'));src={r['path']:r for r in inputs['sources']}
e=json.loads((HERE/'runtime-evidence-v6.json').read_text(encoding='utf-8'));mapping=json.loads((HERE/'stage-to-runtime-v6.json').read_text(encoding='utf-8'))
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration/'
pie=prefix+'default-root-pie-03/report.json';native=prefix+'native-tests-01-report/index.json';summary=prefix+'core-runtime-verification.json'
root='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp'
extension='Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp'
rank='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp'
verify=prefix+'verify_default_root_pie.py'
rows=[]
def add(id,state,path,note):
    rows.append(dict(id=id,disposition=state,path=path,sha256=src[path]['sha256'],note=note))
add('DEFAULT-ENTRY','validated-actual-default-WBP-and-seven-route-shell-smoke',pie,'PIE03 finished/pass=true; 159 checks and 15 captures recorded; geometry qualification is separate')
add('N03','formal-native-identity-regression-Success',native,'Exact current test execution exists; supersedes missing-record state from v5 cutoff')
add('I3-01','formal-native-builder-Busy-fixture-Success-service-Busy-open',native,'Confirmed builder content/mutation gating fixture passed; real transport Busy remains separately unverified')
add('CANONICAL-MAPPING','formal-native-fixture-Success',native,'ExactCanonicalPaths passed; this does not rehash all assets or approve typography')
add('N05','seven-default-routes-validated-37-outside-build11-checkpoint',summary,'All44 stage declarations; 7 observed/route validated; later feature-host Source integration continues')
add('GEOMETRY','provisional-pending-independent-expected-paint-validation',pie,'PIE03 still carries40 visible nonfinite landscape records and8 at scroll end; reported pass does not close that condition')
add('V5-VERIFY-01','open-at-selected-unchanged-hash-await-repair-receipt',verify,'Finite-dependent filtering unchanged; no repeated old API/source description required')
add('V5-AVATAR-01','open-at-selected-unchanged-hash-await-repair-receipt',rank,'Host/page source hashes unchanged; NewUI3 has no avatar-backoff test')
add('REAL-SERVICE-BUSY','not-covered-by-current-formal-fixture-or-default-smoke',native,'Do not relabel a supplied Busy Boolean fixture as actual network/outbox Busy')
add('CURRENT-SOURCE-EPOCH','later-unpaired-Root-and-feature-host-integration',root,'Current Root and Extensions hashes differ from tested a0d9... epoch; build11 results do not automatically cover those newer bytes')
matrix=dict(captured_utc=inputs['captured_utc'],report='bounded-v6-evidence-update.md',previous='resolved-open-matrix-v5.json',
            carried_forward='Settled owner-v5 source/overlay corrections unchanged; not requested again',rows=rows)
(HERE/'resolved-open-matrix-v6.json').write_text(json.dumps(matrix,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'resolved-open-matrix-v6.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
drift=[]
for r in inputs['sources']:
    now=hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()
    if now!=r['sha256']:drift.append(dict(path=r['path'],reviewed_sha256=r['sha256'],current_sha256=now,scope='after bounded capture; needs new receipt/delta review'))
ready=dict(team_id='logic-core',phase='bounded-v6-formal-evidence-qualified',updated_utc=datetime.now(timezone.utc).isoformat(),captured_utc=inputs['captured_utc'],
    latest_report='bounded-v6-evidence-update.md',latest_matrix='resolved-open-matrix-v6.json',latest_mapping='stage-to-runtime-v6.csv',
    latest_evidence='runtime-evidence-v6.json',selected_inputs='selected-inputs-v6.json',coverage=mapping['counts'],
    prior_missing_execution_status_superseded=True,formal_tests_executed_success=3,default_PIE03_reported_passed=True,
    geometry_acceptance='provisional',independent_expected_paint_or_culled_validation=False,real_service_Busy_validated=False,
    open_issue_hashes=e['open_issue_selected_hashes'],newer_source_outside_build11=[r for r in e['source_epoch']['selected_source'] if not r['matches_build11_epoch']],
    post_capture_changes=drift,engine_started=False,build_run=False,tests_rerun=False,images_viewed=False,visual_approval=False,
    production_source_edits_by_this_worker=False,handoff_ready=True)
(HERE/'readiness-v6.json').write_text(json.dumps(ready,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(route_smoke_validated=7,native_formal_Success=3,geometry='provisional',service_Busy=False,
    open_issue_sources_unchanged=sum(r['unchanged_since_v5'] for r in e['open_issue_selected_hashes']),post_capture_drift=len(drift))))
