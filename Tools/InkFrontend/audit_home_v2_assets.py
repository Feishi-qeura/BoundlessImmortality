"""Read hard/soft/manage dependencies and dynamic source references; no deletion."""
from pathlib import Path
import json,unreal
ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/HomeV2Integration20261022'
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(True)
opts=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=True,include_soft_management_references=True,include_hard_management_references=True)
texts=[]
for folder in ['Source','Config','Plugins','Tools']:
 for p in (ROOT/folder).rglob('*'):
  if p.suffix.lower() in ['.h','.cpp','.cs','.ini','.py','.json','.yaml','.uplugin'] and not any(s in p.parts for s in ['Intermediate','Binaries','__pycache__']):
   try:texts.append((p.relative_to(ROOT).as_posix(),p.read_text('utf-8-sig')))
   except (UnicodeError,OSError):pass
R=dict(assets=[],redirectors=[],policy='No uasset removal inferred from zero registry references; dynamic parent paths, packaging and uncertainty retain assets',cook_rule='/Game/UI/Ink is DirectoriesToAlwaysCook',passed=False)
for a in reg.get_assets_by_path('/Game/UI/Ink',True):
 pkg=str(a.package_name);name=str(a.asset_name);parent=pkg.rsplit('/',1)[0]
 hardsoft=[str(x) for x in (reg.get_referencers(a.package_name,opts) or [])]
 deps=[str(x) for x in (reg.get_dependencies(a.package_name,opts) or [])]
 refs=[f for f,t in texts if pkg in t or name in t or (parent+'/' in t)]
 clazz=str(a.asset_class_path.asset_name)
 row=dict(asset=pkg,asset_class=clazz,hard_soft_manage_referencers=hardsoft,dependencies=deps,dynamic_or_parent_path_sources=refs,decision='keep-current' if '/HomeV2/' in pkg else 'retain-reference-or-purpose-needs-confirmation')
 R['assets'].append(row)
 if clazz=='ObjectRedirector':R['redirectors'].append(pkg)
R['assets_scanned']=len(R['assets']);R['passed']=True
(OUT/'ue-reference-audit.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),'utf-8')
print('HOME_V2_AUDIT',R['assets_scanned'],'redirectors',len(R['redirectors']))
