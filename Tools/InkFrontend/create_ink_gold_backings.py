"""Import the three generated originals and preserve gold corners with shader nine-slicing."""
from pathlib import Path
import json
import unreal

ROOT=Path(unreal.Paths.project_dir());DEST='/Game/UI/Ink/NiagaraRevision20261022'
M=unreal.MaterialEditingLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();report=[]
for name in ('T_InkGoldNamecard','T_InkGoldLeaderboardPortrait','T_InkGoldLeaderboardLandscape'):
    texture=unreal.load_asset(DEST+'/Textures/'+name)
    if not texture:
        task=unreal.AssetImportTask();task.set_editor_property('filename',str(ROOT/'SourceAssets/UI/NiagaraRevision20261022'/(name+'.png')))
        task.set_editor_property('destination_path',DEST+'/Textures');task.set_editor_property('automated',True);task.set_editor_property('save',True)
        T.import_asset_tasks([task]);texture=unreal.load_asset(DEST+'/Textures/'+name)
    assert texture,name
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)

specs=[
 ('M_InkGoldNamecard','T_InkGoldNamecard',(40,87,1835,755),(1874,839),(368,184)),
 ('M_InkGoldRankCompact','T_InkGoldLeaderboardPortrait',(25,26,933,1600),(958,1642),(300,588)),
 ('M_InkGoldRankPortrait','T_InkGoldLeaderboardPortrait',(25,26,933,1600),(958,1642),(720,1220)),
 ('M_InkGoldRankLandscape','T_InkGoldLeaderboardLandscape',(6,39,1465,996),(1470,1070),(880,640)),
]
for name,source,rect,texsize,size in specs:
    asset=unreal.load_asset(DEST+'/'+name)
    if asset:M.delete_all_material_expressions(asset)
    else:asset=T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    asset.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);asset.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    uv=M.create_material_expression(asset,unreal.MaterialExpressionTextureCoordinate,-600,0)
    code=M.create_material_expression(asset,unreal.MaterialExpressionCustom,-300,0)
    # Retain 64 source pixels in each corner at 12 logical UI pixels, regardless
    # of panel aspect. Only the empty center and edge strips adapt to layout.
    sw,sh=rect[2]-rect[0],rect[3]-rect[1]
    code.set_editor_property('code',f'''
float2 targetEdge=float2(12.0/{size[0]}.0,12.0/{size[1]}.0);
float2 sourceEdge=float2(64.0/{sw}.0,64.0/{sh}.0);
float2 middle=sourceEdge+(UV-targetEdge)*(1.0-2.0*sourceEdge)/(1.0-2.0*targetEdge);
float2 low=UV*sourceEdge/targetEdge;
float2 high=1.0-(1.0-UV)*sourceEdge/targetEdge;
float2 mapped=lerp(middle,low,1.0-step(targetEdge,UV));
mapped=lerp(mapped,high,step(1.0-targetEdge,UV));
return float2({rect[0]/texsize[0]}, {rect[1]/texsize[1]})+mapped*float2({sw/texsize[0]}, {sh/texsize[1]});
''')
    code.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    pin=unreal.CustomInput();pin.set_editor_property('input_name','UV');code.set_editor_property('inputs',[pin]);assert M.connect_material_expressions(uv,'',code,'UV')
    sample=M.create_material_expression(asset,unreal.MaterialExpressionTextureSample,0,0);sample.set_editor_property('texture',unreal.load_asset(DEST+'/Textures/'+source))
    assert M.connect_material_expressions(code,'',sample,'UVs')
    assert M.connect_material_property(sample,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert M.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(asset);assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    report.append(dict(asset=asset.get_path_name(),source=source,rect=rect,logical_size=size,corner_pixels=12))
(ROOT/'Saved/Codex/NiagaraUiRevision20261022/ink-gold-backings.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
