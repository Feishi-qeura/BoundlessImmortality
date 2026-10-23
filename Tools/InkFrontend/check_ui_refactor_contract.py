"""Contract checks on actual PIE geometry reports, never on fabricated UI state."""
import argparse
import json
from pathlib import Path

def check(folder):
    checks = []
    def record(name, passed, actual=None):
        checks.append(dict(name=name, passed=bool(passed), actual=actual))
    for tag in ['02-hud-720x1280', '04-hud-1280x720']:
        data = json.loads((folder / (tag + '.json')).read_text(encoding='utf-8-sig'))
        names = ['OpenCharacter', 'OpenInventory', 'ToggleCultivation', 'OpenLeaderboard', 'ToggleHUDMenu']
        actions = [b for b in data['buttons'] if b['visible'] and b['action'] in names]
        record(tag + ': exactly five navigation actions', len(actions) == 5, len(actions))
        ordered = sorted(actions, key=lambda b: b['x'])
        record(tag + ': cultivate at center of five', [b['action'] for b in ordered] == names, [b['action'] for b in ordered])
        record(tag + ': navigation aligned to bottom band', all(b['y'] >= data['height'] - 180 for b in actions), [(b['action'], b['y']) for b in actions])
        record(tag + ': all HUD targets at least 88', all(b['width'] >= 87.9 and b['height'] >= 87.9 for b in data['buttons'] if b['visible'] and b['name'].startswith('HUD_')), [(b['action'], b['width'], b['height']) for b in data['buttons'] if b['visible'] and b['name'].startswith('HUD_')])
        record(tag + ': new body font', 'F_InkBody' in data.get('font_property', ''), data.get('font_property'))
        identity = [b for b in data['buttons'] if b['visible'] and b['action'] == 'ShowAnchorIdentity']
        record(tag + ': identity can open full name on touch', len(identity) == 1 and identity[0]['width'] >= 88 and identity[0]['height'] >= 88)
    daily = json.loads((folder / '03-signin-720x1280.json').read_text(encoding='utf-8-sig'))
    positions = {t['name']: t.get('y', 0) for t in daily['texts']}
    record('daily details appear before cumulative milestones', positions['SignInTodayDetails'] < positions['SignInMilestones'])
    inventory = json.loads((folder / 'regression-inventory.json').read_text(encoding='utf-8-sig'))
    record('inventory landscape uses eight columns', len({round(c['x'], 1) for c in inventory['inventory_cells']}) == 8)
    result = {'passed': all(c['passed'] for c in checks), 'checks': checks, 'boundary': 'Actual PIE geometry contract; not service/device acceptance'}
    (folder / 'hud-contract.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('folder', type=Path)
    args = parser.parse_args()
    result = check(args.folder)
    print(json.dumps({'passed': result['passed'], 'failed': [x['name'] for x in result['checks'] if not x['passed']]}, ensure_ascii=False, indent=2))
    raise SystemExit(0 if result['passed'] else 1)
