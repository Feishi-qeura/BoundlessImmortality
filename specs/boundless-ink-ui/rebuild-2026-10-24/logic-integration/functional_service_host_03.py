"""Isolated real authority, synthetic actors and controlled test clock; no gameplay rule changes."""
from pathlib import Path
import sys,os,time,json,threading,uuid,hashlib,copy
from datetime import datetime,timezone
H=Path(__file__).resolve().parent;ROOT=H.parents[3];sys.path.insert(0,str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server
APP='NewUIFunctional20261024';USER='functional-anchor';ROOM='functional-room';OFFSET=0.;DONE=set();LOCK=threading.RLock()
D=H/'service-data-03';D.mkdir(exist_ok=False);DB=D/'prototype.sqlite3';RULES=RuleBundle.load(ROOT/'docs/design/2026-10-21-wuling-resource-v1.4')
AUTH=Authority(DB,RULES,lambda:time.time()+OFFSET);CONTROL=H/'functional-service-control-03.json';TRACE=H/'functional-service-trace-03.jsonl'
def write(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2),encoding='utf-8')
def trace(d):
 d.update(utc=datetime.now(timezone.utc).isoformat(),virtual_utc=AUTH.clock())
 with LOCK:
  with TRACE.open('a',encoding='utf-8') as f:f.write(json.dumps(d,ensure_ascii=False)+'\n')
def session(u,r,role):return AUTH.open_session(dict(app=APP,room=r,user=u,role=role,display_name=u,development_only=True,simulated=True,profile='prototype_full'))['token']
def current(u):
 with AUTH.store.transaction() as db:
  row=db.execute('SELECT token FROM session WHERE user=? AND revoked=0',(u,)).fetchone();return row['token'] if row else None
def command(t,op,args=None):
 s=AUTH.snapshot(t);return AUTH.execute(t,dict(request_id=uuid.uuid4().hex,profile='prototype_full',rule_version=RULES.version,expected_version=s['version'],command=op,args=args or {}))
PAIRS=[]
for u,r in [(USER,ROOM),('functional-opponent','functional-opponent-room'),('functional-world','functional-world-room')]:
 a=session(u,r,'anchor');v=session(u+'-viewer',r,'viewer');g=command(a,'CreateGuild',dict(name='隔离验证'+u))['result']['guild']['id'];assert command(v,'JoinGuild',dict(guild_id=g))['status']=='success';AUTH.sync(a,dict(active=True,participants=[dict(token=v,active=True)]));PAIRS.append((u,u+'-viewer'))
write(H/'functional-service-fixture-03.json',dict(declared_synthetic_actors=True,naturally_earned=False,app=APP,user=USER,room=ROOM,world_user='functional-world',world_room='functional-world-room',clock='real wall clock plus explicitly controlled offset; advances in<=4s steps with real session heartbeats; no world/race states or rewards injected',database=str(DB),production_authority_sha256=hashlib.sha256((ROOT/'Tools/CultivationService/authority.py').read_bytes()).hexdigest()))
original_execute=AUTH.execute
def execute(t,b):
 trace(dict(event='command_received',request={k:b.get(k) for k in ('request_id','expected_version','command','args')}));r=original_execute(t,b);state=r.get('result') or {};trace(dict(event='command_result',command=b.get('command'),request_id=b.get('request_id'),status=r.get('status'),error_code=r.get('error_code'),round=state.get('round'),race=state.get('race'),queue=state.get('race_queue'),launch=state.get('launch'),reward_receipts=state.get('reward_receipts')));return r
AUTH.execute=execute
SERVER=make_server(AUTH,0);Handler=SERVER.RequestHandlerClass;reply0=Handler.reply

def reply(handler,status,response):
 c=json.loads(CONTROL.read_text()) if CONTROL.exists() else {}
 if c.get('omit_identity_active') and isinstance(response,dict):
  response=copy.deepcopy(response)
  for state in [response,response.get('state'),response.get('result')]:
   if isinstance(state,dict) and isinstance(state.get('identity'),dict) and state['identity'].get('user')==USER:state['identity'].pop('active',None)
 return reply0(handler,status,response)
Handler.reply=reply
META=dict(pid=os.getpid(),port=SERVER.server_port,app=APP,user=USER,room=ROOM,database=str(DB),stopped=False,scope='Actual production authority; declared isolated actors and test clock; optional missing-field transport fault only')
write(H/'functional-service-ready-03.json',META)
def beats(include_native=False):
 for u,v in PAIRS:
  vt=current(v)
  if vt:AUTH.sync(vt,dict(active=True))
  at=current(u)
  if not at:continue
  if u=='functional-opponent':AUTH.sync(at,dict(active=True))
  elif include_native:
   with AUTH.store.transaction() as db:flag=bool(db.execute('SELECT active FROM session WHERE token=?',(at,)).fetchone()[0])
   AUTH.sync(at,dict(active=flag))
def monitor():
 global OFFSET
 try:
  while True:
   time.sleep(.3);c=json.loads(CONTROL.read_text()) if CONTROL.exists() else {};lease=json.loads((H.parent/'engine-lease.json').read_text(encoding='utf-8-sig'))
   if c.get('stop') or datetime.fromisoformat(lease['expires_at'])<=datetime.now(timezone.utc):SERVER.shutdown();return
   beats()
   for op in ('queue_opponent',):
    if c.get(op) and op not in DONE:
     result=command(current('functional-opponent'),'QueueRace',dict(node_id='NODE_STONE'));assert result['status']=='success',result;DONE.add(op);trace(dict(event=op,result=result))
   seq=c.get('advance_sequence')
   if seq is not None and ('advance',seq) not in DONE:
    seconds=c.get('advance_seconds',0);assert isinstance(seconds,(int,float)) and 0<=seconds<=720
    remaining=seconds
    while remaining>0:
     step=min(4.,remaining);OFFSET+=step;remaining-=step;beats(True)
    DONE.add(('advance',seq));trace(dict(event='clock_advanced',sequence=seq,seconds=seconds,offset=OFFSET));write(H/'functional-clock-state-03.json',dict(sequence=seq,offset=OFFSET,virtual_utc=AUTH.clock()))
 except Exception:
  import traceback
  write(H/'functional-service-monitor-error-03.json',dict(error=traceback.format_exc()));SERVER.shutdown()
threading.Thread(target=monitor,daemon=True).start()
try:SERVER.serve_forever()
finally:SERVER.server_close();META.update(stopped=True,offset=OFFSET);write(H/'functional-service-result-03.json',META)
