"""Run inside UE Editor. Creates only Refactor20261021 assets and migrates seven WBP font references."""
import ast
import hashlib
import json
import struct
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir())
OUT=ROOT/'Saved/Codex/UIRefactor20261021'
DEST='/Game/UI/Ink/Refactor20261021'
FONTS=ROOT/'docs/design/2026-10-21-ink-ui-standard-v1/fonts'
REPORT={'created_or_reused':[],'fonts':[],'materials':[]}
REPAIR='InkRepairUiMaterials' in unreal.SystemLibrary.get_command_line()
lib=unreal.InkUiStyleEditorLibrary
objects=lib.import_refactor_fonts()
assert len(objects)==7, 'Font import failed'

# Reuse the delivered, dependency-free Unicode cmap reader without importing its Pillow-based report generator.
source=ROOT/'docs/design/2026-10-21-ink-ui-standard-v1/scripts/validate-delivery.py'
module=ast.parse(source.read_text(encoding='utf-8-sig'))
fn=next(n for n in module.body if isinstance(n,ast.FunctionDef) and n.name=='font_characters')
exec(compile(ast.Module(body=[fn],type_ignores=[]),str(source),'exec'),globals())
paths=['LXGWWenKai/LXGWWenKai-Regular.ttf','MaShanZheng/MaShanZheng-Regular.ttf','NotoSansSC/NotoSansCJKsc-Regular.otf']
coverage=[]
for name in paths:
    p=FONTS/name
    chars=font_characters(p)
    coverage.append(chars)
    REPORT['fonts'].append({'source':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'glyphs':len(chars)})
missing=sorted(coverage[2]-(coverage[0]|coverage[1]))
ranges=[]
for cp in missing:
    if ranges and cp==ranges[-1]+1:ranges[-1]=cp
    else:ranges.extend([cp,cp])
assert lib.set_missing_glyph_ranges(objects[4],objects[2],ranges)
REPORT['display_fallback']='MaShanZheng primary; WenKai missing glyph fallback; Noto subranges only absent from both'
REPORT['noto_display_missing_ranges']=len(ranges)//2

assets=unreal.AssetToolsHelpers.get_asset_tools()
mel=unreal.MaterialEditingLibrary
scene=unreal.load_asset('/Game/UI/Ink/Cohesion20261021/T_CohesionScenes')
assert scene

def create_material(name,code,scene_inputs=False,texture=None):
    target=DEST+'/Materials/'+name
    existing=unreal.load_asset(target)
    if existing and not REPAIR:
        REPORT['created_or_reused'].append({'path':target,'mode':'reused'})
        return existing
    mat=existing or assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    if existing:mel.delete_all_material_expressions(mat)
    mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    custom=mel.create_material_expression(mat,unreal.MaterialExpressionCustom,0,0)
    custom.set_editor_property('code',code)
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    names=['UV']+(['Atlas','Region','FeatherFraction'] if scene_inputs else [])
    inputs=[]
    for name in names:
        item=unreal.CustomInput()
        item.set_editor_property('input_name',name)
        inputs.append(item)
    custom.set_editor_property('inputs',inputs)
    uv=mel.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate,-500,0)
    assert mel.connect_material_expressions(uv,'',custom,'UV')
    if scene_inputs:
        tex=mel.create_material_expression(mat,unreal.MaterialExpressionTextureObjectParameter,-500,150)
        tex.set_editor_property('parameter_name','Atlas');tex.set_editor_property('texture',texture or scene)
        region=mel.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,-500,300)
        region.set_editor_property('parameter_name','Region');region.set_editor_property('default_value',unreal.LinearColor(0,0,.5,.25))
        feather=mel.create_material_expression(mat,unreal.MaterialExpressionScalarParameter,-500,450)
        feather.set_editor_property('parameter_name','FeatherFraction');feather.set_editor_property('default_value',.18)
        for node,pin in [(tex,'Atlas'),(region,'Region'),(feather,'FeatherFraction')]:assert mel.connect_material_expressions(node,'',custom,pin)
    for color,name in [(True,'color'),(False,'opacity')]:
        mask=mel.create_material_expression(mat,unreal.MaterialExpressionComponentMask,250,0 if color else 200)
        for channel in ['r','g','b']:mask.set_editor_property(channel,color)
        mask.set_editor_property('a',not color)
        assert mel.connect_material_expressions(custom,'',mask,'')
        assert mel.connect_material_property(mask,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if color else unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    REPORT['created_or_reused'].append({'path':target,'mode':'created'})
    return mat

base=create_material('M_InkSceneSoft',
'''float2 p=Region.xy+float2(0.5,0.25)*UV;
float4 c=Texture2DSample(Atlas,AtlasSampler,p);
float a=saturate((1.0-UV.y)/max(FeatherFraction,0.001));
a=a*a*(3.0-2.0*a);
return float4(c.rgb,c.a*a);''',True)
for tile in range(8):
    name='MI_InkSceneSoft_'+str(tile)
    target=DEST+'/Materials/'+name
    instance=unreal.load_asset(target)
    if not instance:
        instance=assets.create_asset(name,DEST+'/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(instance,base)
        x=(tile%2)*.5;y=(tile//2)*.25
        mel.set_material_instance_vector_parameter_value(instance,'Region',unreal.LinearColor(x,y,x+.5,y+.25))
        unreal.EditorAssetLibrary.save_loaded_asset(instance)
    REPORT['materials'].append({'path':target,'tile':tile,'atlas':scene.get_path_name(),'uv':[(tile%2)*.5,(tile//2)*.25,(tile%2+1)*.5,(tile//2+1)*.25],'feather':'runtime 36 logical units / displayed art height'})
create_material('M_InkHudVeil',
'''float wave=sin(UV.x*22.4)*0.018+sin(UV.x*43.0+2.0)*0.012;
float a=smoothstep(0.0,0.68,saturate(UV.y+wave));
return float4(0.006,0.014,0.009,a*0.88);''')
create_material('M_InkCultivateSigil',
'''float4 c=Texture2DSample(Atlas,AtlasSampler,UV);
float shade=0.72+0.28*(1.0-dot(c.rgb,float3(0.299,0.587,0.114)));
return float4(float3(0.95,0.58,0.12)*shade,c.a);''',True,unreal.load_asset('/Game/UI/Ink/Textures/T_InkMeditation'))

for obj in objects:
    unreal.EditorAssetLibrary.save_loaded_asset(obj)
    REPORT['created_or_reused'].append({'path':obj.get_path_name(),'mode':'font/theme'})
REPORT['migration']=lib.migrate_refactor_typography()
assert REPORT['migration'].startswith('Migrated 7 WBP'),REPORT['migration']
for suffix in ['Frontend','MainMenu','Character','Inventory','Leaderboard','Saves','Settings']:
    assert unreal.EditorAssetLibrary.save_asset('/Game/UI/Ink/Widgets/WBP_Ink'+suffix)
(OUT/'theme-import.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('UI_THEME_IMPORT_OK: '+REPORT['migration'])
