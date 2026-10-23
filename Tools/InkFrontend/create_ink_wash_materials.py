"""Ink paper surfaces without stretched border art; navigation alpha increases downwards."""
import json
from pathlib import Path
import unreal
M=unreal.MaterialEditingLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();DEST='/Game/UI/Ink/NiagaraRevision20261022';report=[]
base=r'''
float grain=frac(sin(dot(floor(UV*float2(620,620)),float2(12.9898,78.233)))*43758.5453);
// Layered irregular edge washes retain their proportions on every panel size.
float cloud=sin(UV.x*13.0+UV.y*4.0+sin(UV.y*19.0))*sin(UV.x*5.0-UV.y*11.0);
float detail=sin(UV.x*43.0+sin(UV.y*29.0))*sin(UV.y*37.0+UV.x*7.0);
float edge=pow(saturate(abs(UV.x-.5)*2.0),3.0);
float ink=saturate((edge*.20+pow(UV.y,3.0)*.24)*(.68+.28*cloud+.12*detail));
float3 paper=float3(.80,.78,.70)+(grain-.5)*.025;
paper=lerp(paper,float3(.20,.27,.25),ink);
'''
for name,alpha in [('M_InkWash','0.98'),('M_InkNavigationFade','pow(saturate(UV.y),1.65)')]:
    asset=unreal.load_asset(DEST+'/'+name)
    if asset:M.delete_all_material_expressions(asset)
    else:asset=T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    asset.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);asset.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    uv=M.create_material_expression(asset,unreal.MaterialExpressionTextureCoordinate,-300,0)
    code=M.create_material_expression(asset,unreal.MaterialExpressionCustom,0,0);code.set_editor_property('code',base+'return float4(paper,'+alpha+');');code.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    pin=unreal.CustomInput();pin.set_editor_property('input_name','UV');code.set_editor_property('inputs',[pin]);assert M.connect_material_expressions(uv,'',code,'UV')
    for color in (True,False):
        mask=M.create_material_expression(asset,unreal.MaterialExpressionComponentMask,300,0 if color else 200)
        for k in ('r','g','b'):mask.set_editor_property(k,color)
        mask.set_editor_property('a',not color);assert M.connect_material_expressions(code,'',mask,'');assert M.connect_material_property(mask,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if color else unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(asset);assert unreal.EditorAssetLibrary.save_loaded_asset(asset);report.append(dict(asset=asset.get_path_name(),alpha=alpha,border='native one logical pixel gold'))
(Path(unreal.Paths.project_dir())/'Saved/Codex/NiagaraUiRevision20261022/ink-wash-materials.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
