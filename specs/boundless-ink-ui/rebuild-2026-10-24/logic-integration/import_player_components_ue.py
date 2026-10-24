"""Post-migration bounded import/save; exclusively newUI, never overwrites assets."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib,json,os,traceback
import unreal
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
REPORT=dict(engine_pid=os.getpid(),approved=False,assets=[],passed=False)

def save():
    (HERE/'player-import-result.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')

def main():
    lease=json.loads((HERE.parent/'engine-lease.json').read_text(encoding='utf-8-sig'))
    assert lease['sole_engine_and_content_executor']=='logic-integration' and 'import' in lease['allowed_operations']
    assert datetime.fromisoformat(lease['expires_at'])>datetime.now(timezone.utc)
    assert Path(unreal.Paths.project_dir()).resolve()==ROOT
    migration=json.loads((HERE.parent/'migration/engine-final-verification.json').read_text(encoding='utf-8-sig'))
    assert migration['passed'] and not (ROOT/'Content/UI').exists()
    assert not unreal.EditorLevelLibrary.get_pie_worlds(False)
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    inputs=json.loads((HERE/'player-import-manifest.json').read_text(encoding='utf-8'))['assets']
    tasks=[]
    for row in inputs:
        assert hashlib.sha256(Path(row['source']).read_bytes()).hexdigest()==row['source_sha256']
        assert row['package'].startswith('/Game/newUI/Components/Player/')
        assert not unreal.EditorAssetLibrary.does_asset_exist(row['package']), 'Existing destination; review before retry'
        task=unreal.AssetImportTask()
        task.set_editor_property('filename',row['source'])
        task.set_editor_property('destination_path',row['package'].rsplit('/',1)[0])
        task.set_editor_property('destination_name',row['name'])
        task.set_editor_property('automated',True)
        task.set_editor_property('replace_existing',False)
        task.set_editor_property('save',False)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for row,task in zip(inputs,tasks):
        expected=row['package']+'.'+row['name']
        assert list(task.get_editor_property('imported_object_paths'))==[expected]
        texture=unreal.load_asset(row['package'])
        assert isinstance(texture,unreal.Texture2D)
        texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property('srgb',True)
        texture.set_editor_property('never_stream',True)
        texture.set_editor_property('compression_no_alpha',False)
        assert texture.blueprint_get_size_x()==row['width'] and texture.blueprint_get_size_y()==row['height']
        assert unreal.EditorAssetLibrary.save_loaded_asset(texture,False)
        physical=ROOT/('Content/'+row['package'][6:]+'.uasset')
        REPORT['assets'].append(dict(object=texture.get_path_name(),width=texture.blueprint_get_size_x(),
            height=texture.blueprint_get_size_y(),compression=str(texture.get_editor_property('compression_settings')),
            never_stream=texture.get_editor_property('never_stream'),compression_no_alpha=texture.get_editor_property('compression_no_alpha'),
            source_sha256=row['source_sha256'],uasset_sha256=hashlib.sha256(physical.read_bytes()).hexdigest()))
        save()
    for name in ('F_InkDisplay','F_InkBody'):
        assert isinstance(unreal.load_asset('/Game/newUI/Ink/Fonts/Shared/'+name),unreal.Font)
    blueprint=unreal.InkFrontendEditorLibrary.create_rebuilt_frontend_widget_asset('/Game/newUI/Runtime/WBP_NewUIFrontend')
    assert blueprint and unreal.EditorAssetLibrary.save_loaded_asset(blueprint,False)
    REPORT['default_wbp']=blueprint.get_path_name()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    REPORT['passed']=True;save();unreal.SystemLibrary.quit_editor()

try:main()
except Exception:
    REPORT['error']=traceback.format_exc();REPORT['preserve_editor_memory']=True
    save();unreal.log_error(REPORT['error'])
