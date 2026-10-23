"""BuildCookRun plus a portable local authority. No deployment or platform publishing."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
RULES = ROOT / 'docs/design/2026-10-21-wuling-resource-v1.4'


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def copy_python(source, destination):
    """Redistribute CPython and stdlib; never copy user site packages or credentials."""
    if not (source / 'python312.dll').is_file():
        raise ValueError('PYTHON_312_RUNTIME_REQUIRED')
    destination.mkdir(parents=True, exist_ok=True)
    for name in ('python.exe', 'pythonw.exe', 'python3.dll', 'python312.dll',
                 'vcruntime140.dll', 'vcruntime140_1.dll', 'LICENSE.txt'):
        shutil.copy2(source / name, destination / name)
    dlls = destination / 'DLLs'
    dlls.mkdir(exist_ok=True)
    for file in sorted((source / 'DLLs').iterdir()):
        if file.suffix in ('.pyd', '.dll'):
            shutil.copy2(file, dlls / file.name)
    excluded = {'site-packages', '__pycache__', 'test', 'tests', 'idlelib', 'tkinter', 'turtledemo', 'ensurepip', 'venv'}
    # Keep filesystem-relative stdlib resources available (including the runtime's
    # SSL trust-policy PEM); __file__ inside a ZIP is not an openable directory.
    for file in sorted((source / 'Lib').rglob('*')):
        relative = file.relative_to(source / 'Lib')
        if not file.is_file() or excluded.intersection(relative.parts) or file.suffix not in ('.py', '.pem', '.txt', '.json', '.cfg'):
            continue
        target = destination / 'Lib' / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(file, target)
    (destination / 'python312._pth').write_text('Lib\nDLLs\n.\n', encoding='utf-8')


def assemble(stage, python):
    from Tools.CultivationService.rules import RuleBundle
    rules = RuleBundle.load(RULES)
    # Check UAT's archive before writing any companion files, so a failed build is recoverable.
    game = stage / 'BoundlessImmortality/Binaries/Win64/BoundlessImmortality.exe'
    if not game.is_file():
        raise ValueError('DEVELOPMENT_GAME_EXECUTABLE_MISSING')
    runtime = stage / 'Runtime'
    if (stage / 'release-manifest.json').exists():
        raise ValueError('PACKAGE_ALREADY_ASSEMBLED')
    copy_python(python, runtime)
    service = stage / 'LocalService/Tools/CultivationService'
    service.mkdir(parents=True, exist_ok=True)
    for file in sorted((ROOT / 'Tools/CultivationService').iterdir()):
        if file.suffix in ('.py', '.json', '.sql'):
            shutil.copy2(file, service / file.name)
    (stage / 'LocalService/Tools/__init__.py').write_text('', encoding='utf-8')
    shutil.copy2(Path(__file__).with_name('launcher.py'), stage / 'LocalService/launcher.py')
    shutil.copytree(RULES / 'tables', stage / 'Rules/tables', dirs_exist_ok=True)
    # Cooked data is in paks; no local save or QA fixture is staged by this operation.
    (stage / 'Start-Wuling.cmd').write_bytes(
        b'@echo off\r\nstart "" "%~dp0Runtime\\pythonw.exe" -I -B "%~dp0LocalService\\launcher.py" %*\r\n')
    shutil.copy2(Path(__file__).with_name('PACKAGE-README.txt'), stage / '请先阅读.txt')
    # Python's license includes third party stdlib licenses. UE handles its own staged notices.
    manifest = dict(schema=1, name='BoundlessImmortality Windows local v1.4', configuration='Development',
        profile='prototype_full', rule_version=rules.version, rules_hash=rules.sha256,
        production_platform_verified=False, ui_revision='2026-10-23-portrait-ui', game_executable=game.relative_to(stage).as_posix())
    (stage / 'release-manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
    files = {f.relative_to(stage).as_posix(): digest(f) for f in sorted(stage.rglob('*')) if f.is_file() and f != stage / 'SHA256.json'}
    (stage / 'SHA256.json').write_text(json.dumps(files, ensure_ascii=False, indent=2), encoding='utf-8')
    return manifest


def verify(stage):
    hashes = json.loads((stage / 'SHA256.json').read_text(encoding='utf-8'))
    bad = []
    for relative, expected in hashes.items():
        path = (stage / relative).resolve()
        if not path.is_relative_to(stage.resolve()) or not path.is_file() or digest(path) != expected:
            bad.append(relative)
    if bad:
        raise ValueError('PACKAGE_INTEGRITY_FAILED: ' + ', '.join(bad[:10]))
    return len(hashes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path)
    parser.add_argument('--python', type=Path, default=Path(sys.executable).parent)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--assemble-only', action='store_true', help='Use a completed Development UAT archive at output/Windows.')
    parser.add_argument('--iterate', action='store_true', help='Incremental Cook after an earlier complete Cook; still builds and archives current source.')
    parser.add_argument('--verify-only', action='store_true', help='Verify the assembled package at output/Windows.')
    args = parser.parse_args()
    output = args.output.resolve()
    stage = output / 'Windows'
    if args.verify_only:
        print(json.dumps(dict(verified_files=verify(stage))))
        return
    if not args.assemble_only:
        if output.exists() and any(output.iterdir()):
            parser.error('output must be a new or empty directory')
        if not args.engine:
            parser.error('--engine is required for BuildCookRun')
        if sys.platform == 'win32':
            # Native builds must not race Live Coding or alter an open editor's assets.
            check = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq UnrealEditor.exe', '/FO', 'CSV', '/NH'],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
            if 'UnrealEditor.exe' in check.stdout:
                parser.error('close Unreal Editor before building')
        output.mkdir(parents=True, exist_ok=True)
        uat = args.engine / 'Engine/Build/BatchFiles/RunUAT.bat'
        command = [str(uat), 'BuildCookRun', '-project=' + str(ROOT / 'BoundlessImmortality.uproject'),
            '-noP4', '-platform=Win64', '-clientconfig=Development', '-build', '-cook',
            '-map=/Game/Isometric_World/Sky_Temple/Maps/Level_Sky_Temple_Demo_01',
            '-stage', '-pak', '-archive', '-archivedirectory=' + str(stage), '-package', '-prereqs', '-nodebuginfo',
            '-unattended', '-utf8output', '-NoXGE', '-NoUBA', '-nocompileeditor',
            '-ubtargs=-MaxParallelActions=2 -NoUBA']
        if args.iterate:
            command.append('-iterativecooking')
        with (output / 'BuildCookRun.log').open('wb') as log:
            result = subprocess.run(command, cwd=uat.parent, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError(f'BUILDCookRun_FAILED_{result.returncode}: {output / "BuildCookRun.log"}')
    print(json.dumps(assemble(stage, args.python.resolve()), ensure_ascii=False))
    print(json.dumps(dict(verified_files=verify(stage))))


if __name__ == '__main__':
    # -m from source works; direct execution remains usable without PYTHONPATH.
    if str(ROOT) not in sys.path:
        sys.path.insert(0, str(ROOT))
    main()
