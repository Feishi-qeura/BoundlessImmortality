"""Rendered PIE: actual player-entry Slate clicks, native component clicks and real local HTTP authority."""
import gc
import json
from pathlib import Path
import time
import traceback
from urllib.request import Request,urlopen
import unreal

HERE=Path(__file__).resolve().parent
OUT=HERE/'pie-10-frame-final';OUT.mkdir(exist_ok=True)
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
    path=HERE/'engine-qa-service-04/requests.jsonl'
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



step('real main entry',lambda:click('StartGame'),4)
step('active authority event',lambda:ck('actual launch active',state().get('launch',{}).get('status')=='Active'))
step('final frame portrait capture',lambda:capture('frame-portrait'))
step('native frame actual Slate click',lambda:ck('native countdown Slate opens launch',ordinary('LaunchCountdownHitTarget')))
step('actual countdown route',lambda:page_is('LaunchEvent'))
step('return to HUD',lambda:click('Ext_Close'))
step('Wuling remains accessible',lambda:click('OpenWuling'))
step('actual Wuling route',lambda:page_is('Wuling'))
step('close Wuling',lambda:click('Ext_Close'))
step('landscape actual viewport',lambda:ck('actual landscape resize',L.set_pie_viewport_size(1280,720)),2)
step('final frame landscape capture',lambda:capture('frame-landscape'))
step('landscape countdown Slate click',lambda:ck('landscape countdown opens launch',ordinary('LaunchCountdownHitTarget')))
step('landscape actual route',lambda:page_is('LaunchEvent'))
step('close launch for narrow',lambda:click('Ext_Close'))
step('narrow actual viewport',lambda:ck('actual narrow portrait resize',L.set_pie_viewport_size(680,1280)),2)
step('final frame narrow capture',lambda:capture('frame-narrow'))
step('narrow countdown Slate click',lambda:ck('narrow countdown opens launch',ordinary('LaunchCountdownHitTarget')))
step('narrow actual route',lambda:page_is('LaunchEvent'))
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
