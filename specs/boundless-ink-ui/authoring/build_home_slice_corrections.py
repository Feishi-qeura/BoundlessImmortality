"""Generate versioned review candidates; never modify source PNGs or activate them."""
from pathlib import Path
if (Path(__file__).resolve().parents[1]/'evidence/history-archive-index.json').exists():
    raise RuntimeError('Historical generator frozen after UE integration. Use the external baseline in a separate workspace to reproduce; never overwrite archived evidence.')

import copy
import hashlib
import json
import yaml
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
PREVIEWS = 'evidence/slice-correction-v3'


def load(rel):
    return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def new_text(rel, text):
    path = ROOT/rel
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x', encoding='utf-8') as stream:
        stream.write(text)


def new_image(rel, im):
    path = ROOT/rel
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        im.save(stream, format='PNG')
    with Image.open(path) as saved:
        assert saved.mode == im.mode and saved.size == im.size and saved.tobytes() == im.tobytes()


def same(a, b):
    return a.mode == b.mode and a.size == b.size and a.tobytes() == b.tobytes()


def nine_x(im, margins, width):
    l, r, t, b = margins
    w, h = im.size
    assert l+r < min(w, width) and t+b < h
    result = Image.new('RGBA', (width, h))
    # Horizontal-only: preserve full-height endcaps, resample only the center.
    result.paste(im.crop((0, 0, l, h)), (0, 0))
    result.paste(im.crop((w-r, 0, w, h)), (width-r, 0))
    for y0, y1 in ((0, t), (t, h-b), (h-b, h)):
        center = im.crop((l, y0, w-r, y1))
        if width != w:
            center = center.resize((width-l-r, y1-y0), Image.Resampling.LANCZOS)
        result.paste(center, (l, y0))
    assert same(im.crop((0, 0, l, h)), result.crop((0, 0, l, h)))
    assert same(im.crop((w-r, 0, w, h)), result.crop((width-r, 0, width, h)))
    if width == w:
        assert same(im, result)
    return result


def main():
    target = 'assets/sprites/home-components-trim-v2/sprite-manifest.yaml'
    assert not (ROOT/target).exists(), 'Candidate already exists; do not reset its review state.'
    base_path = 'assets/sprites/home-components-trim-v1/sprite-manifest.yaml'
    base = load(base_path)
    source_hashes = {i['path']: sha(ROOT/i['path']) for i in base['items']}
    for item in base['items']:
        assert source_hashes[item['path']] == item['sha256']
        assert sha(ROOT/item['source_path']) == item['source_sha256']
    sprite = load('contracts/sprite-contract.yaml')
    assert sprite['pack_id'] == 'home-components-trim-v1'
    active_contract_hash = sha(ROOT/'contracts/sprite-contract.yaml')
    old_zip = ROOT/'D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-trim-v1-png.zip'
    old_zip_hash = sha(old_zip)
    assert old_zip_hash == '22c5e48cfa6a51fe565289a013619a79bd59b8d196427db12c110e4b09544f0d'
    candidate = copy.deepcopy(base)
    candidate.update(pack_id='home-components-trim-v2', status='generated',
                     supersedes_pack='home-components-trim-v1', candidate_only=True,
                     pixel_source_policy='reuse original trimmed PNG paths unchanged; metadata revision only')
    candidate['review'] = dict(complete=False, text_free=True, user_approved=False,
                              evidence='evidence/home-slice-correction-visual-review.json',
                              notes=['Candidate policies only; existing active contract and ZIP remain unchanged.'])
    by_id = {i['component_id']: i for i in candidate['items']}
    top = by_id['home-top-tile']
    top['slice'] = dict(type='full', margins=[0, 0, 0, 0])
    top['slice_review'] = 'v2全轴拉伸导致角饰及云纹变形；改用整图等比Fit，居中留透明余量，不强行填满变比例容器。'
    top['allowed_stretch_axes'] = []
    top['height_policy'] = 'uniform full/Fit; center art in container; preserve source aspect ratio'
    top['fit_policy'] = dict(mode='contain', alignment=[0.5, 0.5], preserve_aspect=True,
                            source_aspect_ratio=889/1097, accepts_transparent_remainder=True)
    for key in ('normalized_margins_lrtb', 'minimum_source_pixel_size'):
        top.pop(key, None)
    rank = by_id['home-rank-paper']
    rank['slice'] = dict(type='9-slice', margins=[820, 400, 115, 125])
    rank['normalized_margins_lrtb'] = [820/1575, 400/1575, 115/399, 125/399]
    rank['minimum_source_pixel_size'] = [1221, 241]
    rank['allowed_stretch_axes'] = ['x']
    rank['slice_review'] = 'v3候选扩大完整固定端区至左820/右400px，以覆盖v2跨线云尾；仍须检查分割线和宽窄预览，禁止纵向拉伸。'
    tests = []
    with Image.open(ROOT/top['path']) as original:
        im = original.convert('RGBA')
        for variant, dest in [('wide', (1245, 1097)), ('narrow', (712, 1097)), ('tall', (889, 1481))]:
            scale = min(dest[0]/im.width, dest[1]/im.height)
            size = (round(im.width*scale), round(im.height*scale))
            art = im if size == im.size else im.resize(size, Image.Resampling.LANCZOS)
            origin = ((dest[0]-size[0])//2, (dest[1]-size[1])//2)
            result = Image.new('RGBA', dest)
            result.paste(art, origin)
            assert same(result.crop((*origin, origin[0]+size[0], origin[1]+size[1])), art)
            outside = result.getchannel('A')
            outside.paste(0, (*origin, origin[0]+size[0], origin[1]+size[1]))
            assert outside.getextrema()[1] == 0
            path = f'{PREVIEWS}/home-top-tile-{variant}-fit-v3.png'
            new_image(path, result)
            tests.append(dict(component_id=top['component_id'], variant=variant, mode='full/Fit',
                              path=path, dimensions=list(dest), art_dimensions=list(size), art_origin=list(origin),
                              scale=scale, unchanged_art_pixels=size == im.size,
                              integer_rounding_aspect_error=abs(size[0]/size[1]-im.width/im.height),
                              sha256=sha(ROOT/path), numerical_check_passed=True))
    with Image.open(ROOT/rank['path']) as original:
        im = original.convert('RGBA')
        guide = im.copy()
        draw = ImageDraw.Draw(guide)
        for x in (820, 1175):
            draw.line([(x, 0), (x, 398)], fill=(34, 191, 237, 255), width=3)
        for y in (115, 274):
            draw.line([(0, y), (1574, y)], fill=(34, 191, 237, 255), width=3)
        guide_path = f'{PREVIEWS}/home-rank-paper-guides-v3.png'
        new_image(guide_path, guide)
        for variant, width in [('native', 1575), ('wide', 2205), ('narrow', 1260)]:
            result = nine_x(im, rank['slice']['margins'], width)
            path = f'{PREVIEWS}/home-rank-paper-{variant}-v3.png'
            new_image(path, result)
            tests.append(dict(component_id=rank['component_id'], variant=variant, mode='nine-slice-X-only',
                              path=path, dimensions=[width, 399], art_origin=[0, 0], margins_lrtb=[820, 400, 115, 125],
                              full_height_endcaps_exact=True, center_width=width-1220,
                              center_scale_x=(width-1220)/355, sha256=sha(ROOT/path), numerical_check_passed=True))
    plan = dict(schema_version=1, project_id=ROOT.name, revision='v3', status='generated-pending-visual-review',
                candidate_only=True, active_sprite_contract_unchanged=True,
                base_manifest=base_path, candidate_manifest=target, source_pack='home-components-trim-v1',
                candidate_pack='home-components-trim-v2', coordinate_order='left-right-top-bottom',
                nine_slice_candidate_count=2, full_fit_count=16,
                changed_components=[dict(component_id=top['component_id'], slice=top['slice'], reason=top['slice_review']),
                                    dict(component_id=rank['component_id'], slice=rank['slice'], reason=rank['slice_review'])],
                carried_unchanged='home-bar-track keeps reviewed v2 X-only policy; other 15 full/Fit items unchanged',
                guides=[dict(component_id=rank['component_id'], path=guide_path, dimensions=[1575,399],
                             margins_lrtb=[820,400,115,125], sha256=sha(ROOT/guide_path))],
                tests=tests, visual_review='evidence/home-slice-correction-visual-review.json',
                no_ue_import=True, no_atlas=True,
                fit_layout_limit='Top tile Fit may leave transparent container remainder; future text/hit layout must follow fitted art bounds, without modifying runtime in this stage.')
    pending = [f'{PREVIEWS}/home-top-tile-narrow-fit-v3.png', guide_path,
               f'{PREVIEWS}/home-rank-paper-wide-v3.png', f'{PREVIEWS}/home-rank-paper-narrow-v3.png']
    visual = dict(date='2026-10-22', timezone='Asia/Hong_Kong', stage='home-slice-correction-v3-visual-review',
                  status='pending', complete=False, user_approved=False, per_reply_image_limit=1,
                  inspected_count=0, required_image_count=len(pending), inspected=[], pending=pending,
                  numerically_covered_without_redundant_visual_open=[
                      dict(variants='top tile wide/tall Fit', reason='Original art pixels unchanged; only centered transparent remainder added.'),
                      dict(variants='rank-paper native', reason='Exact RGBA reconstruction of source.')],
                  contract='contracts/home-slice-correction-v3.yaml')
    candidate_contract = copy.deepcopy(sprite)
    candidate_contract.update(pack_id='home-components-trim-v2', status='generated', candidate_only=True,
                              supersedes_pack='home-components-trim-v1', items=candidate['items'], review=candidate['review'])
    candidate_contract['output']['manifest_path'] = target
    candidate_contract['output']['package_path'] = 'packages/home-components-trim-v2-png.zip'
    for rel, data in [(target, candidate), ('contracts/sprite-contract-candidate-v2.yaml', candidate_contract),
                      ('contracts/home-slice-correction-v3.yaml', plan)]:
        new_text(rel, yaml.safe_dump(data, allow_unicode=True, sort_keys=False, width=110))
    new_text('evidence/home-slice-correction-visual-review.json', json.dumps(visual, ensure_ascii=False, indent=2)+'\n')
    assert all(sha(ROOT/path) == digest for path, digest in source_hashes.items())
    assert sha(ROOT/'contracts/sprite-contract.yaml') == active_contract_hash
    assert sha(old_zip) == old_zip_hash
    report = dict(candidate_pack='home-components-trim-v2', preview_revision='v3',
                  reused_png_hashes_unchanged=len(source_hashes), original_zip_unchanged=True,
                  active_contract_unchanged=True, preview_count=7, test_count=len(tests),
                  numerical_checks_passed=True, visual_review_complete=False,
                  pending_visual_images=4, no_package_created=True, no_ue_import=True, no_atlas=True)
    new_text('evidence/home-slice-correction-checks.json', json.dumps(report, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
