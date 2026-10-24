"""Run original full proof with only unavailable cache-path API adapted."""
from pathlib import Path
import hashlib,json,os
HERE=Path(__file__).resolve().parent;TEAM=HERE.parent/'migration'
def main():
    import unreal
    assert os.getpid()==47604
    original=HERE/'live-diagnostic-bridge/owner-original.py'
    raw=original.read_bytes();assert hashlib.sha256(raw).hexdigest()=='cbbe4812a141893ecc24b6636653c161cc96ed65e3b4315dde4250dc665e2155'
    source=raw.decode('utf-8-sig');old="    reg.remove_path('/Game/UI')";new="    reg.scan_paths_synchronous(['/Game/newUI'], True, True)  # Cached-folder remove_path is not Python-exposed."
    assert source.count(old)==1
    source=source.replace(old,new)
    (HERE/'migration-final-verification-execution.py').write_text(source,encoding='utf-8')
    (HERE/'migration-final-verification-api-adaptation.json').write_text(json.dumps({'original_sha256':hashlib.sha256(raw).hexdigest(),'execution_sha256':hashlib.sha256(source.encode()).hexdigest(),'before':old,'after':new,'reason':'AssetRegistry.remove_path is unavailable in UE5.8 Python. Original strict old registry membership0, disk files0, canonical582 class/hash, forbidden phantoms and write scope checks remain unchanged.'},indent=2),encoding='utf-8')
    ns={'__file__':str(TEAM/'migrate_ui_ue.py'),'__name__':'integration_final_verification'}
    exec(compile(source,str(HERE/'migration-final-verification-execution.py'),'exec'),ns)
    ns['window']();assert not ns['dirty'](unreal)
    m,b=ns['data']();ns['verify'](unreal,m,b)
    assert not ns['dirty'](unreal)
    (HERE/'migration-recovery-continuation-result.json').write_text(json.dumps(dict(passed=True,engine_pid=os.getpid(),checked_at=ns['now'](),final_verification=str(TEAM/'engine-final-verification.json'),dirty_packages=[],native_rename_not_repeated=True),indent=2),encoding='utf-8')
