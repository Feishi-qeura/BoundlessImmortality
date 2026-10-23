"""Character ratings and equipped-quality glow; all sample inputs are transient."""
import json, time, traceback
from pathlib import Path
import unreal

BASELINE = '-RatingBaseline' in unreal.SystemLibrary.get_command_line()
OUT = Path(unreal.Paths.project_dir()) / 'Saved/Codex/CharacterRating20261022/PIE' / ('baseline' if BASELINE else 'final')
OUT.mkdir(parents=True, exist_ok=True)
LIB = unreal.InkFrontendEditorLibrary
R = dict(checks=[], errors=[], captures=[])
INDEX = 0
NEXT = 0
START = time.monotonic()
F = P = D = HANDLE = None
PARTICLES=None

def save():
    R['passed'] = bool(R['checks']) and not R['errors'] and all(x['passed'] for x in R['checks'])
    (OUT/'report.json').write_text(json.dumps(R, ensure_ascii=False, indent=2), encoding='utf-8')

def ck(name, result, actual=None):
    R['checks'].append(dict(name=name, passed=bool(result), actual=actual)); save()

def snapshot(): return json.loads(LIB.describe_widget(F))

def capture(name):
    p = OUT/(name+'.png'); ck('capture '+name, LIB.capture_frontend_viewport(str(p)))
    data = snapshot(); (OUT/(name+'.json')).write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    R['captures'].append(str(p)); save(); return data

def fixture(overrides=None, empty=False, numeric=True):
    overrides=overrides or {}
    rows = [('Health','生命','150 / 250',250),('Attack','剑灵攻击','25',25),('Defense','防御','8',8),
            ('SpiritDamage','剑灵伤害','5',5),('Critical','会心一击','10.0%',.10),('Evasion','闪避','6.0%',.06),
            ('MoveSpeed','实际移速上限','420 厘米/秒',420),('CultivationSpeed','修炼速度','2.00 /秒',2),
            ('Luck','幸运（预留）','0',0),('Swordsmanship','剑法（预留）','1.8',1.8),('Cultivation','修为','346 / 449',346),
            ('RealmStone','突破材料','30 / 1',30),('Realm','境界','练气四阶',0)]
    attributes = []
    for key, label, value, number in rows:
        a = unreal.InkCharacterAttribute(); a.set_editor_property('attribute_id',key); a.set_editor_property('label',unreal.Text(label)); a.set_editor_property('value',unreal.Text(value))
        if not BASELINE:
            a.set_editor_property('bHasNumericValue',numeric); a.set_editor_property('numeric_value',overrides.get(key,number))
            a.set_editor_property('rating_base_value',400 if key=='MoveSpeed' else 0)
        attributes.append(a)
    disciplines = []
    for category, name, tier in [('Manual','主修 · 验收',2),('Movement','轻功 · 验收',1),('Inner','内功 · 验收',3),('Martial','武神 · 验收',6)]:
        d = unreal.InkDisciplineEntry(); d.set_editor_property('category',category); d.set_editor_property('name',unreal.Text(name))
        if not BASELINE: d.set_editor_property('quality_tier',tier)
        disciplines.append(d)
    F.set_character_data(unreal.Text('主播 · 评级验收预览'),attributes,[] if empty else disciplines)

def components():return [e.get_niagara_component() for e in P.get_editor_property('cultivation_visual').get_effects()]
def particles():return [json.loads(unreal.InkCultivationEditorLibrary.inspect_component(c)) for c in components()]
def variable(c,kind,name):
    result=getattr(c,'get_variable_'+kind)(name)
    return result[0] if isinstance(result,tuple) else result
def glows():return [x for x in snapshot()['images'] if x['visible'] and x['name'].startswith('ProfileSlotGlow_')]
def image_widget(name):return unreal.find_object(None,next(x['object'] for x in snapshot()['images'] if x['name']==name))
def grade_resource(key):return image_widget('ProfileRating_'+key).get_editor_property('brush').get_editor_property('resource_object').get_name()
def active_count():
    total=0
    for a in unreal.GameplayStatics.get_all_actors_of_class(D,unreal.NiagaraUIActor):
        c=a.get_component_by_class(unreal.NiagaraUIComponent)
        asset=c.get_asset() if c else None
        if asset and '/CharacterProfile20261022/' in asset.get_path_name() and c.is_active():total+=1
    return total

def layout(name):
    data = capture(name)
    texts = [x for x in data['texts'] if x['visible']]
    images = [x for x in data['images'] if x['visible']]
    ck(name+' breakthrough material removed',not any('RealmStone' in x['name'] for x in texts))
    ratings = [x for x in images if x['name'].startswith('ProfileRating_')]
    ck(name+' eight official ratings',len(ratings)==8,len(ratings))
    for key in ('Luck','Swordsmanship','Cultivation'):
        ck(name+' ungraded '+key,not any(x['name']=='ProfileRating_'+key for x in ratings))
    for icon in ratings:
        value = next(x for x in texts if x['name']==icon['name'].replace('ProfileRating_','ProfileValue_'))
        ck(name+' grade before value '+icon['name'],icon['x']+icon['painted_width']<=value['x']+1)
        ck(name+' real rating bitmap '+icon['name'],'/CharacterRating20261022/T_Rating_' in icon['resource'])
    glows=[x for x in images if x['name'].startswith('ProfileSlotGlow_')]
    ck(name+' four equipped glowing frames',len(glows)==4,len(glows))
    values=[x for x in texts if x['name'].startswith('ProfileValue_')]
    rows={}
    for v in values:rows.setdefault(round(v['y']),[]).append(v)
    ck(name+' three attribute columns',bool(rows) and all(len(row)<=3 for row in rows.values()) and max(map(len,rows.values()))==3)
    buttons=[b for b in data['buttons'] if b['visible'] and b['action'].startswith('Ext_ProfileMethod_')]
    buttons.sort(key=lambda b:b['x'])
    ck(name+' four books locked',len(buttons)==5 and [b['enabled'] for b in buttons]==[False,False,True,False,False])
    return data

def finish():
    R['sequence_finished']=True;save();unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()

def tick(delta):
    global INDEX,NEXT,F,P,D,PARTICLES
    if time.monotonic()<NEXT:return
    try:
        if time.monotonic()-START>240:raise RuntimeError('QA timed out')
        if INDEX==0:
            ck('start PIE',LIB.begin_frontend_pie(720,1280));NEXT=time.monotonic()+10
        elif INDEX==1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:return
            directors=unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.InkMenuDirector)
            if not directors:return
            D=directors[0];F=D.get_editor_property('frontend_widget')
            if not F:return
            F.trigger_action('StartGame');NEXT=time.monotonic()+4
        elif INDEX==2:
            D.set_actor_tick_enabled(False);fixture();F.open_feature_panel('CharacterProfile','');NEXT=time.monotonic()+3
        elif INDEX==3:
            layout('portrait')
            if BASELINE:finish();return
            P=F.get_editor_property('extension_pages')['CharacterProfile']
            NEXT=time.monotonic()+1
        elif INDEX==4:
            ctx=unreal.InkAttributeRatingContext()
            lib=unreal.InkAttributeRatingLibrary
            for key in ('Health','Attack','Defense','SpiritDamage','Critical','Evasion','MoveSpeed','CultivationSpeed'):
                for grade,value in enumerate(lib.thresholds(key,ctx)):
                    fixture({key:value})
                    expected=('D','C','B','A','S','SS','SSS')[grade]
                    ck('live grade boundary '+key+' '+expected,grade_resource(key)=='T_Rating_'+expected,grade_resource(key))
            fixture(numeric=False)
            ck('untyped values never parsed for a fake rating',not any(x['visible'] and x['name'].startswith('ProfileRating_') for x in snapshot()['images']))
            fixture()
            widths=[];radii=[]
            for i,tier in ((2,2),(5,1),(6,3),(7,6)):
                m=image_widget('ProfileSlotGlow_'+str(i)).get_dynamic_material()
                widths.append((tier,m.get_scalar_parameter_value('BorderWidth')))
                radii.append((tier,m.get_scalar_parameter_value('GlowRadius')))
                actual=m.get_vector_parameter_value('GlowColor');expected=P.get_editor_property('quality_colors')[tier]
                ck('equipped tier drives continuous frame color '+str(i),abs(actual.r-expected.r)+abs(actual.g-expected.g)+abs(actual.b-expected.b)<.001)
            for label,values in [('border width',widths),('glow radius',radii)]:
                values.sort();ck('quality increases '+label,all(values[i][1]<values[i+1][1] for i in range(len(values)-1)),values)
            NEXT=time.monotonic()+1
        elif INDEX==5:
            PARTICLES=particles();R['particles_before']=PARTICLES;NEXT=time.monotonic()+.4
        elif INDEX==6:
            after=particles();R['particles']=after
            ck('four equipped borders and core active',active_count()==5,active_count())
            for i in (0,1,3,4):ck('locked empty frame no particles '+str(i),not after[i]['active'])
            for i in (2,5,6,7):
                center=variable(components()[i],'vec2','User.Source');extent=variable(components()[i],'vec2','User.Extent')
                previous={p['id']:p for p in PARTICLES[i].get('particles',[])};confined=[];moved=[]
                for p in after[i].get('particles',[]):
                    pos=p['position'];x,y=abs(pos[0]-center.x),abs(pos[2]-center.y)
                    confined.append(x<=extent.x+.1 and y<=extent.y+.1 and max(x/extent.x,y/extent.y)>.98)
                    if p['id'] in previous:
                        b=previous[p['id']]['position'];moved.append(abs(pos[0]-b[0])+abs(pos[2]-b[2])>.05)
                ck('Niagara confined to frame '+str(i),bool(confined) and all(confined))
                ck('Niagara flows '+str(i),bool(moved) and any(moved))
            fixture(empty=True);NEXT=time.monotonic()+1
        elif INDEX==7:
            ck('unequipped slots lose glowing frame',len(glows())==0)
            ck('unequipped slots stop Niagara',active_count()==1,active_count())
            fixture();P.set_editor_property('bPreviewRealm',True);P.set_editor_property('preview_realm_index',16);P.refresh_character_presentation();NEXT=time.monotonic()+1
        elif INDEX==8:
            ck('golden core halo preserved',variable(components()[-1],'float','User.Halo')==1)
            capture('golden-core');LIB.set_pie_viewport_size(1280,720);NEXT=time.monotonic()+2
        elif INDEX==9:
            layout('landscape');ck('responsive rebuild no leaked effects',active_count()==5,active_count())
            F.set_editor_property('bReducedMotion',True);NEXT=time.monotonic()+1
        elif INDEX==10:
            ck('reduced motion stops particles',active_count()==0)
            ck('reduced motion preserves continuous frame',len(glows())==4)
            capture('reduced-motion');F.set_editor_property('bReducedMotion',False);LIB.set_pie_viewport_size(540,960);NEXT=time.monotonic()+2
        elif INDEX==11:
            layout('narrow-portrait');ck('motion resumes',active_count()==5)
            ck('locked book not clickable',not LIB.click_action(F,'Ext_ProfileMethod_0'))
            F.trigger_action('Ext_ProfileMethod_0');ck('locked dispatched action ignored',str(F.get_editor_property('current_page'))=='CharacterProfile')
            F.close_feature_panel(True);NEXT=time.monotonic()+1
        elif INDEX==12:ck('hidden profile stops particles',active_count()==0)
        else:finish();return
        INDEX+=1
    except Exception:
        R['errors'].append(traceback.format_exc());save();finish()

HANDLE=unreal.register_slate_post_tick_callback(tick)
