"""Short final rendered checks, including declared eight-second incense fixture."""
from pathlib import Path
import json
import time
import unreal

ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/GameplayPlan20261021/FinalInteractions';OUT.mkdir(parents=True,exist_ok=True)
LIB=unreal.InkFrontendEditorLibrary;W=HOST=FRONTEND=WORLD=PAWN=None;INDEX=-2;NEXT=0;HANDLE=None;START=time.monotonic();BASE_SPEED=0;BASE_PARTICLES=set();EXTRA_PARTICLES=set()
REPORT=dict(boundary='Windows rendered PIE; seeded isolated fixture; PROP03 duration8s only in QA manifest; not device/platform',checks=[],errors=[],passed=False)


def check(name,condition,actual=None):REPORT['checks'].append(dict(name=name,passed=bool(condition),actual=actual))
def state():return json.loads(W.get_snapshot_json())
def particles():return {c.get_path_name() for c in PAWN.get_components_by_class(unreal.NiagaraComponent) if c.get_asset() and 'InkCultivation' in c.get_asset().get_name()}
def save():
    REPORT['passed']=REPORT.get('sequence_finished',False) and not REPORT['errors'] and all(r['passed'] for r in REPORT['checks'])
    (OUT/'report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
def capture(name):check('capture '+name,LIB.capture_frontend_viewport(str(OUT/(name+'.png'))));save()
def click(action):result=LIB.click_action(FRONTEND,action);check('Slate '+action,result);return result
def command(op,args):check('native '+op,W.submit_prototype_command(op,json.dumps(args)))
def baseline():
    global BASE_SPEED,BASE_PARTICLES,PAWN
    PAWN=unreal.GameplayStatics.get_player_pawn(WORLD,0);BASE_SPEED=PAWN.get_movement_component().get_editor_property('max_walk_speed');BASE_PARTICLES=particles()
    check('configured ink source exists',bool(BASE_PARTICLES),sorted(BASE_PARTICLES))
def check_effect():
    global EXTRA_PARTICLES
    EXTRA_PARTICLES=particles()-BASE_PARTICLES;check('separate incense component active',bool(EXTRA_PARTICLES),sorted(EXTRA_PARTICLES))
STEPS=[('start',lambda:click('StartGame'),8),('baseline',baseline,1),('boots',lambda:command('Equip',dict(item_id='EQ_BOOTS_T1')),3),
       ('movement',lambda:check('actual movement receives2percent bonus',abs(PAWN.get_movement_component().get_editor_property('max_walk_speed')-BASE_SPEED*1.02)<.1),1),
       ('unequip',lambda:command('Unequip',dict(item_id='EQ_BOOTS_T1')),3),('movement restore',lambda:check('actual movement baseline restored',abs(PAWN.get_movement_component().get_editor_property('max_walk_speed')-BASE_SPEED)<.1),1),
       ('incense',lambda:command('UseProp',dict(item_id='PROP_03')),2),('effect active',check_effect,1),('effect capture',lambda:capture('incense-active'),9),
       ('effect cleanup',lambda:check('incense released without altering configured component',particles()==BASE_PARTICLES,sorted(particles())),1),
       ('monthly',lambda:FRONTEND.open_feature_panel('MonthlySignIn',''),3),('calendar',lambda:capture('calendar-weekday-fixed'),1),
       ('breakthrough',lambda:FRONTEND.open_feature_panel('WulingBreakthrough',''),3),('mode',lambda:click('Ext_ToolReworkMode'),2),('mode capture',lambda:capture('quality-rework-mode'),1),
       ('activity',lambda:FRONTEND.open_feature_panel('WulingActivity',''),3),('tasks',lambda:click('Ext_ToolActivityTab_tasks'),1),('task capture',lambda:capture('launch-task-details'),1),
       ('guild',lambda:click('Ext_ToolActivityTab_guild'),1),('guild capture',lambda:capture('launch-guild-milestones'),1),
       ('race wide',lambda:LIB.set_pie_viewport_size(1280,720),3),('race',lambda:FRONTEND.open_feature_panel('WulingRace',''),3),('race capture',lambda:capture('race-all-controls-wide'),1)]
STEPS.extend([
    ('root wide',lambda:FRONTEND.open_feature_panel('Wuling',''),3),('root links',lambda:capture('wuling-root-wide'),1),
    ('management link',lambda:click('Ext_Open_WulingGuild'),3),
    ('join fixture member',lambda:check('QA member join',HOST.simulate_comment('utility-helper','组织测试剑灵','加入')),4),
    ('member select',lambda:click('Ext_RecordRow_0'),1),('remove fixture member',lambda:click('Ext_RecordRemove'),3),
    ('remove confirmed',lambda:check('selected member removed by authority',state()['counts']['members']==0),1),
    ('create own order',lambda:command('CreateListing',dict(item_id='MAT_HERB_T',count=1,unit_price=4)),3),
    ('orders',lambda:click('Ext_RecordTab_1'),3),('order select',lambda:click('Ext_RecordRow_0'),1),
    ('order cancel',lambda:click('Ext_RecordCancel'),3),('order confirmed',lambda:check('own order cancellation confirmed',state()['market']['own_listings'][0]['status']=='Cancelled'),1),
    ('order capture',lambda:capture('own-orders-wide'),1),('offline',lambda:click('Ext_RecordTab_2'),2),
    ('offline capture',lambda:capture('offline-record-wide'),1),('rights',lambda:click('Ext_RecordTab_3'),2),('rights capture',lambda:capture('sword-rights-wide'),1)])



def tick(delta):
    global INDEX,NEXT,W,HOST,FRONTEND,WORLD
    now=time.monotonic()
    if now<NEXT:return
    try:
        if now-START>200:raise TimeoutError('final interaction watchdog')
        if INDEX==-2:LIB.begin_frontend_pie(720,1280);INDEX=-1;NEXT=now+12;return
        if INDEX==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=now+1;return
            WORLD=worlds[0];gi=unreal.GameplayStatics.get_game_instance(WORLD)
            W=next(o for o in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if o.get_outer()==gi)
            HOST=next(o for o in unreal.ObjectIterator(unreal.LiveInteractionSubsystem) if o.get_outer()==gi)
            for d in unreal.GameplayStatics.get_all_actors_of_class(WORLD,unreal.InkMenuDirector):
                FRONTEND=d.get_editor_property('frontend_widget')
                if FRONTEND:break
            if not FRONTEND:NEXT=now+1;return
            INDEX=0;NEXT=now+2;return
        if INDEX>=len(STEPS):REPORT['sequence_finished']=True;save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor();return
        name,fn,delay=STEPS[INDEX];fn();INDEX+=1;NEXT=now+delay;save()
    except Exception as error:
        REPORT['errors'].append(dict(step=INDEX,error=repr(error)));save();unreal.log_error(str(error));unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()


HANDLE=unreal.register_slate_post_tick_callback(tick)
