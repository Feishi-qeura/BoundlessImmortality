"""Rendered character panel acceptance, using transient preview inputs only."""
import json, time, traceback
from pathlib import Path
import unreal

OUT = Path(unreal.Paths.project_dir()) / 'Saved/Codex/CharacterProfile20261022/PIE'
BASELINE = '-ProfileBaseline' in unreal.SystemLibrary.get_command_line()
OUT = OUT / ('baseline' if BASELINE else 'final')
OUT.mkdir(parents=True, exist_ok=True)
LIB = unreal.InkFrontendEditorLibrary
R = dict(checks=[], captures=[], errors=[])
F = D = P = HANDLE = None
INDEX = 0
NEXT = 0
START = time.monotonic()
PARTICLES = None
REALMS = (0, 9, 13, 16, 22)
LABELS = ('练气一阶', '筑基初期', '结晶一阶', '金丹一阶', '元婴')

def save():
    R['passed'] = bool(R['checks']) and not R['errors'] and all(c['passed'] for c in R['checks'])
    (OUT / 'report.json').write_text(json.dumps(R, ensure_ascii=False, indent=2), encoding='utf-8')

def ck(name, result, actual=None):
    R['checks'].append(dict(name=name, passed=bool(result), actual=actual)); save()

def snap(): return json.loads(LIB.describe_widget(F))

def fixture():
    rows = [('Health', '生命', '175 / 175'), ('Attack', '剑灵攻击', '18'), ('Defense', '防御', '0'),
            ('SpiritDamage', '剑灵伤害', '0'), ('Critical', '会心一击', '0.0%'), ('Evasion', '闪避', '0.0%'),
            ('Luck', '幸运（预留）', '0.0'), ('MoveSpeed', '实际移速上限', '400 厘米/秒'), ('Swordsmanship', '剑法（预留）', '1.8'),
            ('CultivationSpeed', '修炼速度', '1.75 /秒'), ('Cultivation', '修为', '346 / 449'), ('RealmStone', '突破材料', '30 / 1'), ('Realm', '境界', '练气一阶')]
    attrs = []
    for key, label, value in rows:
        a = unreal.InkCharacterAttribute(); a.set_editor_property('attribute_id', key)
        a.set_editor_property('label', unreal.Text(label)); a.set_editor_property('value', unreal.Text(value)); attrs.append(a)
    disciplines = []
    for category, name in (('Movement', '测试轻功'), ('Inner', '测试内功'), ('Martial', '测试武神')):
        d = unreal.InkDisciplineEntry(); d.set_editor_property('category', category)
        d.set_editor_property('name', unreal.Text(name)); d.set_editor_property('detail', unreal.Text('仅用于渲染验收'))
        disciplines.append(d)
    F.set_character_data(unreal.Text('主播 · UI验收预览'), attrs, disciplines)

def components():
    return [e.get_niagara_component() for e in P.get_editor_property('cultivation_visual').get_effects()]

def particles():
    return [json.loads(unreal.InkCultivationEditorLibrary.inspect_component(c)) for c in components()]

def variable(c, kind, name):
    result = getattr(c, 'get_variable_' + kind)(name)
    return result[0] if isinstance(result, tuple) else result

def realm(index):
    P.set_editor_property('bPreviewRealm', True)
    P.set_editor_property('preview_realm_index', REALMS[index])
    P.refresh_character_presentation()

def active_profile_effects():
    actors = unreal.GameplayStatics.get_all_actors_of_class(D, unreal.NiagaraUIActor)
    result = []
    for a in actors:
        c = a.get_component_by_class(unreal.NiagaraUIComponent)
        asset = c.get_asset() if c else None
        if asset and '/CharacterProfile20261022/' in asset.get_path_name() and c.is_active(): result.append(c)
    return result

def capture(name):
    path = OUT / (name + '.png')
    ck('capture ' + name, LIB.capture_frontend_viewport(str(path)))
    data = snap()
    (OUT / (name + '.json')).write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    R['captures'].append(str(path)); save()
    return data

def layout(name):
    data = capture(name)
    header = [t for t in data['texts'] if t['visible'] and t['name'] in ('ProfileName', 'ProfileRealm')]
    header.sort(key=lambda t: t['x'])
    ck(name + ' name and realm have separate bounds', len(header) == 2 and header[0]['x']+header[0]['painted_width'] <= header[1]['x']+1)
    for t in header:
        widget = unreal.find_object(None, t['object'])
        ck(name + ' header single line ' + t['name'], not widget.get_editor_property('auto_wrap_text'))
    buttons = [b for b in data['buttons'] if b['visible']]
    obsolete = {'Ext_Open_CharacterEquipment', 'Ext_Open_CharacterSkills', 'Ext_Open_CharacterArtifacts',
                'Ext_Open_CharacterFate', 'Ext_Open_CharacterAppearance', 'Ext_OriginalCharacter'}
    ck(name + ' six old navigation buttons removed', not any(b['action'] in obsolete for b in buttons))
    books = [b for b in buttons if b['action'].startswith('Ext_ProfileMethod_')]
    books.sort(key=lambda b: b['x'])
    ck(name + ' five books', len(books) == 5, len(books))
    ck(name + ' center alone unlocked', len(books) == 5 and [b['enabled'] for b in books] == [False, False, True, False, False])
    for action in ('ProfileLightness', 'ProfileInner', 'ProfileWushen', 'ProfileFabao', 'ProfileItem'):
        ck(name + ' slot ' + action, any(b['action'] == 'Ext_' + action for b in buttons))
    slot_names = [('ProfileMethod_' + str(i), 'ProfileMethodName_' + str(i)) for i in range(5)]
    slot_names += [('ProfileEquipment_' + str(i), 'ProfileEquip_' + str(i)) for i in range(4)]
    slot_names += [('ProfileFabao', 'ProfileEquip_4'), ('ProfileItem', 'ProfileEquip_5')]
    slot_names += [(action, 'ProfileDiscipline_' + str(i)) for i, action in enumerate(('ProfileLightness', 'ProfileInner', 'ProfileWushen'))]
    for action, key in slot_names:
        b = next((b for b in buttons if b['action'] == 'Ext_' + action), None)
        texts = [t for t in data['texts'] if t['visible'] and t['name'] in (key, key+'State')]
        if b and len(texts) == 2:
            ck(name + ' slot labels contained ' + key, all(t['x'] >= b['x']-1 and t['y'] >= b['y']-1 and t['x']+t['painted_width'] <= b['x']+b['painted_width']+1 and t['y']+t['painted_height'] <= b['y']+b['painted_height']+1 for t in texts))
            for t in texts:
                widget = unreal.find_object(None, t['object'])
                ck(name + ' single line ' + t['name'], not widget.get_editor_property('auto_wrap_text'))
    values = [t for t in data['texts'] if t['visible'] and t['name'].startswith('ProfileValue_')]
    rows = {}
    for v in values: rows.setdefault(round(v['y']), []).append(v)
    ck(name + ' attributes three per row', bool(rows) and all(len(row) == 3 for row in rows.values()), [len(row) for row in rows.values()])
    icons = [i for i in data['images'] if i['visible'] and i['name'].startswith('ProfileAttributeIcon_')]
    ck(name + ' every attribute has icon', len(icons) == len(values) and bool(values))
    return data

def finish():
    R['sequence_finished'] = True; save()
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play()
    unreal.SystemLibrary.quit_editor()

def tick(delta):
    global INDEX, NEXT, F, D, P, PARTICLES
    if time.monotonic() < NEXT: return
    try:
        if time.monotonic() - START > 360: raise RuntimeError('PIE acceptance timed out')
        if INDEX == 0:
            ck('begin rendered PIE', LIB.begin_frontend_pie(720, 1280)); NEXT = time.monotonic() + 10
        elif INDEX == 1:
            worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds: return
            directors = unreal.GameplayStatics.get_all_actors_of_class(worlds[0], unreal.InkMenuDirector)
            if not directors: return
            D = directors[0]; F = D.get_editor_property('frontend_widget')
            if not F: return
            F.trigger_action('StartGame'); NEXT = time.monotonic() + 4
        elif INDEX == 2:
            D.set_actor_tick_enabled(False)
            fixture()
            F.open_feature_panel('CharacterProfile', ''); NEXT = time.monotonic() + 3
        elif INDEX == 3:
            layout('portrait')
            if BASELINE: finish(); return
            P = F.get_editor_property('extension_pages')['CharacterProfile']
            P.set_editor_property('node_qualities', [0, 0, 2, 0, 0, 1, 3, 6])
            realm(0); NEXT = time.monotonic() + 3
        elif INDEX == 4:
            PARTICLES = particles(); R['particles_before'] = PARTICLES
            ck('eight slot border systems and one core', len(PARTICLES) == 9)
            ck('nine systems contain live CPU particles', len(PARTICLES) == 9 and all(x['active'] and sum(x.get('counts', [])) > 0 for x in PARTICLES))
            cs = components()
            for i in (0, 1, 3, 4):
                c = variable(cs[i], 'color', 'User.Color'); ck('locked node black ' + str(i), max(c.r, c.g, c.b) < .01)
            for i in (2, 5, 6, 7):
                c = variable(cs[i], 'color', 'User.Color'); q = P.get_editor_property('node_qualities')[i]
                expected = P.get_editor_property('quality_colors')[q]
                ck('node quality drives particle color ' + str(i), abs(c.r-expected.r)+abs(c.g-expected.g)+abs(c.b-expected.b) < .001)
            NEXT = time.monotonic() + .35
        elif INDEX == 5:
            after = particles(); R['particles_after'] = after
            for i in range(8):
                before = {p['id']: p for p in PARTICLES[i].get('particles', [])}
                center = variable(components()[i], 'vec2', 'User.Source')
                extent = variable(components()[i], 'vec2', 'User.Extent')
                moved, confined, border = [], [], []
                for p in after[i].get('particles', []):
                    b = p['position']; x, y = abs(b[0]-center.x), abs(b[2]-center.y)
                    confined.append(x <= extent.x+.1 and y <= extent.y+.1)
                    border.append(max(x/extent.x, y/extent.y) > .98)
                    if p['id'] in before:
                        a = before[p['id']]['position']; moved.append(abs(a[0]-b[0])+abs(a[2]-b[2]) > .05)
                ck('real particles move on item border ' + str(i), bool(moved) and any(moved))
                ck('particles stay inside their own frame ' + str(i), bool(confined) and all(confined))
                ck('particles remain on border without gathering lines ' + str(i), bool(border) and all(border))
            capture('portrait-qi'); realm(1); NEXT = time.monotonic() + 2
        elif 6 <= INDEX <= 9:
            stage = INDEX - 5
            visual = P.get_editor_property('cultivation_visual')
            color = visual.get_dantian_color(); expected = P.get_editor_property('dantian_colors')[stage]
            ck('realm color ' + LABELS[stage], abs(color.r-expected.r)+abs(color.g-expected.g)+abs(color.b-expected.b) < .001)
            halo = variable(components()[-1], 'float', 'User.Halo')
            ck('golden core alone has halo ' + LABELS[stage], abs(halo-(1 if stage == 3 else 0)) < .001)
            capture('portrait-stage-' + str(stage))
            if stage < 4: realm(stage+1)
            else: LIB.set_pie_viewport_size(1280, 720)
            NEXT = time.monotonic() + 2
        elif INDEX == 10:
            layout('landscape'); ck('rebuild leaves exactly nine active profile effects', len(active_profile_effects()) == 9)
            ck('locked book refuses Slate click', not LIB.click_action(F, 'Ext_ProfileMethod_0'))
            F.trigger_action('Ext_ProfileMethod_0'); ck('locked book rejects dispatched action', str(F.get_editor_property('current_page')) == 'CharacterProfile')
            F.set_editor_property('bReducedMotion', True); NEXT = time.monotonic() + .7
        elif INDEX == 11:
            ck('reduced motion stops Niagara simulation', len(active_profile_effects()) == 0)
            capture('landscape-reduced-motion'); F.set_editor_property('bReducedMotion', False); NEXT = time.monotonic() + 1
        elif INDEX == 12:
            ck('motion can resume', len(active_profile_effects()) == 9)
            ck('unlocked center book clickable', LIB.click_action(F, 'Ext_ProfileMethod_2')); NEXT = time.monotonic() + 1
        elif INDEX == 13:
            ck('center book opens method configuration', str(F.get_editor_property('current_page')) == 'CharacterSkills')
            ck('hidden profile effects stopped', len(active_profile_effects()) == 0)
            F.open_feature_panel('CharacterProfile', ''); NEXT = time.monotonic() + 1
        elif INDEX == 14:
            ck('lightness slot clickable', LIB.click_action(F, 'Ext_ProfileLightness'))
            ck('lightness shows supplied data', '测试轻功' in str(P.get_editor_property('preview_feedback')))
            ck('fabao slot clickable', LIB.click_action(F, 'Ext_ProfileFabao')); NEXT = time.monotonic() + .8
        elif INDEX == 15:
            ck('fabao opens treasure inventory', str(F.get_editor_property('current_page')) == 'Inventory' and str(F.get_editor_property('inventory_category')) == 'Treasure')
            F.open_feature_panel('CharacterProfile', ''); NEXT = time.monotonic() + .8
        elif INDEX == 16:
            ck('item slot clickable', LIB.click_action(F, 'Ext_ProfileItem')); NEXT = time.monotonic() + .8
        elif INDEX == 17:
            ck('item slot opens consumables', str(F.get_editor_property('current_page')) == 'Inventory' and str(F.get_editor_property('inventory_category')) == 'Consumable')
            F.open_feature_panel('CharacterProfile', ''); NEXT = time.monotonic() + 1
        elif INDEX == 18:
            F.close_feature_panel(True); NEXT = time.monotonic() + 1
        elif INDEX == 19:
            ck('closing panel stops every profile effect', len(active_profile_effects()) == 0)
            F.open_feature_panel('CharacterProfile', ''); LIB.set_pie_viewport_size(540, 960); NEXT = time.monotonic() + 2
        elif INDEX == 20:
            layout('narrow-portrait'); F.close_feature_panel(True); NEXT = time.monotonic() + .7
        else: finish(); return
        INDEX += 1
    except Exception:
        R['errors'].append(traceback.format_exc()); save(); finish()

HANDLE = unreal.register_slate_post_tick_callback(tick)
