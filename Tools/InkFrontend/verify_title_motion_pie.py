"""Cheap rendered CPU-motion proof: cache three components, sample five adjacent frames.

No screenshots, widget-tree serialization, ObjectIterator, or disk writes in the
sampling window. Uses a transient base-realm fixture for comparable HUD quality.
"""
from pathlib import Path
import json
import math
import time
import traceback
import unreal

OUT = Path(unreal.Paths.project_dir()) / 'Saved/Codex/NiagaraUiRevision20261022/TitleMotion'
LIB = unreal.InkFrontendEditorLibrary
INSPECT = unreal.InkCultivationEditorLibrary.inspect_component
REPORT = {'checks': [], 'frames': [], 'finished': False,
          'boundary': 'Windows rendered PIE CPU particle positions; no screenshot or mobile claim'}
PHASE = 'boot'
NEXT = 0.0
STARTED = time.monotonic()
DIRECTOR = FRONTEND = WORLD = STATE = None
TARGETS = []


def check(name, passed, actual=None):
    REPORT['checks'].append({'name': name, 'passed': bool(passed), 'actual': actual})


def finish(error=None):
    if error:
        REPORT['error'] = error
    REPORT['finished'] = not bool(error)
    REPORT['passed'] = REPORT['finished'] and all(c['passed'] for c in REPORT['checks'])
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / 'report.json').write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play()
    unreal.SystemLibrary.quit_editor()


def float_parameter(component, name):
    value = component.get_variable_float(name)
    if isinstance(value, (tuple, list)):
        return {'value': float(value[0]), 'valid': bool(value[1])}
    return {'value': float(value), 'valid': True}


def evaluate():
    frames = REPORT['frames']
    for label, widget, component in TARGETS:
        matched = []
        previous = []
        for frame in frames:
            s = frame['targets'][label]
            for p in s.get('particles', []):
                if p.get('displacement', 0) > .0001:
                    previous.append({'id': p['id'], 'displacement': p['displacement'],
                                     'position': p['position'], 'previous_position': p.get('previous_position')})
        for left, right in zip(frames, frames[1:]):
            older = {p['id']: p for p in left['targets'][label].get('particles', [])}
            for p in right['targets'][label].get('particles', []):
                before = older.get(p['id'])
                if before:
                    distance = math.sqrt(sum((x-y)**2 for x, y in zip(p['position'], before['position'])))
                    if distance > .0001:
                        matched.append({'id': p['id'], 'distance': distance,
                                        'from': before['position'], 'to': p['position']})
        check(label + ' has actual CPU particles', any(sum(f['targets'][label].get('counts', [])) > 0 for f in frames))
        check(label + ' same live IDs move across adjacent rendered frames', bool(matched), matched[:8])
        check(label + ' Previous.Position independently proves within-frame motion', bool(previous), previous[:8])

    realm = [f['targets']['HUDRealm'] for f in frames]
    avatar = [f['targets']['HUDFrame'] for f in frames]
    check('HUD higher quality uses higher spawn rate',
          avatar[-1]['quality'] > realm[-1]['quality'] and
          avatar[-1]['rate']['valid'] and realm[-1]['rate']['valid'] and
          avatar[-1]['rate']['value'] > realm[-1]['rate']['value'],
          {'low_quality': realm[-1]['quality'], 'high_quality': avatar[-1]['quality'],
           'low_rate': realm[-1]['rate'], 'high_rate': avatar[-1]['rate']})
    check('HUD higher quality uses larger moving sprites', avatar[-1]['size']['value'] > realm[-1]['size']['value'],
          {'low_size': realm[-1]['size'], 'high_size': avatar[-1]['size']})
    low = sum(sum(s.get('counts', [])) for s in realm) / len(realm)
    high = sum(sum(s.get('counts', [])) for s in avatar) / len(avatar)
    check('HUD higher quality has more live CPU particles', high > low, {'low_mean': low, 'high_mean': high})
    finish()


def tick(delta):
    global PHASE, NEXT, DIRECTOR, FRONTEND, WORLD, STATE, TARGETS
    if time.monotonic() - STARTED > 100:
        finish('Timed out in phase ' + PHASE)
        return
    if time.monotonic() < NEXT:
        return
    try:
        if PHASE == 'boot':
            check('begin rendered PIE', LIB.begin_frontend_pie(720, 1280))
            PHASE = 'start'
            NEXT = time.monotonic() + 12
            return
        if PHASE == 'start':
            worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                NEXT = time.monotonic() + .3
                return
            WORLD = worlds[0]
            directors = unreal.GameplayStatics.get_all_actors_of_class(WORLD, unreal.InkMenuDirector)
            if not directors:
                NEXT = time.monotonic() + .3
                return
            DIRECTOR = directors[0]
            FRONTEND = DIRECTOR.get_editor_property('frontend_widget')
            if not FRONTEND:
                NEXT = time.monotonic() + .3
                return
            started = DIRECTOR.start_game()
            check('StartGame enters gameplay', started)
            if not started:
                raise RuntimeError(str(DIRECTOR.get_editor_property('presentation_status')))
            PHASE = 'fixture'
            NEXT = time.monotonic() + .35  # Allow startup actor/subsystem work before state requests.
            return
        if PHASE == 'fixture':
            DIRECTOR.set_actor_tick_enabled(False)
            STATE = unreal.LiveWeaponEditorLibrary.get_cultivation_state(WORLD)
            STATE.set_editor_property('persist_local_records', False)
            records = json.loads(STATE.export_records())
            anchor_id = STATE.get_anchor_id()
            anchor = next((r for r in records['records'] if r['participant_id'] == anchor_id), None)
            if anchor:
                anchor['realm_index'] = 0
                anchor['version'] = anchor.get('version', 0) + 1
                check('transient base-realm quality fixture accepted', STATE.import_records(json.dumps(records, ensure_ascii=False), False))
            for actor in unreal.GameplayStatics.get_all_actors_of_class(WORLD, unreal.LiveGMOverlay):
                adapter = actor.get_editor_property('audience')
                if adapter:
                    adapter.add_simulated_audience(1)
                    break
            FRONTEND.set_editor_property('reduced_motion', False)
            PHASE = 'cache'
            NEXT = time.monotonic() + 3
            return
        if PHASE == 'cache':
            # Traverse once; neither traversal nor widget serialization occurs during samples.
            candidates = [w for w in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget)
                          if w.get_effect_widget() and w.is_effect_active()]
            wanted = [('HUDRealm', 'AnchorRealmTitle'), ('HUDFrame', 'AnchorAvatarFrame'), ('WorldHonor', 'SwordHonor')]
            TARGETS = []
            for label, name in wanted:
                widget = next((w for w in candidates if w.get_name() == name), None)
                if widget is None:
                    NEXT = time.monotonic() + .3
                    return
                TARGETS.append((label, widget, widget.get_effect_widget().get_niagara_component()))
            REPORT['targets'] = {label: widget.get_path_name() for label, widget, component in TARGETS}
            PHASE = 'sample'
            return
        if PHASE == 'sample':
            sample = {'frame': len(REPORT['frames']), 'delta_seconds': delta, 'clock': time.monotonic(), 'targets': {}}
            for label, widget, component in TARGETS:
                state = json.loads(INSPECT(component))
                state.update(quality=widget.get_effect_quality(),
                             rate=float_parameter(component, 'User.Rate'),
                             size=float_parameter(component, 'User.ParticleSize'))
                sample['targets'][label] = state
            REPORT['frames'].append(sample)
            # Five callbacks in succession, with all evidence retained in memory.
            if len(REPORT['frames']) == 5:
                PHASE = 'evaluate'
            return
        if PHASE == 'evaluate':
            evaluate()
    except Exception:
        finish(traceback.format_exc())


HANDLE = unreal.register_slate_post_tick_callback(tick)
