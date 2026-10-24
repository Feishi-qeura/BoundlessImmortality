"""Actual default PIE and native HTTP service, declared isolated fixtures only."""
from pathlib import Path
import gc,json,time,traceback,os,hashlib
from datetime import datetime,timezone
import unreal
HERE=Path(__file__).resolve().parent
OUT=HERE/'service-pie-07';OUT.mkdir(exist_ok=True)
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
 if b.get('paint_expectation')=='expected':return
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
step('actual StartGame',lambda:click('StartGame'),.2)
wait('connected retained isolated authority',lambda:idle() and bool(state().get('guild')))
step('real building route',lambda:route('GuildConstruction'))
wait('native building catalog ready',lambda:catalog_is('buildings'))
step('building actual current data',lambda:capture('building-catalog'))
step('select exact actual BUILD_01 target1 confirmation',lambda:context_click('建设',building_terms()),.5)
step('capture actual build confirmation version',mark_build_confirmation,.1)
step('setup actual native catalog version drift',lambda:native_query('buildings'),.1)
wait('actual changed building version',lambda:catalog_is('buildings') and state()['version']!=SELECTED['version'])
step('stale build cannot dispatch',stale_build_rejected,.2)
step('reopen fresh actual build confirmation',lambda:context_click('建设',building_terms()),.5)
step('fresh native build second confirmation',lambda:ordinary('确认建设'),.1)
wait('actual build receipt',lambda:idle() and count('Build')>SELECTED['build_count'])
step('native build success',lambda:success('Build'))
step('actual target level persisted',lambda:ck('actual BUILD_01 level1 state',any(x.get('building_id')=='BUILD_01' and x.get('level')==1 for x in state().get('buildings',[]))))
step('actual crafting route',lambda:route('WulingCraft'))
wait('actual native recipe catalog',lambda:catalog_is('recipes'))
step('reveal exact recipe',lambda:context_reveal('查看配方',recipe_terms()),.8)
step('select actual recipe',lambda:context_click('查看配方',recipe_terms()),.8)
step('reveal real craft request',lambda:reveal('申请炼制'),.8)
step('real craft confirmation request',lambda:ordinary('申请炼制'),.5)
step('reveal craft second confirmation',lambda:reveal('确认提交'),.8)
step('real craft second confirmation',lambda:ordinary('确认提交'),.1)
wait('actual native Craft completed',lambda:idle() and count('Craft')>0)
step('native craft success',lambda:success('Craft'))
step('native craft confirmed data',lambda:capture('craft-confirmed'))
step('actual market route',lambda:route('Market'))
wait('actual market native catalog',lambda:catalog_is('market'))
step('capture actual seller listing identity',select_seller,.1)
step('reveal actual seller listing',lambda:reveal_entry(SELECTED['seller']),.8)
step('select actual seller listing',lambda:click_entry(SELECTED['seller']),.8)
step('baseline confirmed wallet inventory',baseline_buy,.1)
step('reveal actual market buy',lambda:reveal('确认购买'),.8)
step('market first confirmation',lambda:ordinary('确认购买'),.5)
step('market second confirmation',lambda:ordinary('再次确认购买'),.1)
wait('actual native buy completed',lambda:idle() and count('BuyListing')>0)
step('actual price/item transfer',buy_transfer)
step('market owned query menu',lambda:click('Ext_ToggleRelated'),.4)
step('actual owned-items query entry',lambda:click('Ext_QueryItems'),.1)
wait('actual owned items catalog',lambda:catalog_is('items'))
step('reveal actual tradeable held item',lambda:reveal_entry('MAT_HERB_T'),.8)
step('select actual held item identity',lambda:click_entry('MAT_HERB_T'),.8)
step('reveal quantity input',lambda:reveal_input('数量'),.8)
step('real requested quantity1',lambda:type_hint('数量','1'),.4)
step('reveal real price input',lambda:reveal_input('输入单价'),.8)
step('real requested known seller unit price',lambda:type_hint('输入单价',str(SELECTED['seller_price'])),.6)
step('draft survives native refresh',lambda:ck('actual user draft retained',input_target('数量')['text']=='1' and input_target('输入单价')['text']==str(SELECTED['seller_price'])))
step('reveal actual publish',lambda:reveal('发布寄售'),.8)
step('first publish confirmation',lambda:ordinary('发布寄售'),.5)
step('second publish confirmation',lambda:ordinary('再次确认发布'),.1)
wait('actual native listing created',lambda:idle() and count('CreateListing')>0)
step('native listing create success',lambda:success('CreateListing'))
step('capture actual own listing identity',own_listing,.2)
step('actual orders route',lambda:route('TradeOrders'))
step('reveal actual own listing',lambda:reveal_entry(SELECTED['own_listing']),.8)
step('select actual own listing',lambda:click_entry(SELECTED['own_listing']),.8)
step('reveal actual withdraw',lambda:reveal('撤销寄售'),.8)
step('first withdrawal confirmation',lambda:ordinary('撤销寄售'),.5)
step('second withdrawal confirmation',lambda:ordinary('再次确认撤销'),.1)
wait('actual native listing withdrawn',lambda:idle() and count('CancelListing')>0)
step('native withdrawal success',lambda:success('CancelListing'))
step('orders actual service data',lambda:capture('orders-withdrawn'))

AVATAR={}
def avatar_requests():
 p=HERE/'avatar-tls-fixture-02/requests.jsonl'
 return [json.loads(x) for x in p.read_text().splitlines()] if p.exists() else []
def avatar_fixture():
 pc=unreal.GameplayStatics.get_player_controller(W,0)
 director=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)[0];enabled=director.is_actor_tick_enabled();director.set_actor_tick_enabled(False);clone=F
 AVATAR.update(director=director,tick_enabled=enabled,original_rows=list(F.get_editor_property('leaderboard_entries')))
 entry=unreal.InkLeaderboardEntry();entry.participant_id='declared-avatar-test-subject';entry.display_name=unreal.Text('头像传输测试样本');entry.category='Anchor';entry.rank=1;entry.metric=unreal.Text('声明的显示测试样本');entry.avatar_url=json.loads((HERE/'avatar-service-ready-02.json').read_text())['url']
 clone.set_leaderboard_entries_for_scope([entry],'Local');clone.show_page('Leaderboard')
 AVATAR.update(clone=clone,entry=entry)
 R['avatar_fixture_scope']='Actual default root with Director tick temporarily paused and declared synthetic avatar transport row; display-only fixture separate from authority/player-entry proof. Original rows and tick state restored. HTTPS loopback self-signed fixture uses only process-local peer setting.'
def avatar_retry_latch_reset():
 entry=AVATAR['entry'];entry.metric=unreal.Text('相同身份和 URL，更新显示测试样本');AVATAR['clone'].set_leaderboard_entries_for_scope([entry],'Local');AVATAR['refreshed_at']=time.monotonic()
def avatar_backoff_check():
 rows=avatar_requests();ck('one failed actual HTTPS request during host backoff',len(rows)==1 and rows[0]['status']==503,rows)
 ck('no pending HTTP request retained after expiry',not AVATAR['clone'].get_editor_property('new_rank_requests'))
def avatar_texture():
 board=AVATAR['clone'].get_editor_property('new_rank_board');images=board.get_editor_property('avatar_images_by_participant') if board else {}
 image=images.get('declared-avatar-test-subject')
 return image.get_editor_property('brush').get_editor_property('resource_object') if image else None
def avatar_success():
 rows=avatar_requests();ck('actual HTTPS recovery after failure',len(rows)==2 and rows[-1]['status']==200,rows)
 ck('actual host60second retry backoff',rows[1]['monotonic']-rows[0]['monotonic']>=59,rows)
 texture=avatar_texture();ck('decoded actual HTTPS pixels applied to real participant image',texture is not None,texture.get_path_name() if texture else None)
 R['avatar_https_recovery']=dict(requests=rows,texture=texture.get_path_name())
 data=json.loads(L.describe_widget(AVATAR['clone']));(OUT/'avatar-recovered.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');ck('avatar recovery render',L.capture_frontend_viewport(str(OUT/'avatar-recovered.png')));AVATAR['clone'].set_leaderboard_entries_for_scope(AVATAR['original_rows'],'Local');AVATAR['director'].set_actor_tick_enabled(AVATAR['tick_enabled']);AVATAR['clone'].show_page('HUD')
step('declared avatar transport regression on actual default root',avatar_fixture,.2)
wait('actual first HTTPS avatar failure',lambda:bool(avatar_requests()),30)
wait('actual request expires and releases host slot',lambda:not AVATAR['clone'].get_editor_property('new_rank_requests'),25)
step('force same subject display refresh during real backoff',avatar_retry_latch_reset,2)
step('real host backoff rejects request and releases page latch',avatar_backoff_check,2)
wait('actual retry eventually fills participant avatar',lambda:avatar_texture() is not None,100)
step('assert actual HTTPS retry and image readback',avatar_success,.2)


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
  if time.monotonic()-START>900:raise TimeoutError('Service PIE budget900s')
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
