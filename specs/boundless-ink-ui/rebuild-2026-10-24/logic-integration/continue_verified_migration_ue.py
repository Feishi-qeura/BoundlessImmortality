"""Resume AFTER proven582 native rename; never issues another RenameAssets."""
from pathlib import Path
import json,hashlib,os,traceback,shutil
HERE=Path(__file__).resolve().parent;TEAM=HERE.parent/'migration'
def main():
    import unreal
    assert os.getpid()==47604
    original=HERE/'live-diagnostic-bridge/owner-original.py'
    assert hashlib.sha256(original.read_bytes()).hexdigest()=='cbbe4812a141893ecc24b6636653c161cc96ed65e3b4315dde4250dc665e2155'
    ns={'__file__':str(TEAM/'migrate_ui_ue.py'),'__name__':'integration_original_continuation'}
    exec(compile(original.read_text(encoding='utf-8-sig'),str(original),'exec'),ns)
    read=ns['read'];save=ns['save'];sha=ns['sha'];ns['window']()
    assert read(TEAM/'engine-rename-result.json')['passed']
    for file in ('four-texture-reconciliation-result.json','eight-source-reconciliation-result.json'):
        p=read(HERE/file);assert p['passed'] and p['engine_pid']==os.getpid()
    assert not ns['dirty'](unreal) and not unreal.EditorLevelLibrary.get_pie_worlds(False)
    mapping,backup=ns['data']();reg=ns['registry'](unreal)
    assets=reg.get_all_assets(True);by={str(a.package_name):a for a in assets}
    for row in mapping['assets']:
        assert (ns['ROOT']/row['destination_relative']).is_file()
        dest=by[row['new_package']];assert ns['class_path'](dest)==row['live_class']
    old=[a for a in assets if str(a.package_name).startswith('/Game/UI/')]
    assert all(ns['class_path'](a)=='/Script/CoreUObject.ObjectRedirector' for a in old),'Old real assets still registered; preserve Editor'
    archive=HERE/'migration-attempts/05-recovered-save-failure';archive.mkdir(parents=True,exist_ok=True)
    for name in ('engine-migration-journal.json','engine-failure.json','asset-write-journal.json'):
        src=TEAM/name;dst=archive/name
        if src.exists():
            assert not dst.exists(),'Existing continuation evidence; inspect before retry'
            shutil.copy2(src,dst)
    prior=read(TEAM/'engine-migration-journal.json')
    assert prior['state']=='before_rename' and prior['engine_pid']==os.getpid() and prior['exact_count']==582
    save('engine-migration-journal.json',dict(checked_at=ns['now'](),state='renamed_complete',exact_count=582,map_sha256=sha(TEAM/'migration-map.json'),engine_pid=os.getpid(),recovery_evidence=str(archive),note='Original native582 rename succeeded; same-PID exact source-copy reconciliation completed; canonical582 saved and dirty0. No repeated rename.'))
    ns['record_delta'](backup,mapping)
    unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_editor_world(),'BoundlessNewUI.FixupExactMigrationRedirectors')
    fixup=read(TEAM/'engine-fixup-result.json')
    assert fixup['passed'] and fixup['helper_revision']==2 and fixup['engine_pid']==os.getpid(),fixup
    ns['verify'](unreal,mapping,backup)
    assert not ns['dirty'](unreal),'Final verification has dirty packages; preserve Editor'
    (HERE/'migration-recovery-continuation-result.json').write_text(json.dumps(dict(passed=True,engine_pid=os.getpid(),checked_at=ns['now'](),original_owner_sha256=sha(original),final_verification=str(TEAM/'engine-final-verification.json'),dirty_packages=[]),indent=2),encoding='utf-8')
