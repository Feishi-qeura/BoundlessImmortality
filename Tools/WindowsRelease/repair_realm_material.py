"""Repair only the generated realm mask parent; keep chart texture and 34 instance crops."""
from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.project_dir())
output = root / 'Saved/Codex/ReleaseReadiness20261022'
editing = unreal.MaterialEditingLibrary
path = '/Game/UI/Ink/NiagaraRevision20261022/M_RealmIconTransparent'
material = unreal.load_asset(path)
assert material and material.get_editor_property('material_domain') == unreal.MaterialDomain.MD_UI
texture = unreal.load_asset('/Game/UI/Ink/Cultivation/T_RealmChart')
assert texture
editing.delete_all_material_expressions(material)


def node(kind, x, y):
    return editing.create_material_expression(material, kind, x, y)


def custom(code, kind, names, x, y):
    value = node(unreal.MaterialExpressionCustom, x, y)
    value.set_editor_property('code', code)
    value.set_editor_property('output_type', kind)
    inputs = []
    for name in names:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        inputs.append(pin)
    value.set_editor_property('inputs', inputs)
    return value


def connect(source, output_name, target, input_name):
    assert editing.connect_material_expressions(source, output_name, target, input_name), (output_name, input_name)


uv = node(unreal.MaterialExpressionTextureCoordinate, -600, 0)
rect = node(unreal.MaterialExpressionVectorParameter, -600, 100)
rect.set_editor_property('parameter_name', 'SourceRect')
rect.set_editor_property('default_value', unreal.LinearColor(0, 0, 1, 1))
# Default vector/texture outputs are RGB. Carry alpha through its own scalar pin.
coords = custom('return Rect.xy+UV*float2(Rect.z,Height);', unreal.CustomMaterialOutputType.CMOT_FLOAT2,
    ('UV', 'Rect', 'Height'), -300, 0)
connect(uv, '', coords, 'UV')
connect(rect, '', coords, 'Rect')
connect(rect, 'A', coords, 'Height')
sample = node(unreal.MaterialExpressionTextureSample, 0, 0)
sample.set_editor_property('texture', texture)
connect(coords, '', sample, 'UVs')
assert editing.connect_material_property(sample, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mask = custom('return Alpha*(1-smoothstep(.76,.95,min(Color.r,min(Color.g,Color.b))));',
    unreal.CustomMaterialOutputType.CMOT_FLOAT1, ('Color', 'Alpha'), 300, 100)
connect(sample, '', mask, 'Color')
connect(sample, 'A', mask, 'Alpha')
assert editing.connect_material_property(mask, '', unreal.MaterialProperty.MP_OPACITY)
editing.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material)
(output / 'realm-material-repair.json').write_text(json.dumps(dict(asset=path,
    retained_texture=texture.get_path_name(), preserved_parameter='SourceRect',
    repair='separate scalar alpha inputs instead of swizzling the default RGB output', saved=True), indent=2), encoding='utf-8')
unreal.SystemLibrary.quit_editor()
