"""Summarize selected existing evidence; never run engine/tests or inspect images."""
from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone

HERE=Path(__file__).resolve().parent;BASE=HERE.parents[1];ROOT=HERE.parents[4]
manifest=json.loads((HERE/'selected-inputs-v5.json').read_text(encoding='utf-8'))
sources={r['path']:r for r in manifest['sources']}
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/'
def read(rel):
    return (HERE/sources[prefix+rel]['snapshot']).read_text(encoding='utf-8-sig')
def load(rel):return json.loads(read(rel),parse_constant=lambda x:(_ for _ in ()).throw(ValueError(x)))
integration='logic-integration/'
reports=[]
for attempt in ('01','02'):
    rel=integration+'default-root-pie-'+attempt+'/report.json';d=load(rel)
    route=[]
    for page in ('Main','Saves','Settings','HUD','Character','Inventory'):
        checks=d.get('checks',[])
        names=['actual page '+page,'independent root '+page,'new root runtime version '+page,'actual new WBP '+page,'owner page version '+page]
        observations=[x for x in checks if x.get('name') in names]
        good={x['name'] for x in observations if x.get('passed')}
        route.append(dict(page=page,all_five_assertions_recorded_passed=all(n in good for n in names),
            checked_names=sorted(good),runtime_class=next((x.get('actual') for x in observations if x['name']=='actual new WBP '+page),None)))
    reports.append(dict(attempt=attempt,evidence=prefix+rel,sha256=sources[prefix+rel]['sha256'],
        report_strict_json_valid=True,engine_pid=d.get('engine_pid'),started_at=d.get('started_at'),finished_at=d.get('finished_at'),
        finished=d.get('finished'),passed=d.get('passed'),current_step=d.get('current_step'),source_epoch_sha256=d.get('source_epoch_sha256'),
        error=d.get('error'),failed_checks=[dict(name=x['name'],passed=x['passed']) for x in d.get('checks',[]) if not x.get('passed')],
        finite_checks=[dict(name=x['name'],passed=x['passed']) for x in d.get('checks',[]) if 'finite metrics' in x['name']],
        real_slate_actions=[x['name'] for x in d.get('checks',[]) if x.get('passed') and x['name'].startswith('real Slate click ')],
        page_observations=route,overall_acceptance=False))
rawrel=integration+'default-root-pie-02/raw-snapshot-19.txt';raw=load(rawrel)
bad=[dict(kind=k,name=x.get('name'),visible=x.get('visible'),null_fields=[n for n,v in x.items() if v is None])
     for k in ('texts','images','buttons','borders','scrolls') for x in raw.get(k,[]) if x.get('has_nonfinite_metrics')]
json_geometry=dict(first_attempt_describe_json_failed=True,first_attempt_raw_failure_payload_not_available_in_selected_inputs=True,
    second_attempt_raw_evidence=prefix+rawrel,second_attempt_raw_sha256=sources[prefix+rawrel]['sha256'],
    second_attempt_raw_strict_json_valid=True,nonfinite_widgets=len(bad),visible_nonfinite_widgets=sum(bool(x['visible']) for x in bad),
    visible_geometry_passed=False,nonfinite_records=bad,
    note='null plus explicit diagnostic fixes serialization; it does not turn invalid visible geometry into valid numbers')
epoch=load(integration+'source-epoch.json');epoch_hash=sources[prefix+integration+'source-epoch.json']['sha256']
epoch_map={r['path']:r['sha256'] for r in epoch['files']}
selected_source=[dict(path=k,sha256=v['sha256'],recorded_epoch_sha256=epoch_map.get(k),matches_recorded_epoch=epoch_map.get(k)==v['sha256'])
    for k,v in sources.items() if k.startswith('Source/') and k in epoch_map]
builds=[dict(evidence=prefix+integration+f'build-{n}-result.json',**load(integration+f'build-{n}-result.json')) for n in ('07','08','09')]
imports=load(integration+'player-import-result.json');assets=[]
for r in imports['assets']:
    package=r['object'].split('.',1)[0];physical=ROOT/('Content/'+package[6:]+'.uasset')
    digest=hashlib.sha256(physical.read_bytes()).hexdigest() if physical.exists() else None
    assets.append(dict(object=r['object'],width=r['width'],height=r['height'],reported_uasset_sha256=r['uasset_sha256'],
                       current_uasset_sha256=digest,disk_matches_report=digest==r['uasset_sha256'],runtime_texture_readback_reported=imports.get('passed') is True))
wbp=ROOT/'Content/newUI/Runtime/WBP_NewUIFrontend.uasset'
wbp_record=dict(package='/Game/newUI/Runtime/WBP_NewUIFrontend',reported_saved_object=imports.get('default_wbp'),
    disk_exists=wbp.exists(),bytes=wbp.stat().st_size if wbp.exists() else None,
    current_sha256=hashlib.sha256(wbp.read_bytes()).hexdigest() if wbp.exists() else None,
    creation_helper_parent_and_generated_class_checks='Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp:133-145',
    runtime_class_assertion_observed=all(r['page_observations'][0]['all_five_assertions_recorded_passed'] for r in reports),
    overall_player_entry_accepted=False)
migration=load('migration/engine-final-verification.json')
testnames=['VisibleIdentitySurvivesProjectionChanges','BusyRetainsConfirmedPresentation']
logs=[BASE/'logic-integration/build-07.log',BASE/'logic-integration/default-root-pie-editor-01.log']
hits=[];compiles=[]
for p in logs:
    for n,line in enumerate(p.read_text(encoding='utf-8',errors='replace').splitlines(),1):
        if any(t in line for t in testnames):hits.append(dict(path=p.relative_to(ROOT).as_posix(),line=n,text=line[:700]))
        if 'Compile ' in line and any(t in line for t in ('InkRebuildInventoryIdentityTests.cpp','InkRebuildCanonicalAndBusyTests.cpp')):
            compiles.append(dict(path=p.relative_to(ROOT).as_posix(),line=n,text=line[:400]))
tests=dict(source_files=['Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildInventoryIdentityTests.cpp',
                       'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Tests/InkRebuildCanonicalAndBusyTests.cpp'],
    current_build07_compile_lines=compiles,execution_name_hits_in_two_reviewed_logs=hits,execution_pass_record_found=False,
    N03_scope='native model reorder/removal and TriggerAction identity fixture; not actual Slate default entry',
    Busy_scope='native builder given Data and Busy=false/true; not real service transport Busy in default PIE',
    no_execution_claim='Sources/compilation do not establish test execution; default PIE script does not run these tests')
out=dict(captured_utc=manifest['captured_utc'],builds=builds,epoch=dict(sha256=epoch_hash,static_flags={k:epoch.get(k) for k in ('checked_at','scope','compiled','runtime_verified')},
    selected_source_matches=selected_source,PIE02_epoch_hash_matches_selected_epoch=reports[1]['source_epoch_sha256']==epoch_hash),
    migration=dict(evidence=prefix+'migration/engine-final-verification.json',checked_at=migration['checked_at'],
    canonical_verified=migration['destination_canonical_verified'],preexisting_redirectors_removed=migration['preexisting_redirectors_removed'],
    reported_old_registry_packages=migration['old_registry_package_count'],reported_old_directory_exists=migration['old_directory_exists'],
    current_Content_UI_exists=(ROOT/'Content/UI').exists(),full_asset_rehash_performed=False),
    imported_textures=assets,saved_default_wbp=wbp_record,default_pie_attempts=reports,json_and_geometry=json_geometry,native_regressions=tests,
    visuals_inspected=False,visual_approval=False,overall_runtime_acceptance=False)
(HERE/'runtime-evidence-v5.json').write_text(json.dumps(out,ensure_ascii=False,indent=2,allow_nan=False)+'\n',encoding='utf-8')
print(json.dumps(dict(texture_disk_matches=sum(x['disk_matches_report'] for x in assets),saved_wbp=wbp_record['disk_exists'],
    PIE02_epoch_hash_matches=out['epoch']['PIE02_epoch_hash_matches_selected_epoch'],PIE_passes=[r['passed'] for r in reports],
    route_observations=sum(x['all_five_assertions_recorded_passed'] for x in reports[1]['page_observations']),
    visible_nonfinite=json_geometry['visible_nonfinite_widgets'],native_compile_lines=len(compiles),native_execution_records_found=False)))
