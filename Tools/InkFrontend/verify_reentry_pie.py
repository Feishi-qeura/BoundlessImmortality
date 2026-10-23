"""Actual PIE repro/regression: return from void, then StartGame on the same pawn."""
import json,time
from pathlib import Path
import unreal
OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/VisualRefactor20261022/Reentry';OUT.mkdir(parents=True,exist_ok=True)
L=unreal.InkFrontendEditorLibrary;REPORT=dict(checks=[],passed=False);INDEX=0;NEXT=0;F=D=P=W=S=None;SAFE=None
def save():
    REPORT['passed']=REPORT.get('finished',False) and all(r['passed'] for r in REPORT['checks']) and not REPORT.get('error')
    (OUT/'report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
def check(n,v,actual=None):REPORT['checks'].append(dict(name=n,passed=bool(v),actual=actual));save()
def tick(dt):
    global INDEX,NEXT,F,D,P,W,S,SAFE
    if time.monotonic()<NEXT:return
    try:
        delay=2
        if INDEX==0:L.begin_frontend_pie(720,1280);delay=12
        elif INDEX==1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+1;return
            W=worlds[0];D=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)[0];F=D.get_editor_property('frontend_widget')
            check('start',L.click_action(F,'StartGame'));delay=5
        elif INDEX==2:
            P=unreal.GameplayStatics.get_player_pawn(W,0);SAFE=P.get_actor_location();REPORT['safe']=[SAFE.x,SAFE.y,SAFE.z]
            gi=unreal.GameplayStatics.get_game_instance(W);S=next(s for s in unreal.ObjectIterator(unreal.InkSaveSubsystem) if s.get_outer()==gi)
            L.capture_frontend_viewport(str(OUT/'hud-before.png'));F.open_feature_panel('SignIn','');delay=2
        elif INDEX==3:
            L.capture_frontend_viewport(str(OUT/'signin-before.png'));F.show_page('HUD')
            P.set_actor_location(SAFE+unreal.Vector(50000,50000,-1000),False,False)
            P.get_movement_component().set_movement_mode(unreal.MovementMode.MOVE_FALLING)
            check('isolated out-of-island position save',S.save_slot(1,P))
            D.return_to_main_menu();delay=2
        elif INDEX==4:check('restart from void',L.click_action(F,'StartGame'));delay=3
        elif INDEX==5:
            P=unreal.GameplayStatics.get_player_pawn(W,0);pos=P.get_actor_location()
            check('void restart returns to safe ground',(pos-SAFE).length()<1000,[pos.x,pos.y,pos.z])
            check('old falling velocity cleared',P.get_velocity().z>-10,P.get_velocity().z)
            L.capture_frontend_viewport(str(OUT/'reentry-after.png'))
            check('load isolated out-of-island position',S.load_slot(1,P));delay=3
        elif INDEX==6:
            pos=P.get_actor_location();check('bad saved position resolves to safe ground',(pos-SAFE).length()<1000,[pos.x,pos.y,pos.z])
            check('load clears falling velocity',P.get_velocity().z>-10,P.get_velocity().z)
            D.return_to_main_menu();delay=4
        elif INDEX==7:check('normal restart',L.click_action(F,'StartGame'));delay=2
        elif INDEX==8:
            check('normal return remains on island',(P.get_actor_location()-SAFE).length()<1000)
            S.delete_slot(1);REPORT['finished']=True;save()
            unreal.unregister_slate_post_tick_callback(H);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor();return
        INDEX+=1;NEXT=time.monotonic()+delay
    except Exception as e:
        REPORT['error']=repr(e);save();unreal.unregister_slate_post_tick_callback(H);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
H=unreal.register_slate_post_tick_callback(tick)
