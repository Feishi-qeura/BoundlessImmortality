"""Rendered button regression. Uses transient rows and never reads reference images."""
import json
import time
import traceback
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
BEFORE = '-buttonbefore' in unreal.SystemLibrary.get_command_line().lower()
OUT = ROOT / 'Saved/Codex/ButtonReadability20261022' / ('Before' if BEFORE else 'After')
OUT.mkdir(parents=True, exist_ok=True)
L = unreal.InkFrontendEditorLibrary
R = dict(checks=[], captures=[], finished=False, boundary='Windows rendered PIE; explicit transient preview; no mobile/device validation')
F = D = None
INDEX = -2
NEXT = 0
STEPS = []


def check(name, passed, actual=None):
    R['checks'].append(dict(name=name, passed=bool(passed), actual=actual))


def save():
    R['passed'] = R['finished'] and not R.get('error') and all(c['passed'] for c in R['checks'])
    (OUT / 'report.json').write_text(json.dumps(R, ensure_ascii=False, indent=2), encoding='utf-8')


def setup():
    D.set_actor_tick_enabled(False)
    rows = []
    for category in ('Anchor', 'CultivationSpeed', 'Damage'):
        e = unreal.InkLeaderboardEntry()
        for key, value in dict(participant_id='button-preview', display_name=unreal.Text('主播'), category=category,
                               rank=1, metric=unreal.Text('1 把' if category == 'Anchor' else '125.5 /秒'), numeric_metric=1).items():
            e.set_editor_property(key, value)
        rows.append(e)
    F.set_leaderboard_entries(rows)
    F.set_leaderboard_entries_for_scope(rows, 'Global')
    F.set_global_leaderboard_connection(True, unreal.Text('本地界面预览'))
    F.show_page('Leaderboard')


def snapshot(tag):
    L.move_frontend_pointer_outside(F)
    s = json.loads(L.describe_widget(F))
    (OUT / (tag + '.json')).write_text(json.dumps(s, ensure_ascii=False, indent=2), encoding='utf-8')
    path = OUT / (tag + '.png')
    check(tag + ' screenshot', L.capture_frontend_viewport(str(path)))
    R['captures'].append(str(path))
    buttons = {b['name']: b for b in s['buttons']}
    close = buttons['RankClose']
    check(tag + ' close visible', close['visible'])
    check(tag + ' close at least 48 logical units', close['width'] >= 48 and close['height'] >= 48, [close['width'], close['height']])
    close_obj = unreal.find_object(None, close['object'])
    font = close_obj.get_content().get_editor_property('font')
    check(tag + ' cross at least 32', font.get_editor_property('size') >= 32, font.get_editor_property('size'))
    check(tag + ' square close skin', close_obj.get_editor_property('widget_style').normal.draw_as == unreal.SlateBrushDrawType.ROUNDED_BOX)
    for name in ('RankLocal', 'RankGlobal', 'RankAnchor', 'RankCultivation', 'RankDamage'):
        b = buttons[name]
        check(tag + ' adaptive source skin ' + name, 'Slice' in b['resource'], b['resource'])
        obj = unreal.find_object(None, b['object'])
        label = obj.get_content().get_content()
        font = label.get_editor_property('font')
        check(tag + ' single line ' + name, not b['label_auto_wrap'])
        check(tag + ' label fits ' + name, b['label_desired_width'] <= b['label_width'] + 1)
        if name in ('RankLocal', 'RankAnchor'):
            check(tag + ' light text outline ' + name, font.outline_settings.outline_size >= 1)
    row = next(i for i in s['images'] if i['name'] == 'RankAnchorRowArt_2')
    brush = unreal.find_object(None, row['object']).get_editor_property('brush')
    check(tag + ' empty row outline', brush.outline_settings.width >= 1, brush.outline_settings.width)
    check(tag + ' empty row opacity', brush.tint_color.specified_color.a >= .7, brush.tint_color.specified_color.a)


def click(action):
    check('Slate click ' + action, L.click_action(F, action))


def menu_snapshot(tag):
    L.move_frontend_pointer_outside(F)
    s = json.loads(L.describe_widget(F))
    (OUT / (tag + '.json')).write_text(json.dumps(s, ensure_ascii=False, indent=2), encoding='utf-8')
    path = OUT / (tag + '.png')
    check(tag + ' screenshot', L.capture_frontend_viewport(str(path)))
    R['captures'].append(str(path))
    buttons = [b for b in s['buttons'] if b['visible'] and b['name'].startswith('Main_')]
    check(tag + ' four original actions', {b['action'] for b in buttons} == {'StartGame', 'OpenSaves', 'OpenSettings', 'RequestQuit'})
    for b in buttons:
        style = unreal.find_object(None, b['object']).get_editor_property('widget_style')
        if b['action'] in ('OpenSettings', 'RequestQuit'):
            for state in ('normal', 'hovered', 'pressed', 'disabled'):
                brush = getattr(style, state)
                resource = brush.get_editor_property('resource_object')
                check(tag + ' guarded edge ' + b['action'] + ' ' + state, resource and resource.get_name().endswith('Clean'), resource.get_path_name() if resource else None)
        else:
            check(tag + ' original art ' + b['action'], 'M_ProvidedMenu' in b['resource'] and 'Clean' not in b['resource'])


def feature_close_check():
    s = json.loads(L.describe_widget(F))
    candidates = [b for b in s['buttons'] if b['visible'] and b['action'] == 'Ext_Close']
    check('feature close present', len(candidates) == 1)
    if candidates:
        b = candidates[0]
        obj = unreal.find_object(None, b['object'])
        check('feature close retains 88-unit target', b['width'] >= 88 and b['height'] >= 88)
        size = obj.get_content().get_editor_property('font').size
        check('feature cross enlarged', size >= 40, size)
        check('feature close square skin', obj.get_editor_property('widget_style').normal.draw_as == unreal.SlateBrushDrawType.ROUNDED_BOX)


def step(name, fn, delay=1):
    STEPS.append((name, fn, delay))


if not BEFORE:
    step('main portrait', lambda: menu_snapshot('main-portrait'))
    step('main settings hover', lambda: check('hover settings', L.hover_action(F, 'OpenSettings')))
    step('main hover pixels', lambda: check('main hover screenshot', L.capture_frontend_viewport(str(OUT / 'main-portrait-hover.png'))))
    step('main settings click', lambda: click('OpenSettings'))
    step('settings result', lambda: check('settings opens original page', str(F.get_editor_property('current_page')) == 'Settings'))
    step('main restore', lambda: F.show_page('Main'))
    step('exit request', lambda: click('RequestQuit'))
    step('exit confirmation', lambda: check('exit opens original confirmation', str(F.get_editor_property('pending_confirmation_action')) == 'QuitGame'))
    step('cancel exit', lambda: F.trigger_action('CancelPending'))
    step('main landscape resize', lambda: L.set_pie_viewport_size(1280, 720), 3)
    step('main landscape', lambda: menu_snapshot('main-landscape'))
    step('portrait restore', lambda: L.set_pie_viewport_size(720, 1280), 2)
step('start game', lambda: check('start game', D.start_game()), 4)
step('fixture', setup, 3)
step('portrait check', lambda: snapshot('portrait'))
step('hover pale state', lambda: check('Slate hover selected', L.hover_action(F, 'RankAnchor')))
step('hover capture', lambda: check('hover screenshot', L.capture_frontend_viewport(str(OUT / 'portrait-hover.png'))))
step('switch damage', lambda: click('RankDamage'))
step('switch speed', lambda: click('RankCultivation'))
step('restore anchor', lambda: click('RankAnchor'))
step('switch global', lambda: click('RankGlobal'))
step('restore local', lambda: click('RankLocal'))
step('landscape', lambda: L.set_pie_viewport_size(1280, 720), 3)
step('landscape check', lambda: snapshot('landscape'))
step('close', lambda: click('RankClose'))
step('close result', lambda: check('close returns to HUD', str(F.get_editor_property('current_page')) == 'HUD'))
if not BEFORE:
    step('feature open', lambda: F.open_feature_panel('MonthlySignIn', ''), 3)
    step('feature close check', feature_close_check)
    step('feature close', lambda: click('Ext_Close'))
    step('feature closed', lambda: check('feature close returns to HUD', str(F.get_editor_property('current_page')) == 'HUD'))


def tick(dt):
    global INDEX, NEXT, F, D
    if time.monotonic() < NEXT:
        return
    try:
        if INDEX == -2:
            check('begin PIE', L.begin_frontend_pie(720, 1280))
            INDEX = -1
            NEXT = time.monotonic() + 12
            return
        if INDEX == -1:
            worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                NEXT = time.monotonic() + 1
                return
            directors = unreal.GameplayStatics.get_all_actors_of_class(worlds[0], unreal.InkMenuDirector)
            if not directors:
                NEXT = time.monotonic() + 1
                return
            D = directors[0]
            F = D.get_editor_property('frontend_widget')
            if not F:
                NEXT = time.monotonic() + 1
                return
            INDEX = 0
            NEXT = time.monotonic() + 3
            return
        if INDEX >= len(STEPS):
            R['finished'] = True
            save()
            unreal.unregister_slate_post_tick_callback(HANDLE)
            unreal.EditorLevelLibrary.editor_end_play()
            def quit(dt):
                if unreal.EditorLevelLibrary.get_pie_worlds(False):
                    return
                unreal.unregister_slate_post_tick_callback(QUIT[0])
                unreal.SystemLibrary.quit_editor()
            QUIT[0] = unreal.register_slate_post_tick_callback(quit)
            return
        name, fn, delay = STEPS[INDEX]
        R['step'] = name
        fn()
        INDEX += 1
        NEXT = time.monotonic() + delay
        save()
    except Exception:
        R['error'] = traceback.format_exc()
        save()
        unreal.unregister_slate_post_tick_callback(HANDLE)
        unreal.EditorLevelLibrary.editor_end_play()
        unreal.SystemLibrary.quit_editor()


QUIT = [None]
HANDLE = unreal.register_slate_post_tick_callback(tick)
