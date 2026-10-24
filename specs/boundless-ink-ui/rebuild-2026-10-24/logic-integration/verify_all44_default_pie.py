"""Actual default player startup and Slate actions; isolated process only."""
from pathlib import Path
import gc, json, time, traceback, os, hashlib, re
from datetime import datetime, timezone
import unreal
import runpy
METRICS=runpy.run_path(str(Path(__file__).resolve().parent/'geometry_metric_validation.py'),run_name='geometry_metric_validation')

HERE=Path(__file__).resolve().parent
ATTEMPT=re.search(r'-NewUIDefaultPIEAttempt=(\d+)\b',unreal.SystemLibrary.get_command_line())
OUT=HERE/('default-root-pie-'+(ATTEMPT.group(1) if ATTEMPT else '01'))
OUT.mkdir(exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
LIB=unreal.InkFrontendEditorLibrary
REPORT=dict(scope='Actual default player startup, no direct component construction', checks=[], captures=[], finished=False)
REPORT.update(engine_pid=os.getpid(),started_at=datetime.now(timezone.utc).isoformat(),source_epoch_sha256=hashlib.sha256((HERE/'source-epoch.json').read_bytes()).hexdigest(),visual_approval='pending_user')
FRONTEND=None
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

def capture(page,tag=None):
    snapshot=describe()
    check('actual page '+page,snapshot.get('page')==page,snapshot.get('page'))
    check('independent root '+page,snapshot.get('root_widget')=='NewUIRoot_r1',snapshot.get('root_widget'))
    check('new root runtime version '+page,snapshot.get('runtime_layout_version')=='newui-r1-root-20261024',snapshot)
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
    if page=='HUD':
        for field in ('legacy_weapon_hud_suppression_condition','legacy_gm_anchor_suppression_condition','legacy_status_adapter_collapsed'):
            check('actual legacy suppression '+field+' '+(tag or page),snapshot.get(field) is True,snapshot.get(field))
    tag=tag or page.lower()
    LIB.move_frontend_pointer_outside(FRONTEND)
    image=OUT/(tag+'.png')
    check('rendered viewport '+tag,LIB.capture_frontend_viewport(str(image)))
    (OUT/(tag+'.json')).write_text(json.dumps(snapshot,ensure_ascii=False,indent=2),encoding='utf-8')
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
    snapshot=describe();matches=[x for x in snapshot.get('buttons',[]) if x.get('action')==action and x.get('visible')]
    check('one real action target before scroll '+action,len(matches)==1,matches)
    REPORT.setdefault('action_scroll_before',{})[action]=matches[0];save()
    check('real scroll to action '+action,LIB.scroll_to_frontend_widget(FRONTEND,matches[0]['name']))

def click_after_scroll(action):
    snapshot=describe();matches=[x for x in snapshot.get('buttons',[]) if x.get('action')==action and x.get('visible')]
    check('one painted target after actual scroll '+action,len(matches)==1,matches)
    target=matches[0];REPORT.setdefault('action_scroll_after',{})[action]=target;save()
    check('actual scrolled target expected painted '+action,target.get('paint_expectation')=='expected' and target.get('geometry_arranged'),target)
    check('actual scrolled target inside clipping bounds '+action,target['independent_left']>=target['independent_clip_left']-1 and target['independent_top']>=target['independent_clip_top']-1 and target['independent_right']<=target['independent_clip_right']+1 and target['independent_bottom']<=target['independent_clip_bottom']+1,target)
    click(action)

def click(action):check('real Slate click '+action,LIB.click_action(FRONTEND,action))
def click_label(caption):check('real Slate typed-button click '+caption,LIB.click_labeled_button(FRONTEND,unreal.Text(caption)))
def board(category,scope,tag):
    capture('Leaderboard',tag)
    snapshot=describe();check('actual board category '+tag,snapshot.get('board_category')==category,snapshot.get('board_category'))
    check('actual board scope '+tag,snapshot.get('board_scope')==scope,snapshot.get('board_scope'))
def last_slot_visible():
    snapshot=describe();slot=next(x for x in snapshot['buttons'] if x['name']=='Rebuild_InventorySlot_39')
    scroll=next(x for x in snapshot['scrolls'] if x['name']=='NewUIScroll_Inventory')
    check('last real physical slot arranged after scroll',slot.get('geometry_arranged') and slot.get('visible'),slot)
    check('last slot is inside actual scroll viewport',slot['x']>=scroll['x']-1 and slot['y']>=scroll['y']-1 and slot['x']+slot['width']<=scroll['x']+scroll['width']+1 and slot['y']+slot['height']<=scroll['y']+scroll['height']+1,dict(slot=slot,scroll=scroll))

STEPS=[
    ('main',lambda:capture('Main'),1),
    ('saves entry',lambda:click('OpenSaves'),1),('saves',lambda:capture('Saves'),1),
    ('back from saves',lambda:click('Back'),1),('settings entry',lambda:click('OpenSettings'),1),
    ('settings',lambda:capture('Settings'),1),('back from settings',lambda:click('Back'),1),
    ('start game',lambda:click('StartGame'),4),('hud',lambda:capture('HUD'),1),
    ('character entry',lambda:click('OpenCharacter'),1),('character',lambda:capture('Character'),1),
    ('back from character',lambda:click('Back'),1),('inventory entry',lambda:click('OpenInventory'),1),
    ('inventory',lambda:capture('Inventory'),1),
    ('inventory search',lambda:check('Slate search and retained input',LIB.type_inventory_search(FRONTEND,'不存在的物品')),1),
    ('inventory search rendered',lambda:capture('Inventory','inventory-search'),1),
    ('inventory search clear',lambda:check('Slate search cleared',LIB.type_inventory_search(FRONTEND,'')),1),
    ('inventory refreshed',lambda:capture('Inventory','inventory-cleared'),1),
    ('landscape viewport',lambda:check('resize landscape',LIB.set_pie_viewport_size(1280,720)),2),
    ('landscape inventory',lambda:capture('Inventory','inventory-landscape'),1),
    ('scroll to last real inventory slot',lambda:check('real root scroll to physical slot39',LIB.scroll_to_frontend_widget(FRONTEND,'Rebuild_InventorySlot_39')),1),
    ('last inventory slot visible',last_slot_visible,1),
    ('landscape inventory scroll end',lambda:capture('Inventory','inventory-landscape-end'),1),
    ('landscape back',lambda:click('Back'),1),('landscape hud',lambda:capture('HUD','hud-landscape'),1),
    ('new leaderboard entry',lambda:click('OpenLeaderboard'),2),
    ('native rank Damage tab',lambda:click_label('伤害榜'),1),('rank Damage local',lambda:board('Damage','Local','rank-damage-local'),1),
    ('native rank Cultivation tab',lambda:click_label('修炼速度'),1),('rank Cultivation local',lambda:board('CultivationSpeed','Local','rank-cultivation-local'),1),
    ('native rank Anchor tab',lambda:click_label('主播剑灵'),1),('rank Anchor local',lambda:board('Anchor','Local','rank-anchor-local'),1),
    ('native rank Global scope',lambda:click_label('全服'),1),('rank Global actual state',lambda:board('Anchor','Global','rank-anchor-global'),1),
    ('native rank Local scope',lambda:click_label('本地'),1),('new leaderboard back',lambda:click('Back'),1),
]


# Separate player-entry evidence: actual Slate input through the HUD menu,
# Journey native callbacks, and all four Character feature buttons.
STEPS.extend([
    ('HUD native menu',lambda:click('ToggleHUDMenu'),.6),
    ('HUD native journey entry',lambda:click('OpenExtensions'),1),
    ('journey entry observation',lambda:capture('Extensions','extensions-player-entry'),.5),
    ('journey native world entry',lambda:click_label('进入世界地图'),1),
    ('world entry observation',lambda:capture('WorldCultivation','world-player-entry'),.5),
    ('world native return',lambda:click_label('返回'),.8),
    ('journey ancestor retained',lambda:check('real nested feature back stack',describe().get('page')=='Extensions'),.2),
    ('journey native return HUD',lambda:click_label('返回'),.8),
])
for route in ('EquipmentFabao','CultivationMethods','BreakthroughPrepare','CharacterAppearance'):
    STEPS.extend([
        ('Character source entry '+route,lambda:click('OpenCharacter'),.8),
        ('scroll Character feature entry '+route,lambda r=route:scroll_action('Open'+r),1),
        ('Character native feature entry '+route,lambda r=route:click_after_scroll('Open'+r),1),
        ('Character child observation '+route,lambda r=route:capture(r,r.lower()+'-character-entry'),.4),
        ('Character child native return '+route,lambda:click_label('返回'),.8),
        ('Character parent restored '+route,lambda:check('actual Character parent restored',describe().get('page')=='Character'),.2),
        ('Character back HUD '+route,lambda:click('Back'),.8),
    ])

# These exercise the real default frontend router. Player-entry Slate proof is
# recorded separately; this loop does not claim a click path to every page.
def open_feature(route):
    check('real default frontend router '+route,FRONTEND.open_feature_panel(route,''))

FEATURE_ROUTES=['Wuling', 'SwordSpirit', 'GiftRules', 'CultivationMethods', 'RaceLobby', 'RaceLive', 'RaceSettlement', 'ResourceLease', 'EventSettlement', 'OfflineRewards', 'Shop', 'Market', 'TradeOrders', 'WorldCultivation', 'RegionDetail', 'ResourceContest', 'ResourceDetail', 'SignIn', 'MonthlySignIn', 'DailyFortune', 'Activities', 'DailyActivities', 'TimedActivities', 'DailyActivityDetail', 'TimedActivityDetail', 'LaunchEvent', 'LaunchRank', 'BreakthroughPrepare', 'Tribulation', 'ItemSources', 'GuildConstruction', 'EquipmentFabao', 'Extensions', 'WulingCraft', 'CharacterAppearance', 'ResourceRanking', 'ResourceRewards']
for orientation,width,height in [('portrait',720,1280),('landscape',1280,720)]:
    STEPS.append(('resize '+orientation,lambda w=width,h=height:check('resize feature viewport',LIB.set_pie_viewport_size(w,h)),2))
    for route in FEATURE_ROUTES:
        STEPS.extend([
            ('open '+route,lambda r=route:open_feature(r),1.5),
            ('capture '+route,lambda r=route,o=orientation:capture(r,r.lower()+'-'+o),.5),
            ('typed back '+route,lambda:click_label('返回'),.6),
            ('back reached HUD '+route,lambda r=route:check('feature back to HUD '+r,describe().get('page')=='HUD'),.2),
        ])

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
    global FRONTEND
    if time.monotonic()<STATE['next']:return
    try:
        if time.monotonic()-STATE['start']>660:raise TimeoutError('Default root PIE exceeded 660 seconds')
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
            STATE.update(index=0,next=time.monotonic()+2);return
        if index>=len(STEPS):REPORT['finished']=True;save();finish();return
        name,action,delay=STEPS[index]
        REPORT['current_step']=name;action();STATE.update(index=index+1,next=time.monotonic()+delay);save()
    except Exception:
        REPORT['error']=traceback.format_exc();save();finish()

HANDLE=unreal.register_slate_post_tick_callback(tick)
