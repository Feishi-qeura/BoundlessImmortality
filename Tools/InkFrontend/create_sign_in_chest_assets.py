"""Create only the new claimable-chest UI Niagara/material, never rewrite shared UI assets."""
import json
from pathlib import Path
import unreal
DEST='/Game/UI/Ink/SignIn20261022'
assets=unreal.AssetToolsHelpers.get_asset_tools()
mel=unreal.MaterialEditingLibrary
mat=unreal.load_asset(DEST+'/M_ClaimableChestUI')
if not mat:
    mat=assets.create_asset('M_ClaimableChestUI',DEST,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    custom=mel.create_material_expression(mat,unreal.MaterialExpressionCustom,0,0)
    custom.set_editor_property('code','float2 p=(UV-.5)*2; float r=length(p); float pulse=.65+.35*sin(Clock*3); float ring=exp(-pow((r-.78)*20,2)); float halo=exp(-r*r*3)*.18; float sparks=pow(saturate(cos(atan2(p.y,p.x)*8-Clock*2)),24)*exp(-pow((r-.86)*14,2)); return float4(1,.68,.16,saturate((ring+halo+sparks)*pulse)*.85);')
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs=[]
    for name in ('UV','Clock'):
        item=unreal.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    custom.set_editor_property('inputs',inputs)
    uv=mel.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate,-500,0)
    clock=mel.create_material_expression(mat,unreal.MaterialExpressionTime,-500,150)
    assert mel.connect_material_expressions(uv,'',custom,'UV')
    assert mel.connect_material_expressions(clock,'',custom,'Clock')
    for color in (True,False):
        mask=mel.create_material_expression(mat,unreal.MaterialExpressionComponentMask,250,0 if color else 200)
        for c in ('r','g','b'):mask.set_editor_property(c,color)
        mask.set_editor_property('a',not color)
        assert mel.connect_material_expressions(custom,'',mask,'')
        assert mel.connect_material_property(mask,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if color else unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
system=unreal.load_asset(DEST+'/NS_ClaimableChest')
if not system:
    system=unreal.InkCultivationEditorLibrary.create_sign_in_chest_system(DEST+'/NS_ClaimableChest',mat)
    assert system,'Niagara creation/compilation failed'
    unreal.EditorAssetLibrary.save_loaded_asset(system)
report={'system':system.get_path_name(),'material':mat.get_path_name(),'inspection':json.loads(unreal.InkCultivationEditorLibrary.inspect_system(system))}
p=Path(unreal.Paths.project_dir())/'Saved/Codex/SignInRank20261022/asset-report.json'
p.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
