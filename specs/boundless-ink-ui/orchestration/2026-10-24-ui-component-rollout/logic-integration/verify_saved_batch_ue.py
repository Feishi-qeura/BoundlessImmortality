"""Fresh-process, read-only-on-disk verification after the actual import/migration."""
import hashlib
import json
from pathlib import Path
import unreal

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
batch=json.loads((HERE/'engine-batch.json').read_text(encoding='utf-8'))
prior=json.loads((HERE/'unreal-batch-result.json').read_text(encoding='utf-8'))
registry=unreal.AssetRegistryHelpers.get_asset_registry()
result=dict(fresh_process=True,textures=[],wbps=[],passed=False)
try:
    assert prior['import_verified'] and prior['wbp_migrated']
    for row in batch['assets']:
        texture=unreal.EditorAssetLibrary.load_asset(row['object_path'])
        assert isinstance(texture,unreal.Texture2D)
        assert texture.blueprint_get_size_x()==row['width'] and texture.blueprint_get_size_y()==row['height']
        assert texture.get_editor_property('compression_settings')==unreal.TextureCompressionSettings.TC_EDITOR_ICON
        assert texture.get_editor_property('never_stream') and not texture.get_editor_property('compression_no_alpha')
        assert registry.get_asset_by_object_path(row['object_path']).is_valid()
        result['textures'].append(dict(object_path=texture.get_path_name(),persisted_ui_settings=True))
    options=unreal.AssetRegistryDependencyOptions()
    options.set_editor_property('include_hard_package_references',True)
    options.set_editor_property('include_soft_package_references',True)
    for row in prior['wbps']:
        asset=unreal.EditorAssetLibrary.load_asset(row['package'])
        assert asset
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        dependencies=[str(v) for v in registry.get_dependencies(row['package'],options)]
        assert '/Game/UI/Ink/ComponentRollout20261024/RankWuling/T_RankWulingPaperPanel_Default_v1' in dependencies,dependencies
        result['wbps'].append(dict(package=row['package'],compiled_in_fresh_process=True,dependencies=dependencies))
    result['passed']=True
finally:
    (HERE/'fresh-ue-readback.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
