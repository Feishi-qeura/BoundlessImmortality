"""Owned loopback authority with declared fixtures and transport-only fault injection."""
from pathlib import Path
import base64,hashlib,json,os,socket,struct,sys,threading,time,uuid,zlib
from datetime import datetime,timezone
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];sys.path.insert(0,str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server
from Tools.CultivationService.inventory import Inventory
from Tools.CultivationService.guilds import Guilds
APP='NewUIHostAcceptance20261024';ROOM='newui-room-02';USER='newui-anchor-02'
FOLDER=HERE/'service-data-02';FOLDER.mkdir(exist_ok=True);DB=FOLDER/'prototype.sqlite3'
assert not DB.exists(),'Never overwrite or silently reuse an existing acceptance database'
RULES=RuleBundle.load(ROOT/'docs/design/2026-10-21-wuling-resource-v1.4');AUTH=Authority(DB,RULES)
LOCK=threading.Lock();TLS=threading.local();DROPPED=False;AVATARS=0
TRACE=HERE/'service-command-trace-02.jsonl';CONTROL=HERE/'service-control-02.json';READY=HERE/'service-ready-02.json';RESULT=HERE/'service-process-02.json'
def write(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
def trace(value):
 value.update(utc=datetime.now(timezone.utc).isoformat(),monotonic=time.monotonic())
 with LOCK:
  with TRACE.open('a',encoding='utf-8') as out:out.write(json.dumps(value,ensure_ascii=False)+'\n')
def session(user,room=ROOM,role='anchor'):
 return AUTH.open_session(dict(app=APP,room=room,user=user,role=role,display_name='界面验证样本',development_only=True,simulated=True,profile='prototype_full'))['token']
def seed(token,stone,items,owner=None):
 with AUTH.store.transaction() as db:
  ctx=AUTH._context(db,token);target=owner or ctx.user
  if stone:ctx.money(target,'CUR_SUPPLY' if owner else 'CUR_STONE',stone,'declared-newui-test-fixture','newui-fixture-20261024')
  for item,count in items.items():Inventory.grant(ctx,item,count,dict(id='declared-newui-test-fixture',expiry=ctx.now+7200),target)
def command(token,operation,args):
 version=AUTH.snapshot(token)['version'];return AUTH.execute(token,dict(request_id=uuid.uuid4().hex,profile='prototype_full',rule_version=RULES.version,expected_version=version,command=operation,args=args))
anchor=session(USER);items={'MAT_HERB_T':12,'MAT_IRON_T':12,'MAT_BR01_C01_G1':4}
book=next(r for r in RULES.tables['items-v1.4/method_items.csv'] if r['item_type']=='book' and r['min_major']==0);items[book['item_id']]=1
gear=next(r for r in RULES.tables['items-v1.4/equipment_items.csv'] if r['min_major']==0);items[gear['item_id']]=1
fabao=next(r for r in RULES.tables['items-v1.4/fabao_items.csv'] if r['min_major']==0);items[fabao['item_id']]=1
recipe=next(r for r in RULES.tables['items-v1.4/recipes.csv'] if r['min_major']==0)
for row in RULES.tables['items-v1.4/recipe_inputs.csv']:
 if row['recipe_id']==recipe['recipe_id']:items[row['item_id']]=max(items.get(row['item_id'],0),int(row['count'])*3)
seed(anchor,500,items)
seller=session('newui-seller-fixture','newui-seller-room');seed(seller,10,{'MAT_HERB_T':4})
seller_result=command(seller,'CreateListing',dict(item_id='MAT_HERB_T',count=2,unit_price=4));assert seller_result['status']=='success',seller_result
write(HERE/'service-fixture-02.json',dict(declared_test_fixture=True,naturally_earned=False,database=str(DB),app=APP,room=ROOM,user=USER,rules_version=RULES.version,rules_hash=RULES.sha256,anchor_stone=500,anchor_items=items,book=book['item_id'],method=book['method_id'],equipment=gear['item_id'],fabao=fabao['item_id'],recipe=recipe['recipe_id'],fixture_seller_listing=True))
original_execute=AUTH.execute
def execute(token,body):
 TLS.command=body;trace(dict(event='command_received',request={k:body.get(k) for k in ('request_id','profile','rule_version','expected_version','command','args')},payload_sha256=hashlib.sha256(json.dumps(body,sort_keys=True,separators=(',',':'),ensure_ascii=False).encode()).hexdigest()))
 time.sleep(2.0 if body.get('command')=='CreateGuild' else .7)
 response=original_execute(token,body);trace(dict(event='command_result',request_id=body.get('request_id'),command=body.get('command'),status=response.get('status'),error_code=response.get('error_code'),version=(response.get('result') or {}).get('version')));return response
AUTH.execute=execute
SERVER=make_server(AUTH,0);Handler=SERVER.RequestHandlerClass;original_reply=Handler.reply;original_get=Handler.do_GET
# A valid deterministic PNG transport fixture, not a delivered design asset.
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
PNG=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\x00\x22\x88\x66\xff'))+chunk(b'IEND',b'')
def get(handler):
 global AVATARS
 if handler.path!='/avatar-fixture.png':return original_get(handler)
 with LOCK:AVATARS+=1;number=AVATARS
 trace(dict(event='avatar_request',number=number,status=503 if number==1 else 200))
 if number==1:return original_reply(handler,503,dict(error_code='DECLARED_AVATAR_TRANSPORT_FAILURE'))
 handler.send_response(200);handler.send_header('Content-Type','image/png');handler.send_header('Content-Length',str(len(PNG)));handler.end_headers();handler.wfile.write(PNG)
def reply(handler,status,response):
 global DROPPED
 body=getattr(TLS,'command',{})
 if handler.path=='/v1/wuling/command' and body.get('command')=='CreateGuild' and response.get('status')=='success' and not DROPPED:
  DROPPED=True;trace(dict(event='reply_dropped_after_commit',request_id=body.get('request_id'),command='CreateGuild'))
  try:handler.connection.shutdown(socket.SHUT_RDWR)
  except OSError:pass
  handler.connection.close();return
 return original_reply(handler,status,response)
Handler.do_GET=get;Handler.reply=reply
META=dict(pid=os.getpid(),port=SERVER.server_port,started_at=datetime.now(timezone.utc).isoformat(),database=str(DB),app=APP,room=ROOM,user=USER,scope='actual existing authority; declared isolated data; command delay and one post-commit dropped reply are transport fixtures',stopped=False)
write(READY,META);write(RESULT,META)
DONE=set()
def monitor():
 while True:
  time.sleep(.25)
  lease=json.loads((HERE.parent/'engine-lease.json').read_text(encoding='utf-8-sig'));expired=datetime.fromisoformat(lease['expires_at'])<=datetime.now(timezone.utc)
  control=json.loads(CONTROL.read_text(encoding='utf-8')) if CONTROL.exists() else {}
  if expired or control.get('stop'):SERVER.shutdown();return
  if control.get('prepare_guild_fixture') and 'guild' not in DONE:
   with AUTH.store.transaction() as db:
    row=db.execute('SELECT token FROM session WHERE user=? AND revoked=0',(USER,)).fetchone();token=row['token'] if row else None;ctx=AUTH._context(db,token) if token else None;guild=Guilds.current(ctx) if ctx else None
   if not guild:continue
   member=session('newui-member-fixture',role='viewer');joined=command(member,'JoinGuild',dict(guild_id=guild['id']));assert joined['status']=='success',joined
   seed(token,100,{'GUILD_MAT_01':8,'GUILD_MAT_02':8},guild['id']);DONE.add('guild');write(HERE/'service-guild-fixture-02.json',dict(declared_test_fixture=True,guild_id=guild['id'],member='newui-member-fixture',supply=100,materials={'GUILD_MAT_01':8,'GUILD_MAT_02':8}));trace(dict(event='declared_guild_fixture_ready',guild_id=guild['id']))
threading.Thread(target=monitor,daemon=True).start()
try:SERVER.serve_forever()
finally:
 SERVER.server_close();META.update(stopped=True,finished_at=datetime.now(timezone.utc).isoformat(),avatar_requests=AVATARS,post_commit_reply_drop=DROPPED);write(RESULT,META)
