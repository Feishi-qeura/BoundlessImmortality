"""Record exact pre/post source repair chains tied to actual engine diagnostics."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import shutil
from zoneinfo import ZoneInfo

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['begin','finish']);p.add_argument('path');p.add_argument('--diagnostic',default='');a=p.parse_args()
    ledger_path=HERE/'engine-source-repair-ledger.json'
    ledger=json.loads(ledger_path.read_text(encoding='utf-8')) if ledger_path.exists() else dict(owner_team='logic-integration',repairs=[])
    path=(ROOT/a.path).resolve();relative=path.relative_to(ROOT).as_posix()
    assert relative.startswith('Source/BoundlessImmortality'), 'Repair outside project source refused'
    if a.mode=='begin':
        lease=json.loads((HERE.parent/'engine-lease.json').read_text(encoding='utf-8-sig'))
        now=datetime.now(ZoneInfo('Asia/Hong_Kong'))
        assert lease['state']=='granted' and lease['owner_team']=='logic-integration' and lease['diagnostic_source_repair_authorized']
        assert now<datetime.fromisoformat(lease['expires_at']) and a.diagnostic
        index=len(ledger['repairs'])
        backup=HERE/'engine-source-before'/str(index)/relative;backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,backup)
        authority=HERE/'engine-source-before'/str(index)/'lease.json';authority.write_text(json.dumps(lease,indent=2),encoding='utf-8')
        ledger['repairs'].append(dict(path=relative,before_sha256=sha(path),after_sha256=None,backup=str(backup.relative_to(HERE)).replace('\\','/'),diagnostic=a.diagnostic,authority=str(authority.relative_to(HERE)).replace('\\','/'),authority_sha256=sha(authority),started_at=now.isoformat()))
    else:
        matches=[r for r in ledger['repairs'] if r['path']==relative and r['after_sha256'] is None]
        assert len(matches)==1,'Expected exactly one unfinished repair'
        matches[0]['after_sha256']=sha(path);matches[0]['finished_at']=datetime.now(ZoneInfo('Asia/Hong_Kong')).isoformat()
    ledger_path.write_text(json.dumps(ledger,ensure_ascii=False,indent=2),encoding='utf-8');print(a.mode,relative)


if __name__=='__main__':main()
