"""One guarded native reference reconciliation of four proven equal payloads."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,os,traceback

HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];TEAM=HERE.parent/'migration'
REPORT=HERE/'four-texture-reconciliation-result.json'
NAMES={'T_RankWulingPaperPanel_Default_v1','T_RankSwordRows','T_CohesionIcons','T_PolishOrnaments'}
PROPS=('compression_settings','srgb','lod_group','never_stream','filter','mip_gen_settings','address_x','address_y','virtual_texture_streaming','compression_no_alpha','max_texture_size')
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda:file.read(8*1024*1024),b''):h.update(chunk)
    return h.hexdigest()
def write(report):REPORT.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

def main():
    import unreal
    assert os.getpid()==47604 and Path(unreal.Paths.project_dir()).resolve()==ROOT
    assert not REPORT.exists(),'Existing reconciliation attempt: review state, never repeat blindly'
    lease=read(HERE.parent/'engine-lease.json')
    assert lease['sole_engine_and_content_executor']=='logic-integration' and datetime.fromisoformat(lease['expires_at'])>datetime.now(timezone.utc)
    rename=read(TEAM/'engine-rename-result.json')
    assert rename['passed'] and rename['engine_pid']==os.getpid() and rename['exact_count']==582 and rename['rejected_unknown_dialog_count']==0
    assert not unreal.EditorLevelLibrary.get_pie_worlds(False)
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    mapping=read(TEAM/'migration-map.json')
    rows=[row for row in mapping['assets'] if row['asset_name'] in NAMES]
    assert len(rows)==4 and all(row['source_kind']=='canonical' for row in rows)
    backup=read(HERE/'partial-recovery-backup.json');manifest=Path(backup['manifest'])
    assert backup['verified'] and sha(manifest)==backup['sha256']
    snapshots={Path(row['relative']).as_posix():row for row in read(manifest)['files']}
    proof=None
    for path in sorted(HERE.glob('live-importdata-diagnostic-47604-*.json'),reverse=True):
        candidate=read(path)
        if len(candidate.get('read_only_payload_exports',[]))==4:proof=candidate;break
    assert proof and proof['engine_pid']==os.getpid()
    exports={row['old_object']:row for row in proof['read_only_payload_exports']}
    validated=[]
    for row in rows:
        old=unreal.find_object(None,row['old_object'],follow_redirectors=False)
        new=unreal.find_object(None,row['new_object'],follow_redirectors=False)
        assert isinstance(old,unreal.Texture2D) and isinstance(new,unreal.Texture2D) and old!=new
        assert old.get_path_name()==row['old_object'] and new.get_path_name()==row['new_object']
        assert old.blueprint_get_size_x()==new.blueprint_get_size_x() and old.blueprint_get_size_y()==new.blueprint_get_size_y()
        for key in PROPS:assert old.get_editor_property(key)==new.get_editor_property(key),(row['asset_name'],key)
        old_import=old.get_editor_property('asset_import_data');new_import=new.get_editor_property('asset_import_data')
        assert old_import==new_import and new_import.get_outer()==new
        for relative in (row['source_relative'],row['destination_relative']):
            snapshot=snapshots[relative]
            assert sha(ROOT/relative)==snapshot['source_sha256']==sha(Path(snapshot['backup']))
        exported=exports[row['old_object']]
        assert sha(Path(exported['old_export']))==exported['old_export_sha256']==exported['new_export_sha256']==sha(Path(exported['new_export']))
        validated.append((row,old,new,exported['new_export_sha256']))
    report=dict(engine_pid=os.getpid(),started_at=datetime.now(timezone.utc).isoformat(),passed=False,
        scope='Four exact old real Texture2D objects to their verified already-migrated canonical targets; no redirector delete_asset',
        backup_manifest=str(manifest),backup_sha256=backup['sha256'],operations=[])
    write(report)
    subsystem=unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    try:
        for row,old,new,payload_hash in validated:
            operation=dict(source_old=row['old_object'],retained_target=row['new_object'],state='before_native_reference_reconciliation',payload_sha256=payload_hash)
            report['operations'].append(operation);write(report)
            assert subsystem.consolidate_assets(new,[old]),'Native exact reference reconciliation failed; preserve Editor'
            assert new.get_path_name()==row['new_object'] and (ROOT/row['destination_relative']).exists()
            filename=HERE/'live-texture-payload-exports'/('after-reconcile-'+row['asset_name']+'.png')
            assert not filename.exists()
            task=unreal.AssetExportTask();task.set_editor_property('object',new)
            task.set_editor_property('exporter',unreal.TextureExporterPNG());task.set_editor_property('filename',str(filename))
            task.set_editor_property('automated',True);task.set_editor_property('prompt',False)
            assert unreal.Exporter.run_asset_export_task(task)
            assert sha(filename)==payload_hash,'Canonical texture payload changed; stop and preserve memory'
            operation.update(state='native_reconciled_target_payload_preserved',after_export=str(filename));write(report)
        report['dirty_remaining']=[p.get_name() for p in list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())+list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())]
        report['passed']=True;report['completed_at']=datetime.now(timezone.utc).isoformat();write(report)
    except Exception:
        report['error']=traceback.format_exc();report['preserve_editor_memory']=True;write(report);raise
    return str(REPORT)
