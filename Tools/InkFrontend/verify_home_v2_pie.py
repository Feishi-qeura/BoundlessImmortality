"""Actual player frontend, rendered PIE and physical Slate hit-test checks."""
import gc,hashlib,json,time,traceback
from pathlib import Path
import unreal

BASELINE='-homebaseline' in unreal.SystemLibrary.get_command_line().lower()
POST='-homepostcleanup' in unreal.SystemLibrary.get_command_line().lower()
ROOT=Path(unreal.Paths.project_dir())
OUT=ROOT/'Saved/Codex/HomeV2Integration20261022'/('BaselinePIE' if BASELINE else 'PostCleanupPIE' if POST else 'PIE')
OUT.mkdir(parents=True,exist_ok=True)
L=unreal.InkFrontendEditorLibrary
R=dict(checks=[],captures=[],finished=False,baseline=BASELINE,boundary='Windows rendered PIE; existing player frontend; local simulation explicitly marked; Cook/device/production not run')
R['source_hashes']={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'Source/BoundlessImmortality/Frontend').glob('Ink*.*') if p.is_file()}
I=-2;NEXT=0;F=D=W=None;START=time.monotonic()
def save():
 R['passed']=R['finished'] and not R.get('error') and all(x['passed'] for x in R['checks'])
 (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),'utf-8')
def ck(n,v,a=None):R['checks'].append(dict(name=n,passed=bool(v),actual=a));save()
def snap():return json.loads(L.describe_widget(F))
def capture(tag):
 L.move_frontend_pointer_outside(F);p=OUT/(tag+'.png');ck('pixels '+tag,L.capture_frontend_viewport(str(p)))
 s=snap();(OUT/(tag+'.json')).write_text(json.dumps(s,ensure_ascii=False,indent=2),'utf-8');R['captures'].append(str(p));return s
def click(action):
 before=str(F.get_editor_property('current_page'))
 ck('Slate click '+action,L.click_action(F,action),dict(before=before,after=str(F.get_editor_property('current_page'))))
def page(tag,expected):
 s=capture(tag);ck(tag+' actual page',s['page']==expected,s['page']);return s
def art_check(tag):
 s=capture(tag)
 entry=next(x for x in s['buttons'] if x['action']=='HUDRankView')
 ck(tag+' legacy rank backing absent',not entry['resource'],entry['resource'])
 for name in ['HUDBagIcon','HUDRankIcon','HUDMenuIcon','HUDCultivationIcon']:
  x=next(v for v in s['images'] if v['name']==name)
  ck('approved HomeV2 '+tag+' '+name,'/HomeV2/' in x['resource'],x)
 if not BASELINE:
  for w in unreal.ObjectIterator(unreal.InkHomeArtWidget):
   if w.get_world()!=W:continue
   desc=json.loads(w.describe_art());R.setdefault('art_geometry',[]).append(dict(tag=tag,**desc))
   if desc['component']=='home-rank-paper':
    ck(tag+' rank text/hit use art bounds',desc['caption_inside_art'] and desc['hit_inside_art'],desc)
  for name in ['HUDBagIcon','HUDRankIcon','HUDMenuIcon','HUDCultivationIcon']:
   x=next(v for v in s['images'] if v['name']==name)
   tex=unreal.load_asset(x['resource']);ratio=tex.blueprint_get_size_x()/tex.blueprint_get_size_y()
   ck(tag+' preserved aspect '+name,abs(x['painted_width']/x['painted_height']-ratio)<.01,x)
 return s
def bindings():
 a=unreal.find_object(None,snap()['anchor_status']['object'])
 state=unreal.LiveWeaponEditorLibrary.get_cultivation_state(W)
 health=state.get_anchor_health()
 ck('HUD health reads existing component',a.get_editor_property('health')==health.get_editor_property('health'))
 ck('HUD health maximum reads existing component',a.get_editor_property('max_health')==health.get_editor_property('max_health'))
 ck('zero shield preserves absent-state',a.get_editor_property('shield')==0 and a.get_editor_property('max_shield')==0)
 tex=unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
 ck('explicit local avatar fixture',L.set_frontend_test_anchor_avatar(F,'https://fixture.invalid/home-v2-mask.png',tex))
 R['avatar_fixture']='WhiteSquareTexture only for circular mask verification; not a real provider profile.'
def avatar_check():
 a=unreal.find_object(None,snap()['anchor_status']['object'])
 m=next((v for v in unreal.ObjectIterator(unreal.MaterialInstanceDynamic) if v.get_outer()==a),None)
 ck('real identity texture uses circular UI mask',m is not None and '/HomeV2/Materials/M_HomeAvatarCircle' in m.get_editor_property('parent').get_path_name())
 capture('avatar-circle-fixture')
def load_assets():
 for r in json.loads((ROOT/'Tools/InkFrontend/home_v2_imports.json').read_text('utf-8')):
  ck('fresh persisted asset load '+r['id'],unreal.load_asset(r['asset']) is not None)
 ck('fresh avatar material load',unreal.load_asset('/Game/UI/Ink/HomeV2/Materials/M_HomeAvatarCircle') is not None)
def finish():
 global F,D,W
 unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();F=D=W=None
 def quit_tick(dt):
  if unreal.EditorLevelLibrary.get_pie_worlds(False):return
  unreal.unregister_slate_post_tick_callback(QUIT[0]);gc.collect();unreal.SystemLibrary.quit_editor()
 QUIT[0]=unreal.register_slate_post_tick_callback(quit_tick)
STEPS=[]
def step(n,fn,delay=1):STEPS.append((n,fn,delay))
step('fresh resource loads',load_assets)
step('exact portrait client size',lambda:ck('resize exact portrait',L.set_pie_viewport_size(720,1280)),2)
step('portrait HUD',lambda:art_check('hud-portrait'),1)
step('business bindings and avatar fixture',bindings,2)
step('avatar mask capture',avatar_check)
step('role',lambda:click('OpenCharacter'),2)
step('role capture',lambda:page('character-portrait','CharacterProfile'))
step('close role',lambda:click('Ext_Close'))
step('bag',lambda:click('OpenInventory'),2)
step('bag capture',lambda:page('inventory-portrait','Inventory'))
step('close bag',lambda:click('InventoryCloseIcon'))
step('activities',lambda:click('OpenActivities'),1)
step('activities capture',lambda:page('activities-portrait','Activities'))
step('close feature',lambda:click('Ext_Close'))
step('signin',lambda:click('OpenSignIn'),1)
step('signin capture',lambda:page('signin-portrait','SignIn'))
step('close signin',lambda:click('Ext_Close'))
step('HUD after feature close',lambda:ck('feature closed to HUD',snap()['page']=='HUD'))
step('rank entry',lambda:click('HUDRankView'),2)
step('compact rank capture',lambda:capture('rank-expanded-portrait'))
step('close compact rank',lambda:click('HUDRankClose'),2)
step('rank page',lambda:click('OpenLeaderboard'),2)
step('rank page capture',lambda:page('rank-page-portrait','Leaderboard'))
step('close rank page',lambda:click('RankClose'))
step('landscape resize',lambda:ck('resize landscape',L.set_pie_viewport_size(1280,720)),2)
step('landscape HUD',lambda:art_check('hud-landscape'))
step('menu click',lambda:click('ToggleHUDMenu'))
step('menu capture',lambda:capture('menu-landscape'))
step('menu close',lambda:click('ToggleHUDMenu'))
step('journey',lambda:click('OpenExtensions'))
step('journey capture',lambda:capture('journey-landscape'))
step('journey close',lambda:click('Ext_Close'))
step('cultivate',lambda:click('ToggleCultivation'),2)
step('cultivation capture',lambda:capture('cultivation-landscape'))
step('stop cultivate',lambda:click('ToggleCultivation'),2)
step('safe zone',lambda:ck('safe padding',L.set_frontend_safe_padding(F,unreal.Margin(18,22,24,30))),2)
step('safe zone capture',lambda:art_check('hud-safe-landscape'))
step('return menu',lambda:D.return_to_main_menu(),2)
step('preserved main menu',lambda:capture('main-after'))
def tick(dt):
 global I,NEXT,F,D,W
 if time.monotonic()<NEXT:return
 try:
  if time.monotonic()-START>300:raise TimeoutError('PIE script exceeded 300s')
  if I==-2:
   ck('begin PIE',L.begin_frontend_pie(720,1280));I=-1;NEXT=time.monotonic()+12;return
  if I==-1:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   if not worlds:NEXT=time.monotonic()+1;return
   W=worlds[0];ds=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)
   if not ds:NEXT=time.monotonic()+1;return
   D=ds[0];F=D.get_editor_property('frontend_widget')
   if not F:NEXT=time.monotonic()+1;return
   capture('main-before');ck('existing Director starts gameplay',D.start_game());I=0;NEXT=time.monotonic()+4;return
  if I>=len(STEPS):R['finished']=True;save();finish();return
  n,fn,delay=STEPS[I];R['current_step']=n;fn();I+=1;NEXT=time.monotonic()+delay;save()
 except Exception:R['error']=traceback.format_exc();save();finish()
HANDLE=unreal.register_slate_post_tick_callback(tick);QUIT=[None]
