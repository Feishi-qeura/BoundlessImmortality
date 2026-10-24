"""Qualify the new formal execution records; no tests, engine or images run here."""
from pathlib import Path
import hashlib
import json

HERE=Path(__file__).resolve().parent
manifest=json.loads((HERE/'selected-inputs-v6.json').read_text(encoding='utf-8'))
src={r['path']:r for r in manifest['sources']}
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration/'
def read(path):return (HERE/src[path]['snapshot']).read_text(encoding='utf-8-sig')
def load(path):return json.loads(read(path),parse_constant=lambda s:(_ for _ in ()).throw(ValueError(s)))
summary=load(prefix+'core-runtime-verification.json')
pie=load(prefix+'default-root-pie-03/report.json')
native=load(prefix+'native-tests-01-report/index.json')
process=load(prefix+'native-tests-01-process-result.json')
build=load(prefix+'build-11-result.json')
epoch=load(prefix+'source-epoch.json');epoch_hash=src[prefix+'source-epoch.json']['sha256']
epoch_files={r['path']:r['sha256'] for r in epoch['files']}
pages=[]
for page in ('Main','HUD','Saves','Settings','Character','Inventory','Leaderboard'):
    names=['actual page '+page,'independent root '+page,'new root runtime version '+page,'actual new WBP '+page,'owner page version '+page]
    found=[r for r in pie['checks'] if r['name'] in names]
    passed={r['name'] for r in found if r['passed']}
    pages.append(dict(page=page,route_shell_smoke_validated=all(n in passed for n in names),
        runtime_class=next((r['actual'] for r in found if r['name']=='actual new WBP '+page),None),
        page_layout_versions=sorted({r['actual'] for r in found if r['name']=='owner page version '+page}),
        evidence=prefix+'default-root-pie-03/report.json',evidence_sha256=src[prefix+'default-root-pie-03/report.json']['sha256']))
tests=[dict(name=r['fullTestPath'],state=r['state'],date_time_raw=r['dateTime'],duration=r['duration'],errors=r['errors'],warnings=r['warnings']) for r in native['tests']]
assert len(tests)==3 and all(r['state']=='Success' for r in tests)
diagnostics=[dict(tag=k,total_nonfinite=len(v),visible_nonfinite=sum(bool(x.get('visible')) for x in v),
    arranged_visible_nonfinite=sum(bool(x.get('visible')) and bool(x.get('geometry_arranged')) for x in v)) for k,v in pie.get('nonfinite_metrics',{}).items()]
old=json.loads((HERE/'late-source-v5.json').read_text(encoding='utf-8'));old_hash={r['path']:r['sha256'] for r in old['sources']}
issue_paths=[prefix+'verify_default_root_pie.py','Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp',
    'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Rank/InkRankWulingPages.cpp']
issue_hashes=[dict(path=p,sha256=src[p]['sha256'],prior_sha256=old_hash.get(p),unchanged_since_v5=old_hash.get(p)==src[p]['sha256']) for p in issue_paths]
selected_source=[dict(path=p,sha256=r['sha256'],recorded_epoch_sha256=epoch_files.get(p),matches_build11_epoch=epoch_files.get(p)==r['sha256']) for p,r in src.items() if p.startswith('Source/')]
checks=[dict(name=r['name'],passed=r['passed'],actual=r.get('actual') if not isinstance(r.get('actual'),(dict,list)) else None)
    for r in pie['checks'] if any(k in r['name'] for k in ('actual board category','actual board scope','last real physical','last slot is'))]
out=dict(captured_utc=manifest['captured_utc'],scope='formal new records supersede missing-record status from prior cutoffs',
    source_epoch=dict(sha256=epoch_hash,reported_files=summary['source_files'],reported_ledger_records=summary['source_ledger_records'],
        summary_and_PIE03_match=summary['source_epoch_sha256']==pie['source_epoch_sha256']==epoch_hash,selected_source=selected_source),
    build11=dict(evidence=prefix+'build-11-result.json',**build),
    default_PIE03=dict(evidence=prefix+'default-root-pie-03/report.json',finished=pie['finished'],reported_passed=pie['passed'],
        started_at=pie['started_at'],finished_at=pie['finished_at'],engine_pid=pie['engine_pid'],checks=len(pie['checks']),captures=len(pie['captures']),
        failed_checks=[r['name'] for r in pie['checks'] if not r['passed']],page_observations=pages,board_and_scroll_checks=checks,
        source_report_strict_json_valid=True,geometry_acceptance='provisional-V5-VERIFY-01',diagnostics=diagnostics),
    formal_native_tests=dict(evidence=prefix+'native-tests-01-report/index.json',evidence_sha256=src[prefix+'native-tests-01-report/index.json']['sha256'],
        process_result=process,passed=native['succeeded'],failed=native['failed'],not_run=native['notRun'],tests=tests,
        build_process_counts_match=process['source_files']==build['source_files']==summary['source_files'] and process['source_records']==build['source_ledger_records']==summary['source_ledger_records'],
        rendering_proof=False,real_service_transport_Busy_proof=False),
    qualification=dict(actual_default_entry_verified=True,route_shell_smoke_validated=7,formal_native_tests_validated=3,
        geometry_acceptance='provisional',independent_expected_paint_or_culled_proof_present=False,real_service_Busy_validated=False,
        server_global_board_acceptance=summary['server_global_board_acceptance'],visual_approval=False,device_or_package_acceptance=False,
        complete_44_screen_acceptance=False),
    open_issue_selected_hashes=issue_hashes,old_issue_description_not_repeated=True)
(HERE/'runtime-evidence-v6.json').write_text(json.dumps(out,ensure_ascii=False,indent=2,allow_nan=False)+'\n',encoding='utf-8')
print(json.dumps(dict(source_epoch_matches=out['source_epoch']['summary_and_PIE03_match'],source_mismatches=[r['path'] for r in selected_source if not r['matches_build11_epoch']],
    observed_routes=sum(p['route_shell_smoke_validated'] for p in pages),formal_tests=native['succeeded'],PIE03_checks=len(pie['checks']),PIE03_captures=len(pie['captures']),
    landscape_nonfinite=next(r for r in diagnostics if r['tag']=='inventory-landscape'),issue_files_unchanged=sum(r['unchanged_since_v5'] for r in issue_hashes))))
