"""Separate observed routes, formal fixtures, and provisional geometry coverage."""
from pathlib import Path
import json
import csv

HERE=Path(__file__).resolve().parent
old=json.loads((HERE/'stage-to-runtime-v5.json').read_text(encoding='utf-8'))
e=json.loads((HERE/'runtime-evidence-v6.json').read_text(encoding='utf-8'))
inputs=json.loads((HERE/'selected-inputs-v6.json').read_text(encoding='utf-8'))
source={r['path']:r for r in inputs['sources']}
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration/'
epoch=json.loads((HERE/source[prefix+'source-epoch.json']['snapshot']).read_text(encoding='utf-8-sig'))
epoch_files={r['path']:r['sha256'] for r in epoch['files']}
observed={r['page']:r for r in e['default_PIE03']['page_observations']}
pages=dict(main='Main',home='HUD',saves='Saves',settings='Settings',character='Character',inventory='Inventory',leaderboard='Leaderboard')
tests={r['name'].rsplit('.',1)[-1]:r for r in e['formal_native_tests']['tests']}
report=e['default_PIE03']['evidence'];report_hash=source[report]['sha256'];rows=old['screens']
for row in rows:
    id=row['screen_id'];page=pages.get(id);proof=observed.get(page);success=bool(proof and proof['route_shell_smoke_validated'])
    row['production_wiring_checkpoint']='build11 / 64 Source files / ledger118 / '+e['source_epoch']['sha256']
    row['production_state_basis']='tested build11 checkpoint; later Root/Extensions integration edits are not covered by that build or PIE03'
    row['later_integration_source_delta_present']=True
    row['stage_review']=row['stage_review']+'; prior settled owner corrections carried forward without re-audit'
    row['default_route_observed']=success
    row['default_route_shell_smoke_validated']=success
    row['default_entry_class_assertions_validated']=success
    row['actual_default_runtime_class']=proof['runtime_class'] if success else ''
    row['runtime_assertion_evidence']=report if success else ''
    row['runtime_assertion_evidence_sha256']=report_hash if success else ''
    row['runtime_observation_epoch_sha256']=e['source_epoch']['sha256'] if success else ''
    row['runtime_observation_attempt_passed']=True if success else None
    row['runtime_proof']='PIE03 actual Director/default WBP + five route/root/version assertions and Slate navigation; scope validated, geometry provisional' if success else ''
    row['compile_proof']='build11 exit0, tested checkpoint64/118' if success else ''
    row['geometry_acceptance']='provisional-V5-VERIFY-01' if success else 'not-observed'
    row['independent_expected_paint_or_culled_validation']=False
    row['native_identity_fixture_execution']=''
    row['native_builder_busy_fixture_execution']=''
    row['native_fixture_validation_scope']=''
    row['real_service_Busy_flow_validated']=False
    row['rigorous_geometry_validated']=False
    row['visual_acceptance']='pending-user'
    row['current_shell_source_matches_observed_runtime_epoch']=False
    row['current_body_patch_matches_runtime_epoch']=None
    row['pending_current_body_source_sha256']=''
    row['native_tested_body_epoch_sha256']=''
    if success:
        row['saved_wbp_proof']='PIE03 exact runtime WBP_C assertion, supported by prior saved-asset evidence'
    if id in ('character','inventory'):
        row['observed_body_epoch_sha256']=epoch_files['Source/BoundlessImmortality/Frontend/UIRebuild20261024/Player/InkPlayerCharacterInventoryWidgets.cpp']
    if id=='inventory':
        row['native_identity_fixture_execution']=tests['VisibleIdentitySurvivesProjectionChanges']['state']
        row['native_builder_busy_fixture_execution']=tests['BusyRetainsConfirmedPresentation']['state']
        row['native_regression_execution_proof']=e['formal_native_tests']['evidence']
        row['native_fixture_validation_scope']='N03 native model+TriggerAction identity fixture and builder Data/Busy fixture; not a real service transport Busy scenario'
        row['native_tested_body_epoch_sha256']=row['observed_body_epoch_sha256']
        row['open_dependencies']='N03 and builder Busy formal Success recorded; geometry provisional; actual service Busy/authority mutation/visual acceptance not validated'
    if id=='character':row['open_dependencies']='route/class smoke validated; shared geometry checker provisional; Character real-service Busy and visual acceptance not validated'
    if id=='leaderboard':row['open_dependencies']='local category and Global scope smoke validated; V5-AVATAR-01 unresolved at selected hash; live server global board and rigorous geometry unvalidated'
    row['runtime_verified']=False
    row['player_entry_verified']=False
    row['player_entry_proof']='entry/route confirmed by explicit default_entry_class_assertions_validated; broad final page acceptance withheld' if success else ''
counts=dict(screens=44,stage_declarations=44,
    production_wiring_at_build11={'new_routes_connected':7,'unconnected_core_placeholder':0,'feature_routes_not_yet_in_validated_checkpoint':37},
    actual_default_entry_verified=True,observed_default_routes=7,validated_route_shell_smoke=7,
    formal_native_tests_success=3,formal_native_tests_failed=0,native_identity_fixture_validated=True,native_builder_busy_fixture_validated=True,
    rigorous_geometry_validated=0,geometry_acceptance='provisional',real_service_Busy_flow_validated=False,visual_approval=False,
    complete_44_screen_acceptance=False)
out=dict(captured_utc=inputs['captured_utc'],evidence='runtime-evidence-v6.json',previous='stage-to-runtime-v5.json',
    field_definitions=dict(default_route_shell_smoke_validated='passed actual default route/class/root/version/Slate smoke assertions',
        runtime_verified='legacy broad full-page acceptance flag; false does not negate confirmed route or native fixture successes',
        player_entry_verified='legacy broad final page acceptance; use explicit default_entry_class_assertions_validated for verified entry scope',
        production_wiring_checkpoint='validated build11 epoch, not newer in-progress feature-host Source'),counts=counts,screens=rows)
assert len(rows)==44 and sum(r['default_route_shell_smoke_validated'] for r in rows)==7
(HERE/'stage-to-runtime-v6.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'stage-to-runtime-v6.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
print(json.dumps(counts))
