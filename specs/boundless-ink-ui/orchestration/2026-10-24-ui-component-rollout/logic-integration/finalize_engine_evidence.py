"""Finalize only when all bounded engine evidence and owned-process cleanup are present."""
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import hashlib,json
from PIL import Image

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
def read(name):return json.loads((HERE/name).read_text(encoding='utf-8-sig'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def write(name,value):(HERE/name).write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')

def main():
    now=datetime.now(ZoneInfo('Asia/Hong_Kong')).isoformat()
    build=read('build-11-result.json');assert build['exit_code']==0
    native=read('automation-03/index.json');assert native['succeeded']==21 and not any(native[k] for k in ('succeededWithWarnings','failed','notRun'))
    assert read('automation-03-result.json')['exit_code']==0
    module=read('compiled-module-proof.json');assert module['build']=='build-11-result.json'
    assert module['source_proof_sha256']==sha(HERE/module['source_proof'])
    for row in module['modules']:assert sha(Path(row['path']))==row['sha256']
    source=read(module['source_proof']);assert not source['frozen_panel_source_drift']
    for row in source['new_source']:assert sha(ROOT/row['path'])==row['sha256']
    for row in source['public_source']:assert sha(ROOT/row['path'])==row['after_sha256']
    for row in source['wbp_current']:assert sha(ROOT/row['file'])==row['sha256']
    assets=read('unreal-batch-result.json');assert assets['import_verified'] and assets['wbp_migrated']
    assert len(assets['texture_source_alpha_readback'])==5 and all(v['alpha_matches_source'] for v in assets['texture_source_alpha_readback'])
    assert read('fresh-ue-readback.json')['passed'] and read('fresh-ue-readback-result.json')['exit_code']==0
    cleanup=read('owned-process-cleanup.json');assert not cleanup['owned_editor_pids'] and not cleanup['owned_service_pids'] and cleanup['normal_shutdown_verified']
    suites=[];captures=[]
    for path,binary in [('pie-07/report.json','build09 full logic'),('pie-04-boundaries/report.json','build10 stock/replay/responsive'),('pie-05-viewer/report.json','build11 viewer'),('pie-10-frame-final/report.json','build11 final frame')]:
        report=read(path);assert report['finished'] and report['passed'] and not report.get('error') and all(v['passed'] for v in report['checks']),path
        suites.append(dict(report=path,binary_scope=binary,checks=len(report['checks']),steps=len(report['steps']),sha256=sha(HERE/path)))
        for capture in report['captures']:
            file=Path(capture)
            with Image.open(file) as image:size=list(image.size)
            captures.append(dict(path=str(file.relative_to(HERE)).replace('\\','/'),sha256=sha(file),actual_png_size=size,visual_approved=False))
    assert read('git-verification.json')['diff_check_exit_code']==0
    rendering=read('integration-rendering-override.json');rendering.update(final_smoke_pending=False,final_smoke='pie-10-frame-final/report.json',approved=False)
    write('integration-rendering-override.json',rendering)
    evidence=dict(completed_at=now,scope='FIRST FIVE TEXTURES/TWO WBP + bounded source/UI/authority integration; not all44-screen completion',
        build='build-11-result.json',compiled_module_proof='compiled-module-proof.json',native_automation=dict(report='automation-03/index.json',success=21,failed=0,warnings=0,notRun=0),
        rendered_suites=suites,captures=captures,imported_textures=5,preserving_wbp_migrations=2,alpha_pixel_equal=True,
        route_wired=True,logic_verified=True,visual_pending_user=True,approved=False,rendering_override='integration-rendering-override.json',
        owned_process_cleanup='owned-process-cleanup.json',frozen_source_drift=[],source_proof='current-source-proof.json',
        deferred=['Candidate HUD source-pixel nine-slice not UE accepted; measured full-image runtime fallback verified',
                  '44-screen remaining coverage and batch2 staged work remain separate; no promotion performed',
                  'User visual approval and device/Cook/package/platform/production acceptance not performed in this batch'])
    write('engine-final-verification.json',evidence)
    assets.update(route_wired=True,logic_verified=True,verification_scope=evidence['scope'],verification_report='engine-final-verification.json',awaiting=['user visual review','separate later batches'])
    write('unreal-batch-result.json',assets)
    status=read('status.json');status.update(phase='first_batch_engine_verified_owned_processes_closed',updated_at=now,
        handoff_ready=True,engine_required=False,source_only=False,source_compiled=True,import_verified=True,route_wired=True,logic_verified=True,
        quote_tests_executed=True,visual_pending_user=True,owned_engine_pids=[],owned_service_pids=[],blockers=[],
        verification_scope=evidence['scope'],verification_report='engine-final-verification.json',native_tests_current=21,
        remaining=evidence['deferred'],next_batch_promotion_requested=True)
    status['completed'].append('First bounded engine batch finalized: build11; current native21/21; actual Slate/HTTP full route, stock/replay/viewer and final frame responsive suites passed; own processes closed normally')
    write('status.json',status)
    checkpoint=dict(updated_at=now,phase=status['phase'],owned_editor_pids=[],owned_service_pids=[],latest_build=evidence['build'],
        latest_native='automation-03/index.json',rendered_suites=suites,final_report='engine-final-verification.json',unresolved_first_batch_functional=[],
        deferred=evidence['deferred'],visual_approved=False,batch2_promoted=False,ready_for_coordinator_next_batch_review=True)
    write('engine-checkpoint.json',checkpoint)
    write('next-batch-request.json',dict(requested_at=now,request='Coordinator review first batch evidence and grant a separate batch2 promotion/import/build lease when ready',
        first_batch_report='engine-final-verification.json',owned_processes_closed=True,source_freeze_preserved=True,promotion_performed=False,visual_approval_user_only=True))
    print(json.dumps(dict(build='build11',native_passed=21,rendered_suites=suites,captures=len(captures),owned_processes_closed=True),ensure_ascii=False,indent=2))

if __name__=='__main__':main()
