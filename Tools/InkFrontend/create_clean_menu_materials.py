"""Exclude adjacent atlas frames from Settings/Exit; preserve artwork scale and all source assets."""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
DEST = '/Game/UI/Ink/NiagaraRevision20261022'
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
texture = unreal.load_asset(DEST + '/Textures/T_ProvidedMenuControls')
assert texture
report = []
for key, x0, x1, right in [('Settings', 433, 633, 629.5), ('Exit', 634, 833, 825.5)]:
    for state, y0, y1 in [('Normal', 45, 144), ('Hover', 144, 232), ('Disabled', 232, 325)]:
        name = 'M_ProvidedMenu' + key + state + 'Clean'
        asset = unreal.load_asset(DEST + '/' + name)
        if asset:
            M.delete_all_material_expressions(asset)
        else:
            asset = T.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
        asset.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
        asset.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        uv = M.create_material_expression(asset, unreal.MaterialExpressionTextureCoordinate, -650, 0)
        crop = M.create_material_expression(asset, unreal.MaterialExpressionCustom, -400, 0)
        crop.set_editor_property('code', f'return float2({x0 / 1254:.10f},{y0 / 1254:.10f})+UV*float2({(x1-x0) / 1254:.10f},{(y1-y0) / 1254:.10f});')
        crop.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', 'UV')
        crop.set_editor_property('inputs', [pin])
        assert M.connect_material_expressions(uv, '', crop, 'UV')
        sample = M.create_material_expression(asset, unreal.MaterialExpressionTextureSample, -150, 0)
        sample.set_editor_property('texture', texture)
        assert M.connect_material_expressions(crop, '', sample, 'UVs')
        guard = M.create_material_expression(asset, unreal.MaterialExpressionCustom, 100, 150)
        # Fade inside the transparent gutter; do not stretch or cut into the actual frame.
        guard.set_editor_property('code', f'return Alpha*saturate({right} - ({x0} + UV.x*{x1-x0}));')
        guard.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
        pins = []
        for name in ('UV', 'Alpha'):
            pin = unreal.CustomInput()
            pin.set_editor_property('input_name', name)
            pins.append(pin)
        guard.set_editor_property('inputs', pins)
        assert M.connect_material_expressions(uv, '', guard, 'UV')
        assert M.connect_material_expressions(sample, 'A', guard, 'Alpha')
        assert M.connect_material_property(sample, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        assert M.connect_material_property(guard, '', unreal.MaterialProperty.MP_OPACITY)
        M.recompile_material(asset)
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
        report.append(dict(asset=asset.get_path_name(), source=texture.get_path_name(), crop=[x0, y0, x1, y1], right_guard=right))

OUT = ROOT / 'Saved/Codex/ButtonReadability20261022'
(OUT / 'menu-edge-materials.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.SystemLibrary.quit_editor()
