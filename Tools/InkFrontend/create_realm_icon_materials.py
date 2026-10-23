"""Mask the source realm chart paper in the UI shader; preserve original chart texture."""
import json,re
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir());DEST='/Game/UI/Ink/NiagaraRevision20261022'
M=unreal.MaterialEditingLibrary;T=unreal.AssetToolsHelpers.get_asset_tools()
name='M_RealmIconTransparent';parent=unreal.load_asset(DEST+'/'+name)
if not parent:
    parent=T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    parent.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);parent.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    uv=M.create_material_expression(parent,unreal.MaterialExpressionTextureCoordinate,-600,0)
    rect=M.create_material_expression(parent,unreal.MaterialExpressionVectorParameter,-600,100);rect.set_editor_property('parameter_name','SourceRect');rect.set_editor_property('default_value',unreal.LinearColor(0,0,1,1))
    coord=M.create_material_expression(parent,unreal.MaterialExpressionCustom,-300,0);coord.set_editor_property('code','return Rect.xy+UV*float2(Rect.z,Height);');coord.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    pins=[]
    for pin_name in ('UV','Rect','Height'):
        p=unreal.CustomInput();p.set_editor_property('input_name',pin_name);pins.append(p)
    coord.set_editor_property('inputs',pins);assert M.connect_material_expressions(uv,'',coord,'UV');assert M.connect_material_expressions(rect,'',coord,'Rect');assert M.connect_material_expressions(rect,'A',coord,'Height')
    tex=M.create_material_expression(parent,unreal.MaterialExpressionTextureSample,0,0);tex.set_editor_property('texture',unreal.load_asset('/Game/UI/Ink/Cultivation/T_RealmChart'))
    assert M.connect_material_expressions(coord,'',tex,'UVs');assert M.connect_material_property(tex,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    alpha=M.create_material_expression(parent,unreal.MaterialExpressionCustom,300,100);alpha.set_editor_property('code','return Alpha*(1-smoothstep(.76,.95,min(Color.r,min(Color.g,Color.b))));');alpha.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    p=unreal.CustomInput();p.set_editor_property('input_name','Color');a=unreal.CustomInput();a.set_editor_property('input_name','Alpha');alpha.set_editor_property('inputs',[p,a]);assert M.connect_material_expressions(tex,'',alpha,'Color');assert M.connect_material_expressions(tex,'A',alpha,'Alpha');assert M.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(parent);assert unreal.EditorAssetLibrary.save_loaded_asset(parent)
source=(ROOT/'Source/BoundlessImmortality/Cultivation/CultivationNameplate.cpp').read_text(encoding='utf-8')
block=source[source.index('static const FVector4 Regions[]='):source.index('const auto R=Regions[')]
regions=[tuple(float(v) for v in x) for x in re.findall(r'FVector4\((\d+),(\d+),(\d+),(\d+)\)',block)];assert len(regions)==34
report=[]
for n,(x0,y0,x1,y1) in enumerate(regions):
    name=f'MI_RealmIconStage_{n:02}';asset=unreal.load_asset(DEST+'/'+name) or T.create_asset(name,DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    M.set_material_instance_parent(asset,parent);M.set_material_instance_vector_parameter_value(asset,'SourceRect',unreal.LinearColor(x0/1536,y0/1024,(x1-x0)/1536,(y1-y0)/1024));assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    report.append(dict(stage=n,asset=asset.get_path_name(),rect=[x0,y0,x1,y1]))
(ROOT/'Saved/Codex/NiagaraUiRevision20261022/realm-icon-materials.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
