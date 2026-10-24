"""Find-only diagnostics for current partial migration; never loads/saves assets."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,os

HERE=Path(__file__).resolve().parent
TEAM=HERE.parent/'migration'
ROOT=HERE.parents[3]

def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None
def info(obj):
    if obj is None:return None
    outer=obj.get_outer()
    return dict(path=obj.get_path_name(),asset_class=obj.get_class().get_path_name(),outer=outer.get_path_name() if outer else None)

def main():
    import unreal
    assert os.getpid()==47604 and Path(unreal.Paths.project_dir()).resolve()==ROOT
    result=read(TEAM/'engine-rename-result.json')
    assert result['passed'] and result['engine_pid']==os.getpid() and result['exact_count']==582
    mapping=read(TEAM/'migration-map.json')
    assert mapping['count']==582 and mapping['source_package_count']==588
    reg=unreal.AssetRegistryHelpers.get_asset_registry()
    reg.scan_paths_synchronous(['/Game/UI','/Game/newUI'],True,True)
    assert not reg.is_loading_assets()
    options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,
        include_hard_package_references=True,include_searchable_names=True,
        include_soft_management_references=True,include_hard_management_references=True)
    dirty_objects=list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())+list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    dirty=[p.get_name() for p in dirty_objects]
    classes=(unreal.Texture2D,unreal.Font,unreal.FontFace,unreal.InkUiThemeData)
    if hasattr(unreal,'ObjectRedirector'):classes=classes+(unreal.ObjectRedirector,)
    raw_old={obj.get_path_name():obj for cls in classes
        for obj in unreal.ObjectIterator(cls) if obj.get_path_name().startswith('/Game/UI/')}
    rows=[]
    for row in mapping['assets']+mapping['legacy_redirectors']:
        resolved_old=unreal.find_object(None,row['old_object'])
        direct_old=unreal.find_object(None,row['old_object'],follow_redirectors=False)
        old=direct_old or raw_old.get(row['old_object'],resolved_old);new=unreal.find_object(None,row['new_object'])
        old_exact=old is not None and old.get_path_name()==row['old_object']
        if not old_exact and row['old_package'] not in dirty:continue
        data=dict(old_expected=row['old_object'],new_expected=row['new_object'],old=info(old),new=info(new),resolved_old=info(resolved_old),
            old_exact=old_exact,dirty=row['old_package'] in dirty,
            old_disk_sha256=digest(ROOT/row['source_relative']),new_disk_sha256=digest(ROOT/row['destination_relative']))
        data['old_methods']=[name for name in dir(old) if any(word in name.lower() for word in ('flag','asset','dirty'))] if old_exact else []
        if old_exact and callable(getattr(old,'is_asset',None)):data['old_is_asset']=old.is_asset()
        for label,package in (('old',row['old_package']),('new',row['new_package'])):
            assets=reg.get_assets_by_package_name(package,True,False)
            data[label+'_registry']=[dict(name=str(a.asset_name),asset_class=str(a.asset_class_path.package_name)+'.'+str(a.asset_class_path.asset_name)) for a in assets]
            data[label+'_referencers']=[str(x) for x in reg.get_referencers(package,options)]
        if old_exact and isinstance(old,unreal.Texture2D) and isinstance(new,unreal.Texture2D):
            data['old_size']=[old.blueprint_get_size_x(),old.blueprint_get_size_y()]
            data['new_size']=[new.blueprint_get_size_x(),new.blueprint_get_size_y()]
            props=('compression_settings','srgb','lod_group','never_stream','filter','mip_gen_settings','address_x','address_y','virtual_texture_streaming','compression_no_alpha','max_texture_size')
            data['properties']={}
            for key in props:
                try:data['properties'][key]=dict(old=str(old.get_editor_property(key)),new=str(new.get_editor_property(key)))
                except Exception as error:data['properties'][key]=dict(error=str(error))
            old_data=old.get_editor_property('asset_import_data');new_data=new.get_editor_property('asset_import_data')
            data['old_import_data']=info(old_data);data['new_import_data']=info(new_data)
            data['shared_import_data_identity']=old_data==new_data
        rows.append(data)
    available={name:[method for method in dir(getattr(unreal,name)) if any(word in method.lower() for word in ('dirty','unload','flag','export'))]
        for name in ('Package','PackageTools','EditorAssetLibrary','EditorLoadingAndSavingUtils','EditorAssetSubsystem') if hasattr(unreal,name)}
    report=dict(checked_at=datetime.now(timezone.utc).isoformat(),engine_pid=os.getpid(),read_only=True,
        no_asset_load_or_save=True,dirty_packages=dirty,assets=rows,available_api=available,
        load_object_api=unreal.load_object.__doc__,find_object_api=unreal.find_object.__doc__,
        set_dirty_api=unreal.EditorAssetSubsystem.set_dirty_flag.__doc__,
        unload_api=unreal.EditorLoadingAndSavingUtils.unload_packages.__doc__,
        dirty_object_methods={p.get_name():[name for name in dir(p) if any(word in name.lower() for word in ('flag','dirty','asset'))] for p in dirty_objects})
    control=HERE/'live-diagnostic-control.json'
    if control.exists() and read(control).get('action')=='export_four_texture_payloads':
        expected={'T_RankWulingPaperPanel_Default_v1','T_RankSwordRows','T_CohesionIcons','T_PolishOrnaments'}
        folder=HERE/'live-texture-payload-exports'/datetime.now().strftime('%Y%m%dT%H%M%S')
        folder.mkdir(parents=True)
        exports=[]
        for row in mapping['assets']:
            if row['asset_name'] not in expected:continue
            old=unreal.find_object(None,row['old_object']);new=unreal.find_object(None,row['new_object'])
            assert isinstance(old,unreal.Texture2D) and isinstance(new,unreal.Texture2D)
            assert old.get_path_name()==row['old_object'] and new.get_path_name()==row['new_object'] and old!=new
            exported=dict(old_object=row['old_object'],new_object=row['new_object'])
            for label,obj in (('old',old),('new',new)):
                filename=folder/(row['asset_name']+'-'+label+'.png')
                task=unreal.AssetExportTask();task.set_editor_property('object',obj)
                task.set_editor_property('exporter',unreal.TextureExporterPNG())
                task.set_editor_property('filename',str(filename));task.set_editor_property('automated',True)
                task.set_editor_property('prompt',False);task.set_editor_property('replace_identical',False)
                assert unreal.Exporter.run_asset_export_task(task),'Read-only PNG export failed'
                exported[label+'_export']=str(filename);exported[label+'_export_sha256']=digest(filename)
            exports.append(exported)
        assert len(exports)==4
        report['read_only_payload_exports']=exports
    target=HERE/f'live-importdata-diagnostic-{os.getpid()}-{datetime.now().strftime("%Y%m%dT%H%M%S")}.json'
    target.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    unreal.log('NEWUI_READONLY_IMPORTDATA_DIAGNOSTIC '+str(target))
    return str(target)
