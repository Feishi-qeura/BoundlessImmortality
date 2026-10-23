"""Rendered PIE QA. Run with -ExecCmds="py ..." and isolated Wuling/GWLocalTest flags."""
import json
import struct
import time
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir())
OUT=ROOT/'Saved/Codex/UIRefactor20261021/Resume20261021/Verification'
OUT.mkdir(parents=True,exist_ok=True)
LIB=unreal.InkFrontendEditorLibrary
audit_path=ROOT/'Tools/InkFrontend/audit_ui_refactor_assets.py'
audit_ns={'__name__':'__audit__'}
exec(compile(audit_path.read_text(encoding='utf-8'),str(audit_path),'exec'),audit_ns)
audit_ns['run']()
delivery_audit=ROOT/'Tools/InkFrontend/audit_ui_refactor_delivery.py'
delivery_ns={'__name__':'__delivery_audit__'}
exec(compile(delivery_audit.read_text(encoding='utf-8'),str(delivery_audit),'exec'),delivery_ns)
delivery_ns['run']()
REPORT={'boundary':'Windows rendered PIE; no mobile/platform/service completion claim','checks':[],'captures':[],'errors':[]}
FRONTEND=None
WORLD=None
DIRECTOR=None
HANDLE=None
START=time.monotonic()
INVENTORY_FIXTURE=False
NEXT=START
INDEX=-2
STEPS=[]

def save():
    REPORT['passed']=not REPORT['errors'] and all(c['passed'] for c in REPORT['checks'])
    (OUT/'pie-report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')

def ck(name,value,actual=None):
    REPORT['checks'].append({'name':name,'passed':bool(value),'actual':actual})

def snapshot():return json.loads(LIB.describe_widget(FRONTEND))
def page(name):FRONTEND.show_page(name)
def identity(name):
    ck('isolated mock identity set',LIB.set_frontend_test_anchor_name(FRONTEND,name))
def resize(w,h):ck('request resize '+str((w,h)),LIB.set_pie_viewport_size(w,h))
def inventory_fixture():
    global INVENTORY_FIXTURE
    if not FRONTEND.get_inventory_model().get_entries():
        item=unreal.InkInventoryItem()
        item.set_editor_property('item_id','UI_QA_MATERIAL')
        item.set_editor_property('name',unreal.Text('UI测试·灵草'))
        item.set_editor_property('category','Material')
        ck('isolated inventory fixture added',FRONTEND.add_inventory_item(item,2)==2)
        INVENTORY_FIXTURE=True
        REPORT['fixtures']=['UI测试·灵草: local test inventory only; removed before exit; no service grant']
def clear_inventory_fixture():
    if INVENTORY_FIXTURE:FRONTEND.set_inventory_items([])
def click(action):
    ok=LIB.click_action(FRONTEND,action)
    ck('actual Slate click '+action,ok)
    if not ok:FRONTEND.trigger_action(action) # continue diagnostics, failure remains in report

def capture(tag,expected,w,h):
    LIB.move_frontend_pointer_outside(FRONTEND)
    data=snapshot()
    data['font_property']=str(FRONTEND.get_editor_property('text_font'))
    png=OUT/(tag+'.png')
    captured=LIB.capture_frontend_viewport(str(png))
    ck(tag+' screenshot',captured)
    size=struct.unpack('>II',png.read_bytes()[16:24]) if captured else None
    ck(tag+' pixel size',size==(w,h),size)
    ck(tag+' page',data['page']==expected,data['page'])
    (OUT/(tag+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    REPORT['captures'].append({'name':tag,'size':size,'page':data['page'],'path':str(png)})
    texts=[t for t in data['texts'] if t['visible'] and t.get('text')]
    ck(tag+' visible UMG fonts',all('/Refactor20261021/Fonts/F_Ink' in t.get('font','') for t in texts),[{'text':t['text'],'font':t.get('font')} for t in texts if '/Refactor20261021/Fonts/F_Ink' not in t.get('font','')])
    if expected=='HUD':
        actions=[b for b in data['buttons'] if b['visible'] and b['action'] in ['OpenCharacter','OpenInventory','ToggleCultivation','OpenLeaderboard','ToggleHUDMenu']]
        ordered=sorted(actions,key=lambda b:b['x'])
        ck(tag+' five bottom actions',[b['action'] for b in ordered]==['OpenCharacter','OpenInventory','ToggleCultivation','OpenLeaderboard','ToggleHUDMenu'])
        ck(tag+' HUD hit targets',all(b['width']>=87.9 and b['height']>=87.9 for b in data['buttons'] if b['visible'] and b['name'].startswith('HUD_')))
    if expected=='SignIn':
        claims=[b for b in data['buttons'] if b['visible'] and b['action']=='Ext_SignInClaim']
        ck(tag+' unconnected claim disabled',len(claims)==1 and not claims[0]['enabled'])
        ck(tag+' sample rendered hit targets',all(b['width']>=87.9 and b['height']>=87.9 for b in data['buttons'] if b['visible'] and b['action'].startswith('Ext_') and b['width']>0 and b['height']>0))
        ck(tag+' no baked local date',not any(t['name']=='CalendarMonth' and '年' in t['text'] for t in texts))
    save()

def step(name,fn,delay=2):STEPS.append((name,fn,delay))
step('prepare actual main menu',lambda:DIRECTOR.return_to_main_menu(),3)
step('main',lambda:capture('01-main-720x1280','Main',720,1280))
step('start',lambda:click('StartGame'),5)
step('hud',lambda:capture('02-hud-720x1280','HUD',720,1280))
step('signin',lambda:click('OpenSignIn'))
step('signin capture',lambda:capture('03-signin-720x1280','SignIn',720,1280))
step('select reward',lambda:click('Ext_SignInDay_7'))
step('selection state',lambda:ck('selected day retained',any(t.get('text','').startswith('第 7 天奖励明细') for t in snapshot()['texts'])))
step('rotate while open',lambda:resize(1280,720),4)
step('rotation state',lambda:ck('rotation retains selected reward',any(t.get('text','').startswith('第 7 天奖励明细') for t in snapshot()['texts'])))
step('landscape signin',lambda:capture('05-signin-1280x720','SignIn',1280,720))
step('back',lambda:click('Ext_Back'))
step('landscape hud',lambda:capture('04-hud-1280x720','HUD',1280,720))
step('open menu',lambda:click('ToggleHUDMenu'))
step('menu keeps center action',lambda:ck('center action remains visible with menu',any(b['visible'] and b['action']=='ToggleCultivation' for b in snapshot()['buttons'])))
step('close menu',lambda:click('ToggleHUDMenu'))
step('menu focus restored',lambda:ck('closing HUD menu restores navigation focus',any(b.get('keyboard_focused') and b['action']=='ToggleHUDMenu' for b in snapshot()['buttons'])))
step('cultivate',lambda:click('ToggleCultivation'),3)
step('cultivate screenshot',lambda:capture('06-cultivating-1280x720','HUD',1280,720))
step('cultivation is active',lambda:ck('actual cultivate state shows stop action',any(t['name']=='HUDCultivationLabel' and t['text']=='收功' for t in snapshot()['texts'])))
step('stop cultivating',lambda:click('ToggleCultivation'),3)
step('cultivation has stopped',lambda:ck('actual stop state shows cultivate action',any(t['name']=='HUDCultivationLabel' and t['text']=='修炼' for t in snapshot()['texts'])))
step('freeze current scene for layout matrix',lambda:ck('layout snapshot pause (not gameplay validation)',unreal.GameplayStatics.set_game_paused(WORLD,True)))
step('long Chinese name',lambda:identity('青云问道逍遥自在修仙客'))
step('long name capture',lambda:capture('boundary-longname-1280x720','HUD',1280,720))
step('open full identity',lambda:click('ShowAnchorIdentity'))
step('full identity content',lambda:ck('identity modal shows complete name',any(t['visible'] and t.get('text')=='青云问道逍遥自在修仙客' for t in snapshot()['texts'])))
step('identity modal capture',lambda:capture('boundary-fullname-1280x720','HUD',1280,720))
step('close full identity',lambda:click('ConfirmPending'))
step('long Latin name',lambda:identity('CultivatorWithAnExtraordinarilyLongName0123456789'))
step('Latin name capture',lambda:capture('boundary-latin-1280x720','HUD',1280,720))
step('restore test display name',lambda:identity('主播'))
step('missing glyph display name',lambda:identity('问道𠮷仙客 Ω'))
step('missing glyph capture',lambda:capture('boundary-glyphs-1280x720','HUD',1280,720))
step('restore name after glyph test',lambda:identity('主播'))
for w,h in [(360,640),(540,960),(720,1280),(1080,1920),(1080,2340),(960,720),(1280,720),(1920,1080),(2340,1080)]:
    step('resize '+str((w,h)),lambda w=w,h=h:resize(w,h),3)
    step('matrix HUD',lambda w=w,h=h:capture(f'matrix-{w}x{h}-hud','HUD',w,h))
    step('matrix open sign in',lambda:click('OpenSignIn'))
    step('matrix sign in',lambda w=w,h=h:capture(f'matrix-{w}x{h}-signin','SignIn',w,h))
    step('matrix back',lambda:click('Ext_Back'))
step('reset size',lambda:resize(720,1280),3)
step('safe inset simulation',lambda:ck('transient SafeZone padding',LIB.set_frontend_safe_padding(FRONTEND,unreal.Margin(24,0,24,36))),3)
step('safe HUD capture',lambda:capture('boundary-safezone-720x1280','HUD',720,1280))
step('stream inset simulation',lambda:FRONTEND.set_editor_property('extra_stream_bottom_inset',100),3)
step('stream HUD capture',lambda:capture('boundary-streaminset-720x1280','HUD',720,1280))
step('restore stream inset',lambda:FRONTEND.set_editor_property('extra_stream_bottom_inset',0))
step('restore safe inset',lambda:ck('clear transient SafeZone padding',LIB.set_frontend_safe_padding(FRONTEND,unreal.Margin(0,0,0,0))),3)
step('monthly open',lambda:click('OpenSignIn'))
step('monthly select',lambda:click('Ext_GroupTab_MonthlySignIn'))
step('monthly capture',lambda:capture('07-monthly-720x1280','SignIn',720,1280))
step('monthly rotate',lambda:resize(1280,720),3)
step('monthly wide capture',lambda:capture('08-monthly-1280x720','SignIn',1280,720))
step('switch verse',lambda:click('Ext_GroupTab_DailyFortune'))
step('verse capture',lambda:capture('09-method-1280x720','SignIn',1280,720))
step('restore daily tab',lambda:click('Ext_GroupTab_SignIn'))
step('daily day retained',lambda:ck('tab switching retains daily reward selection',any(t.get('text','').startswith('第 7 天奖励明细') for t in snapshot()['texts'])))
step('return to HUD',lambda:click('Ext_Back'))
for action,target in [('OpenCharacter','CharacterProfile'),('OpenInventory','Inventory'),('OpenLeaderboard','Leaderboard')]:
    step('regression '+target,lambda action=action:click(action))
    step('regression capture '+target,lambda target=target:capture('regression-'+target.lower(),target,1280,720))
    step('regression return',lambda:page('HUD'))
step('inventory interactions open',lambda:click('OpenInventory'))
step('isolated inventory fixture',inventory_fixture)
step('inventory search input',lambda:ck('actual Slate inventory search',LIB.type_inventory_search(FRONTEND,'灵')))
step('inventory search state',lambda:ck('inventory retains search text',json.loads(LIB.describe_inventory(FRONTEND))['search']=='灵'))
step('inventory no-match query',lambda:ck('actual Slate no-match search',LIB.type_inventory_search(FRONTEND,'不存在的测试物品')))
step('inventory no-match state',lambda:ck('search hides nonmatching entry without deleting inventory',not any(c['visible'] and c['action']=='InventorySelect_0' for c in json.loads(LIB.describe_inventory(FRONTEND))['controls']) and json.loads(LIB.describe_inventory(FRONTEND))['occupied']>0))
step('inventory clear search',lambda:ck('actual Slate clear search',LIB.type_inventory_search(FRONTEND,'')))
step('inventory touch selection',lambda:click('InventorySelect_0'))
step('inventory selection state',lambda:ck('inventory selected entry has detail',bool(json.loads(LIB.describe_inventory(FRONTEND))['selected_entry']) and json.loads(LIB.describe_inventory(FRONTEND))['detail_visible']))
step('inventory detail capture',lambda:capture('regression-inventory-detail','Inventory',1280,720))
step('inventory clear isolated fixture',clear_inventory_fixture)
step('inventory return',lambda:page('HUD'))
step('save regression',lambda:page('Saves'))
step('save capture',lambda:capture('regression-saves','Saves',1280,720))
step('settings regression',lambda:page('Settings'))
step('settings capture',lambda:capture('regression-settings','Settings',1280,720))
step('main regression',lambda:page('Main'))
step('main capture',lambda:capture('regression-main-landscape','Main',1280,720))

def tick(delta):
    global INDEX,NEXT,FRONTEND,WORLD,DIRECTOR
    now=time.monotonic()
    if now<NEXT:return
    try:
        if now-START>600:raise RuntimeError('QA watchdog timeout')
        if INDEX==-2:
            ck('PIE request',LIB.begin_frontend_pie(720,1280));INDEX=-1;NEXT=now+12;return
        if INDEX==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=now+2;return
            WORLD=worlds[0]
            directors=unreal.GameplayStatics.get_all_actors_of_class(WORLD,unreal.InkMenuDirector)
            for director in directors:
                DIRECTOR=director
                FRONTEND=director.get_editor_property('frontend_widget')
                if FRONTEND:break
            if not FRONTEND:NEXT=now+2;return
            resize(720,1280);INDEX=0;NEXT=now+4;return
        if INDEX>=len(STEPS):
            REPORT['sequence_finished']=True;save()
            unreal.unregister_slate_post_tick_callback(HANDLE)
            unreal.EditorLevelLibrary.editor_end_play()
            unreal.SystemLibrary.quit_editor();return
        name,fn,delay=STEPS[INDEX]
        fn();INDEX+=1;NEXT=now+delay
    except Exception as exc:
        REPORT['errors'].append({'step':INDEX,'error':str(exc)});save()
        unreal.log_error('UI_QA_ERROR '+str(exc))
        unreal.unregister_slate_post_tick_callback(HANDLE)
        unreal.EditorLevelLibrary.editor_end_play()
        unreal.SystemLibrary.quit_editor()

HANDLE=unreal.register_slate_post_tick_callback(tick)
unreal.log('UI_QA: registered '+str(len(STEPS))+' stages')
