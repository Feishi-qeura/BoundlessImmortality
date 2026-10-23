"""Import intact generated atlas PNGs; create title crops and adjustable row nine-slices."""
import json
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir());SOURCE=ROOT/'SourceAssets/UI/NiagaraRevision20261022';DEST='/Game/UI/Ink/NiagaraRevision20261022'
T=unreal.AssetToolsHelpers.get_asset_tools();M=unreal.MaterialEditingLibrary
regions=json.loads((SOURCE/'rank-theme-regions.json').read_text(encoding='utf-8'));report=[]
for key in regions:
    name=regions[key].get('texture','T_Rank'+key);tex=unreal.load_asset(DEST+'/Textures/'+name)
    if not tex:
        task=unreal.AssetImportTask();task.set_editor_property('filename',str(SOURCE/(name+'.png')));task.set_editor_property('destination_path',DEST+'/Textures');task.set_editor_property('automated',True);task.set_editor_property('save',True);T.import_asset_tasks([task]);tex=unreal.load_asset(DEST+'/Textures/'+name)
    assert tex,key
    tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);unreal.EditorAssetLibrary.save_loaded_asset(tex)

def node(asset,kind,x,y):return M.create_material_expression(asset,kind,x,y)
def connect(a,p,b,q):assert M.connect_material_expressions(a,p,b,q)
def vector(asset,name,value,x,y):
    n=node(asset,unreal.MaterialExpressionVectorParameter,x,y);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',unreal.LinearColor(*value));return n
def material(name):
    a=unreal.load_asset(DEST+'/'+name)
    if a:M.delete_all_material_expressions(a)
    else:a=T.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    a.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);a.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);return a

parent=material('M_RankTierSlice');uv=node(parent,unreal.MaterialExpressionTextureCoordinate,-700,0)
origin=vector(parent,'CropOrigin',(0,0,0,0),-700,100);extent=vector(parent,'CropSize',(1,1,0,0),-700,180);target=vector(parent,'PanelSize',(600,88,0,0),-700,260)
source=vector(parent,'SourceSize',(600,88,0,0),-700,340)
code=node(parent,unreal.MaterialExpressionCustom,-300,0)
code.set_editor_property('code','''
float2 srcEdge=float2(.23,.18);
float2 edge=float2(min(.49,srcEdge.x*Source.x/max(Source.y,1)*Panel.y/max(Panel.x,1)),.18);
float2 mapped=srcEdge+(UV-edge)*(1-2*srcEdge)/(1-2*edge);
mapped=lerp(mapped,UV*srcEdge/edge,1-step(edge,UV));
mapped=lerp(mapped,1-(1-UV)*srcEdge/edge,step(1-edge,UV));
return Origin.xy+mapped*Extent.xy;
''');code.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
pins=[]
for name in ('UV','Origin','Extent','Panel','Source'):
    pin=unreal.CustomInput();pin.set_editor_property('input_name',name);pins.append(pin)
code.set_editor_property('inputs',pins);connect(uv,'',code,'UV');connect(origin,'',code,'Origin');connect(extent,'',code,'Extent');connect(target,'',code,'Panel');connect(source,'',code,'Source')
sample=node(parent,unreal.MaterialExpressionTextureSampleParameter2D,0,0);sample.set_editor_property('parameter_name','TierTexture');sample.set_editor_property('texture',unreal.load_asset(DEST+'/Textures/T_RankSwordRows'));connect(code,'',sample,'UVs');assert M.connect_material_property(sample,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);assert M.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY);M.recompile_material(parent);unreal.EditorAssetLibrary.save_loaded_asset(parent)
for theme in ('Sword','Speed','Damage'):
    meta=regions[theme+'Rows'];w,h=meta['size'];tex=unreal.load_asset(DEST+'/Textures/'+meta.get('texture','T_Rank'+theme+'Rows'))
    for rank in range(1,11):
        r=meta['regions'][10-rank];name=f'M_Rank{theme}Tier{rank:02d}';a=unreal.load_asset(DEST+'/'+name)
        if not a:a=T.create_asset(name,DEST,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        M.set_material_instance_parent(a,parent);M.set_material_instance_texture_parameter_value(a,'TierTexture',tex);M.set_material_instance_vector_parameter_value(a,'CropOrigin',unreal.LinearColor(r[0]/w,r[1]/h,0,0));M.set_material_instance_vector_parameter_value(a,'CropSize',unreal.LinearColor((r[2]-r[0])/w,(r[3]-r[1])/h,0,0));unreal.EditorAssetLibrary.save_loaded_asset(a);report.append(dict(asset=a.get_path_name(),theme=theme,rank=rank,rect=r))
    meta=regions[theme+'Header'];w,h=meta['size'];r=meta['regions'][0];a=material('M_Rank'+theme+'Header');uv=node(a,unreal.MaterialExpressionTextureCoordinate,-700,0)
    code=node(a,unreal.MaterialExpressionCustom,-300,0);code.set_editor_property('code',f'return float2({r[0]/w},{r[1]/h})+UV*float2({(r[2]-r[0])/w},{(r[3]-r[1])/h});');code.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    pin=unreal.CustomInput();pin.set_editor_property('input_name','UV');code.set_editor_property('inputs',[pin]);connect(uv,'',code,'UV')
    sample=node(a,unreal.MaterialExpressionTextureSample,0,0);sample.set_editor_property('texture',unreal.load_asset(DEST+'/Textures/T_Rank'+theme+'Header'));connect(code,'',sample,'UVs');assert M.connect_material_property(sample,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);assert M.connect_material_property(sample,'A',unreal.MaterialProperty.MP_OPACITY);M.recompile_material(a);unreal.EditorAssetLibrary.save_loaded_asset(a);report.append(dict(asset=a.get_path_name(),theme=theme,header_text_baked=True,rect=r))
(ROOT/'Saved/Codex/NiagaraUiRevision20261022/rank-theme-assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.SystemLibrary.quit_editor()
