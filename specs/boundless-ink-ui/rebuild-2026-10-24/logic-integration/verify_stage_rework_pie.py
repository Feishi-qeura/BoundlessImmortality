"""Actual default PIE and native HTTP service, declared isolated fixtures only."""
from pathlib import Path
import gc,json,time,traceback,os,hashlib
from datetime import datetime,timezone
import unreal
HERE=Path(__file__).resolve().parent
OUT=HERE/'stage-pie-pending'
L=unreal.InkFrontendEditorLibrary
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
R=dict(checks=[],captures=[],finished=False,scope='Actual DB02 service continuation: member/build version guard/craft/market through Slate plus declared avatar transport regression on default root',engine_pid=os.getpid(),source_epoch_sha256=hashlib.sha256((HERE/'source-epoch.json').read_bytes()).hexdigest(),started_at=datetime.now(timezone.utc).isoformat())
F=S=W=None
I=-2;NEXT=0;START=time.monotonic();QUIT=[None];STEPS=[];WAIT={};SELECTED={}

def save():
 R['passed']=R['finished'] and not R.get('error') and all(x['passed'] for x in R['checks'])
 (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding='utf-8')
def ck(name,value,actual=None):
 R['checks'].append(dict(name=name,passed=bool(value),actual=actual));save()
 if not value:raise AssertionError(name+': '+str(actual))
def snap():return json.loads(L.describe_widget(F))
def state():return json.loads(S.get_snapshot_json())
def trace():return [json.loads(x) for x in (HERE/'service-command-trace-02.jsonl').read_text(encoding='utf-8').splitlines()]
def count(operation):return len([x for x in trace() if x.get('event')=='command_received' and x.get('request',{}).get('command')==operation])
def capture(tag):
 data=snap();(OUT/(tag+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
 (OUT/(tag+'-authority.json')).write_text(json.dumps(state(),ensure_ascii=False,indent=2),encoding='utf-8')
 ck('actual rendered '+tag,L.capture_frontend_viewport(str(OUT/(tag+'.png'))));R['captures'].append(tag);save()
def click(action):ck('Slate action '+action,L.click_action(F,action))
def label(caption):ck('Slate native button '+caption,L.click_labeled_button(F,unreal.Text(caption)))
def route(page):ck('actual default router '+page,F.open_feature_panel(page,''))
def find(caption=None,intent=None):
 candidates=[b for b in snap()['buttons'] if b.get('visible') and (caption is None or b.get('label')==caption) and (intent is None or b.get('intent_action')==intent)]
 ck('unique native target '+str(caption or intent),len(candidates)==1,candidates)
 return candidates[0]
def reveal(caption=None,intent=None):
 b=find(caption,intent)
 inside=b.get('paint_expectation')=='expected' and all(b.get('independent_'+edge,0)>=b.get('independent_clip_'+edge,0)-1 for edge in ('left','top')) and all(b.get('independent_'+edge,0)<=b.get('independent_clip_'+edge,0)+1 for edge in ('right','bottom'))
 if inside:ck('already independently inside visible clip '+str(caption or intent),True,b);return
 ck('actual scroll '+str(caption or intent),L.scroll_to_frontend_widget(F,b['name']))
def ordinary(caption=None,intent=None):
 b=find(caption,intent);ck('painted native target '+str(caption or intent),b.get('paint_expectation')=='expected',b)
 ck('actual native Slate click '+str(caption or intent),L.click_rollout_button(F,unreal.find_object(None,b['object'])))
def type_text(name,text):ck('actual Slate text '+name,L.type_frontend_text(F,name,text))
def wait_for(name,predicate,timeout=45):
 if predicate():WAIT.pop(name,None);ck(name,True);return True
 started=WAIT.setdefault(name,time.monotonic())
 if time.monotonic()-started>timeout:raise TimeoutError(name+'; '+S.get_status())
 return False
def step(name,action,delay=1):STEPS.append((name,action,delay))
def wait(name,predicate,timeout=45):step(name,lambda:wait_for(name,predicate,timeout),.1)
def native_query(kind):ck('setup: existing native catalog '+kind,S.request_catalog(kind,0,False))
def idle():return S.is_connected() and not S.is_busy()
def resource(tag,path):
 expected=path+'.'+path.rsplit('/',1)[-1]
 ck('actual runtime brush '+tag,any(x.get('visible') and x.get('resource')==expected for x in snap()['images']),expected)
def idempotency():
 rows=[x for x in trace() if x.get('event')=='command_received' and x.get('request',{}).get('command')=='CreateGuild']
 ck('post-commit dropped reply was exercised',any(x.get('event')=='reply_dropped_after_commit' for x in trace()))
 ck('actual native outbox retried',len(rows)>=2,len(rows))
 ck('same id and payload on retry',len({(x['request']['request_id'],x['payload_sha256']) for x in rows})==1,rows)
def busy_inventory():
 ck('actual native Busy while still connected',S.is_busy() and S.is_connected(),dict(busy=S.is_busy(),connected=S.is_connected()))
 before=state();R['busy_authority_inventory_before']=before.get('inventory')
 click('OpenInventory')
def busy_observation():
 ck('actual Inventory page during pending request',snap().get('page')=='Inventory')
 ck('confirmed inventory retained during Busy',state().get('inventory')==R['busy_authority_inventory_before'])
 capture('inventory-real-pending')
FIXTURE=json.loads((HERE/'service-fixture-02.json').read_text())
def inventory_entry(item):
 rows=json.loads(L.describe_inventory(F))['entries'];matches=[r for r in rows if r['item_id']==item]
 ck('one actual inventory instance '+item,len(matches)==1,matches);return matches[0]
def player_item_target(item,inventory=False):
 name=inventory_entry(item)['name']
 rows=[b for b in snap()['buttons'] if b.get('visible') and b.get('widget_class','').endswith('.InkPlayerCActionButton') and b.get('label','').startswith(name+'\n') and (('数量：' in b.get('label',''))==inventory)]
 ck('one actual player item row '+item,len(rows)==1,rows);return rows[0]
def reveal_player_item(item,inventory=False):ck('scroll actual player item '+item,L.scroll_to_frontend_widget(F,player_item_target(item,inventory)['name']))
def click_player_item(item,inventory=False):
 b=player_item_target(item,inventory);ck('Slate actual player item '+item,L.click_rollout_button(F,unreal.find_object(None,b['object'])))
def entry_target(entry):
 rows=[b for b in snap()['buttons'] if b.get('visible') and b.get('intent_entry')==entry and b.get('intent_action')=='None']
 ck('one real stable entry '+entry,len(rows)==1,rows);return rows[0]
def reveal_entry(entry):ck('scroll stable entry '+entry,L.scroll_to_frontend_widget(F,entry_target(entry)['name']))
def click_entry(entry):
 b=entry_target(entry);ck('Slate select stable entry '+entry,L.click_rollout_button(F,unreal.find_object(None,b['object'])))
def success(operation):
 rows=[x for x in trace() if x.get('event')=='command_result' and x.get('command')==operation]
 ck('actual authority success '+operation,bool(rows) and rows[-1].get('status')=='success',rows[-1] if rows else rows)
def sign_confirmed():
 success('SignIn');a=state().get('activities',{});ck('actual signed state and native zero-reward contract',a.get('signed') is True and a.get('sign_in_reward_mode')=='placeholder' and a.get('sign_in_qi_reward')==0,a)
def confirm_stale_guild():
 SELECTED['rename_commands']=count('RenameGuild');click('Ext_GuildConfirm')
def no_stale_write():ck('changed version confirmation dispatches no rename',count('RenameGuild')==SELECTED['rename_commands'])

def context_target(caption,required):
 data=snap();rows=[]
 for button in data['buttons']:
  if not button.get('visible') or button.get('label')!=caption:continue
  parent=unreal.find_object(None,button['object']).get_parent()
  while isinstance(parent,(unreal.SizeBox,unreal.Border)):parent=parent.get_parent()
  texts=[x.get('text','') for x in data['texts'] if parent and x.get('parent_object')==parent.get_path_name()]
  if all(any(term in text for text in texts) for term in required):rows.append(button)
 ck('one real contextual row '+caption+' '+str(required),len(rows)==1,rows);return rows[0]
def context_reveal(caption,required):
 b=context_target(caption,required)
 if b.get('paint_expectation')=='expected' and b.get('independent_top',-1)>=b.get('independent_clip_top',0)-1 and b.get('independent_bottom',1)<=b.get('independent_clip_bottom',0)+1:return
 ck('scroll contextual target',L.scroll_to_frontend_widget(F,b['name']))
def context_click(caption,required):
 b=context_target(caption,required);ck('real contextual Slate '+caption,L.click_rollout_button(F,unreal.find_object(None,b['object'])))
def building_terms():
 row=next(x for x in state()['catalog']['rows'] if x['building_id']=='BUILD_01' and x['level']==1)
 return [row['name'],'可查看目标等级 1']
def recipe_terms():
 row=next(x for x in state()['catalog']['rows'] if x['recipe_id']==FIXTURE['recipe'])
 return [row.get('name',row['recipe_id'])]
def mark_build_confirmation():
 SELECTED.update(build_count=count('Build'),version=state()['version']);b=find('确认建设');SELECTED['old_build_button']=unreal.find_object(None,b['object'])
def stale_build_rejected():
 ck('version drift removes stale build confirmation',not [b for b in snap()['buttons'] if b.get('visible') and b.get('label')=='确认建设'])
 ck('stale detached button has no Slate hit path',not L.click_rollout_button(F,SELECTED['old_build_button']))
 ck('no build dispatched from stale confirmation',count('Build')==SELECTED['build_count'])
def select_seller():
 rows=[x for x in state()['catalog']['rows'] if not x.get('is_own') and x.get('status')=='Listed']
 ck('actual seller listing exists',bool(rows),rows);SELECTED['seller']=rows[0]['id'];SELECTED['seller_price']=rows[0]['unit_price'];SELECTED['seller_count']=rows[0]['count'];SELECTED['seller_item']=rows[0]['item_id']
def baseline_buy():SELECTED.update(stone_before=state()['wallet'].get('CUR_STONE',0),items_before=state()['inventory'].get(SELECTED['seller_item'],0))
def buy_transfer():
 success('BuyListing');ck('actual listing item transfer',state()['inventory'].get(SELECTED['seller_item'],0)==SELECTED['items_before']+SELECTED['seller_count']);ck('actual listing price paid',state()['wallet'].get('CUR_STONE',0)==SELECTED['stone_before']-SELECTED['seller_count']*SELECTED['seller_price'])
def input_target(hint):
 rows=[x for x in snap()['inputs'] if x.get('visible') and x.get('hint')==hint];ck('one real input '+hint,len(rows)==1,rows);return rows[0]
def reveal_input(hint):
 b=input_target(hint)
 if b.get('paint_expectation')!='expected':ck('scroll actual input '+hint,L.scroll_to_frontend_widget(F,b['name']))
def type_hint(hint,value):type_text(input_target(hint)['name'],value)
def own_listing():
 rows=[x for x in state()['market']['own_listings'] if x['status']=='Listed'];ck('actual new own listing',len(rows)==1,rows);SELECTED['own_listing']=rows[0]['id']
def catalog_is(kind):return idle() and state().get('catalog',{}).get('kind')==kind
import re
SCENARIO=re.search(r'-NewUIStageScenario=(cancel|commit|rework)\b',unreal.SystemLibrary.get_command_line()).group(1)
ATTEMPT=re.search(r'-NewUIStageAttempt=(\d+)\b',unreal.SystemLibrary.get_command_line()).group(1)
OUT=HERE/('stage-pie-'+SCENARIO+'-'+ATTEMPT);OUT.mkdir(exist_ok=False)
ACTORS=json.loads((HERE/'service-stage-fixtures.json').read_text())['actors']
ACTOR=next(x for x in ACTORS if x['user']==('newui-stage-cancel' if SCENARIO=='cancel' else 'newui-stage-rework'))
MATERIAL='MAT_BR04_C01_G1' if SCENARIO=='cancel' else 'MAT_BR01_C01_G2' if SCENARIO=='rework' else 'MAT_BR01_C01_G1'
OP='ReworkQuality' if SCENARIO=='rework' else 'PrepareBreakthrough'
R['scope']='Actual native stage '+SCENARIO+'; separate declared synthetic stage actor, real quote/prepare/cancel/rework through Slate; no earned-progression claim'
R['declared_stage_actor']={k:ACTOR[k] for k in ('app','room','user','stage','items')}
def material_terms():return [inventory_entry(MATERIAL)['name']+' ×']
def quote_ready():
 q=state().get('breakthrough_quote') or {}
 return idle() and (q.get('stage') or {}).get('stage_id')==ACTOR['stage']['stage_id'] and ((q.get('kind')=='rework') if SCENARIO=='rework' else 'kind' not in q) and any(x.get('visible') and x.get('label')=='申请突破' and x.get('enabled') for x in snap()['buttons'])
def quote_contract():
 q=state()['breakthrough_quote'];SELECTED.update(material_before=state()['inventory'].get(MATERIAL,0),command_count=count(OP),tuple=[q['stage'][k] for k in ('stage_id','from_realm_index','to_realm_index')])
 ck('actual optional quote display fields are absent',all(k not in q for k in ('q_normalized','growth_min','growth_max','sources','slots','readiness')),q)
 ck('missing optional display fields do not disable native request',find('申请突破').get('enabled') is True)
 ck('actual quote echoes one selected actual material',q['selection']==[dict(item_id=MATERIAL,count=1)],q['selection'])
def prepared():
 if not idle() or count(OP)<=SELECTED['command_count']:return False
 if SCENARIO=='rework':return True
 t=state().get('tribulation') or {}
 return t.get('stage')==ACTOR['stage']['stage_id'] and t.get('status')==('Prepare' if SCENARIO=='cancel' else 'Succeeded')
def verify_prepare():
 success(OP)
 if SCENARIO=='cancel':ck('actual core material reserved only after receipt',state()['inventory'].get(MATERIAL,0)==SELECTED['material_before']-1)
 elif SCENARIO=='commit':ck('native non-tribulation transition committed target realm',state()['growth']['realm_index']==ACTOR['stage']['to_realm_index'])
step('actual StartGame',lambda:click('StartGame'),.2)
wait('connected separate declared stage authority',lambda:idle() and state().get('identity',{}).get('user')==ACTOR['user'])
step('actual stage page route',lambda:route('BreakthroughPrepare'))
wait('actual held-item catalog ready',lambda:catalog_is('items'))
step('initial stage data',lambda:capture('initial-stage'))
step('reveal real stage core material',lambda:context_reveal('加入材料选择',material_terms()),.5)
step('select one actual core material',lambda:context_click('加入材料选择',material_terms()),.5)
if SCENARIO=='rework':
 step('reveal rework mode',lambda:reveal('重整品质 · 关'),.5)
 step('actual rework mode switch',lambda:ordinary('重整品质 · 关'),.4)
step('reveal native quote',lambda:reveal('获取实际报价'),.5)
step('request real authority quote',lambda:ordinary('获取实际报价'),.1)
wait('actual quote AND actual presented view have refreshed',quote_ready,15)
step('actual native quote contract and submit eligibility',quote_contract,.1)
step('actual quote rendered',lambda:capture('actual-native-quote'))
step('reveal first stage confirmation',lambda:reveal('申请突破'),.5)
step('actual request confirmation boundary',lambda:ordinary('申请突破'),.4)
step('reveal actual second stage confirmation',lambda:reveal('确认重整' if SCENARIO=='rework' else '确认申请'),.5)
step('actual confirmed stage and selection rendered',lambda:capture('stage-confirmation'),.1)
step('actual final prepare/rework boundary',lambda:ordinary('确认重整' if SCENARIO=='rework' else '确认申请'),.1)
wait('actual native stage command receipt',prepared,30)
step('verify actual prepare/rework receipt',verify_prepare,.1)
if SCENARIO=='cancel':
 step('real active tribulation page',lambda:route('Tribulation'),.3)
 step('reveal actual cancel preparation',lambda:reveal('申请取消准备'),.3)
 step('actual cancel first confirmation',lambda:ordinary('申请取消准备'),.3)
 step('reveal actual cancel second confirmation',lambda:reveal('确认取消'),.3)
 step('actual cancel second confirmation',lambda:ordinary('确认取消'),.1)
 wait('actual native cancellation receipt',lambda:idle() and (state().get('tribulation') or {}).get('status')=='Cancelled',10)
 step('native cancellation success',lambda:success('CancelBreakthrough'),.1)
 step('actual reserved material returned',lambda:ck('actual available material restored',state()['inventory'].get(MATERIAL,0)==SELECTED['material_before']),.1)
step('actual stage result data',lambda:capture('actual-stage-result'))

def finish():
 unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play()
 def quit_tick(delta):
  if unreal.EditorLevelLibrary.get_pie_worlds(False):return
  unreal.unregister_slate_post_tick_callback(QUIT[0]);gc.collect()
  dirty=list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())+list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
  R['dirty_packages_after_pie']=[p.get_name() for p in dirty];R['finished_at']=datetime.now(timezone.utc).isoformat();save()
  if not dirty:unreal.SystemLibrary.quit_editor()
 QUIT[0]=unreal.register_slate_post_tick_callback(quit_tick)
def tick(delta):
 global I,NEXT,F,S,W
 if time.monotonic()<NEXT:return
 try:
  if time.monotonic()-START>300:raise TimeoutError('Service PIE budget300s')
  if I==-2:
   ck('isolated Saved directory',Path(unreal.Paths.project_saved_dir()).resolve().is_relative_to(HERE));ck('begin actual default PIE',L.begin_frontend_pie(720,1280));I=-1;NEXT=time.monotonic()+12;return
  if I==-1:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   if not worlds:return
   W=worlds[0];directors=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)
   if not directors:return
   F=directors[0].get_editor_property('frontend_widget')
   if not F:return
   gi=unreal.GameplayStatics.get_game_instance(W);S=next(x for x in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if x.get_outer()==gi)
   ck('actual new default class',F.get_class().get_path_name()=='/Game/newUI/Runtime/WBP_NewUIFrontend.WBP_NewUIFrontend_C');I=0;NEXT=time.monotonic()+2;return
  if I>=len(STEPS):R['finished']=True;save();finish();return
  name,action,delay=STEPS[I];R['current_step']=name;result=action()
  if result is not False:I+=1
  NEXT=time.monotonic()+delay;save()
 except Exception:
  R['error']=traceback.format_exc();save()
  try:
   if F and S:capture('failure-current')
  except Exception:R['failure_capture_error']=traceback.format_exc();save()
  finish()
HANDLE=unreal.register_slate_post_tick_callback(tick)
