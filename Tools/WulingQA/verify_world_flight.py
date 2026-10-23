"""Actual rendered sword impacts; never calls SubmitStrike or changes actors."""
from pathlib import Path
import json
import sqlite3
import time
import unreal

ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/GameplayPlan20261021/WorldFlight'
LIB=unreal.InkFrontendEditorLibrary;W=FRONTEND=None;INDEX=0;NEXT=0;START=time.monotonic();HANDLE=None;STAGE_TIMES={};CAPTURED=set()
SAMPLE='-WulingSampleOnly' in unreal.SystemLibrary.get_command_line()
REPORT=dict(boundary='actual Windows rendered PIE sword impacts; realm0 and no gear/gifts; server skips unused waiting only',sample_only=SAMPLE,passed=False,errors=[])


def save():
    (OUT/('sample-report.json' if SAMPLE else 'world-report.json')).write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')


def finish():
    save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()


def jump(utc):
    temp=OUT/'control.pending';temp.write_text(json.dumps(dict(utc=utc)),encoding='utf-8');temp.replace(OUT/'control.json')


def tick(delta):
    global W,FRONTEND,INDEX,NEXT
    now=time.monotonic()
    if now<NEXT:return
    NEXT=now+.25
    try:
        if now-START>680:raise TimeoutError('world flight watchdog')
        if INDEX==0:LIB.begin_frontend_pie(720,1280);INDEX=1;NEXT=now+12;return
        if INDEX==1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            world=worlds[0];gi=unreal.GameplayStatics.get_game_instance(world)
            W=next(o for o in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if o.get_outer()==gi)
            for d in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.InkMenuDirector):
                FRONTEND=d.get_editor_property('frontend_widget')
                if FRONTEND:break
            if not FRONTEND:return
            if not LIB.click_action(FRONTEND,'StartGame'):return
            INDEX=2;return
        if not W.is_connected() or W.is_busy():return
        state=json.loads(W.get_snapshot_json())
        if INDEX==2:
            assert state['growth']['realm_index']==0,state['growth']
            assert not state['inventory'],state['inventory']
            if not W.start_world_round('ENC_SOLO','guard'):return
            INDEX=3;return
        round_=state.get('round')
        if not round_:return
        if INDEX==3:jump(round_['start']+61);INDEX=4;return
        stage=round_['stage'];completed=round_['completed']
        if round_['status']=='Running' and stage not in STAGE_TIMES:STAGE_TIMES[stage]=now
        if round_['status']=='Running' and stage not in CAPTURED:
            LIB.move_frontend_pointer_outside(FRONTEND);LIB.capture_frontend_viewport(str(OUT/f'stage-{stage}.png'));CAPTURED.add(stage)
        fixture=json.loads((OUT/'fixture.json').read_text(encoding='utf-8'))
        with sqlite3.connect('file:'+fixture['db'].replace('\\','/')+'?mode=ro',uri=True) as db:
            hits=[(created,json.loads(data)) for created,data in db.execute("SELECT created,data FROM fact WHERE kind='world_action' ORDER BY created") if json.loads(data).get('action')=='strike']
        REPORT.update(hit_count=len(hits),completed=completed,stage=stage,round_status=round_['status'])
        if SAMPLE and len(hits)>=12:
            elapsed=hits[-1][0]-hits[0][0];rate=(len(hits)-1)/elapsed
            REPORT.update(passed=True,hits_per_second=rate,observed_server_seconds=elapsed,base_damage=5,boss_seconds_at_source_hp=360/(rate*5),suggested_hp_factor=min(1,rate/.8),hits=hits)
            finish();return
        if SAMPLE and now-START>180:raise TimeoutError('could not observe 12 real accepted sword hits')
        if not SAMPLE:
            for done in completed:
                key='stage_'+str(done)+'_wall_seconds'
                if key not in REPORT:REPORT[key]=now-STAGE_TIMES[done]
            if stage>=2 and 1 not in completed:raise AssertionError('stage1 budget missed with actual flight')
            if stage>=3 and 2 not in completed:raise AssertionError('stage2 budget missed with actual flight')
            if 3 in completed and INDEX<7:jump(round_['start']+601);INDEX=7
            elif 2 in completed and INDEX<6:jump(round_['start']+421);INDEX=6
            elif 1 in completed and INDEX<5:jump(round_['start']+241);INDEX=5
            if round_['status'] in ('Settling','Completed') and 3 not in completed:raise AssertionError('boss budget missed with actual flight')
            if round_['status']=='Completed':
                assert state.get('wallet',{}).get('CUR_STONE')==80,state.get('wallet')
                assert state['inventory'].get('MAT_BREAK')==12,state['inventory']
                REPORT.update(passed=True,wallet=state['wallet'],inventory=state['inventory'],hits=hits);finish();return
        save()
    except Exception as error:REPORT['errors'].append(repr(error));unreal.log_error(str(error));finish()


HANDLE=unreal.register_slate_post_tick_callback(tick)
