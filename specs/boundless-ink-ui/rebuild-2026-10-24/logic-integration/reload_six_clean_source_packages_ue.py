"""Reload exact six clean source packages from backed disk; no package writes."""
from pathlib import Path
import json,os,hashlib
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
NAMES={'DA_InkUITheme_v1','FF_InkMaShanZheng','FF_InkNotoFallback','FF_InkWenKai','F_InkDisplay','F_InkFallback'}
def main():
    import unreal
    assert os.getpid()==47604
    assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages() and not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    rows=[r for r in read(HERE.parent/'migration/migration-map.json')['assets'] if r['asset_name'] in NAMES]
    assert len(rows)==6
    report={'engine_pid':os.getpid(),'operations':[],'passed':False}
    hashes={r[k]:sha(ROOT/r[k]) for r in rows for k in ('source_relative','destination_relative')}
    packages=[]
    for row in rows:
        old=unreal.find_object(None,row['old_object'],follow_redirectors=False)
        new=unreal.find_object(None,row['new_object'],follow_redirectors=False)
        assert old and new and old!=new and old.get_class()==new.get_class()
        packages.append(old.get_outermost())
        report['operations'].append({'old':old.get_path_name(),'new':new.get_path_name(),'before_class':old.get_class().get_path_name()})
    out=HERE/'six-clean-source-reload-result.json'
    out.write_text(json.dumps(report,indent=2),encoding='utf-8')
    report['reload_result']=str(unreal.EditorLoadingAndSavingUtils.reload_packages(packages,unreal.ReloadPackagesInteractionMode.ASSUME_NEGATIVE))
    for row,op in zip(rows,report['operations']):
        obj=unreal.find_object(None,row['old_object'],follow_redirectors=False)
        op['after_class']=obj.get_class().get_path_name() if obj else None
    report['no_file_changes']=all(sha(ROOT/p)==h for p,h in hashes.items())
    report['dirty_after']=[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    report['passed']=report['no_file_changes'] and not report['dirty_after']
    out.write_text(json.dumps(report,indent=2),encoding='utf-8')
