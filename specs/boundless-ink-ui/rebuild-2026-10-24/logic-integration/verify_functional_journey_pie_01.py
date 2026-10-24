"""Actual default player startup and Slate actions; isolated process only."""
from pathlib import Path
import gc, json, time, traceback, os, hashlib, re
from datetime import datetime, timezone
import unreal
import runpy
METRICS=runpy.run_path(str(Path(__file__).resolve().parent/'geometry_metric_validation.py'),run_name='geometry_metric_validation')

HERE=Path(__file__).resolve().parent
ATTEMPT=re.search(r'-NewUIDefaultPIEAttempt=(\d+)\b',unreal.SystemLibrary.get_command_line())
OUT=HERE/'functional-journey-pie-01'
OUT.mkdir(exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
LIB=unreal.InkFrontendEditorLibrary
REPORT=dict(scope='Actual default player startup, no direct component construction', checks=[], captures=[], finished=False)
REPORT.update(target_orientation='portrait_only',target_aspect_ratio='9:16',target_platform='mobile_live_interaction',device_platform_acceptance=False,engine_pid=os.getpid(),started_at=datetime.now(timezone.utc).isoformat(),source_epoch_sha256=hashlib.sha256((HERE/'source-epoch.json').read_bytes()).hexdigest(),visual_approval='pending_user')
FRONTEND=None
SOURCE_SERVICE=None
LAYOUT_STABILITY={}
STATE=dict(index=-2, next=0., start=time.monotonic())
QUIT=[None]

def save():
    REPORT['passed']=REPORT['finished'] and not REPORT.get('error') and all(x['passed'] for x in REPORT['checks'])
    (OUT/'report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')

def check(name,passed,actual=None):
    REPORT['checks'].append(dict(name=name,passed=bool(passed),actual=actual))
    save()
    if not passed: raise AssertionError(name+': '+str(actual))

def describe():
    raw=LIB.describe_widget(FRONTEND)
    (OUT/('raw-snapshot-'+str(STATE['index'])+'.txt')).write_text(raw,encoding='utf-8')
    def reject_constant(value):raise ValueError("Non-standard JSON numeric constant: "+value)
    return json.loads(raw,parse_constant=reject_constant)

def settled(snapshot,key):
    # This waits for the same complete widget generation across two frames.
    # It never examines finite/paint/clip values and retains every record.
    topology=[(kind,x.get('object_id'),x.get('object')) for kind in METRICS['KINDS'] for x in snapshot.get(kind,[])]
    fingerprint=hashlib.sha256(json.dumps(topology,separators=(',',':')).encode()).hexdigest()
    previous=LAYOUT_STABILITY.get(key);now=time.monotonic()
    if not previous or previous['fingerprint']!=fingerprint:
        count=previous['samples']+1 if previous else 1
        LAYOUT_STABILITY[key]=dict(fingerprint=fingerprint,observed_at=now,samples=count)
        name='pre-layout-generation-'+str(STATE['index'])+'-'+str(count)+'.json'
        (OUT/name).write_text(json.dumps(snapshot,ensure_ascii=False,indent=2),encoding='utf-8')
        REPORT.setdefault('layout_generation_waits',[]).append(dict(step=STATE['index'],key=key,snapshot=name,fingerprint=fingerprint,reason='new widget generation has not yet completed a frame; no geometry-value filter'))
        save();return False
    return now-previous['observed_at']>=.04

def capture(page,tag=None):
    snapshot=describe()
    if not settled(snapshot,'capture-'+str(STATE['index'])):return False
    check('actual page '+page,snapshot.get('page')==page,snapshot.get('page'))
    check('independent root '+page,snapshot.get('root_widget')=='NewUIRoot_r1',snapshot.get('root_widget'))
    check('new root runtime version '+page,snapshot.get('runtime_layout_version')=='newui-r1-root-20261024',snapshot.get('runtime_layout_version'))
    check('actual new WBP '+page,snapshot.get('runtime_class')=='/Game/newUI/Runtime/WBP_NewUIFrontend.WBP_NewUIFrontend_C',snapshot.get('runtime_class'))
    version='newui-r1-rank-leaderboard' if page=='Leaderboard' else ('newui-r1-player-' if page in ('Main','HUD','Saves','Settings','Character','Inventory') else 'newui-r1-feature-'+page.lower())
    check('owner page version '+page,str(snapshot.get('page_layout_version','')).startswith(version),snapshot.get('page_layout_version'))
    if version.startswith('newui-r1-feature-'):
        check('actual new feature host '+page,snapshot.get('extension_runtime_class')=='/Script/BoundlessImmortality.InkRebuildFeatureHost',snapshot.get('extension_runtime_class'))
        check('independent feature root '+page,snapshot.get('extension_root_widget')=='NewFeatureRoot_r1',snapshot.get('extension_root_widget'))
        check('actual owner composition '+page,bool(snapshot.get('feature_composition')),snapshot.get('feature_composition'))
    if page in ('DailyActivities','TimedActivities'):
        check('actual route-specific activity cycle '+page,snapshot.get('activity_cycle')==('daily' if page=='DailyActivities' else 'limited'),snapshot.get('activity_cycle'))
    visible_texts=[x for x in snapshot.get('texts',[]) if x.get('visible') and x.get('text')]
    check('real Chinese fonts in visible text '+page,bool(visible_texts) and all(str(x.get('font','')).startswith('/Game/newUI/Ink/Fonts/Shared/') for x in visible_texts),[dict(name=x.get('name'),font=x.get('font')) for x in visible_texts])
    expected={'Main':'T_MainActionOrnament_v1','HUD':'T_HudRouteGlyphAtlas_v1','Character':'T_CharacterAttributeGlyphAtlas_v1','Inventory':'T_InventoryCategoryGlyphAtlas_v1'}.get(page)
    if expected and tag is None:
        resource='/Game/newUI/Components/Player/'+expected+'.'+expected
        check('actual component texture bound '+page,any(x.get('visible') and x.get('resource')==resource for x in snapshot.get('images',[])),resource)
    resources={x.get('resource') for x in snapshot.get('images',[]) if x.get('visible')}
    component_paths={
      'Wuling':['/Game/newUI/Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1'],
      'Activities':['/Game/newUI/Activities/Icons/T_DailyActivityScroll_v1','/Game/newUI/Activities/Icons/T_LimitedActivityCalendar_v1','/Game/newUI/Activities/Icons/T_PermanentActivityLotus_v1'],
      'DailyActivities':['/Game/newUI/Activities/Icons/T_DailyActivityScroll_v1'],
      'TimedActivities':['/Game/newUI/Activities/Icons/T_LimitedActivityCalendar_v1'],
      'CharacterAppearance':['/Game/newUI/Components/Player/T_AppearancePreviewOrnamentAtlas_v1'],
    }
    for path in component_paths.get(page,[]):
        resource=path+'.'+path.rsplit('/',1)[-1]
        check('actual imported component UImage resource '+page+' '+resource,resource in resources,sorted(str(x) for x in resources))
    if page in ('WorldCultivation','ResourceContest') and SOURCE_SERVICE and SOURCE_SERVICE.is_connected():
        authority=json.loads(SOURCE_SERVICE.get_snapshot_json());journey=authority.get('journey') or {};rows=journey.get('worlds' if page=='WorldCultivation' else 'resources') or []
        positioned=[x for x in rows if isinstance(x.get('x'),(int,float)) and isinstance(x.get('y'),(int,float)) and 0<=x['x']<=1 and 0<=x['y']<=1]
        if positioned:
            pins=[x for x in snapshot['buttons'] if x.get('visible') and x.get('name','').startswith('JourneyPinHitButton') and '.InkJourneyMapPinButton_' in x.get('object','')]
            check('actual provided journey x-y coordinates create native pins '+page,bool(pins),dict(authoritative_positioned_entries=len(positioned),actual_pin_buttons=len(pins)))
    if page=='HUD':
        for field in ('legacy_weapon_hud_suppression_condition','legacy_gm_anchor_suppression_condition','legacy_status_adapter_collapsed'):
            check('actual legacy suppression '+field+' '+(tag or page),snapshot.get(field) is True,snapshot.get(field))
    tag=tag or page.lower()
    LIB.move_frontend_pointer_outside(FRONTEND)
    image=OUT/(tag+'.png')
    check('rendered viewport '+tag,LIB.capture_frontend_viewport(str(image)))
    (OUT/(tag+'.json')).write_text(json.dumps(snapshot,ensure_ascii=False,indent=2),encoding='utf-8')
    if SOURCE_SERVICE and SOURCE_SERVICE.is_enabled():
        authority_raw=SOURCE_SERVICE.get_snapshot_json()
        authority=json.loads(authority_raw) if authority_raw else None
        (OUT/(tag+'-authority.json')).write_text(json.dumps(authority,ensure_ascii=False,indent=2),encoding='utf-8')
        REPORT.setdefault('actual_service_capture_contexts',[]).append(dict(tag=tag,connected=SOURCE_SERVICE.is_connected(),busy=SOURCE_SERVICE.is_busy(),snapshot_present=authority is not None,version=authority.get('version') if authority else None,identity=authority.get('identity') if authority else None,declared_isolated_fixture=True))
    REPORT['captures'].append(str(image));save()
    invalid,failures=METRICS['inspect'](snapshot)
    REPORT.setdefault('nonfinite_metrics',{})[tag]=invalid;save()
    check('finite metrics or independently proven culling '+tag,not failures,failures)
    painted=next((x for kind in METRICS['KINDS'] for x in snapshot.get(kind,[]) if x.get('visible') and x.get('paint_expectation')=='expected'),None)
    check('independent expected-painted witness '+tag,painted is not None)
    injected=dict(painted,has_nonfinite_metrics=True,x=None,geometry_arranged=False)
    _,injected_failures=METRICS['inspect']({'buttons':[injected]})
    check('deliberate nonfinite expected-painted metric rejected '+tag,len(injected_failures)==1,injected_failures)

def scroll_action(action):
    snapshot=describe()
    if not settled(snapshot,'scroll-action-'+str(STATE['index'])):return False
    matches=[x for x in snapshot.get('buttons',[]) if x.get('action')==action and x.get('visible')]
    check('one real action target before scroll '+action,len(matches)==1,matches)
    REPORT.setdefault('action_scroll_before',{})[action]=matches[0];save()
    check('real scroll to action '+action,LIB.scroll_to_frontend_widget(FRONTEND,matches[0]['name']))

def click_after_scroll(action):
    snapshot=describe()
    if not settled(snapshot,'scrolled-action-'+str(STATE['index'])):return False
    matches=[x for x in snapshot.get('buttons',[]) if x.get('action')==action and x.get('visible')]
    check('one painted target after actual scroll '+action,len(matches)==1,matches)
    target=matches[0];REPORT.setdefault('action_scroll_after',{})[action]=target;save()
    check('actual scrolled target expected painted '+action,target.get('paint_expectation')=='expected' and target.get('geometry_arranged'),target)
    check('actual scrolled target inside clipping bounds '+action,target['independent_left']>=target['independent_clip_left']-1 and target['independent_top']>=target['independent_clip_top']-1 and target['independent_right']<=target['independent_clip_right']+1 and target['independent_bottom']<=target['independent_clip_bottom']+1,target)
    click(action)

def click(action):check('real Slate click '+action,LIB.click_action(FRONTEND,action))
def click_label(caption):
    snapshot=describe()
    if not settled(snapshot,'action-'+str(STATE['index'])):return False
    check('real Slate typed-button click '+caption,LIB.click_labeled_button(FRONTEND,unreal.Text(caption)))
def board(category,scope,tag):
    if capture('Leaderboard',tag) is False:return False
    snapshot=describe();check('actual board category '+tag,snapshot.get('board_category')==category,snapshot.get('board_category'))
    check('actual board scope '+tag,snapshot.get('board_scope')==scope,snapshot.get('board_scope'))
def scroll_last_inventory():
    snapshot=describe();physical=[x for x in snapshot['buttons'] if x['name']=='Rebuild_InventorySlot_39' and x.get('visible')]
    actual=[x for x in snapshot['buttons'] if x['name'].startswith('Rebuild_InventoryEntry_') and x.get('visible')]
    check('real inventory has known physical slot or actual authoritative entries',bool(physical or actual))
    target=physical[0] if physical else actual[-1]
    REPORT['inventory_scroll_target']=dict(name=target['name'],action=target.get('action'),mode='known local capacity physical slot39' if physical else 'authority capacity unprovided; last actual GUID entry')
    check('real root scroll to actual inventory target',LIB.scroll_to_frontend_widget(FRONTEND,target['name']))
def last_slot_visible():
    snapshot=describe()
    if not settled(snapshot,'last-inventory-target-'+str(STATE['index'])):return False
    target=REPORT['inventory_scroll_target'];slot=next(x for x in snapshot['buttons'] if x['name']==target['name'])
    scroll=next(x for x in snapshot['scrolls'] if x['name']=='NewUIScroll_Inventory')
    check('last actual inventory target arranged after real scroll',slot.get('geometry_arranged') and slot.get('visible'),slot)
    check('last actual inventory target inside actual scroll viewport',slot['x']>=scroll['x']-1 and slot['y']>=scroll['y']-1 and slot['x']+slot['width']<=scroll['x']+scroll['width']+1 and slot['y']+slot['height']<=scroll['y']+scroll['height']+1,dict(slot=slot,scroll=scroll))


STEPS=[]
REPORT['scope']='Lightweight actual UI entry supplement to formal14 exact9:16 rendering. All navigation uses present UI controls and real Slate hit paths. Related-page view navigation does not establish a business settlement or result trigger.'
REPORT['edges']=[]
REPORT['continues_actual_ui_entry03']=True
REPORT['scope']='Actual portrait UI LaunchStart, exact selected resource detail/queue/cancel, actual matched race, truthful active/missing/inactive refusals and six real native common actions/settlement. Isolated declared actors/test clock, real rules unchanged.'
REPORT['authority_provider_sha256']=hashlib.sha256((HERE.parents[3]/'Tools/CultivationService/authority.py').read_bytes()).hexdigest()
PENDING={}
def pick(spec):
    data=describe()
    matches=[]
    for b in data['buttons']:
        if not b.get('visible') or not b.get('enabled'):continue
        actual=dict(b)
        if 'label' in spec and actual.get('label') is None:
            labels=[]
            def gather(w):
                if isinstance(w,unreal.TextBlock):labels.append(str(w.get_text()))
                elif isinstance(w,unreal.PanelWidget):
                    for i in range(w.get_children_count()):gather(w.get_child_at(i))
            gather(unreal.find_object(None,b['object']));actual['label']='\n'.join(labels)
        if all(actual.get(k)==v for k,v in spec.items()):matches.append(actual)
    raw_count=len(matches);matches=list({b['object']:b for b in matches}.values())
    REPORT.setdefault('navigation_alias_readbacks',[]).append(dict(spec=spec,raw_count=raw_count,unique_object_count=len(matches)))
    check('one actual enabled UI subject '+str(spec),len(matches)==1,matches)
    return matches[0]
def reveal(spec):
    data=describe()
    if not settled(data,'entry-reveal-'+str(STATE['index'])):return False
    b=pick(spec)
    inside=b.get('paint_expectation')=='expected' and b.get('geometry_arranged') and all(b['independent_'+e]>=b['independent_clip_'+e]-1 for e in ('left','top')) and all(b['independent_'+e]<=b['independent_clip_'+e]+1 for e in ('right','bottom'))
    if not inside:check('actual navigation scroll '+str(spec),LIB.scroll_to_frontend_widget(FRONTEND,b['name']))
def hit(spec):
    data=describe()
    if not settled(data,'entry-hit-'+str(STATE['index'])):return False
    b=pick(spec);check('navigation target expected painted '+str(spec),b.get('paint_expectation')=='expected' and b.get('geometry_arranged'),b)
    PENDING.clear();PENDING.update(from_page=data['page'],subject=b)
    check('physical Slate navigation '+str(spec),LIB.click_rollout_button(FRONTEND,unreal.find_object(None,b['object'])))
def observed(target,context=None):
    data=describe()
    if not settled(data,'entry-result-'+str(STATE['index'])):return False
    check('actual clicked destination '+target,data.get('page')==target,data.get('page'))
    check('actual navigation exact9:16',data['width']==720 and data['height']==1280,[data['width'],data['height']])
    active=FRONTEND.get_editor_property('extension_pages')
    host=next((v for k,v in active.items() if str(k)==target),None)
    REPORT.setdefault('context_inspection',[]).append(dict(target=target,active_host=host.get_path_name() if host else None,retired_and_current_candidates=[dict(object=x.get_path_name(),selected=x.get_editor_property('selected_entry')) for x in unreal.ObjectIterator(unreal.InkRebuildFeatureHost) if x.get_world()==FRONTEND.get_world() and str(x.get_editor_property('page_id'))==target]))
    selected=host.get_editor_property('selected_entry') if host else None
    if context is not None:check('actual selected context '+target,selected==context,selected)
    edge=dict(PENDING,to_page=target,actual_selected_context=selected,view_navigation_only=target in ('EventSettlement','RaceSettlement','ResourceRewards'),business_result_trigger_proved=False)
    REPORT['edges'].append(edge)
    (OUT/('entry-'+str(len(REPORT['edges']))+'-'+target+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');save()
def edge(spec,target,context=None):
    STEPS.extend([('reveal '+str(spec),lambda s=spec:reveal(s),.25),('hit '+str(spec),lambda s=spec:hit(s),.55),('destination '+target,lambda t=target,c=context:observed(t,c),.1)])
def act(action,target):edge({'action':action},target)
def label_edge(label,target,context=None):edge({'label':label},target,context)
def back(target):label_edge('返回',target)
def related(route,parent):
    act('Ext_ToggleRelated',parent);act('Ext_Open_'+route,route);back(parent)
def wuling_open():
    act('ToggleHUDMenu','HUD');act('OpenWuling','Wuling')

def state():
    raw=SOURCE_SERVICE.get_snapshot_json();return json.loads(raw) if raw else {}
def idle():return SOURCE_SERVICE.is_connected() and not SOURCE_SERVICE.is_busy()
def trace():return [json.loads(x) for x in (HERE/'functional-service-trace-04.jsonl').read_text().splitlines()]
def count(op):return sum(x.get('event')=='command_received' and x.get('request',{}).get('command')==op for x in trace())
def successful(op):return any(x.get('event')=='command_result' and x.get('command')==op and x.get('status')=='success' for x in trace())
def step(name,fn,delay=.4):STEPS.append((name,fn,delay))
def wait(name,fn):
    def action():
        if not fn():return False
        check(name,True);return True
    step(name,action,.1)
def control(**values):
    p=HERE/'functional-service-control-04.json';d=json.loads(p.read_text()) if p.exists() else {};d.update(values);p.write_text(json.dumps(d),encoding='utf-8')
def advance(seq,seconds):control(advance_sequence=seq,advance_seconds=seconds)
def advanced(seq):
    p=HERE/'functional-clock-state-04.json';return p.exists() and json.loads(p.read_text()).get('sequence')==seq and idle()
SUB={}
def remember_resource():
    row=next(x for x in state()['journey']['resources'] if x['node_id']=='NODE_STONE');SUB.update(row);REPORT['actual_resource_subject']=row

def select_actual_resource():
    spec={'name':'JourneyPinHitButton','label':SUB['name']}
    step('scroll actual provided resource pin',lambda:reveal(spec),.4)
    step('physical provided resource pin',lambda:hit(spec),.5)
    step('selected resource exact existing ID',lambda:observed('ResourceContest',SUB['id']),.1)
def selected_detail():
    act('Ext_ToggleRelated','ResourceContest');act('Ext_Open_ResourceDetail','ResourceDetail')
    step('detail exact selected native resource ID',lambda:observed('ResourceDetail',SUB['id']),.2)
def guarded_zero_write(tag):
    rows=[b for b in describe()['buttons'] if b.get('visible') and b.get('name','').startswith('JourneyRaceActionHit')]
    if not rows:rows=[b for b in describe()['buttons'] if b.get('visible') and '.InkJourneyRaceActionButton_' in b.get('object','')]
    check('actual action rows exist '+tag,len(rows)>=4,rows)
    check('unknown or inactive actual action controls disabled '+tag,all(not b.get('enabled') for b in rows),rows)
    for b in rows:check('disabled physical Slate refusal '+tag,not LIB.click_rollout_button(FRONTEND,unreal.find_object(None,b['object'])))
    check('zero actual race command '+tag,count('SubmitRaceAction')==SUB['baseline'])
def remember_race():
    SUB['race_id']=state()['race']['id'];SUB['baseline']=count('SubmitRaceAction');REPORT['actual_race_subject']=state()['race']
def race_enabled(kind):
    return idle() and any(b.get('visible') and b.get('enabled') and b.get('label')==kind for b in describe()['buttons'])
STEPS.append(('exact portrait calibration',lambda:check('exact initial9:16',LIB.set_pie_viewport_size(720,1280)),2))
act('StartGame','HUD');wait('native real service connected',idle)
act('ToggleHUDMenu','HUD');act('OpenActivities','Activities');act('Ext_ToggleRelated','Activities');act('Ext_Open_LaunchEvent','LaunchEvent')
edge({'label':'开启活动'},'LaunchEvent');edge({'label':'再次确认开启活动'},'LaunchEvent')
wait('actual eligible StartLaunchEvent success and persisted Active',lambda:idle() and successful('StartLaunchEvent') and (state().get('launch') or {}).get('status')=='Active')
step('launch actual rendered receipt',lambda:capture('LaunchEvent','actual-start-launch'))
back('Activities');back('HUD');act('ToggleHUDMenu','HUD');act('OpenExtensions','Extensions');label_edge('进入资源地图','ResourceContest')
step('remember actual provided resource entry',remember_resource)
# The selected ID is taken from current native snapshot; no synthetic route ID.
step('reveal actual node label',lambda:reveal({'name':'JourneyPinHitButton','label':SUB['name']}),.4)
step('select actual native resource node',lambda:hit({'name':'JourneyPinHitButton','label':SUB['name']}),.5)
step('resource host retained exact actual ID',lambda:observed('ResourceContest',SUB['id']))
selected_detail();edge({'label':'继续'},'ResourceDetail');edge({'label':'确认继续'},'ResourceDetail')
wait('actual QueueRace queued receipt',lambda:idle() and successful('QueueRace') and (state().get('race_queue') or {}).get('status')=='Queued')
step('resource detail exact queue subject rendered',lambda:capture('ResourceDetail','resource-selected-queued'))
back('ResourceContest');act('Ext_ToggleRelated','ResourceContest');act('Ext_Open_RaceLobby','RaceLobby');edge({'label':'取消排队'},'RaceLobby')
wait('actual CancelRaceQueue receipt and Cancelled',lambda:idle() and successful('CancelRaceQueue') and (state().get('race_queue') or {}).get('status')=='Cancelled')
step('actual cancellation rendered',lambda:capture('RaceLobby','actual-queue-cancel'))
back('ResourceContest');selected_detail();edge({'label':'继续'},'ResourceDetail');edge({'label':'确认继续'},'ResourceDetail')
wait('second actual queue live',lambda:idle() and (state().get('race_queue') or {}).get('status')=='Queued')
step('real opponent existing-session queue',lambda:control(queue_opponent=True))
wait('actual service created matched race Ready',lambda:idle() and (state().get('race') or {}).get('status')=='Ready')
step('advance real rules past preparation',lambda:advance(1,121));wait('controlled clock121 applied',lambda:advanced(1))
wait('actual race Running and identity active exported true',lambda:idle() and (state().get('race') or {}).get('status')=='Running' and state().get('identity',{}).get('active') is True)
back('ResourceContest');act('Ext_ToggleRelated','ResourceContest');act('Ext_Open_RaceLobby','RaceLobby');act('Ext_ToggleRelated','RaceLobby');act('Ext_Open_RaceLive','RaceLive')
step('remember exact actual race id',remember_race)
step('missing identity.active transport regression only',lambda:control(omit_identity_active=True))
wait('native missing identity.active observed',lambda:idle() and 'active' not in state().get('identity',{}))
step('missing provider refuses physical action',lambda:guarded_zero_write('missing provider'))
step('restore actual provider',lambda:control(omit_identity_active=False))
wait('actual native active projection restored',lambda:idle() and state().get('identity',{}).get('active') is True)
step('native subsystem truthful inactive sync',lambda:SOURCE_SERVICE.set_gameplay_active(False))
wait('native actual inactive session received',lambda:idle() and state().get('identity',{}).get('active') is False)
step('inactive provider refuses physical action',lambda:guarded_zero_write('actual inactive'))
step('native participation restored',lambda:SOURCE_SERVICE.set_gameplay_active(True));wait('eligible active guard enabled',lambda:race_enabled('护阵'))
edge({'label':'护阵'},'RaceLive');wait('first actual native race action committed',lambda:idle() and count('SubmitRaceAction')==SUB['baseline']+1 and successful('SubmitRaceAction'))
step('actual eligible race action rendered',lambda:capture('RaceLive','actual-race-active-success'))
for i,caption in enumerate(['破障','护阵','破障','护阵','破障'],2):
    step('advance actual common slot'+str(i),lambda n=i:advance(n,60));wait('clock slot applied'+str(i),lambda n=i:advanced(n));wait('actual slot control enabled'+str(i),lambda c=caption:race_enabled(c));edge({'label':caption},'RaceLive');wait('actual common response committed'+str(i),lambda n=i:idle() and count('SubmitRaceAction')==SUB['baseline']+n)
step('advance exact backend settlement boundary',lambda:advance(7,181));wait('actual settlement clock applied',lambda:advanced(7))
wait('actual same race settled',lambda:idle() and (state().get('race') or {}).get('id')==SUB['race_id'] and (state().get('race') or {}).get('status')=='Settled')
step('actual settled receipt state',lambda:capture('RaceLive','actual-race-settled'))

def finish():
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play()
    def quit_tick(delta):
        if unreal.EditorLevelLibrary.get_pie_worlds(False):return
        unreal.unregister_slate_post_tick_callback(QUIT[0]);gc.collect()
        dirty=list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())+list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
        REPORT['dirty_packages_after_pie']=[p.get_name() for p in dirty]
        if dirty:REPORT['preserve_editor_memory']=True;save();return
        REPORT['finished_at']=datetime.now(timezone.utc).isoformat();save();unreal.SystemLibrary.quit_editor()
    QUIT[0]=unreal.register_slate_post_tick_callback(quit_tick)

def tick(delta):
    global FRONTEND,SOURCE_SERVICE
    if time.monotonic()<STATE['next']:return
    try:
        if time.monotonic()-STATE['start']>900:raise TimeoutError('Default root PIE exceeded 900 seconds')
        index=STATE['index']
        if index==-2:
            check('isolated Saved directory',Path(unreal.Paths.project_saved_dir()).resolve().is_relative_to(HERE))
            for font in ('F_InkDisplay','F_InkBody'):
                check('actual Chinese font '+font,bool(unreal.load_asset('/Game/newUI/Ink/Fonts/Shared/'+font)))
            check('actual new default asset',bool(unreal.load_asset('/Game/newUI/Runtime/WBP_NewUIFrontend')))
            check('begin actual default PIE',LIB.begin_frontend_pie(720,1280))
            STATE.update(index=-1,next=time.monotonic()+12);return
        if index==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            directors=unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.InkMenuDirector)
            if not directors:return
            FRONTEND=directors[0].get_editor_property('frontend_widget')
            if not FRONTEND:return
            instance=unreal.GameplayStatics.get_game_instance(worlds[0])
            SOURCE_SERVICE=next((x for x in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if x.get_outer()==instance),None)
            REPORT['actual_service_enabled']=bool(SOURCE_SERVICE and SOURCE_SERVICE.is_enabled())
            STATE.update(index=0,next=time.monotonic()+2);return
        if index>=len(STEPS):REPORT['finished']=True;save();finish();return
        name,action,delay=STEPS[index]
        REPORT['current_step']=name;result=action()
        if result is False:
            STATE['next']=time.monotonic()+.06;save();return
        STATE.update(index=index+1,next=time.monotonic()+delay);save()
    except Exception:
        REPORT['error']=traceback.format_exc();save();finish()

HANDLE=unreal.register_slate_post_tick_callback(tick)

