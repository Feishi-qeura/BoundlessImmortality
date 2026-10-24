"""Optional sole-integration post-save readback. Registry reads only, no asset load/save.

Run after actual dirty-package recovery/fixup/empty-root removal, never to bypass
those stages. Full mounted on-disk dependency scan distinguishes saved references
from a live controller's diagnostic phase named 'verify'.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os

HERE = Path(__file__).resolve().parent
TEAM = HERE.parents[1]
ROOT = TEAM.parents[3]


def main(allow_fresh_reader=False):
    import unreal
    assert Path(unreal.Paths.project_dir()).resolve() == ROOT
    lease = json.loads((TEAM.parent / 'engine-lease.json').read_text(encoding='utf-8-sig'))
    assert lease['sole_engine_and_content_executor'] == lease['owner_team'] == 'logic-integration'
    assert lease['state'] in ('active', 'granted') and datetime.fromisoformat(lease['expires_at']) > datetime.now(timezone.utc)
    rename = json.loads((TEAM / 'engine-rename-result.json').read_text(encoding='utf-8-sig'))
    assert rename['passed'] and rename['exact_count'] == 582
    assert allow_fresh_reader or os.getpid() == rename['engine_pid']
    mpath = TEAM / 'migration-map.json'
    mapping = json.loads(mpath.read_text(encoding='utf-8-sig'))
    dirty = [p.get_name() for p in list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
             + list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())]
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    initial = registry.get_all_assets(True)
    roots = sorted({'/Game'} | {'/' + str(a.package_name).split('/')[1] for a in initial if str(a.package_name).startswith('/')})
    registry.scan_paths_synchronous(roots, True, True)
    assert not registry.is_loading_assets()
    assets = registry.get_all_assets(True)
    packages = sorted({str(a.package_name) for a in assets})
    by_package = {str(a.package_name): a for a in assets}
    options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,
        include_soft_package_references=True, include_searchable_names=True,
        include_hard_management_references=True, include_soft_management_references=True,
        include_game_package_references=True, include_editor_only_package_references=True)
    old_edges = []
    for pkg in packages:
        for dep in registry.get_dependencies(pkg, options) or []:
            target = str(dep)
            if target == '/Game/UI' or target.startswith('/Game/UI/'):
                old_edges.append(dict(referencer=pkg, dependency=target))
    old_packages = [p for p in packages if p == '/Game/UI' or p.startswith('/Game/UI/')]
    bad_canonical = []
    for row in mapping['assets']:
        a = by_package.get(row['new_package'])
        cls = None if a is None else str(a.asset_class_path.package_name) + '.' + str(a.asset_class_path.asset_name)
        if a is None or cls != row['live_class'] or str(a.asset_name) != row['asset_name']:
            bad_canonical.append(dict(package=row['new_package'], expected_class=row['live_class'], observed_class=cls))
    report = dict(checked_at=datetime.now(timezone.utc).isoformat(), engine_pid=os.getpid(),
        migration_engine_pid=rename['engine_pid'], fresh_reader=allow_fresh_reader,
        read_only=True, no_asset_load_or_save=True,
        migration_map_sha256=hashlib.sha256(mpath.read_bytes()).hexdigest(),
        all_mounted_packages_scanned=len(packages), scanned_roots=roots,
        combined_hard_soft_searchable_management_editor_game_categories=True,
        old_root_dependency_edges=old_edges, old_root_registry_packages=old_packages,
        bad_canonical=bad_canonical, dirty_packages=dirty,
        old_directory_exists=(ROOT / 'Content/UI').exists(),
        canonical_class_name_count=582 - len(bad_canonical),
        passed=not old_edges and not old_packages and not bad_canonical and not dirty and not (ROOT / 'Content/UI').exists(),
        limitation='On-disk/current registered management graph only; not runtime dynamic loading, UI visual or cook/device acceptance.')
    target = HERE / 'post-save-reference-readback.json'
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('POST_SAVE_REFERENCE_READBACK ' + str(target))
    return str(target)


if __name__ == '__main__':
    main()
