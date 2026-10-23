"""Rendered functionality QA with explicit isolated authority fixtures."""
import json
import time
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/GameplayPlan20261021/Rendered'
OUT.mkdir(parents=True,exist_ok=True)
LIB=unreal.InkFrontendEditorLibrary
REPORT={'boundary':'Windows rendered PIE against declared seeded local prototype; not device/platform/production', 'checks':[], 'screenshots':[], 'errors':[]}
FRONTEND=WORLD=DIRECTOR=WULING=HOST=None
INDEX=-2;NEXT=0;START=time.monotonic();HANDLE=None


def check(name,condition,actual=None):
    REPORT['checks'].append(dict(name=name,passed=bool(condition),actual=actual))


def state():return json.loads(WULING.get_snapshot_json())


def save():
    REPORT['passed']=REPORT.get('sequence_finished',False) and not REPORT['errors'] and bool(REPORT['checks']) and all(r['passed'] for r in REPORT['checks'])
    (OUT/'pie-report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')


def click(action):
    result=LIB.click_action(FRONTEND,action)
    check('Slate click '+action,result)
    return result


def capture(name):
    LIB.move_frontend_pointer_outside(FRONTEND)
    image=OUT/(name+'.png')
    check('rendered '+name,LIB.capture_frontend_viewport(str(image)))
    description=json.loads(LIB.describe_widget(FRONTEND))
    (OUT/(name+'.json')).write_text(json.dumps(description,ensure_ascii=False,indent=2),encoding='utf-8')
    REPORT['screenshots'].append(str(image));save()


def open_page(route):
    check('open '+route,FRONTEND.open_feature_panel(route,''))


def select_core():
    catalog=state().get('catalog',{})
    for i,row in enumerate(catalog.get('rows',[])):
        if row.get('item_id')=='MAT_BR04_C01_G4':
            click('Ext_ToolRow_'+str(i));return True
    click('Ext_ToolNext');return False


def select_inventory(item):
    entry=next(e for e in FRONTEND.get_inventory_model().get_entries() if str(e.item.item_id)==item)
    return click('InventorySelect_'+str(entry.slot_index))


STEPS=[]
def step(name,fn,delay=2):STEPS.append((name,fn,delay))
step('start',lambda:click('StartGame'),8)
step('authority connection',lambda:check('authority is connected',WULING.is_connected(),WULING.get_status()))
step('authority mirror',lambda:check('new-rule realm projected',state().get('growth',{}).get('realm_index')==21))
step('backpack',lambda:FRONTEND.show_page('Inventory'),3)
step('backpack actual data',lambda:check('all authority instances visible',len(FRONTEND.get_inventory_model().get_entries())==len(state()['item_instances'])))
step('backpack capture',lambda:capture('inventory-authority-720x1280'))
step('select robe',lambda:select_inventory('EQ_ROBE_T1'))
step('equip robe via backpack',lambda:click('InventoryEquip'),3)
step('robe confirmed',lambda:check('backpack equip confirmed by authority',state()['loadout']['equipment'].get('robe')=='EQ_ROBE_T1'))
step('original sign-in',lambda:open_page('SignIn'),3)
step('original sign-in claim',lambda:click('Ext_SignInClaim'),3)
step('original sign-in capture',lambda:capture('signin-authority-720x1280'))
step('original monthly calendar',lambda:open_page('MonthlySignIn'),3)
step('calendar capture',lambda:capture('calendar-authority-720x1280'))
step('wuling',lambda:open_page('Wuling'),3)
step('wuling capture',lambda:capture('01-wuling-720x1280'))
for route in ('WulingCraft','WulingSources','WulingEquipment','WulingConstruction','WulingActivity','WulingRace','WulingShop','WulingMarket'):
    step(route,lambda route=route:open_page(route),3)
    step(route+' capture',lambda route=route:capture(route+'-720x1280'))
step('sign-in page',lambda:open_page('WulingActivity'),3)
step('sign-in confirmed',lambda:check('real sign-in confirmed',state().get('activities',{}).get('signed') is True))
step('launch action',lambda:click('Ext_ToolLaunch'),3)
step('launch confirmed',lambda:check('relative launch instance exists',bool(state().get('launch'))))
step('breakthrough page',lambda:open_page('WulingBreakthrough'),3)
step('select core',select_core,3)
step('prepare via Slate',lambda:click('Ext_ToolPrepare'),3)
step('prepare capture',lambda:capture('tribulation-prepare-720x1280'),15)
step('trial capture',lambda:capture('tribulation-trial-720x1280'))
step('simulated helper joins',lambda:check('isolated Join comment accepted',HOST.simulate_comment('qa-helper','试剑灵','加入')),3)
step('simulated helper assists',lambda:check('isolated Assist comment accepted',HOST.simulate_comment('qa-helper','试剑灵','助力')),3)
step('helper confirmed',lambda:check('one distinct helper raises to 51 percent',state().get('tribulation',{}).get('probability_basis_points')==5100,state().get('tribulation')))
step('assisted capture',lambda:capture('tribulation-assisted-720x1280'),62)
step('failure confirmed',lambda:check('fixture draw commits one failure',state().get('tribulation',{}).get('status')=='Failed',state().get('tribulation')))
step('failure capture',lambda:capture('tribulation-result-720x1280'))
step('rotate',lambda:check('landscape viewport',LIB.set_pie_viewport_size(1280,720)),4)
for route in ('WulingBreakthrough','WulingEquipment','WulingRace'):
    step(route+' wide',lambda route=route:open_page(route),3)
    step(route+' wide capture',lambda route=route:capture(route+'-1280x720'))
step('high portrait',lambda:check('high portrait viewport',LIB.set_pie_viewport_size(1080,1920)),4)
step('high portrait page',lambda:open_page('WulingActivity'),3)
step('high portrait capture',lambda:capture('WulingActivity-1080x1920'))


def tick(delta):
    global INDEX,NEXT,FRONTEND,WORLD,DIRECTOR,WULING,HOST
    now=time.monotonic()
    if now<NEXT:return
    try:
        if now-START>400:raise RuntimeError('gameplay QA watchdog')
        if INDEX==-2:
            check('PIE request',LIB.begin_frontend_pie(720,1280));INDEX=-1;NEXT=now+12;return
        if INDEX==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=now+2;return
            WORLD=worlds[0]
            for director in unreal.GameplayStatics.get_all_actors_of_class(WORLD,unreal.InkMenuDirector):
                DIRECTOR=director;FRONTEND=director.get_editor_property('frontend_widget')
                if FRONTEND:break
            if not FRONTEND:NEXT=now+2;return
            game_instance=unreal.GameplayStatics.get_game_instance(WORLD)
            WULING=next(obj for obj in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if obj.get_outer()==game_instance)
            HOST=next(obj for obj in unreal.ObjectIterator(unreal.LiveInteractionSubsystem) if obj.get_outer()==game_instance)
            LIB.set_pie_viewport_size(720,1280);INDEX=0;NEXT=now+4;return
        if INDEX>=len(STEPS):
            REPORT['sequence_finished']=True;save();unreal.unregister_slate_post_tick_callback(HANDLE)
            unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor();return
        name,fn,delay=STEPS[INDEX]
        result=fn()
        if result is not False:INDEX+=1
        NEXT=now+delay
    except Exception as error:
        REPORT['errors'].append(dict(step=INDEX,error=str(error)));save();unreal.log_error(str(error))
        unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()


HANDLE=unreal.register_slate_post_tick_callback(tick)
