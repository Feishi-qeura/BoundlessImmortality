"""New code-native UI materials; preserve all previous material/font/WBP assets."""
from pathlib import Path
import json
import unreal
ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/VisualRefactor20261022';OUT.mkdir(parents=True,exist_ok=True)
DEST='/Game/UI/Ink/Refactor20261022/Materials';unreal.EditorAssetLibrary.make_directory(DEST)
TOOLS=unreal.AssetToolsHelpers.get_asset_tools();M=unreal.MaterialEditingLibrary;REPORT=[]
texture_path='/Game/UI/Ink/Refactor20261022/Textures/T_NavigationAtlas'
if not unreal.load_asset(texture_path):
    task=unreal.AssetImportTask();task.set_editor_property('filename',str(ROOT/'SourceAssets/UI/VisualRefactor20261022/T_NavigationAtlas.png'))
    task.set_editor_property('destination_path','/Game/UI/Ink/Refactor20261022/Textures');task.set_editor_property('automated',True);task.set_editor_property('save',True)
    TOOLS.import_asset_tasks([task])
texture=unreal.load_asset(texture_path);assert texture
texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);unreal.EditorAssetLibrary.save_loaded_asset(texture)
REPORT.append(dict(path=texture_path,status='imported_or_reused',grid=[4,2],alpha='preserved',source='SourceAssets/UI/VisualRefactor20261022/T_NavigationAtlas.png'))
chest_path='/Game/UI/Ink/Refactor20261022/Textures/T_ChestStates'
if not unreal.load_asset(chest_path):
    task=unreal.AssetImportTask();task.set_editor_property('filename',str(ROOT/'SourceAssets/UI/VisualRefactor20261022/T_ChestStates.png'))
    task.set_editor_property('destination_path','/Game/UI/Ink/Refactor20261022/Textures');task.set_editor_property('automated',True);task.set_editor_property('save',True);TOOLS.import_asset_tasks([task])
chest=unreal.load_asset(chest_path);assert chest
chest.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
chest.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
chest.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);unreal.EditorAssetLibrary.save_loaded_asset(chest)
REPORT.append(dict(path=chest_path,status='imported_or_reused',grid=[2,1],alpha='preserved'))

def material(name,code):
    path=DEST+'/'+name;asset=unreal.load_asset(path)
    if asset:M.delete_all_material_expressions(asset)
    asset=asset or TOOLS.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    asset.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);asset.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    custom=M.create_material_expression(asset,unreal.MaterialExpressionCustom,0,0);custom.set_editor_property('code',code);custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    pin=unreal.CustomInput();pin.set_editor_property('input_name','UV');custom.set_editor_property('inputs',[pin])
    uv=M.create_material_expression(asset,unreal.MaterialExpressionTextureCoordinate,-300,0);assert M.connect_material_expressions(uv,'',custom,'UV')
    for color in (True,False):
        mask=M.create_material_expression(asset,unreal.MaterialExpressionComponentMask,300,0 if color else 200)
        for key in ('r','g','b'):mask.set_editor_property(key,color)
        mask.set_editor_property('a',not color);assert M.connect_material_expressions(custom,'',mask,'')
        assert M.connect_material_property(mask,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if color else unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(asset);assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    REPORT.append(dict(path=path,status='created',domain='UserInterface',source='procedural HLSL; no baked text or reference screenshot'))
material('M_InkPaperCloud',r'''
float grain=frac(sin(dot(floor(UV*float2(740,1140)),float2(12.9898,78.233)))*43758.5453);
float3 paper=float3(.896,.831,.723)+(grain-.5)*.014;
float mountain=.055*sin(UV.x*15)+.025*sin(UV.x*39+1.4)+.02*sin(UV.x*73);
float wash=smoothstep(.69+mountain,.99,UV.y)*.055;
paper=lerp(paper,float3(.18,.29,.31),wash);
float cloud=abs(UV.y-(.93+.018*sin(UV.x*24)+.012*sin(UV.x*51)));
float cloudLine=(1-smoothstep(.0007,.0025,cloud))*smoothstep(.5,1,abs(UV.x-.5)*2)*.24;
return float4(lerp(paper,float3(.475,.258,.060),cloudLine),.985);
''')
material('M_InkCloudRibbon',r'''
float y=.68+.075*sin(UV.x*17.0)+.03*sin(UV.x*43.0+1.0);
float envelope=smoothstep(0,.13,UV.x)*(1-smoothstep(.87,1,UV.x));
float mist=exp(-pow((UV.y-y)/.2,2))*envelope;
float stroke=exp(-pow((UV.y-y)/.008,2));
float echo=exp(-pow((UV.y-y-.07)/.0045,2))*.65;
float3 color=lerp(float3(.91,.85,.74),float3(.63,.38,.12),saturate(stroke+echo));
return float4(color,(mist*.65+(stroke+echo)*.22)*envelope);
''')
material('M_InkWheel',r'''
float2 p=(UV-.5)*2;float r=length(p);float a=atan2(p.x,-p.y);a=a<0?a+6.28318530718:a;
float sector=floor(a/.78539816339);float alt=fmod(sector,2);
float3 paper=lerp(float3(.90,.82,.66),float3(.59,.70,.70),alt*.62);
float shade=.04*sin(a*3+r*13)+.018*sin(a*19-r*8);paper+=shade;
float f=frac(a/.78539816339);float radial=(1-smoothstep(0,.008,min(f,1-f)))*step(.24,r)*step(r,.91);
float ring=exp(-pow((r-.955)/.013,2))+exp(-pow((r-.912)/.005,2))*.8+exp(-pow((r-.25)/.012,2));
float ink=smoothstep(.91,.94,r)*(1-smoothstep(.97,.985,r));
paper=lerp(paper,float3(.05,.12,.14),ink);
paper=lerp(paper,float3(.66,.40,.12),saturate(ring+radial*.55));
return float4(paper,1-smoothstep(.982,.997,r));
''')
(OUT/'material-import.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('VISUAL_MATERIALS_IMPORTED '+str(len(REPORT)))
