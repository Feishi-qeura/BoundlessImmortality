"""Import the explicitly requested new art, never edit existing texture packages."""
from pathlib import Path
import json
import unreal

ROOT=Path(unreal.Paths.project_dir())
DEST='/Game/UI/Ink/NiagaraRevision20261022/Textures'
unreal.EditorAssetLibrary.make_directory(DEST)
tools=unreal.AssetToolsHelpers.get_asset_tools()
report=[]
for name in ('T_XianxiaBacking','T_XianxiaButtons','T_XianxiaButtonsRectangular','T_ProvidedMenuControls','T_NavigationComplete','T_XianxiaMeters','T_CultivationPadded','T_InkGoldNamecard','T_InkGoldLeaderboardPortrait','T_InkGoldLeaderboardLandscape'):
    source=ROOT/'SourceAssets/UI/NiagaraRevision20261022'/(name+'.png')
    asset=unreal.load_asset(DEST+'/'+name)
    if not asset:
        task=unreal.AssetImportTask()
        task.set_editor_property('filename',str(source))
        task.set_editor_property('destination_path',DEST)
        task.set_editor_property('automated',True)
        task.set_editor_property('save',True)
        tools.import_asset_tasks([task])
        asset=unreal.load_asset(DEST+'/'+name)
    assert asset,name
    asset.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    asset.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    asset.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    report.append(dict(source=str(source),asset=asset.get_path_name(),alpha='preserved',generator='built-in image_gen'))
(ROOT/'Saved/Codex/NiagaraUiRevision20261022/art-import.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
