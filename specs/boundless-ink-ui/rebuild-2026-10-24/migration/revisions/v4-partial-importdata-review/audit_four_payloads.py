"""Independent offline payload/reference-recovery review; no Unreal and no Content writes.

Rehash only the partial manifest and its eight affected package archives, plus the
twelve PNG evidence files. Do not rehash the unchanged original1759/partial612 archive.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import hashlib
import json
from PIL import Image

HERE = Path(__file__).resolve().parent
TEAM = HERE.parents[1]
ROOT = TEAM.parents[3]
INTEGRATION = TEAM.parent / 'logic-integration'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(b)
    return h.hexdigest()


def image_info(path):
    with Image.open(path) as source:
        source.load()
        rgba = source.convert('RGBA')
        alpha = rgba.getchannel('A')
        hist = alpha.histogram()
        return dict(path=str(path), file_sha256=sha(path), source_mode=source.mode,
                    width=rgba.width, height=rgba.height,
                    rgba_sha256=hashlib.sha256(rgba.tobytes()).hexdigest(),
                    alpha_sha256=hashlib.sha256(alpha.tobytes()).hexdigest(),
                    alpha_extrema=list(alpha.getextrema()), nonopaque_pixels=rgba.width * rgba.height - hist[255])


def main():
    mapping = read(TEAM / 'migration-map.json')
    rename = read(TEAM / 'engine-rename-result.json')
    reconciled = read(INTEGRATION / 'four-texture-reconciliation-result.json')
    assert rename['passed'] and rename['native_rename_succeeded'] and rename['exact_count'] == 582
    assert rename['engine_pid'] == reconciled['engine_pid'] == 47604 and reconciled['passed']
    assert len(reconciled['operations']) == 4
    backup = read(INTEGRATION / 'partial-recovery-backup.json')
    manifest = Path(backup['manifest'])
    assert backup['verified'] and sha(manifest) == backup['sha256'] == reconciled['backup_sha256']
    snapshot = read(manifest)
    archived = {Path(r['relative']).as_posix(): r for r in snapshot['files']}
    assert len(archived) == backup['count'] == 612
    assert all(r['destination_relative'] in archived for r in mapping['assets'])
    diagnostics = [(p, read(p)) for p in sorted(INTEGRATION.glob('live-importdata-diagnostic-47604-*.json'))]
    export_path, exports = next((p, d) for p, d in reversed(diagnostics) if len(d.get('read_only_payload_exports', [])) == 4)
    props_path, props = next((p, d) for p, d in reversed(diagnostics) if all(
        any(r['old_expected'] == op['source_old'] and r.get('old_exact') and r.get('shared_import_data_identity')
            for r in d.get('assets', [])) for op in reconciled['operations']))
    exported = {r['old_object']: r for r in exports['read_only_payload_exports']}
    typed = {r['old_expected']: r for r in props['assets']}
    mapped = {r['old_object']: r for r in mapping['assets']}
    findings = []
    archive_checks = []
    for op in reconciled['operations']:
        row, prop, ex = mapped[op['source_old']], typed[op['source_old']], exported[op['source_old']]
        assert op['retained_target'] == row['new_object'] == ex['new_object']
        assert prop['old']['path'] == row['old_object'] and prop['new']['path'] == row['new_object']
        assert prop['old']['asset_class'] == prop['new']['asset_class'] == '/Script/Engine.Texture2D'
        assert prop['old_import_data']['path'] == prop['new_import_data']['path']
        assert prop['new_import_data']['outer'] == row['new_object']
        assert len(prop['properties']) == 11
        assert all('error' not in values and values['old'] == values['new'] for values in prop['properties'].values())
        assert prop['old_size'] == prop['new_size']
        for relative in (row['source_relative'], row['destination_relative']):
            saved = archived[relative]
            assert sha(Path(saved['backup'])) == saved['source_sha256']
            archive_checks.append(dict(relative=relative, backup=saved['backup'], sha256=saved['source_sha256']))
        images = [image_info(Path(ex['old_export'])), image_info(Path(ex['new_export'])), image_info(Path(op['after_export']))]
        assert len({i['file_sha256'] for i in images}) == 1
        assert images[0]['file_sha256'] == ex['old_export_sha256'] == ex['new_export_sha256'] == op['payload_sha256']
        assert len({i['rgba_sha256'] for i in images}) == len({i['alpha_sha256'] for i in images}) == 1
        assert all([i['width'], i['height']] == prop['new_size'] for i in images)
        findings.append(dict(source_old=row['old_object'], retained_target=row['new_object'],
                             old_object_is_real_texture_not_redirector=True, exact_shared_import_data_proven=True,
                             properties_compared=11, dimensions_compared=True, images=images,
                             file_bytes_equal=True, rgba_pixels_equal=True, alpha_equal=True,
                             evidence_limit='Exported source pixels/properties, not GPU/render/cooked acceptance; post-save fresh UE readback remains required'))
    now = datetime.now(timezone(timedelta(hours=8))).isoformat()
    old_files = sorted(p.relative_to(ROOT).as_posix() for p in (ROOT / 'Content/UI').rglob('*') if p.is_file())
    new_present = sum((ROOT / r['destination_relative']).exists() for r in mapping['assets'])
    report = dict(checked_at=now, owner_review='cleanup', engine_started=False, content_writes=False,
        partial_manifest_sha256=backup['sha256'], partial_manifest_entries=612,
        archive_files_rehashed=len(archive_checks), archive_checks=archive_checks,
        unchanged_original_archive_rehashed=False, full_partial_archive_rehashed=False,
        export_evidence=str(export_path), typed_property_evidence=str(props_path),
        native_rename582_proven=True, four_reconciliations_passed=True, independent_pixel_checks_passed=len(findings),
        findings=findings, reconciliation_dirty_remaining=reconciled['dirty_remaining'],
        old_files_observed=old_files, canonical_files_observed=new_present,
        old_root_zero_verified=False, migration_complete=False,
        note='Current physical list is a timestamped read-only observation while sole executor may continue. No final registry/save proof yet.')
    target = HERE / 'four-texture-independent-audit.json'
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(checked_at=now, canonical_files_observed=new_present, old_files_observed=len(old_files),
        independent_payload_triplets_passed=len(findings), exact_archives_rehashed=len(archive_checks),
        full_archives_rehashed=False, remaining_dirty_report=reconciled['dirty_remaining'],
        sizes={r['retained_target'].split('.')[-1]: [r['images'][0]['width'], r['images'][0]['height']] for r in findings}),
        ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
