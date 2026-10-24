from pathlib import Path
from datetime import datetime,timezone
import json,hashlib
HERE=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
report=read(HERE/'default-root-pie-03/report.json');native=read(HERE/'native-tests-01-report/index.json')
assert report['passed'] and report['finished'] and not report['dirty_packages_after_pie'] and len(report['checks'])==159
assert native['succeeded']==3 and native['failed']==0 and native['notRun']==0
pages={}
for file in (HERE/'default-root-pie-03').glob('*.json'):
    if file.name=='report.json':continue
    snap=read(file)
    if 'runtime_class' not in snap:continue
    assert snap['runtime_class']=='/Game/newUI/Runtime/WBP_NewUIFrontend.WBP_NewUIFrontend_C' and snap['root_widget']=='NewUIRoot_r1'
    pages.setdefault(snap['page'],[]).append({'snapshot':str(file),'screenshot':str(file.with_suffix('.png')),'layout_version':snap['page_layout_version'],'board_category':snap.get('board_category'),'board_scope':snap.get('board_scope')})
assert set(pages)=={'Main','HUD','Character','Inventory','Saves','Settings','Leaderboard'}
checkpoint={'recorded_at':datetime.now(timezone.utc).isoformat(),'core_runtime_verified':True,'core_page_count':7,'required_total_pages':44,'remaining_feature_pages':37,'actual_default_entry_verified':True,'actual_default_wbp':'/Game/newUI/Runtime/WBP_NewUIFrontend','engine_pid':report['engine_pid'],'source_epoch_sha256':report['source_epoch_sha256'],'build':'build-11-result.json','source_files':64,'source_ledger_records':118,'slate_checks':159,'viewport_captures':15,'native_tests':{'passed':3,'failed':0,'report':'native-tests-01-report/index.json','rendered':False},'pages':pages,'candidate_approved':False,'visual_acceptance':'pending_user','server_global_board_acceptance':False,'device_or_package_acceptance':False,'remaining':['37 independent feature owner pages and real new hosts/routes','visual polish and user acceptance, including compact landscape Rank header/row space','service-backed mutation validation separate from unavailable-state UI proof'],'evidence_scope':'Actual default player entry and Slate actions; global board unavailable branch is truthful; source/compile/native tests/PIE/screenshots do not substitute for server/device/platform acceptance.'}
(HERE/'core-runtime-verification.json').write_text(json.dumps(checkpoint,ensure_ascii=False,indent=2),encoding='utf-8')
status=read(HERE/'status.json');status.update(updated_at=datetime.now(timezone.utc).isoformat(),phase='core7_default_player_PIE_verified159_native3_pass_full37_features_next',own_engine_pids=[],runtime_verified=True,core_runtime_verified=True,all44_runtime_verified=False,remaining=checkpoint['remaining']+['37 feature source/route/runtime bindings'])
(HERE/'status.json').write_text(json.dumps(status,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'core_pages':7,'checks':159,'native_tests':3,'remaining_feature_pages':37,'visual_acceptance':'pending_user'}))
