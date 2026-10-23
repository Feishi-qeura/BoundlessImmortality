"""Render the existing anchor HUD in portrait, landscape and inset safe areas."""
import gc
import hashlib
import json
import time
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir())
BEFORE = '-anchorbackgroundbefore' in unreal.SystemLibrary.get_command_line().lower()
OUT = ROOT / 'Saved/Codex/AnchorBackground20261022' / ('Before' if BEFORE else 'After')
OUT.mkdir(parents=True, exist_ok=True)
L = unreal.InkFrontendEditorLibrary
SOURCE = ROOT / 'Source/BoundlessImmortality/Frontend/InkAnchorStatusWidget.cpp'
R = dict(checks=[], captures=[], finished=False, source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
         boundary='Windows rendered PIE with explicit local simulation; no device testing')
F = D = W = None
INDEX = -2
NEXT = 0
START = time.monotonic()


def check(name, value, actual=None):
    R['checks'].append(dict(name=name, passed=bool(value), actual=actual))


def save():
    R['passed'] = R['finished'] and not R.get('error') and all(c['passed'] for c in R['checks'])
    (OUT / 'report.json').write_text(json.dumps(R, ensure_ascii=False, indent=2), 'utf-8')


def capture(tag):
    L.move_frontend_pointer_outside(F)
    path = OUT / (tag + '.png')
    check('capture ' + tag, L.capture_frontend_viewport(str(path)))
    snapshot = json.loads(L.describe_widget(F))
    (OUT / (tag + '.json')).write_text(json.dumps(snapshot, ensure_ascii=False, indent=2), 'utf-8')
    R['captures'].append(str(path))
    check(tag + ' actual HUD', snapshot['page'] == 'HUD')
    status = snapshot['anchor_status']
    check(tag + ' visible status group', status['paint_width'] > 0 and status['paint_height'] > 0, status)
    anchor = unreal.find_object(None, status['object'])
    health = unreal.LiveWeaponEditorLibrary.get_cultivation_state(W).get_anchor_health()
    check(tag + ' health stays bound', anchor.get_editor_property('health') == health.get_editor_property('health'))
    check(tag + ' shield remains real absence', anchor.get_editor_property('max_shield') == 0)


def fixture():
    texture = unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture')
    check('explicit avatar fixture', L.set_frontend_test_anchor_avatar(F, 'https://fixture.invalid/anchor-background.png', texture))


def inset_fixture():
    # Windows has no physical notch. Translate the existing safe canvas to
    # verify that every painted backing patch moves with its status group.
    canvas = next(w for w in unreal.ObjectIterator(unreal.CanvasPanel)
                  if w.get_name() == 'SafeCanvas' and w.get_world() == W)
    canvas.set_render_translation(unreal.Vector2D(18, 22))
    R['safe_area_fixture'] = 'Windows canvas translation (18,22); not mobile notch emulation'


STEPS = [
    ('portrait size', lambda: check('resize portrait', L.set_pie_viewport_size(720, 1280)), 2),
    ('portrait', lambda: capture('hud-portrait'), 1),
    ('avatar fixture', fixture, 2),
    ('avatar', lambda: capture('hud-avatar-fixture'), 1),
    ('landscape size', lambda: check('resize landscape', L.set_pie_viewport_size(1280, 720)), 2),
    ('landscape', lambda: capture('hud-landscape'), 1),
    ('safe area offset fixture', inset_fixture, 2),
    ('safe landscape', lambda: capture('hud-safe-landscape'), 1),
    ('safe portrait size', lambda: check('resize safe portrait', L.set_pie_viewport_size(720, 1280)), 2),
    ('safe portrait', lambda: capture('hud-safe-portrait'), 1),
]


def finish():
    global F, D, W
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play()
    F = D = W = None

    def quit_tick(dt):
        if unreal.EditorLevelLibrary.get_pie_worlds(False):
            return
        unreal.unregister_slate_post_tick_callback(QUIT[0])
        gc.collect()
        unreal.SystemLibrary.quit_editor()

    QUIT[0] = unreal.register_slate_post_tick_callback(quit_tick)


def tick(dt):
    global INDEX, NEXT, F, D, W
    if time.monotonic() < NEXT:
        return
    try:
        if time.monotonic() - START > 180:
            raise TimeoutError('Anchor background PIE exceeded 180s')
        if INDEX == -2:
            check('begin PIE', L.begin_frontend_pie(720, 1280))
            INDEX = -1
            NEXT = time.monotonic() + 12
            return
        if INDEX == -1:
            worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                return
            W = worlds[0]
            directors = unreal.GameplayStatics.get_all_actors_of_class(W, unreal.InkMenuDirector)
            if not directors:
                return
            D = directors[0]
            F = D.get_editor_property('frontend_widget')
            if not F:
                return
            check('existing start game', D.start_game())
            INDEX = 0
            NEXT = time.monotonic() + 4
            return
        if INDEX >= len(STEPS):
            R['finished'] = True
            save()
            finish()
            return
        name, fn, delay = STEPS[INDEX]
        R['current_step'] = name
        fn()
        INDEX += 1
        NEXT = time.monotonic() + delay
        save()
    except Exception:
        R['error'] = traceback.format_exc()
        save()
        finish()


QUIT = [None]
HANDLE = unreal.register_slate_post_tick_callback(tick)
