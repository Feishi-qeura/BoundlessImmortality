"""Check final evidence freshness, actual rendered dimensions and ZIP identity."""
from pathlib import Path
import hashlib
import json
from PIL import Image

ROOT = Path(__file__).resolve().parents[4]
OUT = ROOT / 'Saved/Codex/PortraitUI20261023'

def read(name):
    return json.loads((OUT / name).read_text('utf-8-sig'))

checks = []
def check(name, passed):
    checks.append({'name': name, 'passed': bool(passed)})

for folder, profile, size in [
    ('PackagePortrait', 'PortraitUI20261023FinalP5', (540, 960)),
    ('PackageLandscape', 'PortraitUI20261023FinalL5', (1280, 720)),
]:
    report = read(folder + '/verification.json')
    check(folder + ' fresh final profile', report['profile'] == profile)
    check(folder + ' completed runtime', report['finished'] and report['passed'] and report['game_exit_code'] == 0 and report['authority_stopped'])
    check(folder + ' all runtime assertions', all(c['passed'] for c in report['checks']))
    check(folder + ' isolated extracted ZIP', Path(report['package']).resolve() == (ROOT / 'Saved/Packages/PortraitUI20261023-Extracted/Windows').resolve())
    for name in ['02-hud', 'signin-month-tab', 'signin-verse-tab', 'activities-timed-tab', 'activities-launch-entry', 'hud-announcement-fixture']:
        with Image.open(OUT / folder / (name + '.png')) as im:
            check(folder + '/' + name + ' actual pixel dimensions', im.size == size)

native = read('NativeTests/index.json')
check('native zero failures', native['failed'] == 0)
check('announcement geometry regression', any(t['testDisplayName'] == 'AnnouncementClearance' and t['state'] == 'Success' for t in native['tests']))
check('destroyed Niagara component regression', any(t['testDisplayName'] == 'DestroyedComponentLifecycle' and t['state'] == 'Success' for t in native['tests']))
check('source and backup preservation', read('preservation.json')['passed'])
check('all original UI assets retained', read('asset-final-audit.json')['passed'])
delivery = read('distribution.json')
h = hashlib.sha256()
with Path(delivery['zip']).open('rb') as stream:
    for chunk in iter(lambda: stream.read(1024 * 1024), b''):
        h.update(chunk)
check('delivered ZIP identity unchanged after runtime QA', delivery['passed'] and h.hexdigest() == delivery['sha256'])
result = {'passed': all(c['passed'] for c in checks), 'checks': checks, 'zip': delivery['zip'], 'sha256': delivery['sha256']}
(OUT / 'final-verification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), 'utf-8')
print(json.dumps({'passed': result['passed'], 'checks': len(checks)}))
if not result['passed']:
    raise SystemExit(1)
