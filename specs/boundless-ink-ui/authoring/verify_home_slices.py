"""Read existing art; refresh numerical evidence without rebuilding approved assets."""
from pathlib import Path
from io import BytesIO
import hashlib
import json
import math
import sys
import zipfile

import PIL
from PIL import Image, ImageDraw
import yaml

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT.parents[1]


def load(path):
    return yaml.safe_load((ROOT / path).read_text(encoding='utf-8'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rgba(path):
    with Image.open(path) as im:
        assert im.mode == 'RGBA', path
        im.load()
        return im.copy()


def same(a, b):
    return a.mode == b.mode and a.size == b.size and a.tobytes() == b.tobytes()


def reconstruct(im, margins, dest):
    l, r, t, b = margins
    w, h = im.size
    dw, dh = dest
    assert 0 < l + r < min(w, dw) and 0 < t + b < min(h, dh)
    xs, ys = [0, l, w-r, w], [0, t, h-b, h]
    xd, yd = [0, l, dw-r, dw], [0, t, dh-b, dh]
    result = Image.new('RGBA', dest)
    for row in range(3):
        for col in range(3):
            part = im.crop((xs[col], ys[row], xs[col+1], ys[row+1]))
            size = (xd[col+1]-xd[col], yd[row+1]-yd[row])
            if part.size != size:
                part = part.resize(size, Image.Resampling.LANCZOS)
            result.paste(part, (xd[col], yd[row]))
    return result


def main():
    # This verifier checks the historical v1/v2-preview pack, even after v4 delivery.
    historical = 'contracts/sprite-contract-trim-v1.yaml'
    sprite = load(historical if (ROOT/historical).is_file() else 'contracts/sprite-contract.yaml')
    manifest = load(sprite['output']['manifest_path'])
    review = load('contracts/home-9slice-review.yaml')
    assets = {a['id']: a for a in load('contracts/asset-manifest.yaml')['assets']}
    components = load('contracts/component-contract.yaml')['components']
    approval = load('evidence/approval-home-components.json')
    assert len(components) == len(sprite['items']) == 18
    assert set(approval['approved_assets']) == {c['current_asset_id'] for c in components}
    assert sprite['items'] == manifest['items']
    assert sprite['source'] == manifest['source']
    assert len({i['id'] for i in sprite['items']}) == 18
    assert len({i['path'] for i in sprite['items']}) == 18
    sheet = rgba(ROOT / sprite['source']['sheet_path'])
    assert list(sheet.size) == sprite['source']['sheet_dimensions']
    assert sheet.size == (4096, 7130)
    images, retained, source_pixels, trim_pixels = {}, 0, 0, 0
    for item in sprite['items']:
        source_path, cut_path = ROOT/item['source_path'], ROOT/item['path']
        asset = assets[item['source_asset_id']]
        assert asset['approved'] and asset['status'] == 'approved'
        assert sha(source_path) == item['source_sha256'] == asset['sha256']
        assert sha(cut_path) == item['sha256']
        original, cut = rgba(source_path), rgba(cut_path)
        assert list(original.size) == item['source_dimensions']
        assert list(cut.size) == item['dimensions']
        alpha = original.getchannel('A')
        bbox = alpha.point(lambda a: 255 if a > 1 else 0).getbbox()
        assert list(bbox) == item['source_crop_bbox']
        assert item['padding'] == 8
        expected = Image.new('RGBA', (bbox[2]-bbox[0]+16, bbox[3]-bbox[1]+16))
        expected.paste(original.crop(bbox), (8, 8))
        assert same(cut, expected), item['id']
        outside = alpha.copy()
        outside.paste(0, bbox)
        assert outside.getextrema()[1] <= 1
        x, y, w, h = item['bbox']
        assert same(sheet.crop((x, y, x+w, y+h)), cut)
        images[item['component_id']] = cut
        retained += 1
        source_pixels += original.width*original.height
        trim_pixels += cut.width*cut.height
    candidates = {i['component_id']: i for i in sprite['items'] if i['slice']['type'] == '9-slice'}
    assert set(candidates) == {'home-bar-track', 'home-top-tile', 'home-rank-paper'}
    assert sum(i['slice']['type'] == 'full' for i in sprite['items']) == 15
    assert len(review['guides']) == 3 and len(review['tests']) == 12
    guide_checks = []
    for guide in review['guides']:
        cid = guide['component_id']
        assert guide['margins'] == candidates[cid]['slice']['margins']
        expected = images[cid].copy()
        w, h = expected.size
        l, r, t, b = guide['margins']
        draw = ImageDraw.Draw(expected)
        for x in (l, w-r):
            draw.line([(x, 0), (x, h-1)], fill=(34, 191, 237, 255), width=3)
        for y in (t, h-b):
            draw.line([(0, y), (w-1, y)], fill=(34, 191, 237, 255), width=3)
        assert same(rgba(ROOT/guide['path']), expected)
        guide_checks.append({'path': guide['path'], 'sha256': sha(ROOT/guide['path'])})
    test_results, corner_checks, endcap_checks = [], 0, 0
    assert {(t['component_id'], t['variant']) for t in review['tests']} == {
        (cid, v) for cid in candidates for v in ('native', 'wide', 'narrow', 'tall')}
    for test in review['tests']:
        cid, variant = test['component_id'], test['variant']
        item, im = candidates[cid], images[cid]
        l, r, t, b = item['slice']['margins']
        assert item['allowed_stretch_axes'] == (['x', 'y'] if cid == 'home-top-tile' else ['x'])
        assert item['minimum_source_pixel_size'] == [l+r+1, t+b+1]
        assert item['normalized_margins_lrtb'] == [l/im.width, r/im.width, t/im.height, b/im.height]
        dimensions = {'native': im.size, 'wide': (math.ceil(im.width*1.4), im.height),
                      'narrow': (max(l+r+32, math.ceil(im.width*.8)), im.height),
                      'tall': (im.width, math.ceil(im.height*1.35))}
        dest = dimensions[variant]
        assert list(dest) == test['dimensions']
        actual = rgba(ROOT/test['path'])
        origin = (0, 0)
        locked = variant == 'tall' and cid != 'home-top-tile'
        if locked:
            origin = (0, (dest[1]-im.height)//2)
            expected = Image.new('RGBA', dest)
            expected.paste(im, origin)
        else:
            expected = reconstruct(im, item['slice']['margins'], dest)
        assert list(origin) == test['art_origin']
        assert test['mode'] == ('height-locked-centered-in-taller-container' if locked else 'nine-slice')
        assert same(actual, expected), test['path']
        ox, oy = origin
        dw, dh = (im.width, im.height) if locked else dest
        for src, dst in [((0,0,l,t),(ox,oy,ox+l,oy+t)),
                         ((im.width-r,0,im.width,t),(ox+dw-r,oy,ox+dw,oy+t)),
                         ((0,im.height-b,l,im.height),(ox,oy+dh-b,ox+l,oy+dh)),
                         ((im.width-r,im.height-b,im.width,im.height),(ox+dw-r,oy+dh-b,ox+dw,oy+dh))]:
            assert same(im.crop(src), actual.crop(dst))
            corner_checks += 1
        if cid != 'home-top-tile':
            assert same(im.crop((0,0,l,im.height)), actual.crop((0,oy,l,oy+im.height)))
            assert same(im.crop((im.width-r,0,im.width,im.height)), actual.crop((dest[0]-r,oy,dest[0],oy+im.height)))
            endcap_checks += 2
        if variant == 'native':
            assert same(im, actual)
        test_results.append({'path': test['path'], 'sha256': sha(ROOT/test['path']), 'pixels_match_reconstruction': True})
    package = ROOT/sprite['output']['package_path']
    prior = load('evidence/home-trim-pixel-checks.json')
    assert sha(package) == prior['package_sha256']
    with zipfile.ZipFile(package) as z:
        assert z.testzip() is None
        expected_names = {'sprite-manifest.yaml','home-9slice-review.yaml','README.txt'}
        expected_names.update('items/'+Path(i['path']).name for i in sprite['items'])
        assert set(z.namelist()) == expected_names and len(z.namelist()) == 21
        # Review-only metadata may advance on disk; the original ZIP stays immutable.
        archived = yaml.safe_load(z.read('sprite-manifest.yaml'))
        assert archived['items'] == manifest['items'] and archived['source'] == manifest['source']
        archived_review = yaml.safe_load(z.read('home-9slice-review.yaml'))
        assert archived_review['guides'] == review['guides'] and archived_review['tests'] == review['tests']
        for item in sprite['items']:
            data = z.read('items/'+Path(item['path']).name)
            assert hashlib.sha256(data).hexdigest() == item['sha256']
            with Image.open(BytesIO(data)) as im:
                im.load()
                assert same(im, images[item['component_id']])
    baseline = load('evidence/preservation-baseline.json')
    changed = [b['path'] for b in baseline if not (GAME/b['path']).is_file() or sha(GAME/b['path']) != b['sha256']]
    home = assets['home-v2']
    assert sha(ROOT/home['path']) == home['sha256']
    state = load('contracts/home-state-recipes.yaml')
    assert len(state['controls']) == 9 and sum(len(c['states']) for c in state['controls']) == 63
    report = dict(stage='home-trim-and-9slice-v2-numerical-recheck', python=sys.version.split()[0],
                  pillow=PIL.__version__, pyyaml=yaml.__version__, component_count=retained,
                  retained_rgba_comparisons_passed=retained, source_hashes_unchanged=True,
                  source_sheet_regions_match=retained, source_pixels=source_pixels, trimmed_pixels=trim_pixels,
                  pixel_area_reduction=1-trim_pixels/source_pixels, max_discarded_alpha=1, padding=8,
                  nine_slice_candidates=3, full_fit=15, preview_reconstructions=len(test_results),
                  corner_preservation_checks=corner_checks, full_endcap_preservation_checks=endcap_checks,
                  guide_pixel_checks=guide_checks, preview_pixel_checks=test_results,
                  package=dict(path=package.relative_to(ROOT).as_posix(), entries=21, png_count=18,
                               sha256=sha(package), integrity_passed=True, unchanged=True),
                  protected_files=dict(checked=len(baseline), unchanged=len(baseline)-len(changed), changed=changed),
                  approved_home_hash_unchanged=True, state_records=63,
                  visual_review_complete=False, note='Pixel checks do not prove visual acceptance; see home-9slice-visual-review.json.',
                  runtime_files_written=False, ue_import_performed=False, atlas_created=False)
    (ROOT/'evidence/home-trim-recheck.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: report[k] for k in ('component_count','preview_reconstructions','corner_preservation_checks',
                     'full_endcap_preservation_checks','package','protected_files','visual_review_complete')}, ensure_ascii=False))
    assert not changed, 'Protected files differ from baseline; see evidence/home-trim-recheck.json'


if __name__ == '__main__':
    main()
