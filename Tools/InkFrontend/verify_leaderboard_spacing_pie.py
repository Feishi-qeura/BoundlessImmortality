"""Transient leaderboard layout fixtures; never imports or saves project assets."""
from pathlib import Path
import json, time, traceback
import unreal

ROOT = Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/Codex/LeaderboardSpacing20261022' / ('Before' if '-rankbefore' in unreal.SystemLibrary.get_command_line().lower() else 'After')
OUT.mkdir(parents=True, exist_ok=True)
L = unreal.InkFrontendEditorLibrary
R = dict(checks=[], captures=[], finished=False)
F = D = None
INDEX = -2
NEXT = 0
STEPS = []
def check(name, value, actual=None): R['checks'].append(dict(name=name, passed=bool(value), actual=actual))
def save():
    R['passed'] = R['finished'] and not R.get('error') and all(c['passed'] for c in R['checks'])
    (OUT/'report.json').write_text(json.dumps(R, ensure_ascii=False, indent=2), encoding='utf-8')
def fixture(stress=True):
    D.set_actor_tick_enabled(False)
    rows=[]
    for category in ('Anchor','CultivationSpeed','Damage'):
        for rank in range(1,31):
            e=unreal.InkLeaderboardEntry()
            name='超长修士昵称用于边界检查'+str(rank) if stress else ['墨问','归云','听雨','青竹','望舒','渡尘','长风','知微'][(rank-1)%8]
            metric='12345678.90 /秒' if stress else str((31-rank)*3) if category=='Anchor' else f'{(31-rank)*125.5:.1f} /秒'
            for key,value in dict(participant_id='layout-'+str(rank), display_name=unreal.Text(name), category=category, rank=rank, metric=unreal.Text(metric), numeric_metric=12345678.9 if stress else (31-rank)*125.5, realm_index=(rank*3)%34, realm=unreal.Text('境界'), title_id=['Novice','DivineBlade','Cyberspace'][rank%3]).items():e.set_editor_property(key,value)
            if stress:e.set_editor_property('avatar_texture',unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
            rows.append(e)
            if stress and rank==2:e.set_editor_property('avatar_texture',unreal.load_asset('/Game/UI/Ink/Cultivation/T_RealmBanners'))
    F.set_leaderboard_entries(rows)
    F.set_leaderboard_entries_for_scope(rows,'Global')
    F.set_global_leaderboard_connection(True,unreal.Text('布局预览'))
def capture(tag):
    L.move_frontend_pointer_outside(F)
    path=OUT/(tag+'.png');check('capture '+tag,L.capture_frontend_viewport(str(path)))
    s=json.loads(L.describe_widget(F));(OUT/(tag+'.json')).write_text(json.dumps(s,ensure_ascii=False,indent=2),encoding='utf-8');R['captures'].append(str(path))
    prefix='RankHUD' if tag.startswith('compact') else 'RankAnchor'
    compact=prefix=='RankHUD'
    check(tag+' expected page is visible',s['page']==('HUD' if compact else 'Leaderboard'),s['page'])
    if compact:check(tag+' correct compact category',s['active_leaderboard']==('Damage' if 'damage' in tag else 'CultivationSpeed'),s['active_leaderboard'])
    images={i['name']:i for i in s['images'] if i['name'].startswith(prefix)}
    texts={i['name']:i for i in s['texts'] if i['name'].startswith(prefix)}
    for rank in range(1,31):
        art=images[prefix+'RowArt_'+str(rank)]
        art_widget=unreal.find_object(None,art['object']);pos=art_widget.slot.get_position();size=art_widget.slot.get_size()
        if rank<=10:
            check(tag+' wider row '+str(rank),size.x >= (264 if compact else 624 if tag.startswith('portrait') else 800))
            check(tag+' small outer row gap '+str(rank),pos.y<=2.01)
            # Offscreen widgets can retain painted geometry from a previous viewport.
            # The live row slot and list padding define the gap even when Slate culls paint.
            sizing=art_widget.get_parent().get_parent()
            padding=sizing.slot.get_editor_property('padding')
            check(tag+' tight consecutive rows '+str(rank),sizing.get_editor_property('height_override')-pos.y-size.y<=2.01 and padding.top+padding.bottom==0)
        for part in ('Number','Name','Metric','Avatar','AvatarPlaceholder','TitleEffect','RealmEffect'):
            key=prefix+part+'_'+str(rank)
            item=images.get(key) or texts.get(key)
            if not item:continue
            child=unreal.find_object(None,item['object']);cp=child.slot.get_position();cs=child.slot.get_size()
            check(tag+' artwork contains '+part+' '+str(rank),cp.x>=pos.x-.01 and cp.y>=pos.y-.01 and cp.x+cs.x<=pos.x+size.x+.01 and cp.y+cs.y<=pos.y+size.y+.01)
            check(tag+' isolated paint '+part+' '+str(rank),item['clipping']=='ClipToBoundsAlways')
        a=texts[prefix+'Name_'+str(rank)];b=texts[prefix+'Metric_'+str(rank)]
        check(tag+' separate nickname/metric '+str(rank),a['x']+a['painted_width']<=b['x']+.8)
        if rank<=3:
            child=unreal.find_object(None,a['object'])
            check(tag+' podium information raised '+str(rank),child.slot.get_position().y <= (20 if compact else 78 if tag.startswith('portrait') else 60))
        if not compact and tag.startswith('portrait') and 'sword' in tag and rank<=2:
            for part in ('TitleEffect','RealmEffect'):
                effect=next(fx for fx in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget) if fx.get_path_name().startswith(F.get_path_name()+'.') and fx.get_name()==prefix+part+'_'+str(rank))
                ep=effect.slot.get_position();es=effect.slot.get_size()
                check(tag+' podium effect clears bottom gold rim '+part+' '+str(rank),ep.y+es.y<=pos.y+size.y*.9)
        if rank==2:
            avatar=images[prefix+'Avatar_2'];source=unreal.load_asset('/Game/UI/Ink/Cultivation/T_RealmBanners')
            check(tag+' nonsquare avatar retains aspect',abs(avatar['painted_width']/avatar['painted_height']-source.blueprint_get_size_x()/source.blueprint_get_size_y())<.005)
        for fx in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget):
            if fx.get_path_name().startswith(F.get_path_name()+'.') and fx.get_name() in (prefix+'TitleEffect_'+str(rank),prefix+'RealmEffect_'+str(rank)):
                fp=fx.slot.get_position();fs=fx.slot.get_size()
                check(tag+' title/realm effect inside artwork '+fx.get_name(),fp.x>=pos.x-.01 and fp.y>=pos.y-.01 and fp.x+fs.x<=pos.x+size.x+.01 and fp.y+fs.y<=pos.y+size.y+.01)

def scroll_end(prefix):
    for scroll in unreal.ObjectIterator(unreal.ScrollBox):
        if scroll.get_path_name().startswith(F.get_path_name()+'.') and scroll.get_name()==prefix+'Scroll':
            scroll.scroll_to_end()

def scroll_start(prefix):
    for scroll in unreal.ObjectIterator(unreal.ScrollBox):
        if scroll.get_path_name().startswith(F.get_path_name()+'.') and scroll.get_name()==prefix+'Scroll':
            scroll.set_scroll_offset(0)

def capture_end(tag,prefix):
    L.move_frontend_pointer_outside(F)
    path=OUT/(tag+'.png');check('capture '+tag,L.capture_frontend_viewport(str(path)))
    state=json.loads(L.describe_widget(F));R['captures'].append(str(path))
    text=next(t for t in state['texts'] if t['name']==prefix+'Name_30')
    check(tag+' final participant reachable',text['painted_height']>0 and text['y']>=0 and text['y']+text['painted_height']<=state['height'])

def preview(tag):
    L.move_frontend_pointer_outside(F)
    path=OUT/(tag+'.png');check('capture '+tag,L.capture_frontend_viewport(str(path)));R['captures'].append(str(path))
    state=json.loads(L.describe_widget(F));(OUT/(tag+'.json')).write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
    check(tag+' expected preview page',state['page']==('HUD' if 'compact' in tag else 'Leaderboard'),state['page'])

def select_full(action):
    F.show_page('Leaderboard')
    F.trigger_action(action)
def step(name, fn, delay=1):STEPS.append((name,fn,delay))
step('fixture',fixture)
step('portrait full',lambda:F.show_page('Leaderboard'),2)
for cat,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step(cat,lambda a=action:select_full(a))
    step(cat+' capture',lambda t=cat:capture('portrait-'+t))
step('landscape',lambda:L.set_pie_viewport_size(1280,720),2)
for cat,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step('landscape '+cat,lambda a=action:select_full(a))
    step('landscape '+cat+' capture',lambda t=cat:capture('landscape-'+t))
step('global',lambda:F.trigger_action('RankGlobal'))
for cat,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step('global '+cat,lambda a=action:select_full(a))
    step('global '+cat+' capture',lambda t=cat:capture('global-landscape-'+t))
step('global portrait',lambda:L.set_pie_viewport_size(720,1280),2)
for cat,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step('global portrait '+cat,lambda a=action:select_full(a))
    step('global portrait '+cat+' capture',lambda t=cat:capture('portrait-global-'+t))
step('HUD landscape',lambda:L.set_pie_viewport_size(1280,720),2)
step('full final rows',lambda:scroll_end('RankAnchor'),2)
step('full final rows capture',lambda:capture_end('full-final-rows','RankAnchor'))
step('HUD',lambda:F.show_page('HUD'))
step('open compact',lambda:F.trigger_action('HUDRankView'),2)
step('compact landscape speed',lambda:capture('compact-landscape-speed'))
step('compact damage',lambda:F.trigger_action('HUDRankDamage'),2)
step('compact landscape damage',lambda:capture('compact-landscape-damage'))
step('portrait compact',lambda:L.set_pie_viewport_size(720,1280),2)
step('compact portrait damage',lambda:capture('compact-portrait-damage'))
step('compact speed',lambda:F.trigger_action('HUDRankSpeed'),2)
step('compact portrait speed',lambda:capture('compact-portrait-speed'))
step('compact final rows',lambda:scroll_end('RankHUD'),2)
step('compact final rows capture',lambda:capture_end('compact-final-rows','RankHUD'))
step('sample names and values',lambda:fixture(False))
step('sample full',lambda:F.show_page('Leaderboard'),2)
step('sample local',lambda:F.trigger_action('RankLocal'))
step('sample first rows',lambda:scroll_start('RankAnchor'))
for cat,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step('sample '+cat,lambda a=action:select_full(a))
    step('sample '+cat+' capture',lambda t=cat:preview('preview-portrait-'+t))
step('sample HUD',lambda:F.show_page('HUD'))
step('sample compact open',lambda:F.trigger_action('HUDRankView'),2)
step('sample compact capture',lambda:preview('preview-compact-portrait'))
def tick(dt):
    global INDEX,NEXT,F,D
    if time.monotonic()<NEXT:return
    try:
        if INDEX==-2:
            check('begin PIE',L.begin_frontend_pie(720,1280));INDEX=-1;NEXT=time.monotonic()+12;return
        if INDEX==-1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+1;return
            directors=unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.InkMenuDirector)
            if not directors:NEXT=time.monotonic()+1;return
            D=directors[0];F=D.get_editor_property('frontend_widget')
            if not F:NEXT=time.monotonic()+1;return
            check('start game',D.start_game());INDEX=0;NEXT=time.monotonic()+4;return
        if INDEX>=len(STEPS):
            R['finished']=True;save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play()
            def quit(dt):
                if unreal.EditorLevelLibrary.get_pie_worlds(False):return
                unreal.unregister_slate_post_tick_callback(QUIT[0]);unreal.SystemLibrary.quit_editor()
            QUIT[0]=unreal.register_slate_post_tick_callback(quit);return
        name,fn,delay=STEPS[INDEX];R['step']=name;fn();INDEX+=1;NEXT=time.monotonic()+delay;save()
    except Exception:
        R['error']=traceback.format_exc();save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
QUIT=[None]
HANDLE=unreal.register_slate_post_tick_callback(tick)
