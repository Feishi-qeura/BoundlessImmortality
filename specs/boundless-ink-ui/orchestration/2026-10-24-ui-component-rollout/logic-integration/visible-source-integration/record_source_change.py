"""Exact source-only grant/backup/hash ledger. Never starts Unreal or writes Content."""
from pathlib import Path
from datetime import datetime
from zoneinfo import ZoneInfo
import argparse,difflib,hashlib,json

HERE=Path(__file__).resolve().parent
ROLLOUT=HERE.parent.parent
ROOT=ROLLOUT.parents[3]
LEDGER=HERE/'source-change-ledger.json'
def sha(data):return hashlib.sha256(data).hexdigest()
def now():return datetime.now(ZoneInfo('Asia/Hong_Kong')).isoformat()
def read_ledger():return json.loads(LEDGER.read_text(encoding='utf-8')) if LEDGER.exists() else dict(owner='logic-integration',changes=[])
def save(value):LEDGER.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
def check_grant(relative):
    grant_path=ROLLOUT/'source-promotion-grant-visible-pages.json'
    raw=grant_path.read_bytes();grant=json.loads(raw.decode('utf-8-sig'))
    assert grant['owner_team']=='logic-integration' and grant['state'] in ('source_only_granted','full_ui_refactor_and_bounded_engine_authorized')
    assert grant['batch2_promotion_authorized']
    target=(ROOT/relative).resolve()
    source_file=target.is_relative_to((ROOT/'Source').resolve()) and target.suffix in ('.h','.cpp')
    config_file=target.parent==(ROOT/'Config').resolve() and target.suffix=='.ini'
    exception=grant.get("bounded_backend_projection_exception",{})
    backend_file=relative=="Tools/CultivationService/authority.py" and exception.get("path")==relative and exception.get("owner")=="logic-integration" and target==(ROOT/"Tools/CultivationService/authority.py").resolve()
    assert source_file or config_file or backend_file
    allowed=False
    for item in grant['source_targets']:
        permitted=(ROOT/item).resolve()
        if target==permitted or (item.endswith(('/', '\\')) and target.is_relative_to(permitted)):allowed=True
    assert allowed,'Target not in current explicit grant: '+relative
    return target,raw

def begin(relative,reason,metadata=None):
    target,grant=check_grant(relative);ledger=read_ledger()
    assert not any(v['state']=='editing' for v in ledger['changes']),'Finish previous source change before another mutation'
    prior=[v for v in ledger['changes'] if v['path']==relative]
    before=target.read_bytes() if target.exists() else None
    if prior:assert prior[-1]['after_sha256']==(sha(before) if before is not None else None),'Unexpected target drift since prior exact change'
    index=len(ledger['changes']);folder=HERE/'backups'/str(index);folder.mkdir(parents=True)
    (folder/'grant.json').write_bytes(grant)
    if before is not None:(folder/'before').write_bytes(before)
    row=dict(path=relative,state='editing',reason=reason,started_at=now(),before_sha256=sha(before) if before is not None else None,
             before_exists=before is not None,backup=str(folder.relative_to(HERE)).replace('\\','/'),grant_sha256=sha(grant),metadata=metadata or {})
    ledger['changes'].append(row);save(ledger);return row

def finish(relative):
    target,_=check_grant(relative);ledger=read_ledger();row=ledger['changes'][-1]
    assert row['path']==relative and row['state']=='editing'
    folder=HERE/row['backup'];before=(folder/'before').read_bytes() if row['before_exists'] else b''
    if row['before_exists']:assert sha(before)==row['before_sha256']
    after=target.read_bytes();(folder/'after').write_bytes(after)
    diff=''.join(difflib.unified_diff(before.decode('utf-8-sig').splitlines(True),after.decode('utf-8-sig').splitlines(True),fromfile='before/'+relative,tofile='after/'+relative))
    (folder/'change.diff').write_text(diff,encoding='utf-8')
    row.update(state='recorded',after_sha256=sha(after),finished_at=now(),changed=before!=after);save(ledger);return row

def promote(owner,receipt,staged,target):
    receipt_path=(ROLLOUT/receipt).resolve();stage=(ROLLOUT/staged).resolve()
    owner_roots=[(ROLLOUT/owner).resolve(),(ROOT/'specs/boundless-ink-ui/rebuild-2026-10-24'/owner).resolve()]
    assert any(receipt_path.is_relative_to(root) and stage.is_relative_to(root) for root in owner_roots),'Receipt/stage must be under the same explicit owner directory'
    receipt_raw=receipt_path.read_bytes()
    raw=stage.read_bytes();metadata=dict(owner=owner,resolution=receipt,resolution_sha256=sha(receipt_raw),staged=staged,staged_sha256=sha(raw))
    row=begin(target,'Promote owner-resolved staged component after independent review',metadata)
    folder=HERE/row['backup'];(folder/'owner-resolution').write_bytes(receipt_raw);(folder/'staged').write_bytes(raw)
    assert stage.read_bytes()==raw and receipt_path.read_bytes()==receipt_raw,'Owner stage changed during promotion snapshot; abort before Source write'
    destination,_=check_grant(target);destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(raw)
    return finish(target)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('mode',choices=['begin','finish','promote']);parser.add_argument('target');parser.add_argument('--reason');parser.add_argument('--owner');parser.add_argument('--receipt');parser.add_argument('--staged');args=parser.parse_args()
    if args.mode=='begin':result=begin(args.target,args.reason)
    elif args.mode=='finish':result=finish(args.target)
    else:result=promote(args.owner,args.receipt,args.staged,args.target)
    print(json.dumps(result,ensure_ascii=False,indent=2))
