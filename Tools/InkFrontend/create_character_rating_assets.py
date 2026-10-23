"""Import isolated rank lettering and author the continuous quality-frame material."""
import json
from pathlib import Path
import unreal

DEST='/Game/UI/Ink/CharacterRating20261022'
ROOT=Path(unreal.Paths.project_dir())
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
MEL=unreal.MaterialEditingLibrary
report={'textures':[]}
for grade in ('D','C','B','A','S','SS','SSS'):
    task=unreal.AssetImportTask()
    task.set_editor_property('filename',str(ROOT/f'SourceAssets/UI/CharacterRating20261022/T_Rating_{grade}.png'))
    task.set_editor_property('destination_path',DEST)
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',True)
    TOOLS.import_asset_tasks([task])
    texture=unreal.load_asset(DEST+'/T_Rating_'+grade)
    assert texture,grade
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('srgb',True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    report['textures'].append(texture.get_path_name())

m=unreal.load_asset(DEST+'/M_QualityFrameUI')
if not m:
    m=TOOLS.create_asset('M_QualityFrameUI',DEST,unreal.Material,unreal.MaterialFactoryNew())
    m.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI)
    m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    uv=MEL.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate,-600,0)
    params={}
    for name,value in [('GlowColor',(0.1,.6,1,1)),('CanvasSize',(120,124,0,0)),('FrameHalfSize',(50,52,0,0))]:
        p=MEL.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-600,100+len(params)*80)
        p.set_editor_property('parameter_name',name);p.set_editor_property('default_value',unreal.LinearColor(*value));params[name]=p
    for name,value in [('BorderWidth',2),('GlowRadius',4),('Intensity',1),('CornerRadius',5)]:
        p=MEL.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-600,100+len(params)*80)
        p.set_editor_property('parameter_name',name);p.set_editor_property('default_value',float(value));params[name]=p
    alpha=MEL.create_material_expression(m,unreal.MaterialExpressionCustom,-100,0)
    names=['UV','CanvasSize','FrameHalfSize','BorderWidth','GlowRadius','Intensity','CornerRadius']
    inputs=[]
    for name in names:
        i=unreal.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
    alpha.set_editor_property('inputs',inputs)
    alpha.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    alpha.set_editor_property('code','float2 q=abs((UV-.5)*CanvasSize.xy)-FrameHalfSize.xy+CornerRadius; float d=length(max(q,0))+min(max(q.x,q.y),0)-CornerRadius; float edge=exp(-pow(d/max(BorderWidth,.1),2)); float glow=exp(-abs(d)/max(GlowRadius,.1)); return saturate((edge*.85+glow*.45)*Intensity);')
    assert MEL.connect_material_expressions(uv,'',alpha,'UV')
    for name in names[1:]:assert MEL.connect_material_expressions(params[name],'',alpha,name)
    assert MEL.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    assert MEL.connect_material_property(params['GlowColor'],'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    assert unreal.EditorAssetLibrary.save_loaded_asset(m)
report['material']=m.get_path_name()
(ROOT/'Saved/Codex/CharacterRating20261022/assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
