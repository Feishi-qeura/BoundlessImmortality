"""Rendered service-backed chest flow and stable leaderboard rows, with no reference reads."""
import json,time,traceback
from pathlib import Path
import unreal
OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/SignInRank20261022/PIE'
OUT.mkdir(parents=True,exist_ok=True)
LIB=unreal.InkFrontendEditorLibrary
R={'checks':[],'captures':[],'errors':[]}
F=None;D=None;W=None;HANDLE=None;NEXT=0;INDEX=0;UPDATES=0;AVATAR_ID=None;ROWS=[]
def save():
    R['passed']=not R['errors'] and all(x['passed'] for x in R['checks'])
    (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding='utf-8')
def ck(name,value,actual=None):R['checks'].append(dict(name=name,passed=bool(value),actual=actual));save()
def snap():return json.loads(LIB.describe_widget(F))
def click(action):
    ok=LIB.click_action(F,action);ck('Slate click '+action,ok)
    if not ok:F.trigger_action(action)
def capture(tag):
    p=OUT/(tag+'.png');ck('capture '+tag,LIB.capture_frontend_viewport(str(p)));data=snap();(OUT/(tag+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');R['captures'].append(str(p));save();return data
def obj(name,data=None,group='images'):
    data=data or snap();return next(x for x in data[group] if x['name']==name)
def state():return json.loads(W.get_snapshot_json())['activities']
def check_strip(monthly):
    data=snap();prefix='MonthlyChest' if monthly else 'WeeklyChest'
    ch=sorted([x for x in data['chests'] if x['name'].startswith(prefix)],key=lambda x:x['x'])
    bar=next(x for x in data['progress'] if x['name']==prefix+'Progress')
    ck(prefix+' count',len(ch)==(4 if monthly else 7))
    # The same canvas coordinates must map the bar to both endpoint centers.
    ck(prefix+' bar starts at first chest center',abs(bar['x']-ch[0]['x']-ch[0]['painted_width']/2)<1)
    ck(prefix+' bar ends at final chest center',abs(bar['x']+bar['painted_width']-ch[-1]['x']-ch[-1]['painted_width']/2)<1)
    ck(prefix+' bar crosses chest centers',all(abs(bar['y']+bar['painted_height']/2-x['y']-x['painted_height']/2)<1 for x in ch))
    ck(prefix+' bar behind chests',all(bar.get('z_order',0)<x.get('z_order',0) for x in ch))
    return ch


def particle_check(name,expected):
    actors=unreal.GameplayStatics.get_all_actors_of_class(D,unreal.NiagaraUIActor)
    active=[]
    for a in actors:
        c=a.get_component_by_class(unreal.NiagaraUIComponent)
        if c and c.is_active():
            info=json.loads(unreal.InkCultivationEditorLibrary.inspect_component(c));active.append(info)
    R.setdefault('particle_samples',[]).append(dict(name=name,components=active))
    ck(name+' active count',len(active)==expected,len(active))
    if expected:ck(name+' CPU particle data',all(sum(x.get('counts',[]))>0 for x in active),[list(x) for x in active])

def tick(delta):
    global INDEX,NEXT,F,D,W,UPDATES,AVATAR_ID,ROWS
    if time.monotonic()<NEXT:return
    try:
        if INDEX==0:
            ck('begin PIE',LIB.begin_frontend_pie(1280,720));NEXT=time.monotonic()+10
        elif INDEX==1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+1;return
            directors=unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.InkMenuDirector)
            if not directors:NEXT=time.monotonic()+1;return
            D=directors[0];F=D.get_editor_property('frontend_widget')
            if not F:NEXT=time.monotonic()+1;return
            click('StartGame');NEXT=time.monotonic()+5
            W=unreal.LiveWeaponEditorLibrary.get_wuling_state(worlds[0])
        elif INDEX==2:
            if not W.is_connected():
                ck('service connected',False,W.get_status());raise RuntimeError('service unavailable')
            D.set_actor_tick_enabled(False)
            tex=unreal.load_asset('/Game/UI/Ink/Cohesion20261021/T_CohesionIcons')
            for cat in ('Anchor','Damage','CultivationSpeed'):
                for n in range(1,13):
                    e=unreal.InkLeaderboardEntry();e.set_editor_property('participant_id','qa-viewer-'+str(n));e.set_editor_property('display_name',unreal.Text('剑灵'+str(n)));e.set_editor_property('category',cat);e.set_editor_property('rank',n);e.set_editor_property('numeric_metric',float(100-n));e.set_editor_property('metric',unreal.Text(str(100-n)+'/秒'));e.set_editor_property('realm_index',(n*3)%34);e.set_editor_property('realm',unreal.Text('境界图标验证'));e.set_editor_property('title_id','Novice');e.set_editor_property('avatar_texture',tex);ROWS.append(e)
            F.set_leaderboard_entries(ROWS);capture('hud-entry');ck('HUD boards closed initially',snap()['leaderboard_opacity']==0)
            click('HUDRankView');NEXT=time.monotonic()+1
        elif INDEX==3:
            data=capture('hud-speed');ck('speed faded in',data['active_leaderboard']=='CultivationSpeed' and data['leaderboard_opacity']>.99)
            AVATAR_ID=obj('RankHUDAvatar_1',data)['object_id'];ck('avatar visible',obj('RankHUDAvatar_1',data)['visible'])
            click('HUDRankDamage');NEXT=time.monotonic()+1.3
        elif INDEX==4:
            data=capture('hud-damage');ck('damage tab visible',data['active_leaderboard']=='Damage' and data['leaderboard_opacity']>.99);AVATAR_ID=obj('RankHUDAvatar_1',data)['object_id']
        elif INDEX==5:
            UPDATES+=1
            for e in ROWS:e.set_editor_property('numeric_metric',float(UPDATES));e.set_editor_property('metric',unreal.Text(str(UPDATES)+'.00/秒'))
            F.set_leaderboard_entries(ROWS);data=snap();a=obj('RankHUDAvatar_1',data)
            ck('metric update retains avatar '+str(UPDATES),a['object_id']==AVATAR_ID and a['visible'] and a['resource'].endswith('T_CohesionIcons.T_CohesionIcons'))
            if UPDATES<60:return
            data=capture('hud-avatar-stable');R['avatar_diagnostics']=[]
            for n in range(1,9):
                info=obj('RankHUDAvatar_'+str(n),data);image=unreal.find_object(None,info['object']);R['avatar_diagnostics'].append(dict(rank=n,opacity=image.get_render_opacity(),color=str(image.get_editor_property('color_and_opacity')),brush=str(image.get_editor_property('brush'))))
            click('HUDRankClose');NEXT=time.monotonic()+1
        elif INDEX==6:
            data=snap();ck('close fades out',data['leaderboard_opacity']==0);ck('entry returns after fade',obj('HUDRankView',data,'buttons')['visible']);F.show_page('Leaderboard');NEXT=time.monotonic()+1
        elif INDEX==7:
            click('RankDamage');NEXT=time.monotonic()+1
        elif INDEX==8:
            data=capture('rank-damage-landscape');ck('full damage metric present',any(x['name']=='RankAnchorMetric_1' and '60' in x['text'] for x in data['texts']));ck('realm atlas used',obj('RankAnchorRealm_1',data)['resource'].endswith('T_RealmBanners.T_RealmBanners'));click('RankCultivation');NEXT=time.monotonic()+.6
        elif INDEX==9:
            capture('rank-speed-landscape');ck('portrait resize',LIB.set_pie_viewport_size(720,1280));NEXT=time.monotonic()+2
        elif INDEX==10:
            capture('rank-speed-portrait');D.set_actor_tick_enabled(True);F.open_feature_panel('SignIn','');NEXT=time.monotonic()+2
        elif INDEX==11:
            ch=check_strip(False);capture('weekly-before');particle_check('weekly Niagara live particles',1);ck('today chest has Niagara UI',any(x['glow_active'] and not x['open'] for x in ch));
            today=next(i for i,c in enumerate(state()['weekly_chests']) if c['eligible']);R['today_index']=today;click('Ext_ChestPreview_'+str(today));NEXT=time.monotonic()+1
        elif INDEX==12:
            data=capture('weekly-reward-details');ck('details show 300 qi',any(x['name']=='ChestRewardDetails' and '300' in x['text'] for x in data['texts']));click('Ext_ChestRewardClaim');NEXT=time.monotonic()+3
        elif INDEX==13:
            ck('daily receipt persisted',state()['weekly_chests'][R['today_index']]['claimed']);ch=check_strip(False);ck('claimed daily chest empty/open and no glow',ch[R['today_index']]['open'] and not ch[R['today_index']]['glow_active']);capture('weekly-claimed');click('Ext_GroupTab_MonthlySignIn');NEXT=time.monotonic()+2
        elif INDEX==14:
            ch=check_strip(True);capture('monthly-before');particle_check('monthly Niagara live particles',2);ck('monthly eligible glow',ch[0]['glow_active'] and ch[1]['glow_active']);click('Ext_ChestPreview_0');NEXT=time.monotonic()+1
        elif INDEX==15:
            data=capture('monthly-reward-details');ck('cosmetic details present',any(x['name']=='ChestRewardDetails' and '外观' in x['text'] for x in data['texts']));click('Ext_ChestRewardClaim');NEXT=time.monotonic()+3
        elif INDEX==16:
            ck('monthly receipt persisted',state()['monthly_chests'][0]['claimed']);ch=check_strip(True);ck('claimed month chest open without glow',ch[0]['open'] and not ch[0]['glow_active']);capture('monthly-claimed');F.close_feature_panel(True);NEXT=time.monotonic()+1
        elif INDEX==17:
            particle_check('closing page deactivates all UI effects',0);F.open_feature_panel('MonthlySignIn','');NEXT=time.monotonic()+1
        elif INDEX==18:
            ck('reopened chest remains claimed',check_strip(True)[0]['open']);ck('landscape resize',LIB.set_pie_viewport_size(1280,720));NEXT=time.monotonic()+2
        elif INDEX==19:
            check_strip(True);capture('monthly-landscape');click('Ext_GroupTab_SignIn');NEXT=time.monotonic()+1
        elif INDEX==20:
            check_strip(False);capture('weekly-landscape');F.close_feature_panel(True);NEXT=time.monotonic()+1
        else:
            R['sequence_finished']=True;save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor();return
        INDEX+=1;save()
    except Exception:
        R['errors'].append(traceback.format_exc());save();unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
HANDLE=unreal.register_slate_post_tick_callback(tick)




