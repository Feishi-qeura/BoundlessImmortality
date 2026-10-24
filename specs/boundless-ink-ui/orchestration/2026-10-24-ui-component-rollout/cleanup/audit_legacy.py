"""Offline evidence only. Never imports Unreal, launches processes, or changes assets.

Run baseline once, then audit. Backup only copies baseline packages into this team's
directory and verifies hashes. Missing references never imply an asset is unused.
"""
import argparse
import csv
import hashlib
import json
import re
import shutil
import subprocess
from collections import Counter, defaultdict
from datetime import datetime, timezone, timedelta
from pathlib import Path

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[4]
INK = ROOT / 'Content/UI/Ink'
NEW = 'ComponentRollout20261024'
PATH_RE = re.compile(r'/Game/[A-Za-z0-9_/]+')
BYTE_RE = re.compile(rb'/Game/[A-Za-z0-9_/]+')
ALIAS_RE = re.compile(r'OldName="([^"]+)",NewName="([^"]+)"')
CPP_ALIAS_RE = re.compile(r'\{TEXT\("([^"]+)"\),TEXT\("([^"]+)"\)\}')
TEXT_EXT = {'.cpp', '.h', '.cs', '.ini', '.json', '.csv', '.yaml', '.yml',
            '.py', '.ps1', '.bat', '.cmd', '.uproject', '.uplugin', '.txt', '.lua', '.js'}
LOAD_RE = re.compile(r'LoadObject|LoadClass|StaticLoad|ConstructorHelpers|RequestAsyncLoad|LoadSynchronous|FSoftObjectPath|FString::Printf|InkUiAssets::Resolve')


def now():
    return datetime.now(timezone(timedelta(hours=8))).isoformat()


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def save(name, obj, exclusive=False):
    with (OUT / name).open('x' if exclusive else 'w', encoding='utf-8', newline='\n') as f:
        json.dump(obj, f, ensure_ascii=False, indent=2)
        f.write('\n')


def read(name):
    return json.loads((OUT / name).read_text(encoding='utf-8-sig'))


def package(path):
    return '/Game/' + path.relative_to(ROOT / 'Content').with_suffix('').as_posix()


def files():
    return sorted(p for p in INK.rglob('*') if p.is_file() and NEW not in p.relative_to(INK).parts)


def baseline():
    # Exclusive creation prevents a later import from silently changing the old scope.
    if (OUT / 'baseline.json').exists():
        raise RuntimeError('Baseline already exists; do not overwrite it.')
    before = now()
    rows = []
    for p in files():
        s = p.stat()
        rows.append(dict(relative_path=p.relative_to(ROOT).as_posix(),
                         package=package(p) if p.suffix == '.uasset' else None,
                         size=s.st_size, mtime_ns=s.st_mtime_ns, sha256=digest(p)))
    new_files = sorted(str(p.relative_to(ROOT)) for p in (INK / NEW).rglob('*') if p.is_file())
    result = dict(captured_at=before, finished_at=now(), project=str(ROOT),
                  scope='/Game/UI/Ink', excluded_subtree='/Game/UI/Ink/' + NEW,
                  new_component_files_present_at_capture=new_files,
                  before_new_component_assets=not new_files,
                  file_count=len(rows), asset_count=sum(r['package'] is not None for r in rows),
                  files=rows, asset_mutation=False)
    save('baseline.json', result, exclusive=True)
    print(json.dumps({k: v for k, v in result.items() if k != 'files'}))


def exact_six():
    prior_dir = ROOT / 'Saved/Codex/FullUI20261024'
    pre = json.loads((prior_dir / 'precleanup-backup.json').read_text(encoding='utf-8-sig'))
    prior = json.loads((prior_dir / 'cleanup-result.json').read_text(encoding='utf-8-sig'))
    delivery = json.loads((prior_dir / 'delivery-assets-check.json').read_text(encoding='utf-8-sig'))
    old = {r['package'] for r in prior['files'] if r['legacy_redirector']}
    if len(old) != 6:
        raise RuntimeError('Prior exact-six scope changed.')
    rows = []
    for r in pre['files']:
        p = ROOT / r['relative_path']
        b = Path(r['backup'])
        rows.append(dict(package=r['package'], legacy=r['package'] in old,
                         exists=p.exists(), sha256=digest(p) if p.exists() else None,
                         expected_sha256=r['sha256'].lower(),
                         matches_precleanup=p.exists() and digest(p) == r['sha256'].lower(),
                         backup_exists=b.exists(),
                         backup_matches=b.exists() and digest(b) == r['sha256'].lower()))
    result = dict(checked_at=now(), prior_cleanup_result=prior,
                  prior_delivery_check=delivery, current_files=rows,
                  legacy_absent=sum(r['legacy'] and not r['exists'] for r in rows),
                  canonical_matches=sum(not r['legacy'] and r['matches_precleanup'] for r in rows),
                  backup_matches=sum(r['backup_matches'] for r in rows),
                  repeat_cleanup_forbidden=True)
    save('exact-six-current-check.json', result)
    return result


def known_dynamic_paths(aliases):
    """Expand two finite source-owned arrays; do not infer arbitrary runtime strings."""
    inverse = {}
    for target, old_names in aliases.items():
        for old in old_names:
            if old in inverse and inverse[old] != target:
                raise RuntimeError('Conflicting redirect aliases: ' + old)
            inverse[old] = target
    result = defaultdict(list)
    cases = [
        ('Source/BoundlessImmortality/LiveWeapons/LiveGMControlPanel.cpp', 'Icons',
         'Source/BoundlessImmortality/LiveWeapons/LiveGMControlPanel.cpp', '/Game/UI/Ink/Gifts/T_Gift_',
         'Config PackageRedirects; Button uses LoadObject'),
        ('Source/BoundlessImmortality/Frontend/InkAttributeRating.cpp', 'Names',
         'Source/BoundlessImmortality/Frontend/InkCharacterPanels.cpp', '/Game/UI/Ink/CharacterRating20261022/T_Rating_',
         'InkUiAssets::Load -> Resolve -> LoadObject'),
    ]
    for enum_file, array, load_file, prefix, resolution in cases:
        enum_text = (ROOT / enum_file).read_text(encoding='utf-8-sig')
        load_text = (ROOT / load_file).read_text(encoding='utf-8-sig')
        if array == 'Names':
            # Other rating helpers also declare Names; scope this finite list to GradeName.
            enum_text = enum_text.split('FString UInkAttributeRatingLibrary::GradeName(', 1)[1]
        matches = re.findall(r'const TCHAR\*\s*' + array + r'\[\]\s*=\s*\{([^}]+)\}', enum_text)
        if len(matches) != 1 or prefix + '%s' not in load_text:
            raise RuntimeError('Known dynamic source shape changed: ' + enum_file)
        values = re.findall(r'TEXT\("([A-Za-z]+)"\)', matches[0])
        for value in values:
            old = prefix + value
            canonical = inverse.get(old, old)
            result[canonical].append(dict(assembled_legacy_package=old, array_value=value,
                enumeration_file=enum_file, enumeration_sha256=digest(ROOT / enum_file),
                loader_file=load_file, loader_sha256=digest(ROOT / load_file), resolution=resolution,
                evidence='Static finite path construction reviewed; runtime load not executed'))
    return result


def audit():
    base = read('baseline.json')
    indexed = {r['package']: r for r in base['files'] if r['package']}
    exact = exact_six()
    aliases = defaultdict(set)
    evidence = defaultdict(list)
    corpus = []
    errors = []
    dynamics = []
    runtime_roots = ['Source', 'Config', 'Plugins', 'Content', 'Build', 'Scripts', 'scripts', 'Tools', 'tools']
    candidates = set(ROOT.glob('*.uproject'))
    for name in runtime_roots:
        folder = ROOT / name
        if folder.exists():
            candidates.update(p for p in folder.rglob('*') if p.is_file() and p.suffix.lower() in TEXT_EXT
                              and not {'Intermediate', 'Binaries', 'node_modules', '.git'}.intersection(p.parts))
    for p in sorted(candidates):
        rel = p.relative_to(ROOT).as_posix()
        try:
            raw = p.read_bytes()
            text = raw.decode('utf-8-sig')
        except UnicodeDecodeError:
            try:
                text = raw.decode('utf-16')
            except UnicodeError as exc:
                errors.append(dict(file=rel, error=str(exc)))
                continue
        except OSError as exc:
            errors.append(dict(file=rel, error=str(exc)))
            continue
        corpus.append(dict(file=rel, sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw)))
        for old, new in ALIAS_RE.findall(text) + (CPP_ALIAS_RE.findall(text) if p.name == 'InkUiAssetPaths.cpp' else []):
            aliases[new].add(old)
        for line_no, line in enumerate(text.splitlines(), 1):
            for ref in set(PATH_RE.findall(line)):
                evidence[ref].append(dict(file=rel, line=line_no,
                    kind='alias_table' if p.name == 'InkUiAssetPaths.cpp' else 'config' if rel.startswith('Config/')
                    else 'runtime_source' if rel.startswith('Source/BoundlessImmortality/')
                    else 'runtime_content' if rel.startswith('Content/') else 'tooling_or_editor',
                    text=line.strip()[:1200]))
            if LOAD_RE.search(line):
                dynamics.append(dict(file=rel, line=line_no, text=line.strip()[:1200],
                                     ink_specific='/UI/' in line or 'Ink' in line or 'ink' in line))
    # Registry-free binary string matches are positive leads only, never hard/soft classification.
    all_aliases = set(indexed)
    for target in indexed:
        all_aliases.update(aliases[target])
    binary_refs = defaultdict(list)
    binary_files = list((ROOT / 'Content').rglob('*.uasset')) + list((ROOT / 'Content').rglob('*.umap'))
    plugin_root = ROOT / 'Plugins'
    if plugin_root.exists():
        binary_files += [p for p in plugin_root.rglob('*') if p.suffix in {'.uasset', '.umap'}]
    binary_count = 0
    binary_bytes = 0
    for p in sorted(binary_files):
        rel = p.relative_to(ROOT).as_posix()
        refs = set()
        try:
            with p.open('rb') as f:
                tail = b''
                while True:
                    chunk = f.read(8 * 1024 * 1024)
                    if not chunk:
                        break
                    binary_bytes += len(chunk)
                    scan = tail + chunk
                    refs.update(x.decode('ascii') for x in BYTE_RE.findall(scan) if x.startswith(b'/Game/UI/Ink/'))
                    # UTF-16 ASCII package names: conservative hint, not a package parser.
                    if b'/\x00G\x00a\x00m\x00e\x00/' in scan:
                        refs.update(x.decode('ascii') for x in BYTE_RE.findall(scan.replace(b'\x00', b'')) if x.startswith(b'/Game/UI/Ink/'))
                    tail = scan[-4096:]
            for ref in refs & all_aliases:
                # Package's own name is not an incoming dependency.
                if not (p.is_relative_to(ROOT / 'Content') and package(p) == ref):
                    binary_refs[ref].append(rel)
            binary_count += 1
        except OSError as exc:
            errors.append(dict(file=rel, error=str(exc)))
    save('static-text-evidence.json', dict(checked_at=now(), corpus=corpus, references=dict(evidence),
                                        aliases={k: sorted(v) for k, v in aliases.items()}, errors=errors))
    save('dynamic-load-sites.json', dict(sites=dynamics, unresolved_dynamic_behavior=True,
                                       note='Call-site leads only; formatted paths, Blueprint and external runtime data require owner review.'))
    save('binary-string-leads.json', dict(files_scanned=binary_count, bytes_scanned=binary_bytes,
                references=dict(binary_refs), hard_soft_classification=False,
                absence_is_zero_use_proof=False, errors=errors))
    known_dynamic = known_dynamic_paths(aliases)
    save('known-dynamic-paths.json', dict(checked_at=now(), packages=dict(known_dynamic),
        runtime_tested=False, other_dynamic_sites_still_pending=True))
    decisions = []
    for pkg, old in sorted(indexed.items()):
        p = ROOT / old['relative_path']
        refs = [(r, e) for r in [pkg] + sorted(aliases[pkg]) for e in evidence.get(r, [])]
        nonalias = [(r, e) for r, e in refs if e['kind'] in {'runtime_source', 'runtime_content', 'config'}
                    and 'PackageRedirects=' not in e['text']]
        alias_refs = [(r, e) for r, e in refs if e['kind'] == 'alias_table' or 'PackageRedirects=' in e['text']]
        tooling = [(r, e) for r, e in refs if e['kind'] == 'tooling_or_editor']
        binary = sorted({f for r in [pkg] + sorted(aliases[pkg]) for f in binary_refs.get(r, [])})
        # Conservatively retain config destinations and runtime references; alias-only use
        # is still a compatibility obligation pending migration owner review.
        dynamic = known_dynamic.get(pkg, [])
        decision = ('RetainReferenced' if nonalias else 'RetainDynamicReferenced' if dynamic
                    else 'RetainCompatibility' if alias_refs else 'UnknownRetain')
        decisions.append(dict(package=pkg, relative_path=old['relative_path'], baseline_sha256=old['sha256'],
            current_exists=p.exists(), baseline_hash_matches=p.exists() and digest(p) == old['sha256'],
            decision=decision, deletion_authorized=False, static_runtime_hits=len(nonalias),
            alias_config_hits=len(alias_refs), tooling_hits=len(tooling), known_dynamic_hits=len(dynamic),
            binary_lead_count=len(binary),
            aliases=sorted(aliases[pkg]), runtime_evidence=[dict(path=r, **e) for r, e in nonalias],
            compatibility_evidence=[dict(path=r, **e) for r, e in alias_refs],
            tooling_evidence=[dict(path=r, **e) for r, e in tooling], known_dynamic_evidence=dynamic,
            binary_leads=binary,
            registry_hard='PENDING_LIVE_SCAN', registry_soft='PENDING_LIVE_SCAN',
            registry_management='PENDING_LIVE_SCAN', dynamic_review='PENDING_OWNER_REVIEW',
            zero_use_proven=False, backup_verified=False))
    save('asset-decisions.json', dict(checked_at=now(), baseline_sha256=digest(OUT / 'baseline.json'),
        registry_verified=False, assets=decisions))
    columns = ['package', 'relative_path', 'decision', 'current_exists', 'baseline_hash_matches',
               'static_runtime_hits', 'alias_config_hits', 'tooling_hits', 'known_dynamic_hits', 'binary_lead_count', 'registry_hard',
               'registry_soft', 'registry_management', 'dynamic_review', 'zero_use_proven',
               'backup_verified', 'deletion_authorized', 'baseline_sha256']
    with (OUT / 'asset-decisions.csv').open('w', encoding='utf-8-sig', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=columns, extrasaction='ignore')
        writer.writeheader()
        writer.writerows(decisions)
    git = subprocess.run(['git', 'status', '--short', '--', 'Content/UI/Ink'], cwd=ROOT,
                         capture_output=True, text=True, check=True)
    save('audit-summary.json', dict(checked_at=now(), phase='audit_only', asset_count=len(decisions),
        decisions=dict(Counter(r['decision'] for r in decisions)),
        baseline_drift=[r['package'] for r in decisions if not r['baseline_hash_matches']],
        text_files_scanned=len(corpus), binary_files_scanned=binary_count, scan_errors=errors,
        exact_six_current=dict(legacy_absent=exact['legacy_absent'], canonical_matches=exact['canonical_matches'],
                               backup_matches=exact['backup_matches']),
        git_status_ink=git.stdout.splitlines(), deletion_candidates=0,
        registry_verified=False, dynamic_use_verified=False, assets_changed_by_audit=False))
    print((OUT / 'audit-summary.json').read_text(encoding='utf-8'))


def backup(packages_file):
    # This is a copy-and-verify tool, deliberately has no cleanup/delete command.
    requested = json.loads(Path(packages_file).read_text(encoding='utf-8-sig'))
    if not isinstance(requested, list) or not requested or not all(isinstance(x, str) for x in requested):
        raise ValueError('Input must be a nonempty JSON list of baseline package names.')
    baseline_rows = {r['package']: r for r in read('baseline.json')['files'] if r['package']}
    if len(set(requested)) != len(requested) or not set(requested) <= set(baseline_rows):
        raise ValueError('Duplicates or packages outside immutable baseline.')
    target = OUT / 'backups' / datetime.now().strftime('%Y%m%dT%H%M%S%f')
    rows = []
    for pkg in requested:
        row = baseline_rows[pkg]
        source = (ROOT / row['relative_path']).resolve()
        if not source.is_relative_to(INK.resolve()) or NEW in source.parts:
            raise ValueError('Out of old scope.')
        # Capture current version too, but refuse stale baseline snapshots.
        if digest(source) != row['sha256']:
            raise ValueError('Asset drift: ' + pkg)
        for suffix in ('.uasset', '.uexp', '.ubulk', '.uptnl'):
            companion = source.with_suffix(suffix)
            if not companion.exists():
                continue
            dest = target / companion.relative_to(ROOT)
            dest.parent.mkdir(parents=True, exist_ok=True)
            before = digest(companion)
            shutil.copy2(companion, dest)
            after = digest(dest)
            if before != after or digest(companion) != before:
                raise RuntimeError('Backup hash or concurrent write mismatch.')
            rows.append(dict(package=pkg, source=str(companion), backup=str(dest), sha256=after,
                             size=dest.stat().st_size))
    # Preserve configuration/code compatibility contracts alongside selected packages.
    for rel in ('Config/DefaultEngine.ini', 'Source/BoundlessImmortality/Frontend/UI/InkUiAssetPaths.cpp'):
        src = ROOT / rel
        dest = target / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)
        if digest(src) != digest(dest):
            raise RuntimeError('Compatibility contract backup mismatch.')
        rows.append(dict(source=str(src), backup=str(dest), sha256=digest(dest), size=dest.stat().st_size))
    manifest = dict(created_at=now(), packages=requested, verified=True, files=rows,
                    baseline_sha256=digest(OUT / 'baseline.json'), deletion_authorized=False)
    (target / 'backup-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(str(target / 'backup-manifest.json'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['baseline', 'audit', 'backup'])
    parser.add_argument('--packages', help='JSON list required for backup')
    args = parser.parse_args()
    if args.mode == 'baseline':
        baseline()
    elif args.mode == 'audit':
        audit()
    else:
        if not args.packages:
            parser.error('--packages required')
        backup(args.packages)
