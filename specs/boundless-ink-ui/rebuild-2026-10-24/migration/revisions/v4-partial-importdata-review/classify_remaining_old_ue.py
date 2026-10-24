"""Optional integration-only SAME-PID read-only classification, never load/save/delete.

Use independent live bridge dispatch if needed. Do not replace the active owner
module. Exact find_object(...follow_redirectors=False) distinguishes actual old
objects from old-name lookups that resolve to retained new canonical assets.
"""
from pathlib import Path
from datetime import datetime, timezone
import json
import os

HERE = Path(__file__).resolve().parent
TEAM = HERE.parents[1]
ROOT = TEAM.parents[3]


def main():
    import unreal
    assert os.getpid() == 47604 and Path(unreal.Paths.project_dir()).resolve() == ROOT
    mapping = json.loads((TEAM / 'migration-map.json').read_text(encoding='utf-8-sig'))
    rename = json.loads((TEAM / 'engine-rename-result.json').read_text(encoding='utf-8-sig'))
    assert rename['passed'] and rename['engine_pid'] == os.getpid() and rename['exact_count'] == 582
    originals = {r['old_package']: r for r in mapping['assets'] + mapping['legacy_redirectors']}
    dirty = list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()) + list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    dirty_names = {p.get_name() for p in dirty}
    disk = {('/Game/' + p.relative_to(ROOT / 'Content').with_suffix('').as_posix()): p
            for p in (ROOT / 'Content/UI').rglob('*.uasset')}
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(['/Game/UI', '/Game/newUI'], True, True)
    assert not registry.is_loading_assets()
    options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,
        include_soft_package_references=True, include_searchable_names=True,
        include_hard_management_references=True, include_soft_management_references=True,
        include_game_package_references=True, include_editor_only_package_references=True)
    rows = []
    def info(obj):
        if obj is None: return None
        outer = obj.get_outer()
        return dict(path=obj.get_path_name(), asset_class=obj.get_class().get_path_name(),
                    outer=outer.get_path_name() if outer else None)
    for package in sorted(set(disk) | dirty_names):
        if not package.startswith('/Game/UI/'):
            continue
        r = originals.get(package)
        if not r:
            rows.append(dict(package=package, classification='UNEXPECTED_OLD_PACKAGE_PRESERVE'))
            continue
        raw = unreal.find_object(None, r['old_object'], follow_redirectors=False)
        resolved = unreal.find_object(None, r['old_object'])
        new = unreal.find_object(None, r['new_object'], follow_redirectors=False)
        raw_info = info(raw)
        classification = 'NO_EXACT_OLD_OBJECT_IN_MEMORY_DISK_TYPE_UNKNOWN'
        if raw_info and raw_info['path'] == r['old_object']:
            classification = ('EXACT_OLD_REDIRECTOR' if raw_info['asset_class'] == '/Script/CoreUObject.ObjectRedirector'
                              else 'EXACT_OLD_REAL_OBJECT_NOT_SAFE_FOR_REDIRECTOR_FIXUP')
        rows.append(dict(package=package, source_kind=r['source_kind'], physical_file=package in disk,
            dirty=package in dirty_names, expected_old=r['old_object'], expected_new=r['new_object'],
            raw_old=raw_info, normal_old_lookup=info(resolved), retained_new=info(new), classification=classification,
            disk_registry=[dict(name=str(a.asset_name), asset_class=str(a.asset_class_path.package_name) + '.' + str(a.asset_class_path.asset_name))
                           for a in registry.get_assets_by_package_name(package, True, False)],
            incoming_all_categories=[str(n) for n in (registry.get_referencers(package, options) or [])]))
    # The remaining dirty old packages may have no top-level asset with the expected
    # name. Enumerate their real UObject contents without loading or clearing flags.
    contents = {name: [] for name in dirty_names if name.startswith('/Game/UI/')}
    iterator_noninstance_skips = 0
    if contents:
        for obj in unreal.ObjectIterator(unreal.Object):
            try:
                path = obj.get_path_name()
            except (TypeError, AttributeError):
                # Observed UE iterator also yields native class wrappers; those
                # descriptors require an object arg and are not package exports.
                iterator_noninstance_skips += 1
                continue
            package = path.split('.', 1)[0]
            if package in contents:
                contents[package].append(info(obj))
    all_assets = registry.get_all_assets(True)
    report = dict(checked_at=datetime.now(timezone.utc).isoformat(), engine_pid=os.getpid(),
        read_only=True, asset_loads_saves_renames_deletes=False,
        old_physical_package_count=len(disk), dirty_packages=sorted(dirty_names),
        old_registry_assets=[dict(package=str(a.package_name), asset_class=str(a.asset_class_path.package_name) + '.' + str(a.asset_class_path.asset_name))
                             for a in all_assets if str(a.package_name).startswith('/Game/UI/')],
        remaining=rows, dirty_old_package_uobject_contents=contents,
        iterator_noninstance_wrappers_skipped=iterator_noninstance_skips,
        caution='Empty registry or resolving an old name to new does not prove physical old file is a redirector. No automatic retry/save/close decision.')
    path = HERE / ('remaining-old-' + str(os.getpid()) + '-' + datetime.now().strftime('%Y%m%dT%H%M%S%f') + '.json')
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('CLEANUP_READONLY_REMAINING_CLASSIFICATION ' + str(path))
    return str(path)


if __name__ == '__main__':
    main()
