"""Execute approved package relocation via Unreal AssetTools. No content deletion."""
from pathlib import Path
import json,unreal
ROOT=Path(unreal.Paths.project_dir())
REV=ROOT/'specs/boundless-ink-ui/revisions/2026-10-23-portrait-refactor'
OUT=ROOT/'Saved/Codex/PortraitUI20261023'
assert json.loads((OUT/'backup-manifest.json').read_text('utf-8'))['verified']
manifest=json.loads((REV/'migration-manifest.json').read_text('utf-8'))
reg=unreal.AssetRegistryHelpers.get_asset_registry();reg.search_all_assets(True)
tools=unreal.AssetToolsHelpers.get_asset_tools()
rows=manifest['assets'];result={'moved':[],'already_moved':[],'failed':[],'deleted_assets':0,'redirectors_retained':True}
plans=[]
native_names={'T_InkTitle','T_InkMeditation','T_InkAttributeIcons','T_ProvidedButtons','T_ProvidedRank','T_ProvidedInventory','M_InkTitleFeather','M_InkScrollFade','F_InkBody','NS_InkTitleAura'}
native=[]
for row in rows:
    old,new=row['old_package'],row['proposed_package']
    if unreal.EditorAssetLibrary.does_asset_exist(new):
        existing=unreal.load_asset(new)
        assert existing and existing.get_class().get_name()==row['asset_class'],new
        result['already_moved'].append(new);continue
    obj=unreal.load_asset(old)
    assert obj is not None,old
    assert obj.get_class().get_name()==row['asset_class'],old
    folder,name=new.rsplit('/',1)
    if name in native_names:native.append((row,obj))
    else:plans.append((row,unreal.AssetRenameData(obj,folder,name)))
# Batch renames allow references between the 570 selected assets to be tracked together.
if plans:
    ok=tools.rename_assets([p[1] for p in plans])
    result['rename_returned']=bool(ok)
for row,obj in native:
    assert unreal.InkUiStyleEditorLibrary.relocate_ui_asset(obj,row['proposed_package']),row['old_package']
    plans.append((row,None))
for row,_ in plans:
    new=row['proposed_package'];obj=unreal.load_asset(new)
    if obj and obj.get_class().get_name()==row['asset_class']:
        if unreal.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False):result['moved'].append(new)
        else:result['failed'].append({'package':new,'reason':'save-failed'})
    else:result['failed'].append({'package':new,'reason':'new-package-load-failed'})
result['passed']=not result['failed'] and len(result['moved'])+len(result['already_moved'])==len(rows)
(OUT/'migration-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
assert result['passed'],str(result['failed'])
print('UI_MIGRATION_COMPLETE',len(rows))
