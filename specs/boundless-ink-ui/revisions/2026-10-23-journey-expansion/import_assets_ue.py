"""Add only new atlas textures and new native-parent WBP assets."""
import json
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir())
REV=ROOT/'specs/boundless-ink-ui/revisions/2026-10-23-journey-expansion'
REPORT={'textures':[],'widgets':[]}
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
for source,name in [('world-map-v1.png','T_WorldAtlas_v1'),('resource-map-v1.png','T_ResourceAtlas_v1')]:
    dest='/Game/UI/Ink/Textures/Journey'
    texture=unreal.load_asset(dest+'/'+name)
    if not texture:
        task=unreal.AssetImportTask();task.set_editor_property('filename',str(REV/'assets'/source));task.set_editor_property('destination_path',dest)
        task.set_editor_property('destination_name',name);task.set_editor_property('automated',True);task.set_editor_property('save',True)
        TOOLS.import_asset_tasks([task]);texture=unreal.load_asset(dest+'/'+name)
    assert texture,name
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    REPORT['textures'].append(dict(path=texture.get_path_name(),source=str(REV/'assets'/source)))
for folder,name,parent in [('Activities','WBP_LaunchEvent',unreal.InkLaunchEventPanel),('Wuling','WBP_WulingSect',unreal.InkWulingSectPanel),('Journey','WBP_JourneyMap',unreal.InkJourneyMapPanel),('Journey','WBP_JourneyDestination',unreal.InkJourneyDestinationPanel),('Shared','WBP_RewardReceipt',unreal.InkRewardReceiptPanel)]:
    dest='/Game/UI/Ink/Widgets/'+folder
    widget=unreal.load_asset(dest+'/'+name)
    if not widget:
        factory=unreal.WidgetBlueprintFactory();factory.set_editor_property('parent_class',parent)
        widget=TOOLS.create_asset(name,dest,unreal.WidgetBlueprint,factory)
    assert widget,name
    unreal.BlueprintEditorLibrary.compile_blueprint(widget)
    if name=='WBP_JourneyMap':
        cdo=unreal.get_default_object(widget.generated_class())
        cdo.set_editor_property('world_map_texture',unreal.load_asset('/Game/UI/Ink/Textures/Journey/T_WorldAtlas_v1'))
        cdo.set_editor_property('resource_map_texture',unreal.load_asset('/Game/UI/Ink/Textures/Journey/T_ResourceAtlas_v1'))
    assert unreal.EditorAssetLibrary.save_loaded_asset(widget)
    REPORT['widgets'].append(dict(path=widget.get_path_name(),parent=parent.static_class().get_path_name()))
(REV/'asset-import-report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
