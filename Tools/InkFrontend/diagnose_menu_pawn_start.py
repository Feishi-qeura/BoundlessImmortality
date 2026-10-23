"""One isolated rendered PIE: record actual StartGame guards and floor traces, then exit."""
import json
import time
import traceback
from pathlib import Path
import unreal

OUT = Path(unreal.Paths.project_dir()) / 'Saved/Codex/NiagaraUiRevision20261022/PawnStartDiagnostic'
OUT.mkdir(parents=True, exist_ok=True)
R = {'snapshots': []}
INDEX = 0
NEXT = 0


def save():
    (OUT / 'report.json').write_text(json.dumps(R, ensure_ascii=False, indent=2), encoding='utf-8')


def read(fn):
    try:
        value = fn()
        if value is None or isinstance(value, (bool, str, int, float)):
            return value
        return str(value)
    except Exception as e:
        return {'error': str(e)}


def pawn_info(pawn):
    if not pawn:
        return None
    return {'name': pawn.get_name(), 'class': pawn.get_class().get_path_name(),
            'transform': str(pawn.get_actor_transform()), 'velocity': str(pawn.get_velocity()),
            'movement_mode': read(lambda: pawn.get_movement_component().get_editor_property('movement_mode')),
            'capsule_half': read(lambda: pawn.get_capsule_component().get_scaled_capsule_half_height()),
            'capsule_radius': read(lambda: pawn.get_capsule_component().get_scaled_capsule_radius()),
            'walkable_z': read(lambda: pawn.get_movement_component().get_walkable_floor_z()),
            'capsule_collision': read(lambda: pawn.get_capsule_component().get_collision_profile_name())}


def snapshot(tag, world, director, pc, mode):
    demo = director.get_presentation_pawn()
    player = unreal.GameplayStatics.get_player_pawn(world, 0)
    menu_class = demo.get_class() if demo else None
    info = {'tag': tag, 'status': director.get_editor_property('presentation_status'),
            'menu_active': director.get_editor_property('menu_active'),
            'pc_pawn': pawn_info(player), 'presentation': pawn_info(demo),
            'presentation_pawn_class': read(lambda: menu_class.get_path_name()),
            'gameplay_pawn_class': read(lambda: director.get_editor_property('gameplay_pawn_class')),
            'mode_class': mode.get_class().get_path_name(),
            'mode_default_pawn': read(lambda: mode.get_editor_property('default_pawn_class').get_path_name()),
            'player_is_presentation_subclass': read(lambda: unreal.SystemLibrary.class_is_child_of(player.get_class(), menu_class)),
            'mode_default_is_presentation_subclass': read(lambda: unreal.SystemLibrary.class_is_child_of(mode.get_editor_property('default_pawn_class'), menu_class)),
            'find_player_start': read(lambda: mode.find_player_start(pc)),
            'starts': [], 'traces': []}
    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    for start in starts:
        info['starts'].append({'name': start.get_name(), 'transform': str(start.get_actor_transform())})
    for tag2, actor in [('pawn', player)] + [('start_' + s.get_name(), s) for s in starts]:
        if not actor:
            continue
        pos = actor.get_actor_location()
        for n in (1, 2):
            trace = getattr(unreal.TraceTypeQuery, 'TRACE_TYPE_QUERY' + str(n))
            info['traces'].append({'at': tag2, 'channel_query': n,
                                    'hit': read(lambda: unreal.SystemLibrary.line_trace_single(world, pos + unreal.Vector(0, 0, 12), pos - unreal.Vector(0, 0, 300), trace, False, [p for p in (player, demo) if p], unreal.DrawDebugTrace.NONE, True))})
    R['snapshots'].append(info)
    save()


def tick(delta):
    global INDEX, NEXT
    if time.monotonic() < NEXT:
        return
    try:
        if INDEX == 0:
            unreal.InkFrontendEditorLibrary.begin_frontend_pie(720, 1280)
            NEXT = time.monotonic() + 16
        elif INDEX == 1:
            worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                raise RuntimeError('PIE did not start')
            world = worlds[0]
            director = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.InkMenuDirector)[0]
            pc = unreal.GameplayStatics.get_player_controller(world, 0)
            mode = unreal.GameplayStatics.get_game_mode(world)
            R['status_before'] = read(lambda: director.get_editor_property('presentation_status'))
            R['pc_before'] = read(lambda: unreal.GameplayStatics.get_player_pawn(world, 0).get_class().get_path_name())
            R['demo_before'] = read(lambda: director.get_presentation_pawn().get_class().get_path_name())
            save()
            R['start_result'] = director.start_game()
            R['status_after'] = read(lambda: director.get_editor_property('presentation_status'))
            save()
            try:
                snapshot('after start', world, director, pc, mode)
            except Exception:
                R['optional_snapshot_error'] = traceback.format_exc()
            R['finished'] = True
            save()
            unreal.unregister_slate_post_tick_callback(H)
            unreal.EditorLevelLibrary.editor_end_play()
            unreal.SystemLibrary.quit_editor()
            return
        INDEX += 1
    except Exception:
        R['error'] = traceback.format_exc()
        save()
        unreal.unregister_slate_post_tick_callback(H)
        unreal.EditorLevelLibrary.editor_end_play()
        unreal.SystemLibrary.quit_editor()


H = unreal.register_slate_post_tick_callback(tick)
