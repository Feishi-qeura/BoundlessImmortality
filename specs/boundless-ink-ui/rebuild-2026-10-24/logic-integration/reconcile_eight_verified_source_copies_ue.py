"""Exact source-copy reconciliation after complete per-type equality proof."""
from pathlib import Path
from datetime import datetime,timezone
import json,os,hashlib,runpy,traceback
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];OUT=HERE/'eight-source-reconciliation-result.json'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    import unreal
    assert os.getpid()==47604 and not OUT.exists()
    lease=read(HERE.parent/'engine-lease.json')
    assert lease['sole_engine_and_content_executor']=='logic-integration' and datetime.fromisoformat(lease['expires_at'])>datetime.now(timezone.utc)
    assert not unreal.EditorLevelLibrary.get_pie_worlds(False)
    runpy.run_path(str(HERE/'inspect_eight_source_copies_ue.py'),run_name='integration_fresh_equality_proof')['main']()
    proof=read(HERE/'eight-source-copies-proof.json');assert proof['all_proven'] and len(proof['rows'])==8
    backup=read(HERE/'partial-recovery-backup.json');manifest=Path(backup['manifest'])
    assert backup['verified'] and sha(manifest)==backup['sha256']
    snapshots={Path(r['relative']).as_posix():r for r in read(manifest)['files']}
    by={r['old_object']:r for r in read(HERE.parent/'migration/migration-map.json')['assets']}
    validated=[]
    for p in proof['rows']:
        row=by[p['old']];old=unreal.find_object(None,p['old'],follow_redirectors=False);new=unreal.find_object(None,p['new'],follow_redirectors=False)
        assert old and new and old!=new and old.get_class().get_path_name()==new.get_class().get_path_name()==p['class'] and p['class']!='/Script/CoreUObject.ObjectRedirector'
        for k in ('source_relative','destination_relative'):
            snapshot=snapshots[row[k]];assert sha(ROOT/row[k])==snapshot['source_sha256']==sha(Path(snapshot['backup']))
        validated.append((row,p,old,new))
    report={'engine_pid':os.getpid(),'started_at':datetime.now(timezone.utc).isoformat(),'passed':False,'operations':[],'scope':'Eight exact migrated source real assets; retained canonical target equality proven before all mutations','backup_manifest':str(manifest),'proof_sha256':sha(HERE/'eight-source-copies-proof.json')}
    def write():OUT.write_text(json.dumps(report,indent=2),encoding='utf-8')
    write();subsystem=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    try:
        for row,p,old,new in validated:
            op={'old':p['old'],'retained':p['new'],'state':'before_native_consolidate'};report['operations'].append(op);write()
            assert subsystem.consolidate_assets(new,[old]),'Native reconciliation failed; preserve memory'
            assert new.get_path_name()==p['new'] and (ROOT/row['destination_relative']).exists()
            texture=isinstance(new,unreal.Texture2D)
            file=HERE/'eight-source-copy-exports'/(row['asset_name']+'-after'+('.png' if texture else '.t3d'))
            task=unreal.AssetExportTask();task.set_editor_property('object',new);task.set_editor_property('filename',str(file));task.set_editor_property('automated',True);task.set_editor_property('prompt',False);task.set_editor_property('exporter',unreal.TextureExporterPNG() if texture else unreal.ObjectExporterT3D())
            assert unreal.Exporter.run_asset_export_task(task)
            if texture:assert sha(file)==p['exports']['new']['sha256']
            else:assert file.read_text(encoding='utf-8-sig')==Path(p['exports']['new']['file']).read_text(encoding='utf-8-sig')
            if isinstance(new,unreal.FontFace):assert Path(p['font_source']).read_bytes() in (ROOT/row['destination_relative']).read_bytes()
            op.update(state='reconciled_canonical_payload_preserved',after_export=str(file));write()
        report['dirty_remaining']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
        report['dirty_maps']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
        report['passed']=not report['dirty_remaining'] and not report['dirty_maps'];write()
    except Exception:
        report['error']=traceback.format_exc();report['preserve_editor_memory']=True;write();raise
