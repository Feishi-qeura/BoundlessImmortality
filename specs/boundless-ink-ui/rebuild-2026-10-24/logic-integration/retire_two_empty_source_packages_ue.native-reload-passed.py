"""Retire only two proven empty old package containers, keeping all files/data."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,os,traceback
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];TEAM=HERE.parent/'migration'
EXPECTED={'/Game/UI/Ink/Textures/HUD/T_XianxiaMeters','/Game/UI/Ink/ComponentRollout20261024/Activities/T_HudLaunchCountdownFrame_v1'}
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    import unreal
    assert os.getpid()==47604 and Path(unreal.Paths.project_dir()).resolve()==ROOT
    previous=read(HERE/'remaining-two-save-result.json')
    assert previous['save_packages_returned'] and not previous['map_dirty_after'] and all(not values for values in previous['objects'].values())
    backup=read(HERE/'partial-recovery-backup.json');manifest=Path(backup['manifest'])
    assert sha(manifest)==backup['sha256']
    snapshots={Path(row['relative']).as_posix():row for row in read(manifest)['files']}
    mapping=read(TEAM/'migration-map.json')
    rows=[row for row in mapping['assets'] if row['old_package'] in EXPECTED]
    packages=list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    assert len(rows)==2 and {p.get_name() for p in packages}==EXPECTED and not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    # These exact containers have no remaining UObject exports, not merely no visible assets.
    for obj in unreal.ObjectIterator(unreal.Object):
        try:path=obj.get_path_name()
        except (TypeError,AttributeError):continue
        assert not any(path.startswith(name+'.') or path.startswith(name+':') for name in EXPECTED),'Source package contains objects; preserve memory'
    for row in rows:
        target=unreal.find_object(None,row['new_object'],follow_redirectors=False)
        assert isinstance(target,unreal.Texture2D) and target.get_path_name()==row['new_object']
        assert sha(ROOT/row['destination_relative'])==snapshots[row['destination_relative']]['source_sha256']
        assert sha(ROOT/row['source_relative'])==row['sha256'],'Original old source file changed; inspect before empty-container retirement'
    report=dict(engine_pid=os.getpid(),started_at=datetime.now(timezone.utc).isoformat(),passed=False,
        scope=sorted(EXPECTED),proof='No UObject exports in two old containers; both canonical targets saved/hash match partial snapshot; old files still original snapshot bytes',
        old_files_deleted=False,operations=[])
    output=HERE/'empty-source-package-native-reload-result.json'
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    report['operations']=[dict(package=p.get_name(),action='native_reload_only_proven_empty_containers_from_verified_original_files') for p in packages]
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    result=unreal.EditorLoadingAndSavingUtils.reload_packages(packages,unreal.ReloadPackagesInteractionMode.ASSUME_POSITIVE)
    report['reload_result']=str(result)
    report['dirty_after']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    report['map_dirty_after']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
    reg=unreal.AssetRegistryHelpers.get_asset_registry()
    filenames=[str(path) for path in (ROOT/'Content/UI').rglob('*.uasset')]
    reg.scan_files_synchronous(filenames,True)
    old=reg.get_assets_by_path('/Game/UI',True,True)
    report['old_registry']=[dict(package=str(a.package_name),asset_class=str(a.asset_class_path.package_name)+'.'+str(a.asset_class_path.asset_name)) for a in old]
    report['passed']=not report['dirty_after'] and not report['map_dirty_after']
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    return str(output)
