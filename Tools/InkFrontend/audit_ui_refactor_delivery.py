"""Read-only asset/parameter/font audit. Never opens or decodes reference images."""
import ast
import hashlib
import json
import re
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/Codex/UIRefactor20261021/Resume20261021'

def run():
    OUT.mkdir(parents=True, exist_ok=True)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    mel = unreal.MaterialEditingLibrary
    report = {'boundary': 'Read-only editor audit; no reference images read', 'materials': [], 'fonts': [], 'source_font_locations': [], 'checks': []}
    theme=unreal.load_asset('/Game/UI/Ink/Refactor20261021/Data/DA_InkUITheme_v1')
    report['theme']={'asset':theme.get_path_name() if theme else None,'parameters':{}}
    if theme:
        for key in ['display_font','body_font','data_font','fallback_font','display_size','section_size','body_size','button_size','data_size','secondary_size','hud_label_size','slate_point_scale','outer_padding','min_hit_size','feather_width']:
            value=theme.get_editor_property(key)
            report['theme']['parameters'][key]=value.get_path_name() if hasattr(value,'get_path_name') else value
    report['checks'].append({'name':'theme parameters load from actual DataAsset','passed':bool(theme)})
    for data in registry.get_assets_by_path('/Game/UI/Ink', recursive=True):
        class_name = str(data.asset_class_path.asset_name)
        if class_name not in ('Material', 'MaterialInstanceConstant', 'Font', 'FontFace'):
            continue
        asset = data.get_asset()
        if not asset:
            report['checks'].append({'name': str(data.package_name) + ' load', 'passed': False})
            continue
        if class_name in ('Font', 'FontFace'):
            report['fonts'].append({'path': asset.get_path_name(), 'class': class_name})
            continue
        row = {'path': asset.get_path_name(), 'class': class_name, 'parameters': {}}
        if class_name == 'MaterialInstanceConstant':
            parent = asset.get_editor_property('parent')
            row['parent'] = parent.get_path_name() if parent else None
        for kind in ('scalar', 'vector', 'texture', 'static_switch'):
            names = getattr(mel, 'get_' + kind + '_parameter_names')(asset)
            reader = getattr(mel, 'get_material_instance_' + kind + '_parameter_value' if class_name == 'MaterialInstanceConstant' else 'get_material_default_' + kind + '_parameter_value')
            values = {}
            for name in names:
                value = reader(asset, name)
                if kind == 'vector':
                    value = [value.r, value.g, value.b, value.a]
                elif kind == 'texture':
                    value = value.get_path_name() if value else None
                values[str(name)] = value
            row['parameters'][kind] = values
        report['materials'].append(row)
    # This inventory deliberately includes non-TextBlock world/GM renderers for later migration.
    pattern = re.compile(r'F_LiveNames|FF_LiveNames|SetFont\(|MakeFont\(|FSlateFontInfo|MakeText\(|MakeShapedText\(|TextRender|WidgetComponent')
    for path in sorted((ROOT / 'Source').rglob('*')):
        if path.suffix not in ('.cpp', '.h'):
            continue
        relative = str(path.relative_to(ROOT)).replace('\\', '/')
        for number, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
            if pattern.search(line):
                report['source_font_locations'].append({'file': relative, 'line': number, 'code': line.strip(), 'stage': 'UI stage-one review' if '/Frontend/' in relative else 'retained world/editor/GM reference; not claimed migrated'})
    font_root = ROOT / 'docs/design/2026-10-21-ink-ui-standard-v1/fonts'
    report['font_provenance'] = [{'file': str(p.relative_to(ROOT)), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(font_root.rglob('*')) if p.is_file() and p.suffix.lower() in ('.ttf', '.otf', '.txt')]
    report['checks'].append({'name': 'material parameter snapshots exported', 'passed': len(report['materials']) > 0})
    report['checks'].append({'name': 'new theme fonts present', 'passed': sum('/Refactor20261021/Fonts/F_Ink' in f['path'] and f['class'] == 'Font' for f in report['fonts']) == 3})
    report['passed'] = all(c['passed'] for c in report['checks'])
    (OUT / 'delivery-asset-audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('UI_DELIVERY_AUDIT: ' + str(len(report['materials'])) + ' materials, ' + str(len(report['source_font_locations'])) + ' font source locations; passed=' + str(report['passed']))
    return report

if __name__ == '__main__':
    run()
