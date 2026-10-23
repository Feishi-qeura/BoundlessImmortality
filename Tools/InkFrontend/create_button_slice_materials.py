"""Add adaptive skins from the existing button atlas without editing source art or old materials."""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
DEST = '/Game/UI/Ink/NiagaraRevision20261022'
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
TEXTURE = unreal.load_asset(DEST + '/Textures/T_ProvidedMenuControls')
assert TEXTURE
REPORT = []

# The existing crops are retained exactly. Only the blank center expands;
# the end ornaments scale uniformly with the control height.
REGIONS = {'Normal': (22, 806, 207, 883), 'Selected': (393, 805, 582, 884),
           'Hover': (206, 805, 394, 884), 'Pressed': (206, 805, 394, 884)}
for state, rect in REGIONS.items():
    name = 'M_XianxiaButton' + state + 'Slice'
    asset = unreal.load_asset(DEST + '/' + name)
    if asset:
        M.delete_all_material_expressions(asset)
    else:
        asset = T.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    asset.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
    asset.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    uv = M.create_material_expression(asset, unreal.MaterialExpressionTextureCoordinate, -600, 0)
    code = M.create_material_expression(asset, unreal.MaterialExpressionCustom, -300, 0)
    x0, y0, x1, y1 = rect
    aspect = (x1 - x0) / (y1 - y0)
    # Screen-space derivatives follow DPI, viewport changes and render transforms.
    # Never feed Slate Box UVs into an already cropped atlas material.
    code.set_editor_property('code', f'''
float du = max(length(float2(ddx(UV.x), ddy(UV.x))), 0.000001);
float dv = max(length(float2(ddx(UV.y), ddy(UV.y))), 0.000001);
float aspect = dv / du;
float sourceEdge = 0.22;
float edge = min(0.49, sourceEdge * {aspect:.10f} / max(aspect, 0.001));
float u = sourceEdge + (UV.x - edge) * (1 - 2 * sourceEdge) / (1 - 2 * edge);
if (UV.x < edge) u = UV.x * sourceEdge / edge;
if (UV.x > 1 - edge) u = 1 - (1 - UV.x) * sourceEdge / edge;
return float2({x0 / 1254:.10f}, {y0 / 1254:.10f}) + float2(u, UV.y) * float2({(x1 - x0) / 1254:.10f}, {(y1 - y0) / 1254:.10f});
''')
    code.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    pin = unreal.CustomInput()
    pin.set_editor_property('input_name', 'UV')
    code.set_editor_property('inputs', [pin])
    assert M.connect_material_expressions(uv, '', code, 'UV')
    sample = M.create_material_expression(asset, unreal.MaterialExpressionTextureSample, 0, 0)
    sample.set_editor_property('texture', TEXTURE)
    assert M.connect_material_expressions(code, '', sample, 'UVs')
    assert M.connect_material_property(sample, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert M.connect_material_property(sample, 'A', unreal.MaterialProperty.MP_OPACITY)
    M.recompile_material(asset)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    REPORT.append(dict(asset=asset.get_path_name(), source=TEXTURE.get_path_name(), crop=rect, source_aspect=aspect, source_edge=.22))

OUT = ROOT / 'Saved/Codex/ButtonReadability20261022'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'button-slice-materials.json').write_text(json.dumps(REPORT, indent=2), encoding='utf-8')
unreal.SystemLibrary.quit_editor()
