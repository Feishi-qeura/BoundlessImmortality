"""Compare stage-one files without restoring or attributing concurrent gameplay work to UI."""
import difflib
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Saved/Codex/UIRefactor20261021/Resume20261021'
UI_EDITS = {
    'Source/BoundlessImmortality/Frontend/InkAnchorStatusWidget.cpp',
    'docs/superpowers/plans/2026-10-21-ui-refactor-stage1.md',
    'Source/BoundlessImmortality/Frontend/InkFrontendWidget.h',
    'Source/BoundlessImmortality/Frontend/InkHudLayout.cpp',
    'Source/BoundlessImmortality/Frontend/InkFrontendWidget.cpp',
    'Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp',
    'Source/BoundlessImmortality/Frontend/InkFeatureGroupPanel.cpp',
    'Source/BoundlessImmortality/Frontend/InkSignInSample.cpp',
    'Source/BoundlessImmortality/Frontend/InkInventoryWidget.cpp',
    'Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',
    'Tools/InkFrontend/check_ui_refactor_contract.py',
    'Source/BoundlessImmortality/Frontend/UI/InkUiViewModels.cpp',
    'Source/BoundlessImmortality/Frontend/UI/InkUiResponsiveLayout.cpp',
    'Source/BoundlessImmortality/Frontend/UI/InkUiTheme.h',
    'Source/BoundlessImmortality/Frontend/UI/InkUiTheme.cpp',
    'Source/BoundlessImmortality/Frontend/Tests/InkUiLayoutTests.cpp',
    'Tools/InkFrontend/verify_ui_refactor_pie.py',
}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None

def run():
    baseline = json.loads((OUT / 'resume-baseline.json').read_text(encoding='utf-8'))
    report = {'boundary': 'This UI resume only; concurrent gameplay changes retained without rollback', 'checked': len(baseline), 'ui_changes': [], 'concurrent_changes': [], 'protected_changes': [], 'new_ui_files': []}
    report['captured_utc']=datetime.now(timezone.utc).isoformat()
    binary=ROOT/'Binaries/Win64/UnrealEditor-BoundlessImmortality.dll'
    report['native_binary']={'path':str(binary.relative_to(ROOT)),'sha256':digest(binary)}
    diff_dir = OUT / 'Diff'
    diff_dir.mkdir(exist_ok=True)
    for old in baseline:
        relative = old['path'].replace('\\', '/')
        path = ROOT / relative
        now = digest(path)
        if now == old['sha256'].lower():
            continue
        row = {'path': relative, 'before': old['sha256'], 'after': now}
        own = relative in UI_EDITS
        report['ui_changes' if own else 'concurrent_changes'].append(row)
        if own and path.suffix in ('.cpp', '.h', '.py', '.md') and now:
            before = (OUT / 'Backup' / relative).read_text(encoding='utf-8-sig').splitlines(True)
            after = path.read_text(encoding='utf-8-sig').splitlines(True)
            diff = ''.join(difflib.unified_diff(before, after, fromfile='resume/' + relative, tofile='current/' + relative))
            (diff_dir / (path.name + '.diff')).write_text(diff, encoding='utf-8')
    original = json.loads((OUT.parent / 'preservation-baseline.json').read_text(encoding='utf-8'))
    for old in original['protected']:
        if digest(ROOT / old['path']) != old['sha256'].lower():
            report['protected_changes'].append(old['path'])
    known = {x['path'].replace('\\', '/') for x in baseline}
    for path in (ROOT / 'Tools/InkFrontend').glob('*.py'):
        relative = path.relative_to(ROOT).as_posix()
        if relative not in known:
            report['new_ui_files'].append({'path': relative, 'sha256': digest(path)})
    report['protected_preserved'] = not report['protected_changes']
    report['ui_asset_changes'] = [x for x in report['ui_changes'] + report['concurrent_changes'] if x['path'].startswith('Content/UI/')]
    report['manual_ui_assets_unchanged_in_resume'] = not report['ui_asset_changes']
    (OUT / 'preservation-final.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({k: len(v) if isinstance(v, list) else v for k, v in report.items()}, ensure_ascii=False))
    return report

if __name__ == '__main__':
    run()
