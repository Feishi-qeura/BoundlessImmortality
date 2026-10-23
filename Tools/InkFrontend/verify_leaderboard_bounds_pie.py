"""Transient leaderboard layout fixtures; never imports or saves project assets."""
from pathlib import Path
import json, time, traceback
import unreal

ROOT = Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/Codex/LeaderboardBounds20261022' / ('Before' if '-rankbefore' in unreal.SystemLibrary.get_command_line().lower() else 'After')
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
def fixture():
    D.set_actor_tick_enabled(False)
    rows=[]
    for category in ('Anchor','CultivationSpeed','Damage'):
        for rank in range(1,31):
            e=unreal.InkLeaderboardEntry()
            for key,value in dict(participant_id='layout-'+str(rank), display_name=unreal.Text('超长修士昵称用于边界检查'+str(rank)), category=category, rank=rank, metric=unreal.Text('12345678.90 /秒'), numeric_metric=12345678.9, realm_index=(rank*3)%34, realm=unreal.Text('境界'), title_id=['Novice','DivineBlade','Cyberspace'][rank%3], avatar_texture=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')).items():e.set_editor_property(key,value)
            rows.append(e)
            if rank==2:e.set_editor_property('avatar_texture',unreal.load_asset('/Game/UI/Ink/Cultivation/T_RealmBanners'))
    F.set_leaderboard_entries(rows)
def capture(tag):
    L.move_frontend_pointer_outside(F)
    path=OUT/(tag+'.png');check('capture '+tag,L.capture_frontend_viewport(str(path)))
    s=json.loads(L.describe_widget(F));(OUT/(tag+'.json')).write_text(json.dumps(s,ensure_ascii=False,indent=2),encoding='utf-8');R['captures'].append(str(path))
    prefix='RankHUD' if tag.startswith('compact') else 'RankAnchor'
    images={i['name']:i for i in s['images'] if i['name'].startswith(prefix)}
    texts={i['name']:i for i in s['texts'] if i['name'].startswith(prefix)}
    regions=json.loads((ROOT/'SourceAssets/UI/NiagaraRevision20261022/rank-theme-regions.json').read_text(encoding='utf-8'))
    for rank in range(1,31):
        art=images[prefix+'RowArt_'+str(rank)]
        art_widget=unreal.find_object(None,art['object']);pos=art_widget.slot.get_position();size=art_widget.slot.get_size()
        if rank<=10:
            theme='Damage' if 'DamageTier' in art.get('material_parent','') else 'Speed' if 'SpeedTier' in art.get('material_parent','') else 'Sword'
            crop=regions[theme+'Rows']['regions'][10-rank]
            check(tag+' original crop aspect '+str(rank),abs(size.x/size.y-(crop[2]-crop[0])/(crop[3]-crop[1]))<.005)
            if art['painted_height']>0:check(tag+' painted crop aspect '+str(rank),abs(art['painted_width']/art['painted_height']-size.x/size.y)<.005)
        for part in ('Number','Name','Metric','Avatar','AvatarPlaceholder','TitleEffect','RealmEffect'):
            key=prefix+part+'_'+str(rank)
            item=images.get(key) or texts.get(key)
            if not item:continue
            child=unreal.find_object(None,item['object']);cp=child.slot.get_position();cs=child.slot.get_size()
            check(tag+' artwork contains '+part+' '+str(rank),cp.x>=pos.x-.01 and cp.y>=pos.y-.01 and cp.x+cs.x<=pos.x+size.x+.01 and cp.y+cs.y<=pos.y+size.y+.01)
            check(tag+' isolated paint '+part+' '+str(rank),item['clipping']=='ClipToBoundsAlways')
        a=texts[prefix+'Name_'+str(rank)];b=texts[prefix+'Metric_'+str(rank)]
        check(tag+' separate nickname/metric '+str(rank),a['x']+a['painted_width']<=b['x']+.8)
        if rank==2:
            avatar=images[prefix+'Avatar_2'];source=unreal.load_asset('/Game/UI/Ink/Cultivation/T_RealmBanners')
            check(tag+' nonsquare avatar retains aspect',abs(avatar['painted_width']/avatar['painted_height']-source.blueprint_get_size_x()/source.blueprint_get_size_y())<.005)
        for fx in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget):
            if fx.get_path_name().startswith(F.get_path_name()+'.') and fx.get_name() in (prefix+'TitleEffect_'+str(rank),prefix+'RealmEffect_'+str(rank)):
                fp=fx.slot.get_position();fs=fx.slot.get_size()
                check(tag+' title/realm effect inside artwork '+fx.get_name(),fp.x>=pos.x-.01 and fp.y>=pos.y-.01 and fp.x+fs.x<=pos.x+size.x+.01 and fp.y+fs.y<=pos.y+size.y+.01)
def step(name, fn, delay=1):STEPS.append((name,fn,delay))
step('fixture',fixture)
step('portrait full',lambda:F.show_page('Leaderboard'),2)
for cat,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step(cat,lambda a=action:F.trigger_action(a))
    step(cat+' capture',lambda t=cat:capture('portrait-'+t))
step('landscape',lambda:L.set_pie_viewport_size(1280,720),2)
step('landscape capture',lambda:capture('landscape-damage'))
step('HUD',lambda:F.show_page('HUD'))
step('open compact',lambda:F.trigger_action('HUDRankView'),2)
step('compact landscape',lambda:capture('compact-landscape'))
step('portrait compact',lambda:L.set_pie_viewport_size(720,1280),2)
step('compact portrait',lambda:capture('compact-portrait'))
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
