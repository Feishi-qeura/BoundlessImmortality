"""Isolated actual Authority HTTP service with declared, test-only fixtures/failure controls."""
import json
from pathlib import Path
import sys
import threading
import time
import uuid
import os
import socket
from datetime import datetime

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.inventory import Inventory
from Tools.CultivationService.http_server import make_server

RUN=HERE/(sys.argv[1] if len(sys.argv)>1 else 'engine-qa-service');RUN.mkdir(exist_ok=True)
CONTROL=dict(offset=0,delay=0,offline=False,bump_next='')
if os.environ.get('ROLLOUT_QA_BASE_UTC'):
    CONTROL['offset']=datetime.fromisoformat(os.environ['ROLLOUT_QA_BASE_UTC']).timestamp()-time.time()
TOKENS={}
AUDIT=RUN/'requests.jsonl'
APP='rollout-qa-20261024';ROOM='rollout-room';USER='rollout-anchor';PORT=18789


def write_audit(row):
    with AUDIT.open('a',encoding='utf-8') as stream:stream.write(json.dumps(row,ensure_ascii=False)+'\n')


class RecordedAuthority(Authority):
    def open_session(self,body):
        if body.get('user')==USER and CONTROL.get('session_mode')=='viewer':
            body=dict(body,user='rollout-viewer',role='viewer')
        result=super().open_session(body)
        if body.get('user')==USER:TOKENS['anchor']=result['token']
        return result

    def execute(self,token,body):
        operation=body.get('command')
        if operation==CONTROL['bump_next']:
            CONTROL['bump_next']=''
            # Explicit concurrent-version fixture; the real handler below produces STALE_VERSION.
            with self.store.transaction() as db:
                ctx=self._context(db,token)
                db.execute('UPDATE namespace SET version=version+1 WHERE id=?',(ctx.scope,))
            write_audit(dict(origin='declared-version-race-fixture',command=operation))
        if CONTROL['delay']:time.sleep(CONTROL['delay'])
        reply=super().execute(token,body)
        write_audit(dict(origin='actual-http-command',command=operation,request=body,response=reply))
        if operation==CONTROL.get('drop_next'):
            CONTROL['drop_next']='';CONTROL['drop_request_id']=body['request_id']
        return reply


authority=RecordedAuthority(RUN/(uuid.uuid4().hex+'.sqlite3'),RuleBundle.load(ROOT/'docs/design/2026-10-21-wuling-resource-v1.4'),lambda:time.time()+CONTROL['offset'])
authority.random_draw=lambda _:0  # Declared deterministic test roll, real success/settlement path.
authority.open_session(dict(app=APP,room=ROOM,user=USER,role='anchor',development_only=True,simulated=True,profile='prototype_full'))


def seed(case='normal'):
    with authority.store.transaction() as db:
        ctx=authority._context(db,TOKENS['anchor'])
        db.execute('UPDATE growth SET realm=21,qi=10000 WHERE scope=? AND user=?',(ctx.scope,ctx.user))
        if case!='unowned':Inventory.grant(ctx,'MAT_BR04_C01_G1',1,dict(id='declared-pie-fixture'))
        ctx.put('combat',ctx.user,dict(health=1000,max_health=1000,last_combat=0,safe=case!='unsafe'))
        ctx.count(ctx.user,'sign-month:'+ctx.day[:7],3)
    write_audit(dict(origin='declared-fixture',case=case,realm=21,qi=10000,core_count=case!='unowned',month_count=3))


seed()
server=make_server(authority,PORT)
OriginalHandler=server.RequestHandlerClass


class QAHandler(OriginalHandler):
    def reply(self,status,response):
        if self.path=='/v1/wuling/command' and CONTROL.get('drop_request_id') and response.get('request_id')==CONTROL['drop_request_id']:
            CONTROL.pop('drop_request_id',None)
            write_audit(dict(origin='declared-response-loss-after-real-commit',request_id=response['request_id']))
            self.close_connection=True
            self.connection.shutdown(socket.SHUT_RDWR)
            return
        super().reply(status,response)

    def do_POST(self):
        if self.path=='/qa/control':
            body=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
            if 'advance_seconds' in body:CONTROL['offset']+=float(body['advance_seconds'])
            if 'delay' in body:CONTROL['delay']=max(0,min(5,float(body['delay'])))
            if 'offline' in body:CONTROL['offline']=bool(body['offline'])
            if 'bump_next' in body:CONTROL['bump_next']=str(body['bump_next'])
            if 'drop_next' in body:CONTROL['drop_next']=str(body['drop_next'])
            if 'session_mode' in body:CONTROL['session_mode']=body['session_mode']
            if body.get('safe') is not None:
                with authority.store.transaction() as db:
                    ctx=authority._context(db,TOKENS['anchor']);state=ctx.entity('combat',ctx.user) or {}
                    state.update(health=1000,max_health=1000,last_combat=0,safe=bool(body['safe']));ctx.put('combat',ctx.user,state)
            if body.get('upgrade_core'):
                with authority.store.transaction() as db:
                    ctx=authority._context(db,TOKENS['anchor']);old=ctx.entity('quality',ctx.user+':BR04')
                    rows=[r for r in authority.rules.tables['items-v1.4/breakthrough_material_items.csv'] if r['stage_id']=='BR04' and r['category']=='core' and r['quality_weight']>old['core_score']]
                    upgrade=max(rows,key=lambda r:r['quality_weight']);Inventory.grant(ctx,upgrade['item_id'],1,dict(id='declared-rework-fixture'))
                CONTROL['upgrade_item']=upgrade['item_id']
            write_audit(dict(origin='declared-control',control=body))
            self.reply(200,dict(ok=True,control=CONTROL));return
        if self.path=='/qa/shutdown':
            self.reply(200,dict(ok=True));threading.Thread(target=server.shutdown,daemon=True).start();return
        if CONTROL['offline']:
            self.reply(503,dict(error_code='QA_DECLARED_OFFLINE'));return
        super().do_POST()


server.RequestHandlerClass=QAHandler
metadata=dict(app=APP,room=ROOM,user=USER,port=PORT,database=str(authority.store.path),test_only=True,
              declared_server_clock=os.environ.get('ROLLOUT_QA_BASE_UTC','real wall clock'),
              fixture='realm21/qi10000/core1/month attendance3; deterministic roll0; no production database, player save or rule changes')
(RUN/'fixture.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(metadata,ensure_ascii=False),flush=True)
try:server.serve_forever()
finally:server.server_close()
