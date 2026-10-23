"""Local aggregate audit export; never exports session tokens or user identities."""
import argparse
import json
import math
import os
from pathlib import Path
import sqlite3
from datetime import datetime,timedelta,timezone
from .contracts import require


class Metrics:
    @staticmethod
    def validate_output(db_path,out_path):
        db_path=Path(db_path).resolve(strict=True);out_path=Path(out_path).resolve()
        for protected in (db_path,Path(str(db_path)+'-wal'),Path(str(db_path)+'-shm')):
            require(out_path!=protected and not (protected.exists() and out_path.exists() and os.path.samefile(protected,out_path)),'OUTPUT_OVERWRITES_DATABASE')

    @staticmethod
    def export(path,start,end):
        require(math.isfinite(start) and math.isfinite(end) and start<end,'INVALID_TIME_RANGE')
        path=Path(path).resolve(strict=True)
        db=sqlite3.connect(path.as_uri()+'?mode=ro',uri=True)
        db.row_factory=sqlite3.Row
        try:
            db.execute('BEGIN')
            result=dict(boundary='local simulated prototype aggregates; item/visit audit begins when this schema was installed; existing legacy gifts not imported',start_utc=start,end_utc=end,scopes=[])
            tables={r[0] for r in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
            for ns in db.execute('SELECT * FROM namespace'):
                scope=ns['id'];args=(scope,start,end)
                commands={}
                for r in db.execute("SELECT data FROM fact WHERE scope=? AND kind='command_audit' AND created>=? AND created<?",args):
                    op=json.loads(r[0])['operation'];commands[op]=commands.get(op,0)+1
                worlds=[json.loads(r[0]) for r in db.execute("SELECT data FROM entity WHERE scope=? AND kind='world'",(scope,))]
                worlds=[w for w in worlds if start<=w['start']<end]
                viewers={r['user'] for r in db.execute("SELECT user FROM identity WHERE scope=? AND role='viewer'",(scope,))}
                participants={r['user'] for r in db.execute("SELECT DISTINCT user FROM fact WHERE scope=? AND kind='world_action' AND created>=? AND created<?",args)}&viewers
                active=set();returning=set()
                for r in db.execute("SELECT id,data FROM entity WHERE scope=? AND kind='presence_history'",(scope,)):
                    spans=[s for s in json.loads(r['data'])['spans'] if s['start']<end and s['end']>start]
                    if not spans:continue
                    if r['id'] in viewers:active.add(r['id'])
                    last_day=datetime.fromtimestamp(min(end,max(s['end'] for s in spans))-.000001,timezone(timedelta(hours=8))).strftime('%Y-%m-%d')
                    if 'visit' in tables and db.execute('SELECT 1 FROM visit WHERE scope=? AND user=? AND day<?',(scope,r['id'],last_day)).fetchone():returning.add(r['id'])
                item_flows={}
                if 'item_ledger' in tables:
                    for r in db.execute('SELECT item,amount,paid FROM item_ledger WHERE scope=? AND created>=? AND created<?',args):
                        flow=item_flows.setdefault(r['item'],dict(granted=0,consumed=0,paid_origin_granted=0))
                        flow['granted' if r['amount']>0 else 'consumed']+=abs(r['amount'])
                        if r['paid'] and r['amount']>0:flow['paid_origin_granted']+=r['amount']
                wallet=[dict(r) for r in db.execute('SELECT currency,reason,sum(CASE WHEN amount>0 THEN amount ELSE 0 END) inflow,sum(CASE WHEN amount<0 THEN -amount ELSE 0 END) outflow FROM wallet_ledger WHERE scope=? AND created>=? AND created<? GROUP BY currency,reason',args)]
                complete=sum(w.get('status')=='Completed' and len(w.get('completed',[]))==3 for w in worlds)
                result['scopes'].append(dict(scope=scope,app=ns['app'],profile=ns['profile'],rules=ns['rules'],rules_hash=ns['rules_hash'],window_start=ns['start'],window_end=ns['end'],commands=commands,
                    active_viewers=len(active),participating_viewers=len(participants),participation_rate=len(participants&active)/len(active) if active else None,
                    worlds_started=len(worlds),worlds_completed=complete,completion_rate=complete/len(worlds) if worlds else None,
                    returning_users=len(returning),item_flows=item_flows,wallet_flows=wallet,real_revenue=None,real_conversion=None,
                    definitions=dict(active_viewers='observed heartbeat spans intersecting [start,end)',participating_viewers='world_action facts committed in [start,end); race actions separate',completion_rate='fully completed, non-interrupted worlds / worlds started in [start,end)',returning_users='active users of either role with an earlier Hong Kong visit date')))
            return result
        finally:db.close()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db',type=Path,required=True);parser.add_argument('--start',type=float,required=True);parser.add_argument('--end',type=float,required=True);parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();Metrics.validate_output(args.db,args.out)
    report=Metrics.export(args.db,args.start,args.end)
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(f'Local aggregate audit saved: {args.out}')


if __name__=='__main__':main()
