"""Author only character-profile CPU Niagara systems and their UI materials."""
import json
from pathlib import Path
import unreal

DEST = '/Game/UI/Ink/CharacterProfile20261022'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary

def material(name, core=False):
    m = unreal.load_asset(DEST + '/' + name)
    if m: return m
    m = TOOLS.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
    # Translucent blending keeps locked black border motes visible on ivory paper.
    m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -600, 0)
    if not core:
        vertex = MEL.create_material_expression(m, unreal.MaterialExpressionVertexColor, -600, 200)
        dot = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -200, 0)
        dot.set_editor_property('code', 'float2 p=(UV-.5)*2; return pow(saturate(1-dot(p,p)),2);')
        dot.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
        item = unreal.CustomInput(); item.set_editor_property('input_name', 'UV'); dot.set_editor_property('inputs', [item])
        assert MEL.connect_material_expressions(uv, '', dot, 'UV')
        alpha = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, 0, 0)
        assert MEL.connect_material_expressions(dot, '', alpha, 'A')
        assert MEL.connect_material_expressions(vertex, 'A', alpha, 'B')
        assert MEL.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
        assert MEL.connect_material_property(vertex, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        color = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 180)
        color.set_editor_property('parameter_name', 'CoreColor'); color.set_editor_property('default_value', unreal.LinearColor(.04, .32, 1, 1))
        halo = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 300)
        halo.set_editor_property('parameter_name', 'Halo'); halo.set_editor_property('default_value', 0.)
        pulse = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 400)
        pulse.set_editor_property('parameter_name', 'Pulse'); pulse.set_editor_property('default_value', 1.)
        for opacity in (False, True):
            custom = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -200, 0 if opacity else 200)
            inputs = []
            for name in ('UV', 'C', 'H', 'P'):
                item = unreal.CustomInput(); item.set_editor_property('input_name', name); inputs.append(item)
            custom.set_editor_property('inputs', inputs)
            custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1 if opacity else unreal.CustomMaterialOutputType.CMOT_FLOAT3)
            custom.set_editor_property('code',
                'float r=length((UV-.5)*2); return saturate(pow(saturate(1-r),4)*2.2+H*exp(-pow((r-.80)*52,2))*.9)*P;'
                if opacity else 'float r=length((UV-.5)*2); return lerp(C.rgb,float3(1,1,.96),pow(saturate(1-r*3),3)*.85);')
            for source, output, target in ((uv, '', 'UV'), (color, '', 'C'), (halo, '', 'H'), (pulse, '', 'P')):
                assert MEL.connect_material_expressions(source, output, custom, target)
            assert MEL.connect_material_property(custom, '', unreal.MaterialProperty.MP_OPACITY if opacity else unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m)
    assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

mote = material('M_SlotMoteUI')
material('M_DantianUI', True)
blueprint = unreal.load_asset(DEST + '/WBP_CharacterProfile')
if not blueprint:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.InkCharacterProfilePanel)
    blueprint = TOOLS.create_asset('WBP_CharacterProfile', DEST, unreal.WidgetBlueprint, factory)
    assert blueprint
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
report = dict(systems=[], materials=['M_SlotMoteUI', 'M_DantianUI'])
for kind, name in ((4, 'NS_NodeBorder'), (5, 'NS_Dantian')):
    system = unreal.load_asset(DEST + '/' + name)
    if not system:
        system = unreal.InkCultivationEditorLibrary.create_title_particle_system(DEST + '/' + name, mote, kind)
    assert system, name
    assert unreal.InkCultivationEditorLibrary.compile_system(system), name
    assert unreal.EditorAssetLibrary.save_loaded_asset(system), name
    report['systems'].append(dict(name=name, inspection=json.loads(unreal.InkCultivationEditorLibrary.inspect_system(system))))
out = Path(unreal.Paths.project_dir()) / 'Saved/Codex/CharacterProfile20261022/assets.json'
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.SystemLibrary.quit_editor()
