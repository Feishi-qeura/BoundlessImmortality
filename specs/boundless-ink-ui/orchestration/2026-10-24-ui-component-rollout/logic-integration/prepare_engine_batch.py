"""Measure only assets that the first source integration actually references."""
from datetime import datetime
import difflib
import hashlib
import json
import sys
from pathlib import Path
from zoneinfo import ZoneInfo
from PIL import Image

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
ROLLOUT=HERE.parent


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    current_proof='--current-proof' in sys.argv
    selected=[
        ('panel-journey','assets/sprites/world-node-available-v1.png','Journey','T_Journey_WorldNodeAvailable','world-node-available'),
        ('panel-journey','assets/sprites/world-node-locked-v1.png','Journey','T_Journey_WorldNodeLocked','world-node-locked'),
        ('panel-rank','assets/rank-wuling-paper-panel-default-v1.png','RankWuling','T_RankWulingPaperPanel_Default_v1','rank-wuling-paper-panel'),
        ('panel-activities','assets/hud-launch-countdown-frame-neutral-v1.png','Activities','T_HudLaunchCountdownFrame_v1','hud-launch-countdown-frame'),
        ('panel-player','assets/breakthrough-material-slot-frame-default-v1.png','Player','T_BreakthroughMaterialSlotFrame_Default_v1','breakthrough-material-slot-frame-default-v1'),
    ]
    assets=[]
    for team,file,directory,name,component in selected:
        path=ROLLOUT/team/file
        if team=='panel-player':
            frame_manifest=json.loads((ROLLOUT/team/'additive-frame-manifest.json').read_text(encoding='utf-8'))
            assert sha(path)==frame_manifest['sha256'],'Recovered frame differs from additive manifest'
            assert frame_manifest['slice']['type']=='full' and not frame_manifest['slice']['nine_slice']
        with Image.open(path) as im:
            assert im.mode=='RGBA', path
            if team=='panel-player':assert list(im.size)==frame_manifest['dimensions_px']
            alpha=im.getchannel('A')
            assets.append(dict(component_id=component,source=str(path.relative_to(ROOT)).replace('\\','/'),
                               sha256=sha(path),width=im.width,height=im.height,alpha_extrema=alpha.getextrema(),
                               alpha_bbox=alpha.getbbox(),approved=False,
                               slice=frame_manifest['slice'] if team=='panel-player' else None,
                               object_path=f'/Game/UI/Ink/ComponentRollout20261024/{directory}/{name}.{name}'))
    baseline=json.loads((HERE/'source-integration-baseline.json').read_text(encoding='utf-8-sig'))
    diff=[]
    public=[]
    for v in baseline:
        path=ROOT/v['path']
        backup=HERE/'source-before'/Path(v['path']).relative_to('Source/BoundlessImmortality/Frontend')
        assert sha(backup)==v['sha256'], 'Pre-edit backup changed: '+str(backup)
        public.append(dict(path=v['path'],before_sha256=v['sha256'],after_sha256=sha(path),changed=sha(path)!=v['sha256']))
        diff.extend(difflib.unified_diff(backup.read_text(encoding='utf-8').splitlines(True),path.read_text(encoding='utf-8').splitlines(True),fromfile='before/'+v['path'],tofile='after/'+v['path']))
    freeze=json.loads((ROLLOUT/'first-source-freeze.json').read_text(encoding='utf-8-sig'))
    # The original freeze remains immutable. Only exact before/after corrections
    # explicitly owned by this team may differ, with a verified backup and grant.
    ledger_path=HERE/'quote-source-correction-ledger.json'
    ledger=json.loads(ledger_path.read_text(encoding='utf-8-sig'))
    repair_path=HERE/'engine-source-repair-ledger.json'
    repairs=json.loads(repair_path.read_text(encoding='utf-8'))['repairs'] if repair_path.exists() else []
    for repair in repairs:
        assert repair['after_sha256'] and repair['diagnostic'],'Unfinished or undocumented engine repair'
        assert sha(HERE/repair['backup'])==repair['before_sha256'],'Engine repair backup changed'
        authority_path=HERE/repair['authority']
        assert sha(authority_path)==repair['authority_sha256'],'Engine repair authority changed'
        authority=json.loads(authority_path.read_text(encoding='utf-8'))
        assert authority['owner_team']=='logic-integration' and authority['diagnostic_source_repair_authorized']
    def repaired_hash(path,expected):
        key=str(path.resolve()).lower()
        for repair in repairs:
            if str((ROOT/repair['path']).resolve()).lower()!=key:continue
            assert repair['before_sha256']==expected,'Engine source repair chain mismatch'
            expected=repair['after_sha256']
        return expected
    first_repair={}
    for repair in repairs:first_repair.setdefault(repair['path'],repair)
    for path,repair in first_repair.items():
        assert sha(ROOT/path)==repaired_hash(ROOT/path,repair['before_sha256']),'Current engine-repaired source changed outside exact chain'
    grant_path=ROLLOUT/ledger['grant']
    assert sha(grant_path)==ledger['grant_sha256'],'Correction grant changed'
    assert sha(ROLLOUT/'first-source-freeze.json')==ledger['first_source_freeze_sha256'],'Original freeze changed'
    grant=json.loads(grant_path.read_text(encoding='utf-8-sig'))
    assert grant['owner_team']==ledger['owner_team']=='logic-integration'
    assert ledger['phase']=='applied-source-only'
    allowed={str((ROOT/v['path']).resolve()).lower():v for v in grant['allowed_files']}
    corrections={};correction_diff=[]
    for row in ledger['files']:
        path=ROOT/row['path'];key=str(path.resolve()).lower()
        assert key in allowed and allowed[key]['before_sha256']==row['before_sha256'],'Unauthorized correction baseline'
        backup=HERE/'quote-source-before'/row['path']
        assert sha(backup)==row['before_sha256'],'Correction backup changed'
        assert sha(path)==repaired_hash(path,row['after_sha256']),'Authorized corrected source changed outside repair ledger'
        corrections[key]=row
        correction_diff.extend(difflib.unified_diff(backup.read_text(encoding='utf-8').splitlines(True),path.read_text(encoding='utf-8').splitlines(True),fromfile='before-quote/'+row['path'],tofile='after-quote/'+row['path']))
    assert set(corrections)==set(allowed),'Correction ledger does not match exact grant files'
    drift=[];authorized_drift=[]
    for row in freeze['files']:
        path=Path(row['path']);current=sha(path).lower()
        if current==row['sha256'].lower():continue
        correction=corrections.get(str(path.resolve()).lower())
        if correction and correction['before_sha256']==row['sha256'].lower() and repaired_hash(path,correction['after_sha256'])==current:
            authorized_drift.append(correction)
        elif repaired_hash(path,row['sha256'].lower())==current:
            authorized_drift.append(dict(path=row['path'],before_sha256=row['sha256'].lower(),after_sha256=current,reason='exact diagnostic engine repair chain'))
        else:drift.append(row['path'])
    source_root=ROOT/'Source/BoundlessImmortality/Frontend/UiRollout20261024'
    source=[dict(path=str(p.relative_to(ROOT)).replace('\\','/'),sha256=sha(p)) for p in sorted(source_root.rglob('*')) if p.suffix in ('.h','.cpp')]
    wbps=['/Game/UI/Ink/Widgets/Shared/WBP_InkFrontend','/Game/UI/Ink/Widgets/Leaderboard/WBP_InkLeaderboard']
    wbp_baseline=[]
    for package in wbps:
        path=ROOT/'Content'/Path(package.removeprefix('/Game/')).with_suffix('.uasset')
        assert path.is_file(),path
        wbp_baseline.append(dict(package=package,file=str(path.relative_to(ROOT)).replace('\\','/'),sha256=sha(path)))
    wbp_current=wbp_baseline
    if current_proof:
        wbp_baseline=json.loads((HERE/'engine-batch.json').read_text(encoding='utf-8'))['wbp_baseline']
    batch=dict(updated_at=datetime.now(ZoneInfo('Asia/Hong_Kong')).isoformat(),assets=assets,public_source=public,
               frozen_panel_source_drift=drift,authorized_frozen_source_corrections=authorized_drift,
               correction_ledger_sha256=sha(ledger_path),new_source=source,wbp_baseline=wbp_baseline,
               diagnostic_engine_repairs=repairs,
               deferred_assets='Player copied HUD pack, explored/resource/fortune glyphs: no first-batch consumer/authority state; do not import unused assets',
               engine_executed=False,compiled=False,import_verified=False,route_wired=False,visual_pending_user=True)
    if current_proof:
        batch['purpose']='Current static source/hash verification after diagnostic repair; original import batch and WBP pre-migration baseline preserved'
        batch['wbp_current']=wbp_current
    (HERE/('current-source-proof.json' if current_proof else 'engine-batch.json')).write_text(json.dumps(batch,ensure_ascii=False,indent=2),encoding='utf-8')
    (HERE/'source-integration.diff').write_text(''.join(diff),encoding='utf-8')
    (HERE/'quote-source-correction.diff').write_text(''.join(correction_diff),encoding='utf-8')
    print(json.dumps(dict(assets=len(assets),public_files_changed=sum(v['changed'] for v in public),new_source_files=len(source),frozen_source_drift=drift,authorized_frozen_corrections=len(authorized_drift),engine_executed=False),ensure_ascii=False,indent=2))
    return 1 if drift else 0


if __name__=='__main__': raise SystemExit(main())
