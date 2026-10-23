"""Runs inside each actual UE process; all gameplay commands use native client."""
from pathlib import Path
import json
import os
import time
import unreal

ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/GameplayPlan20261021/TwoUE'
SIDE='a' if '-WulingQASide=a' in unreal.SystemLibrary.get_command_line() else 'b'
LIB=unreal.InkFrontendEditorLibrary;W=HOST=FRONTEND=None;STAGE=0;NEXT=0;START=time.monotonic();SENT=set();ACKED=set();RUN='';HANDLE=None


def save(event,**extra):
    temp=OUT/(SIDE+'.pending');temp.write_text(json.dumps(dict(event=event,run=RUN,pid=os.getpid(),**extra),ensure_ascii=False),encoding='utf-8');temp.replace(OUT/(SIDE+'.json'))


def close():
    unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()


def tick(delta):
    global W,HOST,FRONTEND,STAGE,NEXT,RUN
    now=time.monotonic()
    if now<NEXT:return
    NEXT=now+.15
    try:
        instruction=json.loads((OUT/'instruction.json').read_text(encoding='utf-8'));RUN=instruction['run'];event=instruction['event']
        if event=='quit':close();return
        if now-START>360:raise TimeoutError('UE race client watchdog')
        if STAGE==0:LIB.begin_frontend_pie(320,568);STAGE=1;NEXT=now+12;return
        if STAGE==1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            world=worlds[0];gi=unreal.GameplayStatics.get_game_instance(world)
            W=next(o for o in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if o.get_outer()==gi)
            HOST=next(o for o in unreal.ObjectIterator(unreal.LiveInteractionSubsystem) if o.get_outer()==gi)
            for d in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.InkMenuDirector):
                FRONTEND=d.get_editor_property('frontend_widget')
                if FRONTEND:break
            if not FRONTEND:return
            # NullRHI has no rendered hit-test window; invoke the same native action dispatcher.
            FRONTEND.trigger_action('StartGame')
            STAGE=2;return
        if not W.is_connected() or W.is_busy():return
        state=json.loads(W.get_snapshot_json())
        if STAGE==2:
            if not state.get('guild'):
                if 'guild' not in SENT:W.create_guild('青云甲' if SIDE=='a' else '青云乙');SENT.add('guild')
                return
            if 'join' not in SENT:HOST.simulate_comment('ue-helper-'+SIDE,'竞采剑灵'+SIDE,'加入');SENT.add('join');return
            if state.get('counts',{}).get('members',0)<1:return
            if 'queue' not in SENT:W.queue_race('NODE_STONE');SENT.add('queue');return
            race=state.get('race')
            if not race:return
            save('ready',race_start=race['start']);STAGE=3;return
        if event in ('boot','quit') or event in ACKED:return
        if abs(state.get('server_utc',0)-instruction['utc'])>.01:return
        if event=='warmup' or event.startswith('presence-'):save(event);ACKED.add(event);return
        race=state.get('race') or {};user=state['identity']['user'];team=next((t for t in race.get('teams',[]) if t['anchor']==user),None)
        if not team:return
        if event=='result':
            if race['status']!='Settled':return
            lease=state.get('lease') or {};save(event,score=team['score'],stone=state.get('wallet',{}).get('CUR_STONE'),material=state.get('inventory',{}).get('MAT_BREAK'),lease_seconds=lease.get('end',0)-lease.get('start',0));ACKED.add(event);return
        if race['status']!='Running':return
        kind=instruction['kind']
        if event not in SENT:
            if kind=='anchor':accepted=W.submit_prototype_command('SubmitRaceAction',json.dumps(dict(race_id=race['id'],kind='anchor')))
            else:accepted=HOST.simulate_comment('ue-helper-'+SIDE,'竞采剑灵'+SIDE,'共修' if kind=='representative' else instruction['action'])
            if not accepted:raise RuntimeError('native command rejected '+event)
            SENT.add(event);return
        expected=int(event.split('-')[1])+1
        actual=len(team['common'])+len(team['anchor_actions'])+sum(len(v) for v in team['responses'].values())
        if actual<expected:return
        save(event,score=team['score'],facts=actual);ACKED.add(event)
    except (json.JSONDecodeError,FileNotFoundError):return
    except Exception as error:
        save(event if 'event' in locals() else 'error',error=repr(error));unreal.log_error(str(error));close()


HANDLE=unreal.register_slate_post_tick_callback(tick)
