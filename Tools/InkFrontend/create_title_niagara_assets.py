"""Rebuild exactly the five tool-owned flowing-title assets, with disk backups.

Run in a fresh rendered editor after building BoundlessImmortalityEditor:
  UnrealEditor.exe BoundlessImmortality.uproject -ExecutePythonScript=<this file>
The four systems keep their paths. Motion is CPU Niagara position and tangent
rotation; the feathered silk UI material has no Time/Panner expressions.
"""
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path

import unreal

DEST = '/Game/UI/Ink/NiagaraRevision20261022/Flowing20261022'
KINDS = ('Honor', 'Realm', 'RealmIcon', 'AvatarFrame')
OWNED_NAMES = ('M_TitleMoteUI',) + tuple('NS_' + name + 'Particles' for name in KINDS)
PROJECT = Path(unreal.Paths.project_dir()).resolve()
OUT = PROJECT / 'Saved/Codex/NiagaraUiRevision20261022'
BACKUP = OUT / ('TitleFlowBackup_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
assets = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def backup_owned_assets():
    """Exact file copies before mutation; no folder-wide asset operations."""
    BACKUP.mkdir(parents=True, exist_ok=False)
    records = []
    asset_dir = PROJECT / 'Content/UI/Ink/NiagaraRevision20261022/Flowing20261022'
    for name in OWNED_NAMES:
        for extension in ('.uasset', '.uexp', '.ubulk'):
            source = asset_dir / (name + extension)
            if source.is_file():
                target = BACKUP / source.name
                shutil.copy2(source, target)
                records.append({'source': str(source), 'backup': str(target),
                                'sha256': hashlib.sha256(target.read_bytes()).hexdigest()})
    (BACKUP / 'manifest.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
    return records


def material_graph():
    path = DEST + '/M_TitleMoteUI'
    material = unreal.load_asset(path)
    if not material:
        material = assets.create_asset('M_TitleMoteUI', DEST, unreal.Material, unreal.MaterialFactoryNew())
    assert material, path
    mel.delete_all_material_expressions(material)
    material.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
    material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    uv = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -500, 0)
    # NiagaraUIRenderer sends particle tint through Slate VertexColor.
    particle = mel.create_material_expression(material, unreal.MaterialExpressionVertexColor, -500, 200)
    feather = mel.create_material_expression(material, unreal.MaterialExpressionCustom, -200, 0)
    feather.set_editor_property('code', '''
float u = saturate(UV.x);
float centre = .5 + .045 * sin(u * 6.283185);
float taper = pow(saturate(sin(u * 3.141593)), .65) * (.30 + .70 * u);
float d = abs(UV.y - centre) / max(.32 * taper, .008);
float soft = exp2(-d * d * 3.8);
float spine = exp2(-d * d * 22.0);
float ends = smoothstep(0.0, .20, u) * (1.0 - smoothstep(.84, 1.0, u));
return saturate((soft * .62 + spine * .38) * ends);
''')
    feather.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    item = unreal.CustomInput()
    item.set_editor_property('input_name', 'UV')
    feather.set_editor_property('inputs', [item])
    assert mel.connect_material_expressions(uv, '', feather, 'UV')
    alpha = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 50, 100)
    assert mel.connect_material_expressions(feather, '', alpha, 'A')
    assert mel.connect_material_expressions(particle, 'A', alpha, 'B')
    assert mel.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    assert mel.connect_material_property(particle, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(material)
    assert unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def rebuild():
    report = {'revision': 'open-contour-flowing-silk-v2', 'backup_dir': str(BACKUP),
              'backup_files': backup_owned_assets(), 'systems': [],
              'motion_source': 'Niagara CPU Particles.Position and tangent SpriteRotation',
              'material_shape': 'long feathered tapered silk, translucent VertexColor',
              'quality_parameter': 'User.Quality float 0..6; 1..4 parallel strands',
              'boundary': 'Asset authoring and Niagara compile; rendered PIE and visual QA are separate'}
    material = material_graph()
    report['material'] = material.get_path_name()
    for kind, name in enumerate(KINDS):
        path = DEST + '/NS_' + name + 'Particles'
        # Replacement is limited to this exact, previously backed-up allowlist.
        assert path.rsplit('/', 1)[1] in OWNED_NAMES
        system = unreal.load_asset(path)
        if not system:
            system = unreal.InkCultivationEditorLibrary.create_title_particle_system(path, material, kind)
        assert system, f'Could not create or compile {path}; original backup: {BACKUP}'
        assert unreal.InkCultivationEditorLibrary.compile_system(system), path
        assert unreal.EditorAssetLibrary.save_loaded_asset(system), path
        report['systems'].append({'kind': name, 'path': system.get_path_name(),
                                  'inspection': json.loads(unreal.InkCultivationEditorLibrary.inspect_system(system))})
        system = None
    OUT.mkdir(parents=True, exist_ok=True)
    output = OUT / 'title-particle-assets.json'
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('Rebuilt flowing title Niagara assets: ' + str(output))


try:
    rebuild()
finally:
    unreal.SystemLibrary.quit_editor()
