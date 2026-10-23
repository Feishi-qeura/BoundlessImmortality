"""Compare populated tier content to the opaque paper band measured from original PNG alpha."""
from pathlib import Path
import json
import unreal

SOURCE = Path(unreal.Paths.project_dir()) / 'Tools/InkFrontend/verify_leaderboard_spacing_pie.py'
exec(compile(SOURCE.read_text(encoding='utf-8'), str(SOURCE), 'exec'), globals())
OUT = ROOT / 'Saved/Codex/LeaderboardContent20261022' / ('Before' if '-rankbefore' in unreal.SystemLibrary.get_command_line().lower() else 'After')
OUT.mkdir(parents=True, exist_ok=True)
BANDS = {(b['Theme'], b['Rank']): b for b in json.loads((OUT.parent / 'paper-bands.json').read_text(encoding='utf-8-sig'))}
R.update(boundary='Windows rendered PIE, original atlas alpha bands, transient populated local/global fixtures; no mobile/device validation')
STEPS.clear()


def scroll_to_tier_four(compact=False):
    prefix = 'RankHUD' if compact else 'RankAnchor'
    for scroll in unreal.ObjectIterator(unreal.ScrollBox):
        if scroll.get_path_name().startswith(F.get_path_name() + '.') and scroll.get_name() == prefix + 'Scroll':
            scroll.set_scroll_offset(176 if compact else 344 if LAYOUT == 'portrait' else 432)


def alignment_capture(tag, theme, compact=False):
    L.move_frontend_pointer_outside(F)
    s = json.loads(L.describe_widget(F))
    (OUT / (tag + '.json')).write_text(json.dumps(s, ensure_ascii=False, indent=2), encoding='utf-8')
    path = OUT / (tag + '.png')
    check(tag + ' screenshot', L.capture_frontend_viewport(str(path)))
    R['captures'].append(str(path))
    prefix = 'RankHUD' if compact else 'RankAnchor'
    items = {i['name']: i for group in ('images', 'texts') for i in s[group]}
    check(tag + ' populated category', any(theme in (i.get('material_parent', '') + i.get('resource', '')) for i in s['images'] if i['name'].startswith(prefix + 'RowArt_')))
    for rank in range(4, 11):
        art = unreal.find_object(None, items[prefix + 'RowArt_' + str(rank)]['object'])
        pos, size = art.slot.get_position(), art.slot.get_size()
        band = BANDS[(theme, rank)]
        top = pos.y + size.y * band['FractionTop'] + (1 if compact else 2)
        bottom = pos.y + size.y * band['FractionBottom'] - (1 if compact else 2)
        top_actual, bottom_actual = [], []
        for part in ('Number', 'Name', 'Metric', 'AvatarPlaceholder', 'Avatar', 'TitleEffect', 'RealmEffect'):
            name = prefix + part + '_' + str(rank)
            if part.endswith('Effect'):
                obj = next(w for w in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget) if w.get_path_name().startswith(F.get_path_name() + '.') and w.get_name() == name)
            else:
                obj = unreal.find_object(None, items[name]['object'])
            cp, cs = obj.slot.get_position(), obj.slot.get_size()
            check(tag + ' paper contains ' + name, cp.y >= top - .06 and cp.y + cs.y <= bottom + .06,
                  dict(top=cp.y, bottom=cp.y + cs.y, safe_top=top, safe_bottom=bottom))
            top_actual.append(cp.y)
            bottom_actual.append(cp.y + cs.y)
            if part in ('Name', 'Metric'):
                check(tag + ' readable font ' + name, obj.get_editor_property('font').size >= (8 if compact else 16), obj.get_editor_property('font').size)
        check(tag + ' centered content ' + str(rank), abs((min(top_actual) + max(bottom_actual)) / 2 - (top + bottom) / 2) <= .1)
        row = art.get_parent().get_parent()
        check(tag + ' original row height ' + str(rank), abs(row.get_editor_property('height_override') - (48 if compact else 96 if LAYOUT == 'portrait' else 116)) < .01)


def resize(width, height, layout):
    global LAYOUT
    LAYOUT = layout
    check(layout + ' resize', L.set_pie_viewport_size(width, height))


LAYOUT = 'portrait'
step('populated fixture', lambda: fixture(False))
step('portrait exact viewport', lambda: resize(720, 1280, 'portrait'), 2)
for layout, width, height in [('portrait', 720, 1280), ('landscape', 1280, 720)]:
    step(layout + ' resize', lambda w=width, h=height, t=layout: resize(w, h, t), 2)
    for scope in ('Local', 'Global'):
        for theme, action in [('Damage', 'RankDamage'), ('Sword', 'RankAnchor'), ('Speed', 'RankCultivation')]:
            tag = layout + '-' + scope.lower() + '-' + theme.lower()
            step(tag + ' select', lambda a=action, sc=scope: (select_full(a), F.trigger_action('Rank' + sc)))
            step(tag + ' scroll', scroll_to_tier_four)
            step(tag + ' pixels and paper containment', lambda t=tag, th=theme: alignment_capture(t, th))
    step(layout + ' HUD', lambda: F.show_page('HUD'))
    step(layout + ' compact open', lambda: F.trigger_action('HUDRankView'), 2)
    for theme, action in [('Speed', 'HUDRankSpeed'), ('Damage', 'HUDRankDamage')]:
        tag = 'compact-' + layout + '-' + theme.lower()
        step(tag + ' select', lambda a=action: F.trigger_action(a), 2)
        step(tag + ' scroll', lambda: scroll_to_tier_four(True))
        step(tag + ' pixels and paper containment', lambda t=tag, th=theme: alignment_capture(t, th, True))
step('full final row', lambda: (F.show_page('Leaderboard'), scroll_end('RankAnchor')), 2)
step('full final row reachable', lambda: capture_end('full-final-rows', 'RankAnchor'))
