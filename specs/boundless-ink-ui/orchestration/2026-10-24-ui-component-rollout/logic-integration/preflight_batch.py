"""Read-only preparation inventory. Does not launch UE/UBT or import any asset."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
import sys
from zoneinfo import ZoneInfo

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
ROLLOUT = HERE.parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    teams = {}
    for team in ('panel-player', 'panel-rank', 'panel-journey', 'panel-activities', 'logic-core'):
        folder = ROLLOUT / team
        status_file = folder / 'status.json'
        status = json.loads(status_file.read_text(encoding='utf-8-sig')) if status_file.exists() else {}
        handoff = folder / 'handoff.md'
        teams[team] = dict(phase=status.get('phase'), handoff_ready=status.get('handoff_ready', False),
                           source_ready=status.get('source_ready', False), assets_ready=status.get('assets_ready', False),
                           handoff_exists=handoff.exists(),
                           handoff_sha256=sha(handoff) if handoff.exists() else None)
    baseline = json.loads((HERE / 'public-baseline-sha256.json').read_text(encoding='utf-8-sig'))
    changed = [v['path'] for v in baseline if sha(ROOT / v['path']).lower() != v['sha256'].lower()]
    source = ROOT / 'Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicJourney'
    inventory = [dict(path=str(p.relative_to(ROOT)).replace('\\','/'), sha256=sha(p), bytes=p.stat().st_size)
                 for p in sorted(source.glob('*')) if p.is_file()]
    lease = json.loads((ROLLOUT / 'engine-lease.json').read_text(encoding='utf-8-sig'))
    report = dict(updated_at=datetime.now(ZoneInfo('Asia/Hong_Kong')).isoformat(),
                  scope='disk-only inventory; not compile, import, route or runtime verification',
                  lease=lease, dependencies=teams, source_inventory=inventory,
                  public_files_changed_since_baseline=changed,
                  any_panel_batch_ready=any(v['handoff_ready'] and v['handoff_exists'] and v['source_ready'] and v['assets_ready']
                                            for k,v in teams.items() if k.startswith('panel-')),
                  engine_started=False)
    (HERE / 'preflight-result.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
