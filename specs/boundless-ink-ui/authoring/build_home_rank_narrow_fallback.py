"""Version the narrow-row policy; preserve source pixels and the reviewed wide branch."""
from pathlib import Path
if (Path(__file__).resolve().parents[1]/'evidence/history-archive-index.json').exists():
    raise RuntimeError('Historical generator frozen after UE integration. Use the external baseline in a separate workspace to reproduce; never overwrite archived evidence.')

import copy
import hashlib
import json
import yaml
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(rel):
    return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))


def new_text(rel, text):
    path = ROOT/rel
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x', encoding='utf-8') as stream:
        stream.write(text)


def main():
    output = 'evidence/slice-correction-v4/home-rank-paper-narrow-fit-v4.png'
    assert not (ROOT/output).exists(), 'v4 candidate already generated; preserve later review state.'
    manifest_rel = 'assets/sprites/home-components-trim-v2/sprite-manifest.yaml'
    contract_rel = 'contracts/sprite-contract-candidate-v2.yaml'
    manifest = load(manifest_rel)
    contract = load(contract_rel)
    base = load('contracts/home-slice-correction-v3.yaml')
    snapshot = 'evidence/candidate-v3-snapshot'
    for rel in (manifest_rel, contract_rel):
        new_text(snapshot+'/'+Path(rel).name, (ROOT/rel).read_text(encoding='utf-8'))
    rank = next(i for i in manifest['items'] if i['component_id'] == 'home-rank-paper')
    hashes = {i['path']: sha(ROOT/i['path']) for i in manifest['items']}
    old_zip = ROOT/'D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-trim-v1-png.zip'
    old_zip_hash = sha(old_zip)
    active_contract_hash = sha(ROOT/'contracts/sprite-contract.yaml')
    assert hashes[rank['path']] == rank['sha256']
    with Image.open(ROOT/rank['path']) as source:
        im = source.convert('RGBA')
        assert im.size == (1575, 399)
        dest = (1260, 399)
        scale = min(dest[0]/im.width, dest[1]/im.height)
        size = (round(im.width*scale), round(im.height*scale))
        assert size == (1260, 319)
        art = im.resize(size, Image.Resampling.LANCZOS)
        origin = ((dest[0]-size[0])//2, (dest[1]-size[1])//2)
        result = Image.new('RGBA', dest)
        result.paste(art, origin)
        assert result.crop((0, 40, 1260, 359)).tobytes() == art.tobytes()
        assert result.getchannel('A').crop((0, 0, 1260, 40)).getextrema() == (0, 0)
        assert result.getchannel('A').crop((0, 359, 1260, 399)).getextrema() == (0, 0)
        path = ROOT/output
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as stream:
            result.save(stream, format='PNG')
        with Image.open(path) as actual:
            assert actual.mode == result.mode and actual.size == result.size and actual.tobytes() == result.tobytes()
    policy = dict(type='aspect-dependent', source_dimensions=[1575,399],
                  coordinate_space='normalize destination height to 399 source pixels before comparing widths',
                  selection='if destination_width/destination_height < 1575/399 use full/Fit; otherwise use height-locked X-only nine-slice',
                  narrow=dict(mode='full/Fit', alignment=[0.5,0.5], preserve_aspect=True,
                              accepts_transparent_remainder=True, art_bounds_define_text_and_hit_layout=True),
                  wide=dict(mode='nine-slice-X-only', margins_lrtb=[820,400,115,125],
                            minimum_policy_width_at_source_height=1575,
                            tested_widths_at_source_height=[1575,2205],
                            height_locked=True),
                  importer_requirement='A single generic Box margin cannot express this policy; implement the aspect branch before integration.')
    rank['responsive_render_policy'] = policy
    rank['minimum_source_pixel_size'] = [1575, 399]
    rank['technical_nine_slice_minimum_source_pixel_size'] = [1221, 241]
    rank['slice_review'] = 'v4：窄于原生宽高比时整图Fit，避免v3中心纸纹压成竖带；达到原生宽高比才使用820/400px固定端区、锁高X轴九宫格。'
    manifest['policy_revision'] = 'v4'
    manifest['review']['evidence'] = 'evidence/home-slice-correction-v4-visual-review.json'
    contract['items'] = copy.deepcopy(manifest['items'])
    contract['review'] = copy.deepcopy(manifest['review'])
    contract['policy_revision'] = 'v4'
    for rel, value in ((manifest_rel, manifest), (contract_rel, contract)):
        (ROOT/rel).write_text(yaml.safe_dump(value, allow_unicode=True, sort_keys=False, width=110), encoding='utf-8')
    preview = dict(component_id='home-rank-paper', variant='narrow', mode='full/Fit-responsive-fallback',
                   path=output, dimensions=[1260,399], art_dimensions=[1260,319], art_origin=[0,40], scale=0.8,
                   integer_rounding_aspect_error=abs(1260/319-1575/399),
                   sha256=sha(ROOT/output), numerical_check_passed=True)
    plan = copy.deepcopy(base)
    plan.update(revision='v4', supersedes='contracts/home-slice-correction-v3.yaml',
                visual_review='evidence/home-slice-correction-v4-visual-review.json',
                responsive_policy=policy, policy_scope='Only rank-paper narrow rendering changes from v3; other reviewed art and branches are carried unchanged.')
    plan.pop('review_progress', None)
    plan['tests'] = [t for t in base['tests'] if not (t['component_id']=='home-rank-paper' and t['variant']=='narrow')]+[preview]
    for changed in plan['changed_components']:
        if changed['component_id'] == 'home-rank-paper':
            changed.update(reason=rank['slice_review'], responsive_render_policy=policy)
    visual = dict(date='2026-10-22', timezone='Asia/Hong_Kong', stage='home-slice-correction-v4-visual-review',
                  status='pending', complete=False, user_approved=False, per_reply_image_limit=1,
                  inspected_count=0, required_image_count=1, inspected=[], pending=[output],
                  carried_review='evidence/home-slice-correction-visual-review.json',
                  carried_passes=['home-top-tile narrow Fit v3', 'home-rank-paper guides v3', 'home-rank-paper wide v3'],
                  rejected_preview='D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/evidence/slice-correction-v3/home-rank-paper-narrow-v3.png',
                  contract='contracts/home-slice-correction-v4.yaml')
    assert all(sha(ROOT/path)==digest for path,digest in hashes.items())
    assert sha(old_zip)==old_zip_hash and sha(ROOT/'contracts/sprite-contract.yaml')==active_contract_hash
    report = dict(policy_revision='v4', reused_png_hashes_unchanged=18, original_zip_unchanged=True,
                  active_contract_unchanged=True, preview=preview, fit_bytes_verified=True,
                  transparent_top_bottom_rows=[40,40], root_cause='v3 narrow compressed 355px paper center to 40px, producing visible vertical texture band',
                  policy_fix='Do not shrink the textured nine-slice center below native width; use whole-art Fit below native aspect ratio.',
                  visual_review_complete=False, no_package_created=True, no_ue_import=True)
    new_text('contracts/home-slice-correction-v4.yaml', yaml.safe_dump(plan,allow_unicode=True,sort_keys=False,width=110))
    new_text('evidence/home-slice-correction-v4-visual-review.json', json.dumps(visual,ensure_ascii=False,indent=2)+'\n')
    new_text('evidence/home-slice-correction-v4-checks.json', json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({'new_preview':output,'art_dimensions':[1260,319],'transparent_top_bottom':40,
                      'originals_unchanged':18,'candidate_activated':False,'visual_review_pending':1}))


if __name__ == '__main__':
    main()
