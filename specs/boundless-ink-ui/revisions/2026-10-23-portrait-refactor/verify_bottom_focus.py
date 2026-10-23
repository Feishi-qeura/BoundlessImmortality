"""Rendered PIE route, resize, tab and fixed-action regression evidence."""
import gc,json,time,traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/PortraitUI20261023/BottomDiagnostic';OUT.mkdir(parents=True,exist_ok=True)
L=unreal.InkFrontendEditorLibrary
R=dict(checks=[],captures=[],finished=False,boundary='Windows rendered PIE layout and Slate clicks; authority gameplay validated separately in packaged runtime')
I=-2;NEXT=0;F=D=W=None;START=time.monotonic();STEPS=[];QUIT=[None]
def save():
 R['passed']=R['finished'] and not R.get('error') and all(v['passed'] for v in R['checks'])
 (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),'utf-8')
def ck(name,value,actual=None):R['checks'].append(dict(name=name,passed=bool(value),actual=actual));save()
def snap():return json.loads(L.describe_widget(F))
def capture(tag):
 L.move_frontend_pointer_outside(F);p=OUT/(tag+'.png');ck('capture '+tag,L.capture_frontend_viewport(str(p)))
 s=snap();(OUT/(tag+'.json')).write_text(json.dumps(s,ensure_ascii=False,indent=2),'utf-8');R['captures'].append(str(p))
 papers=[v for v in s['borders'] if v['name']=='FeaturePaper'];ck('single paper '+tag,len(papers)<=1,len(papers))
 for v in papers:ck('canonical paper '+tag,'/Materials/' in v['resource'] and '/Refactor' not in v['resource'],v['resource'])
 return s
def click(action):
 R.setdefault('click_events',[]).append(dict(action=action,before=str(F.current_page),pending=str(F.pending_confirmation_action)));ck('Slate click '+action,L.click_action(F,action));R['click_events'][-1]['after']=str(F.current_page)
def open_page(route,context=''):ck('open '+route,F.open_feature_panel(route,context))
def step(name,fn,delay=.7):STEPS.append((name,fn,delay))
def size(w,h):ck('resize '+str((w,h)),L.set_pie_viewport_size(w,h))
def show(page):F.show_page(page)
def body_scroll_to_end():
 for v in snap()['scrolls']:
  if v['name']=='FeatureContentScroll':
   o=unreal.find_object(None,v['object']);o.scroll_to_end();return
def reveal(action):
 candidates=[v for v in snap()['buttons'] if v['action']==action]
 if not candidates:ck('reveal exists '+action,False);return
 obj=unreal.find_object(None,candidates[0]['object']);parent=obj.get_parent()
 while parent:
  if isinstance(parent,unreal.ScrollBox):
   if candidates[0]['width']==0:parent.scroll_to_end()
   else:parent.scroll_widget_into_view(obj,False,unreal.DescendantScrollDestination.CENTER,0)
  parent=parent.get_parent()
def fixed(tag):
 s=capture(tag)
 primary=[v for v in s['buttons'] if v['action'].startswith('Ext_EnterRegion_') or v['action'] in ['Ext_SignInClaim','Ext_Demo_DrawOnce','Ext_Demo_DrawTen','Ext_Demo_DrawSkip']]
 ck('fixed controls present '+tag,bool(primary))
 for v in primary:ck('fixed control geometry '+tag+' '+v['action'],v['width']>0 and v['height']>0,v)

routes=[('Extensions',''),('WorldCultivation',''),('ResourceContest',''),('Expedition',''),('RegionDetail','青云山'),('ResourceDetail','玄铁灵矿'),('ExpeditionDetail','山村义诊'),('DailyActivityDetail','静心修炼'),('TimedActivityDetail','仙缘寻宝'),('ResourceRanking',''),('ResourceRewards',''),('Draw',''),('CharacterEquipment',''),('CharacterSkills',''),('CharacterArtifacts',''),('CharacterFate',''),('CharacterAppearance',''),('CharacterProfile','')]
for label,width,height in [('portrait',720,1280),('landscape',1280,720)]:
 step('size '+label,lambda w=width,h=height:size(w,h),2)
 step('HUD '+label,lambda:show('HUD'));step('capture HUD '+label,lambda t=label:capture('hud-'+t))
 step('sign in '+label,lambda:open_page('SignIn'),1.2)
 step('weekly tab '+label,lambda:click('Ext_GroupTab_SignIn'),1)
 step('weekly '+label,lambda t=label:capture('signin-'+t))
 step('verse tab '+label,lambda:click('Ext_GroupTab_DailyFortune'),1)
 step('verse '+label,lambda t=label:capture('verse-'+t))
 step('month tab '+label,lambda:click('Ext_GroupTab_MonthlySignIn'),1)
 step('month '+label,lambda t=label:capture('monthly-'+t))
 step('activities '+label,lambda:open_page('Activities'),1)
 step('daily '+label,lambda t=label:capture('daily-'+t))
 step('timed tab '+label,lambda:click('Ext_GroupTab_TimedActivities'),1)
 step('timed '+label,lambda t=label:capture('timed-'+t))
 for route,ctx in routes:
  step(route+' '+label,lambda r=route,c=ctx:open_page(r,c),.8)
  step('capture '+route+' '+label,lambda t=route+'-'+label:capture(t))
 for page in ['Inventory','Leaderboard','Settings','Saves']:
  step(page+' '+label,lambda p=page:show(p),1)
  step('capture '+page+' '+label,lambda t=page+'-'+label:capture(t))
 for route,action,expected in [('DailyActivities','Ext_Open_DailyActivityDetail|丹道研习','DailyActivityDetail'),('TimedActivities','Ext_Open_TimedActivityDetail|云山问道','TimedActivityDetail'),('Extensions','Ext_Open_CharacterProfile','CharacterProfile'),('ResourceContest','Ext_Open_ResourceRewards','ResourceRewards')]:
  step('open lower '+route+' '+label,lambda r=route:open_page(r),1)
  step('reveal lower '+route+' '+label,lambda a=action:reveal(a),1)
  step('capture lower '+route+' '+label,lambda t=route+'-bottom-'+label:capture(t))
  step('click lower '+route+' '+label,lambda a=action:click(a),1)
  step('check lower '+route+' '+label,lambda p=expected:ck('lower action target '+p,snap()['page']==p,snap()['page']))
def appearance_offset_check():
 values=[unreal.find_object(None,v['object']).get_scroll_offset() for v in snap()['scrolls'] if v['name']=='AppearanceOptionsScroll']
 ck('appearance lower options retained after apply',len(values)==1 and values[0]>100,values)
step('appearance scroll regression',lambda:open_page('CharacterAppearance'),1)
step('reveal lower style',lambda:reveal('Ext_Style_华丽'),1)
step('apply lower style',lambda:click('Ext_Style_华丽'),1)
step('retained inner offset',appearance_offset_check)
step('appearance applied screenshot',lambda:capture('appearance-applied-landscape'))
for label,width,height in [('compact',540,960),('tablet',960,1280),('ultratall',720,1560),('small',360,640)]:
 step('size '+label,lambda w=width,h=height:size(w,h),2)
 for route in ['SignIn','Activities','Extensions','WorldCultivation']:
  step(route+' '+label,lambda r=route:open_page(r),1)
  step('capture '+route+' '+label,lambda t=route+'-'+label:capture(t))
for count in range(5):
 step('reopen sign '+str(count),lambda:open_page('SignIn'),.4)
 step('capture reopen '+str(count),lambda t=count:capture('reopen-'+str(t)),.3)
 step('close sign '+str(count),lambda:click('Ext_Close'),.4)
step('draw fixed test',lambda:open_page('Draw'),1)
step('draw scroll',body_scroll_to_end,1)
step('draw footer',lambda:fixed('draw-fixed'))
step('world fixed test',lambda:open_page('WorldCultivation'),1)
step('world scroll',body_scroll_to_end,1)
step('world footer',lambda:capture('world-fixed'))
step('safe padding',lambda:ck('safe padding',L.set_frontend_safe_padding(F,unreal.Margin(18,22,24,30))),1)
step('safe capture',lambda:capture('safe-world'))
step('return main',lambda:D.return_to_main_menu(),1)
step('main capture',lambda:capture('main-after'))
def finish():
 global F,D,W
 unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();F=D=W=None
 def quit_tick(dt):
  if unreal.EditorLevelLibrary.get_pie_worlds(False):return
  unreal.unregister_slate_post_tick_callback(QUIT[0]);gc.collect();unreal.SystemLibrary.quit_editor()
 QUIT[0]=unreal.register_slate_post_tick_callback(quit_tick)
def tick(dt):
 global I,NEXT,F,D,W
 if time.monotonic()<NEXT:return
 try:
  if time.monotonic()-START>600:raise TimeoutError('PIE verification exceeded 600 seconds')
  if I==-2:
   assert 'WulingPrototype' not in unreal.SystemLibrary.get_command_line(),'This suite verifies preview UI; service UI is verified by the packaged runtime suite.'
   ck('begin PIE',L.begin_frontend_pie(720,1280));I=-1;NEXT=time.monotonic()+12;return
  if I==-1:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   if not worlds:NEXT=time.monotonic()+1;return
   W=worlds[0];ds=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)
   if not ds:NEXT=time.monotonic()+1;return
   D=ds[0];F=D.get_editor_property('frontend_widget')
   if not F:NEXT=time.monotonic()+1;return
   F.on_action_triggered.add_callable(lambda action:R.setdefault('dispatched',[]).append(str(action)))
   capture('main-before');ck('start gameplay',D.start_game());I=0;NEXT=time.monotonic()+4;return
  if I>=len(STEPS):R['finished']=True;save();finish();return
  name,fn,delay=STEPS[I];R['current_step']=name;fn();I+=1;NEXT=time.monotonic()+delay;save()
 except Exception:R['error']=traceback.format_exc();save();finish()
STEPS=[s for s in STEPS if s[0]=='size portrait' or ('lower' in s[0] and 'portrait' in s[0]) or s[0]=='main capture']
HANDLE=unreal.register_slate_post_tick_callback(tick)
