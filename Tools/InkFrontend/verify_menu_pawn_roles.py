"""PIE role/lifetime regression. Run in an isolated -UserDir process, not a user session.

Does not save assets. Writes only a fresh empty manual save slot, then deletes it.
The editor is left open after PIE; the coordinating launcher owns its lifetime.
"""
import json
import time
from pathlib import Path

import unreal

OUT = Path(unreal.Paths.project_dir()) / 'Saved/Codex/NiagaraUiRevision20261022/PawnRoles'
OUT.mkdir(parents=True, exist_ok=True)
PRESENTATION_CLASS = '/Game/1/BP_TopDown.BP_TopDown_C'
GAMEPLAY_CLASS = '/Game/Isometric_World/Demo/Blueprints/BP_TopDownCharacter.BP_TopDownCharacter_C'
L = unreal.InkFrontendEditorLibrary
REPORT = {'checks': [], 'snapshots': [], 'passed': False, 'boundary': 'Windows PIE role, lifetime and safe reentry only'}
INDEX = 0
NEXT = 0
W = D = F = P = DEMO = SAVES = PC = SAFE = None
SLOT = None
DEMO_CAMERA = None
GAME_CAMERA = None
GAME_COLLISION = None
GAME_CAMERA_SETTINGS = None


def save():
    REPORT['passed'] = REPORT.get('finished', False) and all(c['passed'] for c in REPORT['checks']) and not REPORT.get('error')
    (OUT / 'report.json').write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding='utf-8')


def check(name, condition, actual=None):
    REPORT['checks'].append({'name': name, 'passed': bool(condition), 'actual': actual})
    save()


def alive(actor):
    return bool(actor) and unreal.SystemLibrary.is_valid(actor)


def camera_settings(actor):
    def transform_values(component):
        t = component.get_relative_transform()
        return [t.translation.x, t.translation.y, t.translation.z,
                t.rotation.x, t.rotation.y, t.rotation.z, t.rotation.w,
                t.scale3d.x, t.scale3d.y, t.scale3d.z]
    return [{'name': c.get_name(), 'fov': c.get_editor_property('field_of_view'),
             'aspect': c.get_editor_property('aspect_ratio'),
             'constraint': c.get_editor_property('constrain_aspect_ratio'),
             'relative': transform_values(c)}
            for c in actor.get_components_by_class(unreal.CameraComponent)]


def snapshot(tag):
    player = unreal.GameplayStatics.get_player_pawn(W, 0)
    demo = D.get_presentation_pawn()
    REPORT['snapshots'].append({'tag': tag, 'player': player.get_path_name() if player else None,
                                'player_class': player.get_class().get_path_name() if player else None,
                                'demo': demo.get_path_name() if alive(demo) else None,
                                'menu': D.get_editor_property('menu_active')})
    save()


def gameplay_checks(tag):
    player = unreal.GameplayStatics.get_player_pawn(W, 0)
    check(tag + ': gameplay instance retained', player == P)
    check(tag + ': authored gameplay class retained', player.get_class().get_path_name() == GAMEPLAY_CLASS,
          player.get_class().get_path_name())
    check(tag + ': presentation instance destroyed', not alive(DEMO))
    check(tag + ': presentation reference cleared', not alive(D.get_presentation_pawn()))
    check(tag + ': controller views gameplay', PC.get_view_target() == P)
    check(tag + ': gameplay camera settings retained', camera_settings(P) == GAME_CAMERA_SETTINGS)
    check(tag + ': gameplay collision retained', P.get_actor_enable_collision() == GAME_COLLISION)
    check(tag + ': not falling', P.get_velocity().z > -10, P.get_velocity().z)
    snapshot(tag)


def finish():
    if SAVES and SLOT is not None:
        SAVES.delete_slot(SLOT)
    save()
    unreal.unregister_slate_post_tick_callback(HANDLE)
    unreal.EditorLevelLibrary.editor_end_play()
    if '-inkqaautoexit' in unreal.SystemLibrary.get_command_line().lower():
        unreal.SystemLibrary.quit_editor()


def tick(delta):
    global INDEX, NEXT, W, D, F, P, DEMO, SAVES, PC, SAFE, SLOT, GAME_CAMERA_SETTINGS, GAME_COLLISION, DEMO_CAMERA
    if time.monotonic() < NEXT:
        return
    try:
        delay = 2
        if INDEX == 0:
            if '-userdir=' not in unreal.SystemLibrary.get_command_line().lower():
                raise RuntimeError('Use a dedicated -UserDir so the save/load regression cannot affect player data')
            L.begin_frontend_pie(720, 1280)
            delay = 12
        elif INDEX == 1:
            worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:
                NEXT = time.monotonic() + 1
                return
            W = worlds[0]
            D = unreal.GameplayStatics.get_all_actors_of_class(W, unreal.InkMenuDirector)[0]
            F = D.get_editor_property('frontend_widget')
            PC = unreal.GameplayStatics.get_player_controller(W, 0)
            initial_player = unreal.GameplayStatics.get_player_pawn(W, 0)
            DEMO = D.get_presentation_pawn()
            check('configured BP_TopDownCharacter2 is presentation instance', DEMO.get_actor_label() == 'BP_TopDownCharacter2', DEMO.get_actor_label())
            check('presentation class identity', DEMO.get_class().get_path_name() == PRESENTATION_CLASS, DEMO.get_class().get_path_name())
            REPORT['initial_pc_class'] = initial_player.get_class().get_path_name() if initial_player else None
            # This authored map auto-possesses the placed menu pawn. The independent
            # gameplay pawn is created by StartGame, not assumed to exist beforehand.
            check('initial controller views menu presentation', PC.get_view_target() == DEMO)
            DEMO_CAMERA = camera_settings(DEMO)
            snapshot('initial menu')
            started = D.start_game()
            check('start succeeds', started, D.get_editor_property('presentation_status'))
            if not started:
                raise RuntimeError('StartGame failed: ' + D.get_editor_property('presentation_status'))
            P = unreal.GameplayStatics.get_player_pawn(W, 0)
            check('spawned gameplay pawn differs from original presentation', P != DEMO)
            GAME_CAMERA_SETTINGS = camera_settings(P)
            GAME_COLLISION = P.get_actor_enable_collision()
            delay = 4
        elif INDEX == 2:
            gameplay_checks('first start')
            SAFE = P.get_actor_transform()
            gi = unreal.GameplayStatics.get_game_instance(W)
            SAVES = next(s for s in unreal.ObjectIterator(unreal.InkSaveSubsystem) if s.get_outer() == gi)
            SLOT = next((i for i, info in enumerate(SAVES.get_slots()) if not info.exists), None)
            if SLOT is None:
                raise RuntimeError('Isolated profile has no empty manual save slot; no slot overwritten')
            P.set_actor_location(SAFE.translation + unreal.Vector(50000, 50000, -1000), False, False)
            P.get_movement_component().set_movement_mode(unreal.MovementMode.MOVE_FALLING)
            check('save deliberately invalid gameplay position in empty slot', SAVES.save_slot(SLOT, P))
            D.return_to_main_menu()
            delay = 4
        elif INDEX == 3:
            DEMO = D.get_presentation_pawn()
            check('return recreates presentation class only', alive(DEMO) and DEMO.get_class().get_path_name() == PRESENTATION_CLASS)
            check('menu keeps original gameplay pawn possessed', unreal.GameplayStatics.get_player_pawn(W, 0) == P)
            check('menu views presentation pawn', PC.get_view_target() == DEMO)
            check('recreated presentation camera framing retained', camera_settings(DEMO) == DEMO_CAMERA)
            snapshot('return from void')
            check('restart from void succeeds', D.start_game())
            delay = 4
        elif INDEX == 4:
            gameplay_checks('restart from void')
            check('restart returned to gameplay safe point', (P.get_actor_location() - SAFE.translation).length() < 200)
            check('load invalid save succeeds with safe fallback', SAVES.load_slot(SLOT, P))
            delay = 3
        elif INDEX == 5:
            gameplay_checks('load invalid save')
            check('load retained gameplay safe point', (P.get_actor_location() - SAFE.translation).length() < 200)
            D.return_to_main_menu()
            delay = 3
        elif INDEX == 6:
            DEMO = D.get_presentation_pawn()
            snapshot('normal return')
            check('normal restart succeeds', D.start_game())
            delay = 3
        elif INDEX == 7:
            gameplay_checks('normal restart')
            check('normal return preserves valid position', (P.get_actor_location() - SAFE.translation).length() < 200)
            REPORT['finished'] = True
            finish()
            return
        INDEX += 1
        NEXT = time.monotonic() + delay
    except Exception as error:
        REPORT['error'] = repr(error)
        finish()


HANDLE = unreal.register_slate_post_tick_callback(tick)
