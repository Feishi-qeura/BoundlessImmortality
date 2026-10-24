"""Correct V1 physical-file assumptions from the live UE inventory; preserve its snapshot."""
import copy
import csv
import json
import re
from pathlib import Path
import prepare_migration as prep

OUT, ROOT = prep.OUT, prep.ROOT
FROZEN = OUT / 'revisions/v1-before-live-redirector-correction/migration-map.json'
LIVE = OUT.parent / 'logic-integration/migration-live-source-inventory.json'


def main():
    original = json.loads(FROZEN.read_text(encoding='utf-8-sig'))
    live = json.loads(LIVE.read_text(encoding='utf-8-sig'))
    originals = {r['old_package']: r for r in original['assets']}
    inventory = {r['package']: r for r in live['assets']}
    assert len(inventory) == live['source_count'] == len(originals) == 588
    assert set(inventory) == set(originals)
    classes = {}
    for name, row in inventory.items():
        match = re.search(r'package_name: "([^"]+)", asset_name: "([^"]+)"', row['asset_class'])
        if not match:
            raise RuntimeError('Unknown live class serialization; do not retain pointer repr.')
        classes[name] = match[1] + '.' + match[2]
    legacy = {r['package']: r for r in live['redirectors']}
    assert len(legacy) == 6
    canonical = {k: copy.deepcopy(v) for k, v in originals.items() if k not in legacy}
    assert len(canonical) == 582
    for name, row in canonical.items():
        assert classes[name] != '/Script/CoreUObject.ObjectRedirector'
        row.update(live_class=classes[name], class_evidence='Live UE inventory normalized to package.asset; pointer repr discarded',
                   source_kind='canonical', referencers_live='PENDING_FRESH_V2_PREFLIGHT')
    redirects = []
    for name, observed in legacy.items():
        assert classes[name] == '/Script/CoreUObject.ObjectRedirector'
        tag = re.fullmatch(r"[^']+'(/Game/UI/[^']+)\.(\w+)'", observed['destination_tag'])
        assert tag, observed['destination_tag']
        target, target_name = tag.groups()
        assert target in canonical and target_name == canonical[target]['asset_name']
        assert observed['dependencies'] == [target]
        row = copy.deepcopy(originals[name])
        row.update(source_kind='preexisting_redirector', live_class=classes[name],
            canonical_old_package=target, destination_asset_name=target_name,
            old_destination_object=target + '.' + target_name,
            forbidden_projected_new_package=row['new_package'],
            new_package=canonical[target]['new_package'], new_object=canonical[target]['new_object'],
            destination_relative=canonical[target]['destination_relative'],
            class_evidence='Live UE redirector class + exact DestinationObject tag and dependency',
            observed_referencers=observed['referencers'], observed_destination_tag=observed['destination_tag'],
            action='Do not rename; exact raw redirector fixup to migrated real canonical')
        redirects.append(row)
    m = {k: v for k, v in original.items() if k != 'assets'}
    m.update(schema_version=2, revision='v2-live-canonical-redirector-split', revised_at=prep.now(),
        original_physical_map=str(FROZEN), original_physical_map_sha256=prep.sha(FROZEN),
        live_inventory=str(LIVE), live_inventory_sha256=prep.sha(LIVE), live_inventory_engine_pid=live['engine_pid'],
        count=582, source_package_count=588, preexisting_redirector_count=6,
        canonical_asset_bytes=sum(r['bytes'] for r in canonical.values()),
        preexisting_redirector_bytes=sum(r['bytes'] for r in redirects),
        assets=sorted(canonical.values(), key=lambda r: r['old_package']),
        legacy_redirectors=sorted(redirects, key=lambda r: r['old_package']),
        expected_old_packages=sorted(originals),
        collision_policy='582 case-fold unique real canonical destinations; six legacy entries share their real canonical destinations and must never create projected new packages',
        expected_old_root_count_after=0, expected_new_canonical_count=582)
    assert len({r['new_package'].casefold() for r in m['assets']}) == 582
    assert not set(r['forbidden_projected_new_package'] for r in redirects) & set(r['new_package'] for r in m['assets'])
    prep.save(OUT / 'migration-map.json', m)
    columns = ['source_kind', 'old_package', 'new_package', 'old_object', 'new_object', 'live_class',
               'canonical_old_package', 'forbidden_projected_new_package', 'sha256', 'bytes',
               'source_relative', 'destination_relative']
    with (OUT / 'migration-map.csv').open('w', encoding='utf-8-sig', newline='') as f:
        w = csv.DictWriter(f, fieldnames=columns, extrasaction='ignore')
        w.writeheader()
        w.writerows(m['assets'] + m['legacy_redirectors'])
    prep.save(OUT / 'v2-correction-evidence.json', dict(checked_at=prep.now(),
        original_physical_packages=588, canonical_renames=582, preexisting_redirector_fixups=6,
        live_inventory_sha256=prep.sha(LIVE), new_map_sha256=prep.sha(OUT / 'migration-map.json'),
        original_snapshot_unchanged=True, backup_manifest_sha256=m['backup_manifest_sha256'],
        original_physical_bytes=m['old_ui_bytes'], canonical_bytes=m['canonical_asset_bytes'],
        redirector_bytes=m['preexisting_redirector_bytes'], redirects=m['legacy_redirectors']))
    print('MAP_V2_READY canonical=582 preexisting_redirectors=6 physical_scope=588 backup unchanged')


if __name__ == '__main__':
    main()
