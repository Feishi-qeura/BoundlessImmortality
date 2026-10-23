"""Run the bundled runtime and rendered game; copy only explicit QA evidence back to the workspace."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--profile', required=True)
    parser.add_argument('--width', type=int, default=720)
    parser.add_argument('--height', type=int, default=1280)
    parser.add_argument('--default-window', action='store_true', help='Exercise the launcher portrait default without resolution overrides.')
    args = parser.parse_args()
    package = args.package.resolve()
    output = args.evidence.resolve()
    output.mkdir(parents=True, exist_ok=True)
    runtime = package / 'Runtime/python.exe'
    launcher = package / 'LocalService/launcher.py'
    base = [str(runtime), '-I', '-B', str(launcher), '--profile', args.profile]
    with (output / 'smoke-console.log').open('wb') as stream:
        subprocess.run(base + ['--smoke-check'], cwd=output, stdout=stream, stderr=subprocess.STDOUT, timeout=40, check=True)
    display = () if args.default_window else ('-windowed', '-ForceRes', '-ResX=' + str(args.width), '-ResY=' + str(args.height))
    game_command = base + ['--game-arg=' + arg for arg in ('-WulingPackageAcceptance', *display, '-nosound', '-unattended', '-NoSplash')]
    started = time.time()
    with (output / 'game-launcher-console.log').open('wb') as stream:
        result = subprocess.run(game_command, cwd=output, stdout=stream, stderr=subprocess.STDOUT, timeout=390)
    data = Path(os.environ['LOCALAPPDATA']) / 'BoundlessImmortality/LocalV14' / args.profile
    # Preserve actionable evidence even when the game crashes before Finish writes its report.
    game_saved = data / 'Game/Saved'
    evidence_dir = game_saved / 'PackageAcceptance'
    if evidence_dir.is_dir():
        for path in sorted(evidence_dir.iterdir()):
            if path.suffix in ('.json', '.png'):
                shutil.copy2(path, output / path.name)
    for name in ('launcher.log', 'authority.log'):
        if (data / name).is_file():
            shutil.copy2(data / name, output / name)
    game_log = game_saved / 'Logs/BoundlessImmortality.log'
    if game_log.is_file():
        shutil.copy2(game_log, output / 'game.log')
    reports = list(data.rglob('PackageAcceptance/report.json'))
    if len(reports) != 1 or reports[0].stat().st_mtime < started:
        (output / 'verification.json').write_text(json.dumps(dict(passed=False, finished=False,
            profile=args.profile, game_exit_code=result.returncode, reason='FRESH_GAME_ACCEPTANCE_REPORT_MISSING'), indent=2), encoding='utf-8')
        raise RuntimeError('FRESH_GAME_ACCEPTANCE_REPORT_MISSING: ' + str(data))
    report_path = reports[0]
    report = json.loads(report_path.read_text(encoding='utf-8-sig'))
    report['game_exit_code'] = result.returncode
    report['resolution'] = [540, 960] if args.default_window else [args.width, args.height]
    report['used_launcher_portrait_default'] = args.default_window
    report['package'] = str(package)
    report['profile'] = args.profile
    report['authority_stopped'] = not list(data.glob('ready-*.json'))
    report['passed'] = bool(report.get('passed') and report.get('finished') and result.returncode == 0 and report['authority_stopped'])
    (output / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'], checks=len(report['checks']), screenshots=report['screenshot_count'], exit=result.returncode)))
    if not report['passed']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
