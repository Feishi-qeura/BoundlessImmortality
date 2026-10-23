"""Save redirected referencers, remove redirectors with AssetTools, audit retained assets."""
import json,unreal
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir());REV=ROOT/'specs/boundless-ink-ui/revisions/2026-10-23-portrait-refactor';OUT=ROOT/'Saved/Codex/PortraitUI20261023'
rows=json.loads((REV/'migration-manifest.json').read_text('utf-8'))['assets']
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(True)
canonical={r['proposed_package'] for r in rows};refs=set()
for row in rows:refs.update(row.get('registry_referencers',[]))
mapping={r['old_package']:r['proposed_package'] for r in rows}
saved=[];failures=[]
cleanup_only='-portraitcleanuponly' in unreal.SystemLibrary.get_command_line().lower()
for path in sorted(canonical|{mapping.get(p,p) for p in refs}):
 obj=unreal.load_asset(path)
 if obj:
  if cleanup_only:continue
  if unreal.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False):saved.append(path)
  else:failures.append('save '+path)
 else:failures.append('load '+path)
assert not failures,failures
if '-portraitauditonly' not in unreal.SystemLibrary.get_command_line().lower():unreal.InkUiStyleEditorLibrary.fix_ui_redirectors()
reg.scan_paths_synchronous(['/Game/UI'],True)
assets=reg.get_assets_by_path('/Game/UI',recursive=True)
redirects=[str(a.package_name) for a in assets if str(a.asset_class_path.asset_name)=='ObjectRedirector']
actual={str(a.package_name):str(a.asset_class_path.asset_name) for a in assets if str(a.asset_class_path.asset_name)!='ObjectRedirector'}
checks=[]
for row in rows:
 obj=unreal.load_asset(row['proposed_package'])
 checks.append(dict(path=row['proposed_package'],expected=row['asset_class'],passed=bool(obj and obj.get_class().get_name()==row['asset_class'])))
report=dict(passed=all(v['passed'] for v in checks) and not failures and len(actual)==len(rows),checks=checks,retained=len(actual),expected=len(rows),actual=actual,redirectors=redirects,saved_referencers=saved,failures=failures,deleted_content_assets=0)
(OUT/'asset-final-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),'utf-8')
assert report['passed'],str((len(actual),len(rows),failures))
print('CANONICAL_UI_AUDIT_PASSED',len(actual),'redirectors',len(redirects))
