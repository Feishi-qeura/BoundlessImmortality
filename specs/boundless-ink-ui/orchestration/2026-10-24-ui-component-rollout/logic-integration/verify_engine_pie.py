"""Rendered PIE: actual player-entry Slate clicks, native component clicks and real local HTTP authority."""
import gc
import json
from pathlib import Path
import time
import traceback
from urllib.request import Request,urlopen
import unreal

HERE=Path(__file__).resolve().parent
OUT=HERE/'pie-07';OUT.mkdir(exist_ok=True)
L=unreal.InkFrontendEditorLibrary
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
R=dict(checks=[],captures=[],steps=[],finished=False,scope='Rendered Windows PIE with actual Slate input; isolated declared fixtures; no visual/user approval')
F=D=S=W=None
I=-2;NEXT=0;START=time.monotonic();STEPS=[];QUIT=[None];OLD_DAY=[''];UPGRADE=['']


def save():
    R['passed']=R['finished'] and not R.get('error') and all(v['passed'] for v in R['checks'])
    (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding='utf-8')


def ck(name,value,actual=None):R['checks'].append(dict(name=name,passed=bool(value),actual=actual));save()
def snap():return json.loads(L.describe_widget(F))
def state():return json.loads(S.get_snapshot_json())
def control(**kwargs):
    response=json.loads(urlopen(Request('http://127.0.0.1:18789/qa/control',data=json.dumps(kwargs).encode(),headers={'Content-Type':'application/json'}),timeout=8).read())
    if 'upgrade_item' in response['control']:UPGRADE[0]=response['control']['upgrade_item']
    R.setdefault('fixture_controls',[]).append(kwargs)


def audit():
    path=HERE/'engine-qa-service-07/requests.jsonl'
    return [json.loads(line) for line in path.read_text(encoding='utf-8').splitlines() if line]


def last_reply(operation):
    rows=[v for v in audit() if v.get('command')==operation and v.get('origin')=='actual-http-command']
    return rows[-1]['response'] if rows else {}


def reveal_widget(widget):
    ancestor=widget;seen=set()
    while ancestor and ancestor.get_path_name() not in seen:
        seen.add(ancestor.get_path_name());parent=ancestor.get_parent()
        if parent:
            if isinstance(parent,unreal.ScrollBox):parent.scroll_widget_into_view(widget,False)
            ancestor=parent
        else:
            owner=ancestor.get_outer()
            while owner and not isinstance(owner,unreal.UserWidget):owner=owner.get_outer()
            ancestor=owner if owner!=ancestor else None


def reveal(action):
    button=next((b for b in snap()['buttons'] if b['action']==action and b['visible']),None)
    if button:reveal_widget(unreal.find_object(None,button['object']))


def click(action):
    hit=L.click_action(F,action)
    R.setdefault('player_entry_clicks',[]).append(dict(action=action,slate_handled=bool(hit),page=str(F.get_editor_property('current_page'))))
    ck('Slate '+action,hit)


def capture(tag):
    L.move_frontend_pointer_outside(F);path=OUT/(tag+'.png')
    ck('rendered capture '+tag,L.capture_frontend_viewport(str(path)))
    (OUT/(tag+'.json')).write_text(json.dumps(snap(),ensure_ascii=False,indent=2),encoding='utf-8')
    R['captures'].append(str(path))
    for widget in unreal.ObjectIterator(unreal.InkLaunchHudCountdownWidget):
        if widget.get_world()!=W:continue
        texture=widget.get_editor_property('frame_texture')
        R.setdefault('countdown_resource_readback',[]).append(dict(capture=tag,object=widget.get_path_name(),texture=texture.get_path_name() if texture else None))
    for widget in unreal.ObjectIterator(unreal.Image):
        if widget.get_name()!='LaunchCountdownFrame' or widget.get_world()!=W:continue
        try:
            brush=widget.get_editor_property('brush');texture=brush.get_editor_property('resource_object')
            R.setdefault('countdown_image_readback',[]).append(dict(capture=tag,object=widget.get_path_name(),texture=texture.get_path_name() if texture else None,draw_as=str(brush.get_editor_property('draw_as')),brush_image_size=str(brush.get_editor_property('image_size')),visibility=str(widget.get_visibility()),opacity=widget.get_render_opacity()))
        except Exception as error:R.setdefault('readback_errors',[]).append(str(error))


def ordinary(name,item=None):
    prefix=F.get_editor_property('extension_pages').get(str(F.get_editor_property('current_page')))
    prefix=prefix.get_path_name() if prefix else ''
    candidates=[]
    for button in unreal.ObjectIterator(unreal.Button):
        if button.get_name()!=name:continue
        if item:
            owner=button.get_outer()
            while owner and not isinstance(owner,unreal.InkPlayerMaterialSlotWidget):owner=owner.get_outer()
            if not owner or str(owner.get_editor_property('item_id'))!=item:continue
        candidates.append(button)
    for button in candidates:
        if L.click_rollout_button(F,button):return True
    return False


def reveal_material(item):
    for widget in unreal.ObjectIterator(unreal.InkPlayerMaterialSlotWidget):
        if str(widget.get_editor_property('item_id'))==item and widget.get_parent():reveal_widget(widget)


def selected(item):ck('real material Slate selection '+item,ordinary('MaterialSlotButton',item))
def page_is(name):ck('actual player route '+name,str(F.get_editor_property('current_page'))==name,str(F.get_editor_property('current_page')))
def no_receipt():
    if F.get_editor_property('reward_receipt_panel'):
        key=unreal.Key();key.set_editor_property('key_name','Escape');ck('receipt close by keyboard',L.press_frontend_key(F,key))
def step(name,fn,delay=.9):STEPS.append((name,fn,delay))


step('main start via actual button',lambda:click('StartGame'),4)
step('connected',lambda:ck('real authority connected',S.is_connected()))
step('hud',lambda:(page_is('HUD'),capture('hud')))
step('activities entry',lambda:click('OpenActivities'))
step('activities capture',lambda:capture('activities'))
step('daily category',lambda:click('Ext_ActivityCycle_daily'))
step('daily actual page',lambda:(page_is('DailyActivities'),capture('daily-activities')))
step('close daily',lambda:click('Ext_Close'))
step('sign entry',lambda:click('OpenSignIn'))
step('sign native claim',lambda:ck('native claim via real Slate',ordinary('JourneySignInClaimButton')),2)
step('sign confirmed',lambda:ck('authority signed after UI claim',state().get('activities',{}).get('signed')))
step('duplicate claim gated',lambda:ck('signed native button refuses duplicate Slate click',not ordinary('JourneySignInClaimButton')))
step('sign capture',lambda:capture('sign-in'))
step('month tab',lambda:click('Ext_GroupTab_MonthlySignIn'))
step('month capture',lambda:capture('monthly-sign-in'))
step('native calendar inspect',lambda:ck('calendar cell via real Slate',ordinary('JourneyCalendarDayHitTarget')))
step('monthly chest reveal',lambda:reveal('Ext_ChestPreview_0'))
step('monthly chest inspect',lambda:click('Ext_ChestPreview_0'))
step('monthly claim reveal',lambda:reveal('Ext_ChestRewardClaim'))
step('monthly claim',lambda:click('Ext_ChestRewardClaim'),2)
step('monthly real receipt',lambda:ck('real monthly transaction',last_reply('ClaimSignInChest').get('status')=='success',last_reply('ClaimSignInChest').get('error_code')))
step('receipt dismiss',no_receipt)
step('close sign',lambda:click('Ext_Close'))
step('rank entry',lambda:click('OpenLeaderboard'))
step('anchor rank',lambda:click('RankAnchor'))
step('rank capture',lambda:capture('leaderboard'))
step('rank close',lambda:click('RankClose'))
step('map entry',lambda:click('OpenExtensions'))
step('map capture',lambda:capture('world-map'))
step('map real node',lambda:click('Ext_AtlasNode_qingyun'))
step('world detail',lambda:(page_is('JourneyWorldDetail'),capture('world-detail')))
step('map close',lambda:click('Ext_Close'))
step('wuling entry',lambda:click('OpenWuling'))
step('guild name player input fixture',lambda:next(w for w in unreal.ObjectIterator(unreal.EditableTextBox) if w.get_name()=='SectNameInput' and w.get_world()==W).set_text('烟雨测试武灵'))
step('guild create reveal',lambda:reveal('Ext_SectCreate'))
step('guild actual create',lambda:click('Ext_SectCreate'),2)
step('guild confirmed',lambda:ck('real guild from UI create',bool(state().get('guild'))))
step('guild close for launch',lambda:click('Ext_Close'))
step('launch real entry',lambda:click('OpenLaunchEvent'))
step('launch primary reveal',lambda:reveal('Ext_LaunchPrimary'))
step('launch real start',lambda:click('Ext_LaunchPrimary'),2)
step('launch confirmed',lambda:ck('real UI launch start',bool(state().get('launch'))))
step('launch close',lambda:click('Ext_Close'))
step('active HUD frame capture',lambda:capture('hud-active-launch'))
step('wuling reopen',lambda:click('OpenWuling'))
step('construction reveal',lambda:reveal('Ext_Open_WulingConstruction'))
step('construction player entry',lambda:click('Ext_Open_WulingConstruction'))
step('tools menu',lambda:click('Ext_ToolNavigation'))
step('break route reveal',lambda:reveal('Ext_ToolRoute_2'))
step('break route actual click',lambda:click('Ext_ToolRoute_2'),3)
step('break actual page',lambda:(page_is('WulingBreakthrough'),capture('breakthrough-materials')))
step('material reveal',lambda:reveal_material('MAT_BR04_C01_G1'))
step('material real quote',lambda:selected('MAT_BR04_C01_G1'),2)
step('native quote no display fields',lambda:ck('native quote arrived with display gaps',bool(state().get('breakthrough_quote')) and 'q_normalized' not in state()['breakthrough_quote']))
step('unsafe declared fixture',lambda:control(safe=False))
step('unsafe prepare',lambda:click('Ext_ToolPrepare'),2)
step('unsafe authority refusal',lambda:ck('actual UE request rejected NOT_SAFE',last_reply('PrepareBreakthrough').get('error_code')=='NOT_SAFE'))
step('safe pending and concurrent version fixture',lambda:control(safe=True,delay=2,bump_next='PrepareBreakthrough'))
step('valid prepare',lambda:click('Ext_ToolPrepare'),.15)
step('pending native gate',lambda:ck('UE subsystem reports pending busy',S.is_busy()),.1)
step('pending duplicate click',lambda:ck('busy UI refuses duplicate prepare',not L.click_action(F,'Ext_ToolPrepare')),6)
step('prepare response',lambda:(control(delay=0),ck('actual UI prepare succeeded',last_reply('PrepareBreakthrough').get('status')=='success'),ck('authority entered Prepare',(state().get('tribulation') or {}).get('status')=='Prepare'),capture('prepare-confirmed')))
step('stale same-id recovery',lambda:ck('UE stale rejection and same request-id retry',any(v.get('response',{}).get('error_code')=='STALE_VERSION' for v in audit()) and len({v['request']['request_id'] for v in audit() if v.get('origin')=='actual-http-command' and v.get('command')=='PrepareBreakthrough' and v.get('response',{}).get('error_code') in ('STALE_VERSION','',None)})==1))
step('advance actual trial fixture',lambda:control(advance_seconds=84),2)
step('actual settled trial',lambda:ck('real deterministic fixture settled success',(state().get('tribulation') or {}).get('status')=='Succeeded'))
step('dismiss result receipt',no_receipt)
step('grant rework fixture',lambda:control(upgrade_core=True),2)
step('rework mode reveal',lambda:reveal('Ext_ToolReworkMode'))
step('rework mode',lambda:click('Ext_ToolReworkMode'),2)
step('upgrade material reveal',lambda:reveal_material(UPGRADE[0]))
step('upgrade material quote',lambda:selected(UPGRADE[0]),2)
step('rework quote native',lambda:ck('actual rework quote kind',(state().get('breakthrough_quote') or {}).get('kind')=='rework'))
step('rework real submit',lambda:click('Ext_ToolPrepare'),2)
step('rework confirmed',lambda:(ck('actual UI ReworkQuality succeeded',last_reply('ReworkQuality').get('status')=='success'),capture('rework-confirmed')))
step('offline fixture',lambda:control(offline=True),2)
step('offline service observed',lambda:(ck('real HTTP offline observed',not S.is_connected()),capture('offline')))
step('restore fixture',lambda:control(offline=False),3)
step('reconnected',lambda:ck('real service reconnects',S.is_connected()))
step('capture old day',lambda:OLD_DAY.__setitem__(0,state().get('activities',{}).get('day')))
step('advance day',lambda:control(advance_seconds=86400),2)
step('server day changes',lambda:(ck('server cross-day reflected',state().get('activities',{}).get('day')!=OLD_DAY[0]),ck('server month boundary reflected',state().get('activities',{}).get('day','')[:7]!=OLD_DAY[0][:7])))
step('close tools',lambda:click('Ext_Close'))
step('sign after day',lambda:click('OpenSignIn'))
step('day new unsigned',lambda:(ck('new day sign state unsigned',state().get('activities',{}).get('signed') is False),capture('next-day-sign')))


def finish():
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play()
    def quit_tick(dt):
        if unreal.EditorLevelLibrary.get_pie_worlds(False):return
        unreal.unregister_slate_post_tick_callback(QUIT[0]);gc.collect();unreal.SystemLibrary.quit_editor()
    QUIT[0]=unreal.register_slate_post_tick_callback(quit_tick)


def tick(dt):
    global I,NEXT,F,D,S,W
    if time.monotonic()<NEXT:return
    try:
        if time.monotonic()-START>600:raise TimeoutError('Owned PIE batch exceeded 600 seconds')
        if I==-2:
            R['project_saved_dir']=unreal.Paths.project_saved_dir()
            if not Path(R['project_saved_dir']).resolve().is_relative_to(HERE):
                raise RuntimeError('PIE saved directory is not in the owned sandbox; refusing player entry')
            ck('begin rendered PIE',L.begin_frontend_pie(720,1280));I=-1;NEXT=time.monotonic()+12;return
        if I==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+1;return
            W=worlds[0];directors=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)
            if not directors:NEXT=time.monotonic()+1;return
            D=directors[0];F=D.get_editor_property('frontend_widget')
            if not F:NEXT=time.monotonic()+1;return
            gi=unreal.GameplayStatics.get_game_instance(W)
            S=next(s for s in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if s.get_outer()==gi)
            I=0;NEXT=time.monotonic()+2;return
        if I>=len(STEPS):R['finished']=True;save();finish();return
        name,fn,delay=STEPS[I];R['current_step']=name;fn();R['steps'].append(name);I+=1;NEXT=time.monotonic()+delay;save()
    except Exception:
        R['error']=traceback.format_exc();save();finish()


HANDLE=unreal.register_slate_post_tick_callback(tick)
