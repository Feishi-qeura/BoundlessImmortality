"""Read-only semantic/native font payload and PNG proof for eight source copies."""
from pathlib import Path
import json,hashlib,os
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
NAMES={'DA_InkUITheme_v1','FF_InkMaShanZheng','FF_InkNotoFallback','FF_InkWenKai','F_InkDisplay','F_InkFallback','T_XianxiaMeters','T_HudLaunchCountdownFrame_v1'}
def main():
    import unreal
    assert os.getpid()==47604
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages() and not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    mapping=json.loads((HERE.parent/'migration/migration-map.json').read_text(encoding='utf-8-sig'))
    rows=[r for r in mapping['assets'] if r['asset_name'] in NAMES]
    assert len(rows)==8
    folder=HERE/'eight-source-copy-exports';folder.mkdir(exist_ok=True)
    report={'engine_pid':os.getpid(),'read_only':True,'rows':[],'all_proven':False}
    for row in rows:
        old=unreal.find_object(None,row['old_object'],follow_redirectors=False);new=unreal.find_object(None,row['new_object'],follow_redirectors=False)
        assert old and new and old!=new and old.get_class()==new.get_class()
        data={'old':row['old_object'],'new':row['new_object'],'class':old.get_class().get_path_name(),'exports':{}}
        texture=isinstance(old,unreal.Texture2D)
        for label,obj in [('old',old),('new',new)]:
            file=folder/(row['asset_name']+'-'+label+('.png' if texture else '.t3d'))
            task=unreal.AssetExportTask();task.set_editor_property('object',obj);task.set_editor_property('filename',str(file));task.set_editor_property('automated',True);task.set_editor_property('prompt',False);task.set_editor_property('replace_identical',True)
            task.set_editor_property('exporter',unreal.TextureExporterPNG() if texture else unreal.ObjectExporterT3D())
            assert unreal.Exporter.run_asset_export_task(task)
            data['exports'][label]={'file':str(file),'sha256':sha(file)}
        if texture:
            data['equal_payload']=data['exports']['old']['sha256']==data['exports']['new']['sha256']
            props=('compression_settings','srgb','lod_group','never_stream','filter','mip_gen_settings','address_x','address_y','virtual_texture_streaming','compression_no_alpha','max_texture_size')
            data['equal_properties']=all(old.get_editor_property(k)==new.get_editor_property(k) for k in props)
        else:
            texts=[Path(data['exports'][k]['file']).read_text(encoding='utf-8-sig').replace('/Game/UI/','/Game/newUI/') for k in ('old','new')]
            data['equal_properties']=texts[0]==texts[1]
            data['equal_payload']=data['equal_properties']
        if isinstance(old,unreal.FontFace):
            source=Path(old.get_editor_property('source_filename'))
            if not source.is_absolute():source=(ROOT/source).resolve()
            data['font_source']=str(source)
            data['same_font_source']=old.get_editor_property('source_filename')==new.get_editor_property('source_filename')
            data['font_source_exists']=source.is_file()
            if source.is_file():
                payload=source.read_bytes();data['font_source_sha256']=hashlib.sha256(payload).hexdigest();data['font_source_bytes']=len(payload)
                data['native_font_payload_in_both_packages']=all(payload in (ROOT/row[k]).read_bytes() for k in ('source_relative','destination_relative'))
            data['equal_payload']=data['equal_payload'] and data.get('native_font_payload_in_both_packages',False)
        data['proven']=data['equal_payload'] and data['equal_properties'];report['rows'].append(data)
    report['all_proven']=all(r['proven'] for r in report['rows'])
    (HERE/'eight-source-copies-proof.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
