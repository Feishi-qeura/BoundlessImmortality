"""Map original acceptance rows to actual evidence without editing the design CSVs."""
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Saved/Codex/GameplayPlan20261021'
TABLES=ROOT/'docs/design/2026-10-21-wuling-resource-v1.4/tables'
TESTS='Tools/CultivationService/tests/'

# Only mark a complete case passed when its stated assertion has direct evidence.
ITEM={
 3:('test_mixed_materials_floor_once_and_core_commit_does_not_charge_old_cost','test_items_acceptance.py'),
 4:('test_rework_keeps_core_score_without_rerolling_or_multiplying_old_quality','test_items_acceptance.py'),
 6:('test_mixed_materials_floor_once_and_core_commit_does_not_charge_old_cost','test_items_acceptance.py'),
 10:('test_focus_shared_daily_six_budget','test_focus_loadouts.py'),
 14:('test_building_debits_public_owner_once','test_economy_activities.py'),
 16:('test_all_new_bound_items_and_public_materials_are_rejected_by_market','test_items_acceptance.py'),
 19:('test_distinct_helpers_cap_and_cutoff','test_tribulation.py'),
 20:('test_sixth_attempt_pity_succeeds_without_helpers','test_tribulation.py'),
 22:('test_outage_reconciliation_does_not_extend_material_expiry','test_tribulation.py'),
 24:('test_failed_attempt_keeps_materials_and_persistent_roll','test_tribulation.py'),
}
LAUNCH={
 2:('test_launch_is_once_per_anchor_and_relative_window','test_economy_activities.py'),
 5:('test_free_facts_award_tasks_once_and_title','test_launch_progress.py'),
 6:('test_numeric_permission_off_still_records_task_without_qi','test_launch_progress.py'),
 7:('test_free_facts_award_tasks_once_and_title','test_launch_progress.py'),
 8:('test_pre_enrollment_stages_are_not_retroactively_completed','test_launch_progress.py'),
 11:('test_pre_enrollment_stages_are_not_retroactively_completed','test_launch_progress.py'),
 12:('test_free_facts_award_tasks_once_and_title','test_launch_progress.py'),
 17:('test_free_facts_award_tasks_once_and_title','test_launch_progress.py'),
 20:('test_free_facts_award_tasks_once_and_title','test_launch_progress.py'),
}


def read(path):
    with path.open(encoding='utf-8-sig',newline='') as stream:return list(csv.DictReader(stream))


def main():
    log=(OUT/'service-suite.log').read_text(encoding='utf-8-sig')
    suite_ok=bool(re.search(r'Ran (\d+) tests',log) and re.search(r'^OK\s*$',log,re.M))
    service_count=int(re.search(r'Ran (\d+) tests',log)[1])
    service_tests=[dict(test_id=module+'.'+name,status='passed') for name,module in re.findall(r'^(test_\w+) \(([^)]+)\) \.\.\. ok$',log,re.M)]
    (OUT/'service-tests.json').write_text(json.dumps(dict(passed=suite_ok,count=service_count,tests=service_tests,log='service-suite.log'),ensure_ascii=False,indent=2),encoding='utf-8')
    entries=[]
    for filename,mapping,group in [('items-v1.4/acceptance.csv',ITEM,'items'),('launch-acceptance.csv',LAUNCH,'launch')]:
        for row in read(TABLES/filename):
            number=int(row['case_id'].rsplit('-',1)[1]);coverage=mapping.get(number)
            passed=bool(coverage and suite_ok and re.search(r'^'+re.escape(coverage[0])+r' .*?\.\.\. ok$',log,re.M))
            entries.append(dict(case_id=row['case_id'],group=group,status='passed' if passed else 'not_run',expected=row['expected'],
                evidence_path=['Saved/Codex/GameplayPlan20261021/service-suite.log',TESTS+coverage[1]+'#'+coverage[0]] if passed else [],
                note='Direct component assertions; simulated authority, not device/platform.' if passed else 'Implemented component may have related checks; the complete original acceptance case has not been independently exercised.'))
    # A UI CSV row requires ALL listed resolutions and that state. Existing rendered
    # checks cover useful flows at 720/1080 portrait and 1280 landscape, not all 284 rows.
    for row in read(TABLES/'ui-acceptance.csv'):
        entries.append(dict(case_id=row['case_id'],group='ui',status='not_run',expected=row['expected'],resolutions=row['resolutions'],state=row['case'],
            evidence_path=['Saved/Codex/GameplayPlan20261021/Rendered/pie-report.json','Saved/Codex/GameplayPlan20261021/FinalInteractions/report.json'],
            note='Related rendered evidence only. Complete state x resolution row, including 540x960, not run. Functional UI remains in scope.'))
    report=dict(boundary='Acceptance traceability, not a claim that all original scenarios passed. Independent visual refactor excluded by user; UI functional/data cases retained.',service_tests=dict(passed=suite_ok,count=service_count),
        summary={group:dict(Counter(e['status'] for e in entries if e['group']==group)) for group in ('items','launch','ui')},cases=entries,
        separate_scope=[dict(case_id='INDEPENDENT_UI_VISUAL_STANDARD',status='excluded_by_user'),dict(case_id='P4_PRODUCTION_AND_FUTURE_MAPS',status='deferred_by_spec'),dict(case_id='MOBILE_DEVICE_AND_REAL_DOUYIN',status='not_run')])
    (OUT/'acceptance-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(dict(service_tests=report['service_tests'],summary=report['summary']),ensure_ascii=False))
    # Exact hashes for the code delivered, including new files absent from baseline.
    baseline={r['path']:r for r in json.loads((OUT/'preservation-baseline.json').read_text(encoding='utf-8'))}
    manifest=[]
    for directory in ('Source','Tools/CultivationService','Tools/WulingQA'):
        for path in sorted((ROOT/directory).rglob('*')):
            if not path.is_file() or '__pycache__' in path.parts or path.suffix=='.pyc':continue
            name=path.relative_to(ROOT).as_posix();digest=hashlib.sha256(path.read_bytes()).hexdigest()
            if name in baseline and digest==baseline[name]['sha256']:continue
            manifest.append(dict(path=name,sha256=digest,status='modified' if name in baseline else 'new',before=baseline.get(name,{}).get('sha256')))
    (OUT/'changes.json').write_text(json.dumps(dict(boundary='Includes separately authorized concurrent UI Source changes; use preservation-final.json categories before rollback.',files=manifest),ensure_ascii=False,indent=2),encoding='utf-8')
    validations={}
    for name,path in [('native','NativeDeliveryFinal/index.json'),('rendered','Rendered/pie-report.json'),('final_interactions','FinalInteractions/report.json'),('world_flight','WorldFlight/world-report.json'),('two_ue','TwoUE/report.json'),('two_process','TwoProcess/report.json'),('preservation','preservation-final.json')]:
        file=OUT/path;data=json.loads(file.read_text(encoding='utf-8-sig'))
        passed=data['failed']==0 and data['notRun']==0 and data['inProcess']==0 if name=='native' else data['passed']
        validations[name]=dict(passed=passed,evidence_path=path,sha256=hashlib.sha256(file.read_bytes()).hexdigest())
        if name=='native':validations[name]['count']=data['succeeded']
        if 'checks' in data:validations[name]['checks']=len(data['checks'])
    for name in ('editor','game'):
        path=OUT/(name+'-build-final.log');validations[name+'_build']=dict(passed='Result: Succeeded' in path.read_text(encoding='utf-8-sig'),evidence_path=path.name)
    validations['service']=dict(passed=suite_ok,count=service_count,evidence_path='service-tests.json')
    (OUT/'validation-summary.json').write_text(json.dumps(dict(passed=all(v['passed'] for v in validations.values()),boundary='Selected executed checks only; full original case matrix has not_run rows in acceptance-report.json',validations=validations),ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__':main()
