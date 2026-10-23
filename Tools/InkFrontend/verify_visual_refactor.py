"""Current rendered evidence: nine HUD/sign-in sizes plus each visual page family."""
from pathlib import Path
import json,time,struct
import unreal
BUSINESS_ONLY='InkBusinessQAOnly' in unreal.SystemLibrary.get_command_line()
SCENE_ONLY='InkSceneQAOnly' in unreal.SystemLibrary.get_command_line()
OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/VisualRefactor20261022'/('RenderedScenes' if SCENE_ONLY else 'RenderedBusiness' if BUSINESS_ONLY else 'Rendered');OUT.mkdir(parents=True,exist_ok=True)
L=unreal.InkFrontendEditorLibrary;REPORT=dict(boundary='Windows rendered PIE, isolated simulated data; not mobile/device',checks=[],captures=[],errors=[],passed=False)
F=D=W=None;INDEX=-2;NEXT=0;START=time.monotonic();STEPS=[];AUTH_WAIT=0
def check(n,v,a=None):REPORT['checks'].append(dict(name=n,passed=bool(v),actual=a))
def save():
    REPORT['passed']=REPORT.get('finished',False) and not REPORT['errors'] and all(c['passed'] for c in REPORT['checks'])
    (OUT/'report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
def capture(tag,page,size):
    L.move_frontend_pointer_outside(F);data=json.loads(L.describe_widget(F));png=OUT/(tag+'.png')
    ok=L.capture_frontend_viewport(str(png));check(tag+' pixels',ok)
    if ok:check(tag+' size',struct.unpack('>II',png.read_bytes()[16:24])==size)
    check(tag+' route',data['page']==page,data['page'])
    texts=[t for t in data['texts'] if t['visible'] and t.get('text')]
    check(tag+' styled fonts',all('/Refactor20261021/Fonts/F_Ink' in t.get('font','') for t in texts),[t['text'] for t in texts if '/Refactor20261021/Fonts/F_Ink' not in t.get('font','')])
    if SCENE_ONLY:
        art=[i for i in data['images'] if i['visible'] and i['painted_width']>0 and i['painted_height']>0 and 'MI_InkSceneSoft' in i.get('resource','')]
        check(tag+' scene art exists',bool(art))
        check(tag+' scene art preserves 3 to 1 aspect',all(i['painted_height']>0 and abs(i['painted_width']/i['painted_height']-3)<.02 for i in art),art)
    if tag=='shop-selected-confirm':
        cost=next(t for t in texts if t['name']=='WulingSelectedDetails')
        confirm=next(b for b in data['buttons'] if b['action']=='Ext_ToolConfirm')
        selected=next(b for b in data['buttons'] if b['action']=='Ext_ToolRow_0')
        check('shop fixed cost fully visible',cost['painted_height']>=cost['desired_height']-1 and cost['y']+cost['height']<=confirm['y']+1,cost)
        check('shop full cost wording','50灵石' in cost['text'] and '灵草×2' in cost['text'],cost['text'])
        check('shop scroll retained after selection',selected['painted_height']>=87.9,selected)
    if page=='SignIn':
        rewards=[t for t in texts if t.get('name','').startswith('WeeklyRewardName_')]
        check(tag+' seven distinct daily rewards',len(rewards)==7 and len({t['text'] for t in rewards})==7,[t['text'] for t in rewards])
        check(tag+' no obsolete qi reward',not any('300' in t['text'] for t in texts))
    if page=='HUD':
        ids=['OpenCharacter','OpenInventory','ToggleCultivation','OpenLeaderboard','ToggleHUDMenu']
        buttons=sorted([b for b in data['buttons'] if b['visible'] and b['action'] in ids],key=lambda b:b['x'])
        check(tag+' five navigation', [b['action'] for b in buttons]==ids)
        check(tag+' navigation touch areas',all(b['width']>=87.9 and b['height']>=87.9 for b in buttons))
    (OUT/(tag+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    REPORT['captures'].append(dict(tag=tag,path=str(png),page=page));save()
def step(name,fn,delay=1):STEPS.append((name,fn,delay))
step('initial resize',lambda:check('initial resize',L.set_pie_viewport_size(720,1280)),2)
step('main',lambda:capture('main-before-start','Main',(720,1280)))
step('start',lambda:check('start Slate click',L.click_action(F,'StartGame')),4)
for width,height in [(360,640),(540,960),(720,1280),(1080,1920),(1080,2340),(960,720),(1280,720),(1920,1080),(2340,1080)]:
    size=(width,height);label=f'{width}x{height}'
    step(label+' resize',lambda w=width,h=height:check('resize '+str((w,h)),L.set_pie_viewport_size(w,h)),2)
    step(label+' HUD',lambda s=size,t=label:capture('hud-'+t,'HUD',s))
    step(label+' sign in',lambda:F.open_feature_panel('SignIn',''),2)
    step(label+' sign capture',lambda s=size,t=label:capture('signin-'+t,'SignIn',s))
    step(label+' back',lambda:check('back sign in',L.click_action(F,'Ext_Back')),1)
for width,height in [(720,1280),(1280,720)]:
    size=(width,height);label=f'{width}x{height}'
    step(label+' family resize',lambda w=width,h=height:L.set_pie_viewport_size(w,h),2)
    for legacy_route in ['Inventory','Leaderboard','Settings','Saves']:
        step(legacy_route+' open',lambda r=legacy_route:F.show_page(r),2)
        step(legacy_route+' capture',lambda r=legacy_route,s=size,t=label:capture(r+'-'+t,r,s))
        step(legacy_route+' back',lambda:L.click_action(F,'Back'))
    for route in ['CharacterProfile','Activities','WorldCultivation','Wuling','WulingEquipment','WulingConstruction','WulingShop','WulingMarket','WulingOrders','Draw']:
        step(route+' open',lambda r=route:F.open_feature_panel(r,''),2)
        step(route+' capture',lambda r=route,s=size,t=label:capture(r+'-'+t,r,s))
        step(route+' back',lambda:check('family back',L.click_action(F,'Ext_Back')))
SHOP_BEFORE=None;SIGN_BEFORE=None

def authority():
    gi=unreal.GameplayStatics.get_game_instance(W)
    subsystem=next(x for x in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if x.get_outer()==gi)
    return json.loads(subsystem.get_snapshot_json())

def remember_sign():
    global SIGN_BEFORE
    SIGN_BEFORE=authority()

def check_sign():
    current=authority();check('sign records server claim',current['activities']['signed'])
    check('sign gives no qi',current['growth']['total_qi']==SIGN_BEFORE['growth']['total_qi'])
    check('sign gives no inventory',current['inventory']==SIGN_BEFORE['inventory'])
    check('sign gives no currency',current['wallet']==SIGN_BEFORE['wallet'])

def scroll_shop():
    buttons=json.loads(L.describe_widget(F))['buttons']
    row=next(b for b in buttons if b['action']=='Ext_ToolRow_0')
    widget=unreal.find_object(None,row['object'])
    while widget and not isinstance(widget,unreal.ScrollBox):widget=widget.get_parent()
    check('shop scroll container',widget is not None)
    if widget:widget.set_scroll_offset(500)

def remember_shop():
    global SHOP_BEFORE
    SHOP_BEFORE=authority()

def check_shop():
    current=authority();check('purchase costs exact stone',current['wallet']['CUR_STONE']==SHOP_BEFORE['wallet']['CUR_STONE']-50)
    check('purchase costs exact herb',current['inventory']['MAT_HERB_B']==SHOP_BEFORE['inventory']['MAT_HERB_B']-2)
    check('purchase grants one heal',current['inventory']['ITEM_HEAL']==SHOP_BEFORE['inventory']['ITEM_HEAL']+1)

step('sign placeholder open',lambda:F.open_feature_panel('SignIn',''),2)
step('sign before',remember_sign)
step('sign placeholder claim',lambda:check('sign placeholder click',L.click_action(F,'Ext_SignInClaim')),3)
step('sign placeholder economics',check_sign)
step('sign claimed screenshot',lambda:capture('signin-placeholder-claimed','SignIn',(1280,720)))
step('sign back',lambda:L.click_action(F,'Ext_Back'))
step('shop selected open',lambda:F.open_feature_panel('WulingShop',''),2)
step('shop scroll',scroll_shop,1)
step('shop select',lambda:check('shop select row',L.click_action(F,'Ext_ToolRow_0')),2)
step('shop confirm screenshot',lambda:capture('shop-selected-confirm','WulingShop',(1280,720)))
step('shop before',remember_shop)
step('shop confirm',lambda:check('shop fixed confirmation click',L.click_action(F,'Ext_ToolConfirm')),3)
step('shop actual costs',check_shop)
step('shop back',lambda:L.click_action(F,'Ext_Back'))
DRAW_RESULTS=None;INVENTORY_BEFORE=None

def draw_results():
    return list(next(p for p in unreal.ObjectIterator(unreal.InkDrawPanel) if p.get_world()==W and p.get_editor_property('demo_results')).get_editor_property('demo_results'))

def inventory_state():
    return sorted((str(e.get_editor_property('authority_instance_id')),e.get_editor_property('item').get_editor_property('count')) for e in F.get_inventory_model().get_entries())

def record_draw():
    global DRAW_RESULTS
    DRAW_RESULTS=draw_results();check('result selected before animation',len(DRAW_RESULTS)==1,DRAW_RESULTS)

def remember_inventory():
    global INVENTORY_BEFORE
    INVENTORY_BEFORE=inventory_state()

step('draw preview',lambda:F.open_feature_panel('Draw',''),2)
step('inventory before demo',remember_inventory)
step('preview once',lambda:check('preview once',L.click_action(F,'Ext_Demo_DrawOnce')),.3)
step('preselected result',record_draw,.1)
step('preview skip',lambda:check('preview skip',L.click_action(F,'Ext_Demo_DrawSkip')),1)
step('skip keeps result',lambda:check('skip result unchanged',draw_results()==DRAW_RESULTS))
step('preview result',lambda:capture('wheel-preview-result','Draw',(1280,720)))
step('wheel close',lambda:L.click_action(F,'Ext_Back'))
step('wheel reopen',lambda:F.open_feature_panel('Draw',''),1)
step('reopen keeps result',lambda:check('reopen result unchanged',draw_results()==DRAW_RESULTS))
step('wheel resize',lambda:L.set_pie_viewport_size(720,1280),2)
step('resize keeps result',lambda:check('resize result unchanged',draw_results()==DRAW_RESULTS))
step('wheel portrait result',lambda:capture('wheel-result-portrait','Draw',(720,1280)))
step('wheel inventory unchanged',lambda:check('demo leaves inventory unchanged',inventory_state()==INVENTORY_BEFORE))
step('reduced motion',lambda:F.set_editor_property('reduced_motion',True))
step('reduced demo',lambda:check('reduced demo once',L.click_action(F,'Ext_Demo_DrawOnce')),1)
step('reduced capture',lambda:capture('wheel-reduced-motion','Draw',(720,1280)))
step('close',lambda:L.click_action(F,'Ext_Back'))
step('final sign preview',lambda:F.open_feature_panel('SignIn',''),2)
step('final sign capture',lambda:capture('signin-final-720x1280','SignIn',(720,1280)))
step('final close',lambda:L.click_action(F,'Ext_Back'))

if BUSINESS_ONLY:
    start=next(i for i,s in enumerate(STEPS) if s[0]=='sign placeholder open')
    STEPS=[('initial resize',lambda:L.set_pie_viewport_size(1280,720),2),('start',lambda:check('start Slate click',L.click_action(F,'StartGame')),4)]+STEPS[start:]

if SCENE_ONLY:
    STEPS=[('initial resize',lambda:L.set_pie_viewport_size(720,1280),2),('start',lambda:check('start Slate click',L.click_action(F,'StartGame')),6)]
    for width,height in [(720,1280),(1280,720)]:
        size=(width,height);label=f'{width}x{height}'
        step(label+' scene resize',lambda w=width,h=height:L.set_pie_viewport_size(w,h),2)
        for route in ['CharacterProfile','WorldCultivation','Activities','WulingShop','SignIn']:
            step(route+' scene open',lambda r=route:F.open_feature_panel(r,''),2)
            step(route+' scene capture',lambda r=route,t=label,s=size:capture(r+'-'+t,r,s))
            step(route+' scene back',lambda:check('scene back',L.click_action(F,'Ext_Back')))

def tick(dt):
    global F,D,W,INDEX,NEXT,AUTH_WAIT
    if time.monotonic()<NEXT:return
    try:
        if time.monotonic()-START>600:raise TimeoutError('visual QA watchdog')
        if INDEX==-2:check('begin PIE',L.begin_frontend_pie(720,1280));INDEX=-1;NEXT=time.monotonic()+12;return
        if INDEX==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+1;return
            W=worlds[0];D=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)[0];F=D.get_editor_property('frontend_widget')
            if not F:NEXT=time.monotonic()+1;return
            INDEX=0
        if INDEX>=len(STEPS):
            REPORT['finished']=True;save();unreal.unregister_slate_post_tick_callback(H);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor();return
        name,fn,delay=STEPS[INDEX]
        if name=='sign placeholder open':
            gi=unreal.GameplayStatics.get_game_instance(W)
            subsystem=next(x for x in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if x.get_outer()==gi)
            if not subsystem.is_connected() or not subsystem.get_snapshot_json():
                if not AUTH_WAIT:AUTH_WAIT=time.monotonic()
                REPORT['authority_wait']=dict(page=str(F.get_editor_property('current_page')),status=subsystem.get_status())
                if str(F.get_editor_property('current_page'))=='Main':L.click_action(F,'StartGame')
                if time.monotonic()-AUTH_WAIT>45:raise TimeoutError('Authority not ready: '+str(REPORT['authority_wait']))
                NEXT=time.monotonic()+2;save();return
        fn();INDEX+=1;NEXT=time.monotonic()+delay;save()
    except Exception as e:
        REPORT['errors'].append(dict(step=INDEX,error=repr(e)));save();unreal.unregister_slate_post_tick_callback(H);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
H=unreal.register_slate_post_tick_callback(tick)
