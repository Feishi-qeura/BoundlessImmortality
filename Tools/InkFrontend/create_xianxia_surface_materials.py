"""Material-backed nine-slices keep atlas and high-resolution texture corners at logical size."""
import unreal,json
from pathlib import Path
DEST='/Game/UI/Ink/NiagaraRevision20261022';M=unreal.MaterialEditingLibrary;T=unreal.AssetToolsHelpers.get_asset_tools()
report=[]
def make(name,texture,rect):
    asset=unreal.load_asset(DEST+'/'+name)
    if asset:M.delete_all_material_expressions(asset)
    else:asset=T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    asset.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);asset.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    uv=M.create_material_expression(asset,unreal.MaterialExpressionTextureCoordinate,-600,0)
    scale=M.create_material_expression(asset,unreal.MaterialExpressionConstant2Vector,-600,100);scale.set_editor_property('r',rect[2]-rect[0]);scale.set_editor_property('g',rect[3]-rect[1])
    bias=M.create_material_expression(asset,unreal.MaterialExpressionConstant2Vector,-300,200);bias.set_editor_property('r',rect[0]);bias.set_editor_property('g',rect[1])
    mul=M.create_material_expression(asset,unreal.MaterialExpressionMultiply,-300,0);add=M.create_material_expression(asset,unreal.MaterialExpressionAdd,0,0)
    assert M.connect_material_expressions(uv,'',mul,'A');assert M.connect_material_expressions(scale,'',mul,'B');assert M.connect_material_expressions(mul,'',add,'A');assert M.connect_material_expressions(bias,'',add,'B')
    sample=M.create_material_expression(asset,unreal.MaterialExpressionTextureSample,300,0);sample.set_editor_property('texture',texture)
    assert M.connect_material_expressions(add,'',sample,'UVs');assert M.connect_material_property(sample,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);assert M.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(asset);assert unreal.EditorAssetLibrary.save_loaded_asset(asset);report.append(dict(asset=asset.get_path_name(),source=texture.get_path_name(),source_rect=rect))
backing=unreal.load_asset(DEST+'/Textures/T_XianxiaBacking')
make('M_XianxiaBacking',backing,(12/2172,16/724,2160/2172,690/724))
buttons=unreal.load_asset(DEST+'/Textures/T_XianxiaButtonsRectangular')
regions=json.loads((Path(unreal.Paths.project_dir())/'SourceAssets/UI/NiagaraRevision20261022/button-regions.json').read_text(encoding='utf-8'))
for name,rect in regions.items():make('M_XianxiaButton'+name,buttons,rect)
provided=unreal.load_asset(DEST+'/Textures/T_ProvidedMenuControls')
assert provided,'Import the exact user menu atlas first'
columns={'Start':(14,229),'Continue':(230,432),'Settings':(433,633),'Exit':(634,833)}
for key,(x0,x1) in columns.items():
    for state,(y0,y1) in {'Normal':(45,144),'Hover':(144,232),'Disabled':(232,325)}.items():
        make('M_ProvidedMenu'+key+state,provided,(x0/1254,y0/1254,x1/1254,y1/1254))
# Shared empty skins from that same reference retain live text on other pages.
for name,rect in {'Normal':(22,806,207,883),'Selected':(393,805,582,884),'Hover':(206,805,394,884),'Pressed':(206,805,394,884)}.items():
    make('M_XianxiaButton'+name,provided,tuple(v/1254 for v in rect))
(Path(unreal.Paths.project_dir())/'Saved/Codex/NiagaraUiRevision20261022/surface-materials.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
