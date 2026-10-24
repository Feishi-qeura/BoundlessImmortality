"""Execute only inside rebuilt UE5.8 after the supervisor grants this exact batch.

First batch: five referenced textures + two inventory-preserving rank WBP migrations.
This module performs no engine operation before lease validation.
"""
import hashlib
import json
from pathlib import Path
import shutil
from datetime import datetime,timezone

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def require_engine_grant():
    lease=json.loads((HERE.parent/'engine-lease.json').read_text(encoding='utf-8-sig'))
    if lease.get('state')!='granted' or lease.get('owner_team')!='logic-integration':
        raise RuntimeError('Engine lease not granted to logic-integration; no engine operation performed')
    if not {'import','migrate_wbp'}.issubset(set(lease.get('allowed_operations',[]))):
        raise RuntimeError('Bounded lease must explicitly allow import and migrate_wbp')
    if datetime.now(timezone.utc)>=datetime.fromisoformat(lease['expires_at']):
        raise RuntimeError('Engine lease expired; do not start another operation')
    return lease


def main():
    lease=require_engine_grant()
    batch=json.loads((HERE/'engine-batch.json').read_text(encoding='utf-8'))
    requested={v['object_path'].split('.')[0] for v in batch['assets']}|{v['package'] for v in batch['wbp_baseline']}
    if requested!=set(lease['allowed_content_writes']):
        raise RuntimeError('Batch Content scope differs from exact lease allowlist')
    repaired={r['path']:r for r in batch.get('diagnostic_engine_repairs',[])}
    for path,repair in repaired.items():
        if sha(ROOT/path)!=repair['after_sha256']:raise RuntimeError('Diagnostic-repaired source changed: '+path)
    if sha(HERE/'quote-source-correction-ledger.json')!=batch['correction_ledger_sha256']:
        raise RuntimeError('Source correction ledger changed; re-audit this batch')
    for row in batch['new_source']:
        if sha(ROOT/row['path'])!=row['sha256']: raise RuntimeError('Source changed; prepare/rebuild this batch first: '+row['path'])
    for row in batch['public_source']:
        if sha(ROOT/row['path'])!=row['after_sha256']: raise RuntimeError('Public source changed: '+row['path'])
    for row in batch['assets']:
        if sha(ROOT/row['source'])!=row['sha256']: raise RuntimeError('Source PNG changed: '+row['source'])
    for row in batch['wbp_baseline']:
        if sha(ROOT/row['file'])!=row['sha256']: raise RuntimeError('WBP changed after source preparation; re-audit: '+row['file'])

    import unreal
    if not hasattr(unreal,'InkFrontendEditorLibrary'):
        raise RuntimeError('Rebuilt project editor module is required')
    for name in ('InkIntegratedActivitiesPanel','InkIntegratedDailyPanel','InkIntegratedLaunchCountdown','InkRolloutMaterialSlotWidget'):
        if not unreal.load_class(None,'/Script/BoundlessImmortality.'+name):
            raise RuntimeError('This editor has not loaded the new rebuilt native class: '+name)
    previous_path=HERE/'unreal-batch-result.json'
    previous=json.loads(previous_path.read_text(encoding='utf-8')) if previous_path.exists() else None
    resume=previous and previous.get('batch_sha256')==sha(HERE/'engine-batch.json') and previous.get('error')
    existing=set()
    for row in batch['assets']:
        if unreal.EditorAssetLibrary.does_asset_exist(row['object_path']):
            texture=unreal.EditorAssetLibrary.load_asset(row['object_path'])
            source=texture.get_editor_property('asset_import_data').get_first_filename() if isinstance(texture,unreal.Texture2D) else ''
            if not resume or Path(source).resolve()!=Path(ROOT/row['source']).resolve():
                raise RuntimeError('Refusing to overwrite unowned candidate texture: '+row['object_path'])
            existing.add(row['object_path'])
    backup=HERE/'engine-backup'
    backup.mkdir(exist_ok=True)
    for row in batch['wbp_baseline']:
        target=backup/(Path(row['file']).name+'.'+row['sha256']+'.bak')
        if not target.exists(): shutil.copy2(ROOT/row['file'],target)
        if sha(target)!=row['sha256']: raise RuntimeError('WBP backup hash mismatch')
    report=dict(import_verified=False,wbp_migrated=False,route_wired=False,logic_verified=False,
                visual_pending_user=True,assets=[],wbps=[],batch_sha256=sha(HERE/'engine-batch.json'))
    report_path=HERE/'unreal-batch-result.json'
    try:
        tasks=[]
        for row in batch['assets']:
            if row['object_path'] in existing:continue  # Resume settings/readback, never reimport or overwrite.
            package,name=row['object_path'].split('.')
            task=unreal.AssetImportTask()
            task.filename=str(ROOT/row['source']);task.destination_path=package.rsplit('/',1)[0]
            task.destination_name=name;task.automated=True;task.replace_existing=False;task.save=True
            tasks.append(task)
        if tasks:unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        exports=HERE/'texture-readback'
        exports.mkdir(exist_ok=True)
        registry=unreal.AssetRegistryHelpers.get_asset_registry()
        for row in batch['assets']:
            texture=unreal.EditorAssetLibrary.load_asset(row['object_path'])
            if not isinstance(texture,unreal.Texture2D): raise RuntimeError('Missing Texture2D: '+row['object_path'])
            texture.set_editor_property('srgb',True)
            texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
            texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            texture.set_editor_property('never_stream',True)
            texture.set_editor_property('compression_no_alpha',False)
            if not unreal.EditorAssetLibrary.save_loaded_asset(texture): raise RuntimeError('Texture save failed')
            width=int(texture.blueprint_get_size_x());height=int(texture.blueprint_get_size_y())
            if (width,height)!=(row['width'],row['height']): raise RuntimeError('Texture dimensions changed')
            export=unreal.AssetExportTask();export.object=texture;export.automated=True
            export.filename=str(exports/(texture.get_name()+'.png'));export.replace_identical=True
            export.exporter=unreal.TextureExporterPNG()
            if not unreal.Exporter.run_asset_export_task(export): raise RuntimeError('Texture source-alpha export failed')
            asset_data=registry.get_asset_by_object_path(row['object_path'])
            if not asset_data.is_valid(): raise RuntimeError('AssetRegistry readback missing')
            report['assets'].append(dict(object_path=texture.get_path_name(),width=width,height=height,
                registry_package=str(asset_data.package_name),compression=str(texture.get_editor_property('compression_settings')),
                srgb=texture.get_editor_property('srgb'),lod_group=str(texture.get_editor_property('lod_group')),
                never_stream=texture.get_editor_property('never_stream'),compression_no_alpha=texture.get_editor_property('compression_no_alpha'),
                exported_png=export.filename,source_sha256=row['sha256']))
        # The native CDO may have been constructed before this first import. Refresh
        # its editor-session reference; subsequent fresh launches resolve it on disk.
        countdown_class=unreal.load_class(None,'/Script/BoundlessImmortality.InkIntegratedLaunchCountdown')
        countdown_defaults=unreal.get_default_object(countdown_class)
        frame_path=next(v['object_path'] for v in batch['assets'] if v['component_id']=='hud-launch-countdown-frame')
        countdown_defaults.set_editor_property('frame_texture',unreal.EditorAssetLibrary.load_asset(frame_path))
        if countdown_defaults.get_editor_property('frame_texture').get_path_name()!=frame_path:
            raise RuntimeError('HUD CDO reference readback failed')
        report['hud_cdo_reference']=frame_path
        for row in batch['wbp_baseline']:
            if not unreal.InkFrontendEditorLibrary.refresh_leaderboard_frontend_asset(row['package']):
                raise RuntimeError('Preserving rank migration rejected WBP: '+row['package'])
            asset=unreal.EditorAssetLibrary.load_asset(row['package'])
            if not asset or not unreal.EditorAssetLibrary.save_loaded_asset(asset): raise RuntimeError('WBP save failed')
            report['wbps'].append(dict(package=row['package'],saved=True,sha256=sha(ROOT/row['file'])))
        report['wbp_migrated']=True
        # External Pillow checks exported alpha before marking import_verified true.
        report['awaiting']='verify_unreal_readback.py + automation/PIE player-entry evidence'
    except Exception as exc:
        report['error']=str(exc)
        raise
    finally:
        report_path.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')


if __name__=='__main__': main()
