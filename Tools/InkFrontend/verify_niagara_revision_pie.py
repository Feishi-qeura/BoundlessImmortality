"""Rendered screenshot corrections, explicit transient identities and real CPU particle motion."""
from pathlib import Path
import json,time,traceback,struct
import unreal

OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/NiagaraUiRevision20261022/RenderedDelivery'
OUT.mkdir(parents=True,exist_ok=True)
L=unreal.InkFrontendEditorLibrary
R=dict(checks=[],captures=[],particle_samples={},boundary='Windows rendered PIE with explicit local fixtures; no mobile or production avatar-provider test',finished=False)
I=-2;NEXT=0;F=D=C=W=H=None;PREVIOUS={};ROWS=[];STEPS=[]

def check(n,v,a=None):R['checks'].append(dict(name=n,passed=bool(v),actual=a))
def save():
    R['passed']=R['finished'] and not R.get('error') and all(x['passed'] for x in R['checks'])
    (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding='utf-8')
def data():return json.loads(L.describe_widget(F))
def capture(tag):
    L.move_frontend_pointer_outside(F);p=OUT/(tag+'.png');check('pixels '+tag,L.capture_frontend_viewport(str(p)))
    s=data();(OUT/(tag+'.json')).write_text(json.dumps(s,ensure_ascii=False,indent=2),encoding='utf-8');R['captures'].append(str(p));save();return s
def effects(active=True):
    return [w for w in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget) if w.get_effect_widget() and (not active or w.is_effect_active())]
def sample(tag):
    values=[]
    for w in effects():
        fx=w.get_effect_widget();c=fx.get_niagara_component()
        if not c:continue
        info=json.loads(unreal.InkCultivationEditorLibrary.inspect_component(c))
        info.update(name=w.get_name(),object=w.get_path_name(),quality=w.get_effect_quality(),system=fx.get_editor_property('niagara_system_reference').get_path_name())
        values.append(info)
    R['particle_samples'][tag]=values;save();return values
def step(n,fn,delay=1):STEPS.append((n,fn,delay))
def click(a):check('Slate click '+a,L.click_action(F,a))
def setup():
    global ROWS
    D.set_actor_tick_enabled(False);C.set_editor_property('persist_local_records',False)
    anchor=C.get_anchor_id();C.register_participant(anchor,'主播 · 本地验证','',True)
    for actor in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.LiveGMOverlay):
        adapter=actor.get_editor_property('audience')
        if adapter:adapter.add_simulated_audience(10);break
    R['identity_fixture']='Explicit cached local textures; not a real streamer profile download'

def check_main():
    s=capture('main-provided-normal');buttons=[b for b in s['buttons'] if b['visible'] and b['name'].startswith('Main_')]
    check('provided main has exact four actions',[b['action'] for b in buttons]==['StartGame','OpenSaves','OpenSettings','RequestQuit'],buttons)
    check('provided main uses exact supplied art',all('M_ProvidedMenu' in b['resource'] for b in buttons))
    check('provided Continue means save selection',any(b['action']=='OpenSaves' and b.get('label')=='继续游戏' for b in buttons))
    L.hover_action(F,'StartGame');capture('main-provided-hover');L.move_frontend_pointer_outside(F)
def profiles():
    records=json.loads(C.export_records());ids=[]
    titles=['Novice','LeftGuardian','SevenStar','SoulEater','Godslayer','DivineBlade','Cyberspace']
    for n,r in enumerate(records['records']):
        r['realm_index']=min(n*4,33);r['title_id']=titles[n%7];r['version']=r.get('version',0)+1;ids.append(r['participant_id'])
    check('transient fixture records imported',C.import_records(json.dumps(records,ensure_ascii=False),False))
    for cat in ('Anchor','CultivationSpeed','Damage'):
        for n in range(1,13):
            e=unreal.InkLeaderboardEntry();e.set_editor_property('participant_id',ids[min(n-1,len(ids)-1)])
            e.set_editor_property('display_name',unreal.Text('观众'+str(n).zfill(4)));e.set_editor_property('category',cat);e.set_editor_property('rank',n)
            e.set_editor_property('metric',unreal.Text(str(100-n)+'.70 /秒'));e.set_editor_property('numeric_metric',float(100-n))
            e.set_editor_property('realm_index',min(n*3,33));e.set_editor_property('realm',C.get_realm_name(min(n*3,33)))
            e.set_editor_property('title_id',titles[(n-1)%7]);ROWS.append(e)
    F.set_leaderboard_entries(ROWS)
def avatar1():
    t=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
    check('cached identity fixture accepted',L.set_frontend_test_anchor_avatar(F,'https://fixture.invalid/streamer-a.png',t))
def avatar2():
    t=unreal.load_asset('/Game/UI/Ink/NiagaraRevision20261022/Textures/T_CultivationPadded')
    check('changed identity fixture accepted',L.set_frontend_test_anchor_avatar(F,'https://fixture.invalid/streamer-b.png',t))
def avatar_check(expected):
    s=data()['anchor_status'];check('current identity URL reaches HUD '+expected,s['avatar_url'].endswith(expected),s)
    check('identity texture decoded/cache supplied '+expected,not s['avatar_placeholder'] and bool(s['avatar_texture']),s)
def hud_check(tag):
    s=capture(tag)
    nav=next(x for x in s['images'] if x['name']=='HUDCultivationIcon')
    check(tag+' standalone padded icon used','T_CultivationPadded' in nav['resource'],nav)
    check(tag+' square icon aspect',abs(nav['painted_width']-nav['painted_height'])<1,nav)
    for name in ('HUDBagIcon','HUDRankIcon','HUDMenuIcon'):
        icon=next(x for x in s['images'] if x['name']==name)
        check(tag+' complete standalone atlas '+name,'T_NavigationComplete' in icon['resource'] and abs(icon['painted_width']-icon['painted_height'])<1,icon)
    anchor=s['anchor_status'];check(tag+' three bars have enclosing panel space',abs(anchor['height']-184)<1,anchor)
    frame=next((x for x in s['images'] if x['name']=='TitleArtwork' and '.AnchorAvatarFrame.' in x['object']),None)
    check(tag+' avatar frame rendered',frame is not None)
    if frame:
        expected=[anchor['x']+84*anchor['width']/368,anchor['y']+60*anchor['height']/184]
        actual=[frame['x']+frame['painted_width']/2,frame['y']+frame['painted_height']/2]
        check(tag+' avatar and frame centers align',all(abs(a-b)<1 for a,b in zip(actual,expected)),dict(actual=actual,expected=expected))
    check(tag+' bottom navigation uses transparent-to-opaque fade',any(x['name']=='HUDNavigationVeil' and 'M_InkNavigationFade' in x['resource'] for x in s['images']))
def board_check(tag,compact=False):
    s=capture(tag);prefix='RankHUD' if compact else 'RankAnchor'
    frame=next(x for x in s['images'] if x['name']==prefix+'Frame')
    variant='Compact' if compact else 'Portrait' if frame['width']<frame['height'] else 'Landscape'
    check(tag+' generated gold ink backing matches panel aspect','M_InkGoldRank'+variant in frame['resource'],frame['resource'])
    if compact:
        buttons=[x for x in s['buttons'] if x['action'] in ('HUDRankSpeed','HUDRankDamage','HUDRankClose')]
        check(tag+' three types contained',len(buttons)==3 and all(x['x']>=frame['x'] and x['y']>=frame['y'] and x['x']+x['painted_width']<=frame['x']+frame['painted_width']+1 and x['y']+x['painted_height']<=frame['y']+frame['painted_height']+1 for x in buttons),buttons)
    else:
        for n in range(1,4):check(tag+' rank numeral '+str(n),any(x['name']==prefix+'Number_'+str(n) and x['visible'] and x['text']==str(n) for x in s['texts']))
    check(tag+' real Niagara title widgets rendered',any(w.get_name().startswith(prefix+'TitleEffect_') for w in effects()))
def motion_check():
    a=R['particle_samples']['hud-a'];b=sample('hud-b');moved=[]
    for x in a:
        y=next((v for v in b if v['object']==x['object']),None)
        if not y:continue
        for p in x.get('particles',[]):
            q=next((v for v in y.get('particles',[]) if v['id']==p['id']),None)
            if q and sum((u-v)**2 for u,v in zip(p['position'],q['position']))>.01:moved.append(x['name']);break
    within=[x['name'] for x in b if any(p.get('displacement',0)>.0001 for p in x.get('particles',[]))];check('actual Niagara particle positions move',len(moved)>=2 or len(within)>=2,dict(across_frames=moved,within_frames=within))
    check('HUD frame and realm CPU particles',any(x['name']=='AnchorAvatarFrame' and sum(x.get('counts',[]))>0 for x in b) and any(x['name']=='AnchorRealmTitle' and sum(x.get('counts',[]))>0 for x in b))
def reduced_check():
    sample('reduced');check('reduced motion has zero title systems',len(effects())==0,[x.get_name() for x in effects()]);capture('hud-reduced-motion')
def menu_check():
    check('menu reclaims title effects',len(effects())==0,[x.get_name() for x in effects()]);capture('main-return')

step('fixture setup',setup,1)
step('fixture profiles',profiles,2)
step('HUD default',lambda:hud_check('hud-placeholder-720x1280'))
step('avatar a',avatar1,1)
step('avatar a reaches HUD',lambda:avatar_check('streamer-a.png'))
step('avatar b',avatar2,1)
step('avatar b reaches HUD',lambda:avatar_check('streamer-b.png'))
step('clear test avatar before visual review',lambda:C.register_participant(C.get_anchor_id(),'主播 · 本地验证','',True),1)
step('shield sample',lambda:F.get_editor_property('anchor_status_widget').set_shield_presentation(60,100),1)
step('HUD animated a',lambda:(capture('hud-particles-a'),sample('hud-a')),.45)
step('HUD actual motion',lambda:(motion_check(),capture('hud-particles-b')),1)
step('open compact board',lambda:click('HUDRankView'),2)
step('compact board',lambda:board_check('compact-720x1280',True))
step('switch damage',lambda:click('HUDRankDamage'),1)
step('compact damage',lambda:board_check('compact-damage-720x1280',True))
step('full board',lambda:F.show_page('Leaderboard'),2)
step('full board portrait',lambda:board_check('full-720x1280'))
step('particle tiers',lambda:sample('leaderboard-tiers'))
step('full board landscape resize',lambda:L.set_pie_viewport_size(1280,720),2)
step('full board landscape',lambda:board_check('full-1280x720'))
step('back HUD',lambda:F.show_page('HUD'),1)
for width,height in [(360,640),(540,960),(720,1280),(1080,1920),(1080,2340),(960,720),(1280,720),(1920,1080),(2340,1080)]:
    step('resize '+str((width,height)),lambda w=width,h=height:L.set_pie_viewport_size(w,h),1)
    step('HUD '+str((width,height)),lambda tag='hud-'+str(width)+'x'+str(height):hud_check(tag))
step('reduced motion on',lambda:F.set_editor_property('reduced_motion',True),1)
step('reduced motion check',reduced_check)
step('reduced motion off',lambda:F.set_editor_property('reduced_motion',False),1)
step('return menu',lambda:D.return_to_main_menu(),1)
step('menu cleanup check',menu_check)

def tick(dt):
    global I,NEXT,F,D,C,W,H
    if time.monotonic()<NEXT:return
    try:
        if I==-2:
            check('begin rendered PIE',L.begin_frontend_pie(720,1280));I=-1;NEXT=time.monotonic()+14;return
        if I==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+1;return
            W=worlds[0];D=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)[0]
            F=D.get_editor_property('frontend_widget');C=unreal.LiveWeaponEditorLibrary.get_cultivation_state(W);H=unreal.LiveWeaponEditorLibrary.get_live_interaction_state(W)
            if not F:NEXT=time.monotonic()+1;return
            check_main()
            started=D.start_game();check('start game',started,D.get_editor_property('presentation_status'))
            if not started:raise RuntimeError('StartGame did not enter HUD: '+str(D.get_editor_property('presentation_status')))
            I=0;NEXT=time.monotonic()+4;return
        if I>=len(STEPS):
            R['finished']=True;save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play()
            if '-inkrunpawnrolesafter' in unreal.SystemLibrary.get_command_line().lower() or '-inkruntitlemotionafter' in unreal.SystemLibrary.get_command_line().lower():
                def run_next(delta):
                    if unreal.EditorLevelLibrary.get_pie_worlds(False):return
                    unreal.unregister_slate_post_tick_callback(CHAIN[0])
                    name='verify_title_motion_pie.py' if '-inkruntitlemotionafter' in unreal.SystemLibrary.get_command_line().lower() else 'verify_menu_pawn_roles.py'
                    script=Path(unreal.Paths.project_dir())/'Tools/InkFrontend'/name
                    exec(compile(script.read_text(encoding='utf-8'),str(script),'exec'),{'__name__':'__pawn_role_qa__'})
                CHAIN[0]=unreal.register_slate_post_tick_callback(run_next)
            else:
                # Drop Python-held UObject wrappers before the editor tears down.
                F=D=C=W=H=None
                def quit_after_pie(delta):
                    if unreal.EditorLevelLibrary.get_pie_worlds(False):return
                    unreal.unregister_slate_post_tick_callback(CHAIN[0])
                    import gc
                    gc.collect()
                    unreal.SystemLibrary.quit_editor()
                CHAIN[0]=unreal.register_slate_post_tick_callback(quit_after_pie)
            return
        name,fn,delay=STEPS[I];R['current_step']=name;fn();I+=1;NEXT=time.monotonic()+delay;save()
    except Exception:
        R['error']=traceback.format_exc();save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
HANDLE=unreal.register_slate_post_tick_callback(tick)
CHAIN=[None]
