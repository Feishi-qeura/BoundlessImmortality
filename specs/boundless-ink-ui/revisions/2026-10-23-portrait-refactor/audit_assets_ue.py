"""Read-only registry audit. Never rename, save or delete an Unreal asset."""
from pathlib import Path
import json
from collections import Counter
import unreal

ROOT = Path(unreal.Paths.project_dir())
OUT = ROOT / 'specs/boundless-ink-ui/revisions/2026-10-23-portrait-refactor'
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reg.search_all_assets(True)
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True, include_searchable_names=True, include_soft_management_references=True, include_hard_management_references=True)
assets = []
for a in reg.get_assets_by_path('/Game/UI', True):
    assets.append(dict(package=str(a.package_name), asset_class=str(a.asset_class_path.asset_name),
        referencers=sorted(str(x) for x in reg.get_referencers(a.package_name, opts) or []),
        dependencies=sorted(str(x) for x in reg.get_dependencies(a.package_name, opts) or [])))
materials = []
for path in ['/Game/UI/Ink/Refactor20261022/Materials/M_InkPaperCloud', '/Game/UI/Ink/NiagaraRevision20261022/M_InkWash', '/Game/UI/Ink/NiagaraRevision20261022/M_InkGoldRankPortrait']:
    obj = unreal.load_asset(path)
    materials.append(dict(package=path, loaded=bool(obj), blend_mode=str(obj.get_editor_property('blend_mode')) if obj else None, domain=str(obj.get_editor_property('material_domain')) if obj else None))
result = dict(scope='/Game/UI', read_only=True, asset_count=len(assets), classes=dict(Counter(a['asset_class'] for a in assets)), assets=assets, materials=materials, runtime_visual_tested=False)
(OUT/'registry-audit.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
print('PORTRAIT_REFACTOR_AUDIT_COMPLETE',len(assets))
