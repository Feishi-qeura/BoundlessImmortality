"""Owned idle GUI Editor controller; keeps preflight/migration in the same PID."""
from pathlib import Path
import hashlib, json, os, runpy, time, traceback
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)

HERE = Path(__file__).resolve().parent
MIGRATION = HERE.parent / 'migration' / 'migrate_ui_ue.py'
STATE = {'sequence': 0, 'last_poll': 0., 'busy': False}
SOURCE = MIGRATION.read_text(encoding='utf-8-sig')
# Kismet console dispatch requires a real Editor world context.
OLD_DISPATCH = "unreal.SystemLibrary.execute_console_command(None, 'BoundlessNewUI.FixupExactMigrationRedirectors')"
NEW_DISPATCH = "unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), 'BoundlessNewUI.FixupExactMigrationRedirectors')"
OLD_RENAME = '        ok = unreal.AssetToolsHelpers.get_asset_tools().rename_assets(rename)'
NEW_RENAME = """        unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(), 'BoundlessNewUI.RenameExactReviewedAssets')
        native_rename = read(OUT / 'engine-rename-result.json')
        ok = native_rename.get('passed') and native_rename.get('engine_pid') == os.getpid() and native_rename.get('exact_count') == len(rename)"""
def execution_source(raw):
    assert raw.count(OLD_DISPATCH) <= 1 and raw.count(OLD_RENAME) == 1
    return raw.replace(OLD_DISPATCH, NEW_DISPATCH).replace(OLD_RENAME, NEW_RENAME)
EXECUTION_SOURCE = execution_source(SOURCE)
NAMESPACE = {'__file__': str(MIGRATION), '__name__': 'integration_owned_migration'}
exec(compile(EXECUTION_SOURCE, str(MIGRATION), 'exec'), NAMESPACE)
(HERE / 'migration-execution-source.py').write_text(EXECUTION_SOURCE, encoding='utf-8')
(HERE / 'migration-execution-source-review.json').write_text(json.dumps(dict(
    owner_source=str(MIGRATION), owner_source_sha256=hashlib.sha256(MIGRATION.read_bytes()).hexdigest(),
    execution_source_sha256=hashlib.sha256(EXECUTION_SOURCE.encode()).hexdigest(),
    dispatch_replacement_applied=OLD_DISPATCH in SOURCE,
    native_rename_replacement=dict(before=OLD_RENAME,after=NEW_RENAME),
    exact_single_replacement=dict(before=OLD_DISPATCH, after=NEW_DISPATCH) if OLD_DISPATCH in SOURCE else None), indent=2), encoding='utf-8')

def write(value):
    path = HERE / 'migration-controller-result.json'
    tmp = path.with_suffix('.tmp')
    tmp.write_text(json.dumps(dict(engine_pid=os.getpid(), **value), indent=2), encoding='utf-8')
    tmp.replace(path)

def tick(delta):
    if STATE['busy'] or time.monotonic() - STATE['last_poll'] < 1:
        return
    STATE['last_poll'] = time.monotonic()
    control = HERE / 'migration-controller-command.json'
    if not control.exists():
        return
    request = json.loads(control.read_text(encoding='utf-8-sig'))
    sequence, phase = request['sequence'], request['phase']
    if sequence <= STATE['sequence']:
        return
    STATE['sequence'] = sequence
    STATE['busy'] = True
    try:
        if phase == 'quit':
            if (unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
                    or unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
                raise RuntimeError('Refusing shutdown with dirty packages; preserve Editor memory.')
            write(dict(sequence=sequence, phase=phase, state='normal_shutdown_requested'))
            unreal.unregister_slate_post_tick_callback(HANDLE)
            unreal.SystemLibrary.quit_editor()
            return
        if phase == 'inspect':
            diagnostic=HERE.parent/'migration/revisions/v3-known-warning-cancel/inspect_memory_readonly.py'
            assert hashlib.sha256(diagnostic.read_bytes()).hexdigest()=='495d9b27e5035e2ae59aca4bfdfa4957f91c8d8f0bdeacf578019b379ad431ff'
            runpy.run_path(str(diagnostic),run_name='integration_readonly_diagnostic')['main']()
            write(dict(sequence=sequence,phase=phase,state='passed'))
            return
        if phase == 'diagnose':
            NAMESPACE['window']()
            reg = NAMESPACE['registry'](unreal)
            options = unreal.AssetRegistryDependencyOptions(
                include_soft_package_references=True, include_hard_package_references=True,
                include_searchable_names=True, include_soft_management_references=True,
                include_hard_management_references=True)
            rows = []
            for asset in reg.get_assets_by_path('/Game/UI', True, True):
                row = dict(package=str(asset.package_name), name=str(asset.asset_name),
                           asset_class=str(asset.asset_class_path))
                if 'ObjectRedirector' in row['asset_class']:
                    row['destination_tag'] = str(asset.get_tag_value('DestinationObject'))
                    row['referencers'] = [str(x) for x in reg.get_referencers(asset.package_name, options)]
                    row['dependencies'] = [str(x) for x in reg.get_dependencies(asset.package_name, options)]
                rows.append(row)
            (HERE / 'migration-live-source-inventory.json').write_text(json.dumps(dict(
                engine_pid=os.getpid(), source_count=len(rows), assets=rows,
                redirectors=[x for x in rows if 'ObjectRedirector' in x['asset_class']]), indent=2), encoding='utf-8')
            write(dict(sequence=sequence, phase=phase, state='passed'))
            return
        if phase not in ('preflight', 'migrate', 'verify'):
            raise ValueError(phase)
        # Re-read the reviewed owner script for the next explicit phase, so a
        # rejected read-only preflight can be repaired without dropping memory.
        source = execution_source(MIGRATION.read_text(encoding='utf-8-sig'))
        NAMESPACE.clear()
        NAMESPACE.update(__file__=str(MIGRATION), __name__='integration_owned_migration')
        exec(compile(source, str(MIGRATION), 'exec'), NAMESPACE)
        (HERE / f'migration-execution-source-{sequence}.py').write_text(source, encoding='utf-8')
        write(dict(sequence=sequence, phase=phase, state='running'))
        NAMESPACE['main'](phase)
        write(dict(sequence=sequence, phase=phase, state='passed'))
    except Exception:
        details = traceback.format_exc()
        unreal.log_error(details)
        write(dict(sequence=sequence, phase=phase, state='failed', error=details,
                   preserve_editor_memory=True))
    finally:
        STATE['busy'] = False

HANDLE = unreal.register_slate_post_tick_callback(tick)
write(dict(sequence=0, phase='idle', state='ready'))
