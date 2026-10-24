"""Save only the two exact dirty source packages left after verified reconciliation."""
from pathlib import Path
from datetime import datetime,timezone
import json,os,traceback
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];TEAM=HERE.parent/'migration'
EXPECTED={'/Game/UI/Ink/Textures/HUD/T_XianxiaMeters','/Game/UI/Ink/ComponentRollout20261024/Activities/T_HudLaunchCountdownFrame_v1'}
def main():
    import unreal
    assert os.getpid()==47604 and Path(unreal.Paths.project_dir()).resolve()==ROOT
    prior=json.loads((HERE/'four-texture-reconciliation-result.json').read_text())
    assert prior['passed'] and len(prior['operations'])==4
    packages=list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert {p.get_name() for p in packages}==EXPECTED
    report=dict(engine_pid=os.getpid(),started_at=datetime.now(timezone.utc).isoformat(),scope=sorted(EXPECTED),objects={name:[] for name in EXPECTED})
    for obj in unreal.ObjectIterator(unreal.Object):
        try:path=obj.get_path_name()
        except (TypeError,AttributeError):continue # Native class wrappers are not source texture package exports.
        for name in EXPECTED:
            if path.startswith(name+'.') or path.startswith(name+':'):
                report['objects'][name].append(dict(path=path,asset_class=obj.get_class().get_path_name()))
    output=HERE/'remaining-two-save-result.json'
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    try:
        report['save_packages_returned']=unreal.EditorLoadingAndSavingUtils.save_packages(packages,False)
        report['dirty_after']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
        report['map_dirty_after']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]
        report['passed']=report['save_packages_returned'] and not report['dirty_after'] and not report['map_dirty_after']
    except Exception:report['error']=traceback.format_exc()
    report['preserve_editor_memory']=True
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    return str(output)
