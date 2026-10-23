"""Rendered Windows PIE, actual Slate clicks and authority-backed claims/travel."""
import gc,json,time,traceback
from pathlib import Path
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/JourneyExpansion20261023/PIE';OUT.mkdir(parents=True,exist_ok=True)
L=unreal.InkFrontendEditorLibrary;R=dict(checks=[],captures=[],finished=False,boundary='Windows rendered PIE, isolated local authority with declared attendance fixture; no device/platform approval')
F=D=W=S=None;I=-2;NEXT=0;START=time.monotonic();STEPS=[];QUIT=[None]
def save():
 R['passed']=R['finished'] and not R.get('error') and all(c['passed'] for c in R['checks'])
 (OUT/'report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding='utf-8')
def ck(name,value,actual=None):R['checks'].append(dict(name=name,passed=bool(value),actual=actual));save()
def snap():return json.loads(L.describe_widget(F))
def state():return json.loads(S.get_snapshot_json())
def capture(tag):
 L.move_frontend_pointer_outside(F);p=OUT/(tag+'.png');ck('capture '+tag,L.capture_frontend_viewport(str(p)))
 data=snap();(OUT/(tag+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8');R['captures'].append(str(p));return data
def click(action):
 before=len(R.get('dispatch',[]));ck('Slate hit '+action,L.click_action(F,action));ck('dispatch '+action,len(R.get('dispatch',[]))>before and R['dispatch'][-1]==action)
def page(name,context=''):ck('open '+name,F.open_feature_panel(name,context))
def reveal(action):
 rows=[v for v in snap()['buttons'] if v['action']==action and v['visible']]
 ck('reveal exists '+action,bool(rows))
 if not rows:return
 obj=unreal.find_object(None,rows[0]['object']);ancestor=obj;seen=set()
 while ancestor and ancestor.get_path_name() not in seen:
  seen.add(ancestor.get_path_name());parent=ancestor.get_parent()
  if parent:
   if isinstance(parent,unreal.ScrollBox):parent.scroll_widget_into_view(obj,False,unreal.DescendantScrollDestination.CENTER,0)
   ancestor=parent
  else:
   owner=ancestor.get_outer()
   while owner and not isinstance(owner,unreal.UserWidget):owner=owner.get_outer()
   ancestor=owner if owner!=ancestor else None
def escape():
 key=unreal.Key();key.set_editor_property('key_name','Escape');ck('Escape closes receipt',L.press_frontend_key(F,key))
def name_input():
 panel=F.get_editor_property('extension_pages').get('Wuling')
 inputs=[o for o in unreal.ObjectIterator(unreal.EditableTextBox) if o.get_name()=='SectNameInput' and o.get_path_name().startswith(panel.get_path_name())]
 ck('guild name editor exists',len(inputs)==1,[o.get_path_name() for o in inputs])
 if inputs:inputs[0].set_text('青云测试武灵')
def step(name,fn,delay=.8):STEPS.append((name,fn,delay))
def texts():return '\n'.join(t['text'] for t in snap()['texts'])
def receipt():return F.get_editor_property('reward_receipt_panel')
def type_check(route,needle):
 panel=F.get_editor_property('extension_pages').get(route);ck('native class '+route, bool(panel) and needle in panel.get_class().get_name(),panel.get_class().get_name() if panel else None)
def details(resource):
 t=texts();ck('resource detail omits difficulty' if resource else 'world detail has difficulty',('关卡难度' not in t) if resource else ('关卡难度' in t),t)
 ck('resource ownership field' if resource else 'world rewards field',('当前拥有的主播' in t) if resource else ('三阶段完成奖励' in t))
def setting_check(tag):
 data=capture('settings-'+tag);qs=[b for b in data['buttons'] if b['action'] in ['QualityLow','QualityMedium','QualityHigh']]
 ck('quality option count '+tag,len(qs)==3)
 if len(qs)==3:
  qs.sort(key=lambda b:b['x']);ck('quality row equal width '+tag,max(b['width'] for b in qs)-min(b['width'] for b in qs)<2,qs)
  ck('quality row same y '+tag,max(b['y'] for b in qs)-min(b['y'] for b in qs)<2,qs)

step('connected',lambda:ck('authority connected',S.is_connected()),2)
step('hud',lambda:F.show_page('HUD'))
step('hud portrait',lambda:capture('hud-portrait'))
step('HUD launch',lambda:click('OpenLaunchEvent'))
step('launch class',lambda:type_check('LaunchEvent','LaunchEvent'))
step('launch empty',lambda:capture('launch-unstarted'))
step('launch home',lambda:click('Ext_Close'))
step('HUD sect',lambda:click('OpenWuling'))
step('sect class',lambda:type_check('Wuling','WulingSect'))
step('sect empty',lambda:capture('wuling-empty'))
step('enter guild name',name_input)
step('create guild reveal',lambda:reveal('Ext_SectCreate'))
step('create guild',lambda:click('Ext_SectCreate'),2)
step('guild created',lambda:ck('guild created by UI',bool(state().get('guild'))))
step('sect active',lambda:capture('wuling-portrait'))
step('open launch',lambda:page('LaunchEvent'))
step('launch start',lambda:click('Ext_LaunchPrimary'),2)
step('launch active',lambda:ck('launch started by UI',bool(state().get('launch'))))
for tab in ['overview','tasks','guild','honors']:
 step('launch '+tab,lambda t=tab:click('Ext_LaunchTab_'+t))
 step('capture launch '+tab,lambda t=tab:capture('launch-'+t+'-portrait'))
step('HUD map',lambda:click('Ext_Close'))
step('HUD journey',lambda:click('OpenExtensions'))
step('map class',lambda:type_check('Extensions','JourneyMap'))
step('world map',lambda:capture('world-map-portrait'))
step('world pin',lambda:click('Ext_AtlasNode_qingyun'))
step('world sheet',lambda:details(False))
step('world capture',lambda:capture('world-detail-portrait'))
step('world confirm view',lambda:click('Ext_DestinationGo'))
step('world confirm capture',lambda:capture('world-confirm-portrait'))
step('world cancel reveal',lambda:reveal('Ext_DestinationCancel'))
step('world cancel',lambda:click('Ext_DestinationCancel'))
step('cancel no round',lambda:ck('cancel did not start world',state().get('round') is None))
step('map back',lambda:click('Ext_Back'))
step('locked detail',lambda:click('Ext_AtlasNode_xuanbing'))
step('locked screenshot',lambda:capture('world-locked-portrait'))
step('locked disabled',lambda:ck('locked travel button disabled',not next(b for b in snap()['buttons'] if b['action']=='Ext_DestinationGo')['enabled']))
step('locked back',lambda:click('Ext_Back'))
step('resource tab',lambda:click('Ext_AtlasResource'))
step('resource map',lambda:capture('resource-map-portrait'))
step('resource pin',lambda:click('Ext_AtlasNode_NODE_QI:1'))
step('resource sheet',lambda:details(True))
step('resource detail',lambda:capture('resource-detail-portrait'))
step('resource back',lambda:click('Ext_Back'))
step('preserve resource tab',lambda:ck('resource tab retained',F.get_editor_property('extension_pages').get('Extensions').get_editor_property('resource_map')))
step('monthly',lambda:page('MonthlySignIn'))
step('chest reveal',lambda:reveal('Ext_ChestPreview_0'))
step('chest inspect',lambda:click('Ext_ChestPreview_0'))
step('claim reveal',lambda:reveal('Ext_ChestRewardClaim'))
step('claim chest',lambda:click('Ext_ChestRewardClaim'),2)
step('real receipt',lambda:ck('reward overlay from real claim',bool(receipt())))
step('reward portrait',lambda:capture('reward-portrait'))
step('close reward key',escape,1)
step('receipt gone',lambda:ck('receipt closed and does not replay',not receipt()),2)
step('receipt no replay after poll',lambda:ck('receipt remains closed',not receipt()))
step('landscape',lambda:ck('resize landscape',L.set_pie_viewport_size(1280,720)),2)
for route,context,tag in [('LaunchEvent','','launch-landscape'),('Wuling','','wuling-landscape'),('WorldCultivation','','world-map-landscape'),('ResourceContest','','resource-map-landscape'),('JourneyWorldDetail','xuantie','world-detail-landscape'),('JourneyResourceDetail','NODE_STONE:1','resource-detail-landscape')]:
 step('open '+tag,lambda r=route,c=context:page(r,c))
 step('capture '+tag,lambda t=tag:capture(t))
step('settings landscape',lambda:F.show_page('Settings'))
step('settings landscape capture',lambda:setting_check('landscape'))
step('quality UI',lambda:click('QualityMedium'))
step('fps reveal',lambda:reveal('Frame60'))
step('fps UI',lambda:click('Frame60'))
step('motion reveal',lambda:reveal('ToggleReducedMotion'))
step('motion UI',lambda:click('ToggleReducedMotion'))
step('settings apply',lambda:click('ApplySettings'))
step('settings save',lambda:ck('quality applies',unreal.GameUserSettings.get_game_user_settings().get_overall_scalability_level()==1))
step('portrait',lambda:ck('resize portrait',L.set_pie_viewport_size(720,1280)),2)
step('settings portrait capture',lambda:setting_check('portrait'))
step('start region sheet',lambda:page('JourneyWorldDetail','qingyun'))
step('start region intent',lambda:click('Ext_DestinationGo'))
step('start region confirm',lambda:click('Ext_DestinationConfirm'),2)
step('world started',lambda:ck('world entered from map confirmation',(state().get('round') or {}).get('region_id')=='qingyun' and str(F.get_editor_property('current_page'))=='HUD'))
step('world gameplay',lambda:capture('entered-world-portrait'))

def finish():
 global F,D,W,S
 unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();F=D=W=S=None
 def quit_tick(dt):
  if unreal.EditorLevelLibrary.get_pie_worlds(False):return
  unreal.unregister_slate_post_tick_callback(QUIT[0]);gc.collect();unreal.SystemLibrary.quit_editor()
 QUIT[0]=unreal.register_slate_post_tick_callback(quit_tick)
def tick(dt):
 global I,NEXT,F,D,W,S
 if time.monotonic()<NEXT:return
 try:
  if time.monotonic()-START>500:raise TimeoutError('PIE exceeded 500 seconds')
  if I==-2:ck('begin rendered PIE',L.begin_frontend_pie(720,1280));I=-1;NEXT=time.monotonic()+12;return
  if I==-1:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   if not worlds:NEXT=time.monotonic()+1;return
   W=worlds[0];actors=unreal.GameplayStatics.get_all_actors_of_class(W,unreal.InkMenuDirector)
   if not actors:NEXT=time.monotonic()+1;return
   D=actors[0];F=D.get_editor_property('frontend_widget')
   if not F:NEXT=time.monotonic()+1;return
   gi=unreal.GameplayStatics.get_game_instance(W);S=next(o for o in unreal.ObjectIterator(unreal.WulingPrototypeSubsystem) if o.get_outer()==gi)
   F.on_action_triggered.add_callable(lambda a:R.setdefault('dispatch',[]).append(str(a)))
   ck('start gameplay',D.start_game());I=0;NEXT=time.monotonic()+4;return
  if I>=len(STEPS):R['finished']=True;save();finish();return
  name,fn,delay=STEPS[I];R['current_step']=name;fn();I+=1;NEXT=time.monotonic()+delay;save()
 except Exception:R['error']=traceback.format_exc();save();finish()
HANDLE=unreal.register_slate_post_tick_callback(tick)
