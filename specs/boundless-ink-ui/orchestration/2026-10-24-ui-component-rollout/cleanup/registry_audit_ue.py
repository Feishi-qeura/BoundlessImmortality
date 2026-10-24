"""Read-only UE5.8 AssetRegistry evidence. Execute ONLY during a cleanup registry lease.

Does not load assets, save packages, fix redirectors, or delete anything. API fields
are checked against installed UE5.8 IAssetRegistry.h (dependency options lines 146+).
Engine execution has NOT been performed by preparation/static verification.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import hashlib
import json

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[4]


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def require_lease():
    lease = json.loads((OUT.parent / 'engine-lease.json').read_text(encoding='utf-8-sig'))
    if not (lease.get('owner_team') == 'cleanup' and lease.get('state') in {'granted', 'active'}
            and lease.get('cleanup_state') == 'registry_audit'):
        raise RuntimeError('Requires coordinator-issued cleanup / granted|active / registry_audit engine lease.')
    expiry = lease.get('expires_at')
    if not expiry or datetime.fromisoformat(expiry).utcoffset() is None:
        raise RuntimeError('Lease must include timezone-aware expires_at.')
    if datetime.now(timezone.utc) >= datetime.fromisoformat(expiry):
        raise RuntimeError('Cleanup registry lease expired.')
    return lease


def main():
    lease = require_lease()  # Check BEFORE importing Unreal or touching registry.
    import unreal
    if Path(unreal.Paths.project_dir()).resolve() != ROOT:
        raise RuntimeError('Wrong Unreal project.')
    baseline_path = OUT / 'baseline.json'
    base = json.loads(baseline_path.read_text(encoding='utf-8-sig'))
    static_path = OUT / 'static-text-evidence.json'
    static = json.loads(static_path.read_text(encoding='utf-8-sig'))
    indexed = {r['package']: r for r in base['files'] if r['package']}
    drift = [r['relative_path'] for r in indexed.values()
             if not (ROOT / r['relative_path']).exists() or sha(ROOT / r['relative_path']) != r['sha256']]
    text_drift = [r['file'] for r in static['corpus']
                  if not (ROOT / r['file']).exists() or sha(ROOT / r['file']) != r['sha256']]
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    reg.search_all_assets(True)
    # Full mounted roots, including plugins and newly imported component packages.
    before = reg.get_all_assets(True)
    roots = sorted({'/Game'} | {'/' + str(a.package_name).split('/')[1] for a in before
                               if str(a.package_name).startswith('/')})
    reg.scan_paths_synchronous(roots, True, True)
    if reg.is_loading_assets():
        raise RuntimeError('AssetRegistry still loading; cannot attest complete scan.')
    all_assets = reg.get_all_assets(True)
    all_packages = {str(a.package_name) for a in all_assets}
    groups = {
        'hard_package': dict(include_hard_package_references=True),
        'soft_package': dict(include_soft_package_references=True),
        'searchable_name': dict(include_searchable_names=True),
        'hard_management': dict(include_hard_management_references=True),
        'soft_management': dict(include_soft_management_references=True),
    }
    options = {}
    for category, enabled in groups.items():
        fields = dict(include_hard_package_references=False, include_soft_package_references=False,
                      include_searchable_names=False, include_hard_management_references=False,
                      include_soft_management_references=False, include_game_package_references=True,
                      include_editor_only_package_references=True)
        fields.update(enabled)
        options[category] = unreal.AssetRegistryDependencyOptions(**fields)
    rows = []
    for pkg in sorted(indexed):
        require_lease()
        aliases = static['aliases'].get(pkg, [])
        incoming = {}
        outgoing = {}
        for category, opts in options.items():
            incoming[category] = {name: sorted(str(x) for x in (reg.get_referencers(name, opts) or []))
                                  for name in [pkg] + aliases}
            outgoing[category] = sorted(str(x) for x in (reg.get_dependencies(pkg, opts) or []))
        datas = reg.get_assets_by_package_name(pkg, True, False)
        rows.append(dict(package=pkg, registry_present=pkg in all_packages,
                         asset_classes=sorted({str(a.asset_class_path) for a in datas}),
                         incoming=incoming, outgoing=outgoing, aliases=aliases,
                         zero_use_proven=False, deletion_authorized=False))
    require_lease()
    result = dict(checked_at=datetime.now(timezone(timedelta(hours=8))).isoformat(),
        read_only=True, engine_version=unreal.SystemLibrary.get_engine_version(),
        lease=lease, baseline_sha256=sha(baseline_path), static_evidence_sha256=sha(static_path),
        all_mounted_asset_count=len(all_assets), all_mounted_package_count=len(all_packages),
        scanned_roots=roots, registry_scan_finished=not reg.is_loading_assets(),
        baseline_drift=drift, static_corpus_drift=text_drift,
        missing_baseline_packages=sorted(set(indexed) - all_packages),
        includes_editor_only_and_game_references=True, assets=rows,
        scan_denylist_ignored=True, management_registration_verified=False,
        dynamic_runtime_use_verified=False,
        note='On-disk registry evidence only; no package load, no save, no fixup, no deletion. '
             'A zero referencer count alone does not establish zero business/dynamic use.')
    stamp = datetime.now().strftime('%Y%m%dT%H%M%S%f')
    target = OUT / ('registry-live-' + stamp + '.json')
    target.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('CLEANUP_REGISTRY_AUDIT_COMPLETE ' + str(target))


if __name__ == '__main__':
    main()
