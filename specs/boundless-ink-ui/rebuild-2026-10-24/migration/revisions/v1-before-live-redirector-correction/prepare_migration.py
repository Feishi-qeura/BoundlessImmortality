"""Capture an exact UI -> newUI map and hash-verified rollback snapshot. No UE mutation."""
import argparse
import hashlib
import json
import re
import shutil
from collections import Counter
from datetime import datetime, timezone, timedelta
from pathlib import Path

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[3]
OLD = ROOT / 'Content/UI'
NEW = ROOT / 'Content/newUI'
BACKUPS = ROOT / 'Saved/Codex/UIRebuild20261024/Backups'
PATH_RE = re.compile(r'/Game/UI(?:/[A-Za-z0-9_/]+)?')
TEXT_EXT = {'.cpp', '.h', '.ini', '.json', '.py', '.ps1', '.cs', '.csv', '.yaml', '.yml', '.txt', '.uproject', '.uplugin'}
TYPES = dict(T='Texture2D', M='Material', MI='MaterialInstanceConstant', NS='NiagaraSystem',
             WBP='WidgetBlueprint', F='Font', FF='FontFace', DA='InkUiThemeData')


def now():
    return datetime.now(timezone(timedelta(hours=8))).isoformat()


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(b)
    return h.hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def package(path, mount, content):
    if path.suffix.lower() in {'.uasset', '.umap', '.uexp', '.ubulk', '.uptnl'}:
        return mount + '/' + path.relative_to(content).with_suffix('').as_posix()
    return None


def prepare():
    if (OUT / 'migration-map.json').exists():
        raise RuntimeError('Mapping already exists. Preserve it; use a new reviewed revision to refresh.')
    auth = json.loads((OUT.parent / 'migration-authorization.json').read_text(encoding='utf-8-sig'))
    assert auth['migration_and_source_directory_removal_authorized']
    assert Path(auth['source_directory']).resolve() == OLD.resolve()
    assert Path(auth['destination_directory']).resolve() == NEW.resolve()
    source_files = sorted(p for p in OLD.rglob('*') if p.is_file())
    rows = []
    seen = set()
    collisions = []
    for p in source_files:
        if p.suffix != '.uasset':
            if p.suffix not in {'.uexp', '.ubulk', '.uptnl'}:
                raise RuntimeError('Unexpected old-root non-package file: ' + str(p))
            continue
        old = package(p, '/Game', ROOT / 'Content')
        new = '/Game/newUI' + old[len('/Game/UI'):]
        dest = NEW / p.relative_to(OLD)
        key = str(dest).casefold()
        if key in seen or dest.exists():
            collisions.append(dict(source=old, destination=new, existing=str(dest)))
        seen.add(key)
        rows.append(dict(old_package=old, new_package=new, asset_name=p.stem,
            old_object=old + '.' + p.stem, new_object=new + '.' + p.stem,
            source_relative=p.relative_to(ROOT).as_posix(), destination_relative=dest.relative_to(ROOT).as_posix(),
            bytes=p.stat().st_size, sha256=sha(p),
            class_hint=TYPES.get(p.stem.split('_')[0], 'UNKNOWN'),
            class_evidence='Filename convention only; fresh live AssetRegistry required before execution',
            live_class=None, referencers_live='PENDING_ENGINE_PREFLIGHT'))
    if not rows or collisions:
        save(OUT / 'collision-rejection.json', dict(collisions=collisions, count=len(rows)))
        raise RuntimeError('Empty mapping or destination collision. Never overwrite or merge automatically.')
    # Full project content archive includes every possible in-project binary referencer.
    snapshot = BACKUPS / datetime.now().strftime('%Y%m%dT%H%M%S%f')
    snapshot.mkdir(parents=True, exist_ok=False)
    sources = {}
    for base in ('Content', 'Source', 'Config'):
        for p in (ROOT / base).rglob('*'):
            if p.is_file():
                sources[p] = package(p, '/Game', ROOT / 'Content') if base == 'Content' else None
    for p in ROOT.glob('*.uproject'):
        sources[p] = None
    for plugin in (ROOT / 'Plugins').rglob('*.uplugin'):
        sources[plugin] = None
        content = plugin.parent / 'Content'
        for p in content.rglob('*'):
            if p.is_file():
                sources[p] = package(p, '/' + plugin.stem, content)
    copied = []
    for count, (p, pkg) in enumerate(sorted(sources.items()), 1):
        rel = p.relative_to(ROOT)
        target = snapshot / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        before = sha(p)
        shutil.copy2(p, target)
        after = sha(target)
        if before != after or sha(p) != before:
            raise RuntimeError('Concurrent write or backup mismatch: ' + str(rel))
        copied.append(dict(relative_path=rel.as_posix(), package=pkg, backup=str(target),
                           sha256=after, bytes=target.stat().st_size))
        if count % 300 == 0:
            print('BACKUP_VERIFIED', count, flush=True)
    old_paths = {r['source_relative'] for r in rows}
    backup_index = {r['relative_path']: r for r in copied}
    for r in rows:
        if backup_index[r['source_relative']]['sha256'] != r['sha256']:
            raise RuntimeError('Old UI changed between mapping and backup: ' + r['old_package'])
    current_old = {p.relative_to(ROOT).as_posix() for p in OLD.rglob('*.uasset')}
    if current_old != old_paths:
        raise RuntimeError('Old UI package membership changed during snapshot.')
    manifest = dict(captured_at=now(), root=str(ROOT), snapshot=str(snapshot), files=copied,
                    all_old_ui_backed_up=True, old_ui_count=len(rows),
                    all_project_content_and_project_plugin_content_backed_up=True,
                    source_config_on_disk_snapshot=True, unsaved_editor_memory_backed_up=False,
                    total_bytes=sum(r['bytes'] for r in copied), verified=True)
    save(snapshot / 'backup-manifest.json', manifest)
    mapping = dict(schema_version=1, prepared_at=now(), old_root='/Game/UI', new_root='/Game/newUI',
        collision_policy='Abort on any exact or Windows case-fold destination collision; no overwrite, merge or dedup',
        count=len(rows), old_ui_bytes=sum(r['bytes'] for r in rows),
        backup_manifest=str(snapshot / 'backup-manifest.json'),
        backup_manifest_sha256=sha(snapshot / 'backup-manifest.json'),
        source_config_owner='logic-integration', source_asset_owner='cleanup',
        migration_executed=False, assets=rows)
    save(OUT / 'migration-map.json', mapping)
    save(OUT / 'backup-result.json', dict(checked_at=now(), backup_manifest=mapping['backup_manifest'],
        backup_manifest_sha256=mapping['backup_manifest_sha256'], files=len(copied),
        total_bytes=manifest['total_bytes'], old_ui_count=len(rows), old_ui_bytes=mapping['old_ui_bytes'],
        hash_verified=True, collision_count=0, class_hints=dict(Counter(r['class_hint'] for r in rows)),
        live_registry_verified=False, migration_executed=False))
    print(json.dumps(json.loads((OUT / 'backup-result.json').read_text()), indent=2), flush=True)


def static_plan():
    mapping = json.loads((OUT / 'migration-map.json').read_text(encoding='utf-8-sig'))
    aliases = []
    canonical = {r['old_package']: r['new_package'] for r in mapping['assets']}
    ini = ROOT / 'Config/DefaultEngine.ini'
    for line, text in enumerate(ini.read_text(encoding='utf-8-sig').splitlines(), 1):
        match = re.search(r'OldName="([^"]+)",NewName="([^"]+)"', text)
        if match and any(x.startswith('/Game/UI/') for x in match.groups()):
            old, new = match.groups()
            aliases.append(dict(file='Config/DefaultEngine.ini', line=line, old_name=old,
                old_destination=new, new_destination='/Game/newUI' + new[len('/Game/UI'):] if new.startswith('/Game/UI/') else new,
                preserve_historical_old_name=True))
    refs = []
    for base in ('Source', 'Config', 'Tools', 'Scripts', 'Content'):
        for p in (ROOT / base).rglob('*'):
            if not p.is_file() or p.suffix.lower() not in TEXT_EXT:
                continue
            try:
                text = p.read_text(encoding='utf-8-sig')
            except UnicodeError:
                continue
            rel = p.relative_to(ROOT).as_posix()
            for line_no, line in enumerate(text.splitlines(), 1):
                matches = list(PATH_RE.finditer(line))
                if not matches:
                    continue
                alias_table = p.name == 'InkUiAssetPaths.cpp'
                ini_alias = 'PackageRedirects=' in line
                is_runtime = rel.startswith('Source/BoundlessImmortality/') and not alias_table
                refs.append(dict(file=rel, line=line_no, sha256=sha(p), original=line.strip(),
                    category='cpp_compatibility_resolver' if alias_table else 'config_compatibility_alias' if ini_alias
                    else 'runtime_loading_or_literal' if is_runtime else 'editor_tool_config_or_data',
                    paths=[m.group() for m in matches],
                    recommendation='Preserve old-name keys; update only destination values' if alias_table or ini_alias
                    else 'Use newUI canonical literal or format prefix; preserve exact business suffix and object name'))
    new_compat = [dict(old_name=k, new_name=v, reason='canonical package relocated') for k, v in canonical.items()]
    save(OUT / 'reference-replacement-plan.json', dict(checked_at=now(), writer='logic-integration',
        canonical_package_count=len(canonical), current_config_aliases=aliases,
        additional_exact_canonical_compatibility_aliases=new_compat,
        line_evidence=refs, execution_warning='Apply runtime/config changes after migration or suspend exact process aliases during source package loads; no wildcard redirects',
        migration_map_sha256=sha(OUT / 'migration-map.json')))
    print('REFERENCE_PLAN', len(refs), 'line records;', len(aliases), 'existing ini aliases;', len(canonical), 'canonical aliases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['snapshot', 'static-plan'])
    args = parser.parse_args()
    prepare() if args.mode == 'snapshot' else static_plan()
