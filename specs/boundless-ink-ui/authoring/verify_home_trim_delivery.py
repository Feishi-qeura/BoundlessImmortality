"""Fresh verification of the delivered policy, pixels, portable ZIP and preservation baseline."""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import zipfile
import yaml
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT.parents[1]


def load(rel):
    return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rgba(path):
    with Image.open(path) as im:
        assert im.mode == 'RGBA', path
        im.load()
        return im.copy()


def same(a, b):
    return a.mode == b.mode and a.size == b.size and a.tobytes() == b.tobytes()


def nine_x(im, margins, width):
    l, r, t, b = margins
    w, h = im.size
    assert l+r < min(w, width) and t+b < h
    result = Image.new('RGBA', (width, h))
    result.paste(im.crop((0, 0, l, h)), (0, 0))
    result.paste(im.crop((w-r, 0, w, h)), (width-r, 0))
    for y0, y1 in ((0,t), (t,h-b), (h-b,h)):
        part = im.crop((l,y0,w-r,y1))
        if width != w:
            part = part.resize((width-l-r,y1-y0),Image.Resampling.LANCZOS)
        result.paste(part,(l,y0))
    return result


def main():
    contract = load('contracts/sprite-contract.yaml')
    manifest = load(contract['output']['manifest_path'])
    assert contract['pack_id'] == manifest['pack_id'] == 'home-components-trim-v2'
    assert contract['items'] == manifest['items']
    assert contract['review']['complete'] and not contract['review']['user_approved']
    assert contract['review'] == manifest['review'] and not contract['candidate_only'] and not manifest['candidate_only']
    assert contract['policy_revision'] == manifest['policy_revision'] == 'v4'
    base = load('assets/sprites/home-components-trim-v1/sprite-manifest.yaml')
    base_items = {i['id']:i for i in base['items']}
    approval = load('evidence/approval-home-components.json')
    assert {i['source_asset_id'] for i in manifest['items']} == set(approval['approved_assets'])
    assert len(manifest['items']) == 18
    sheet = rgba(ROOT/manifest['source']['sheet_path'])
    images = {}
    for item in manifest['items']:
        original, cut = rgba(ROOT/item['source_path']), rgba(ROOT/item['path'])
        assert sha(ROOT/item['path']) == item['sha256'] == base_items[item['id']]['sha256']
        assert sha(ROOT/item['source_path']) == item['source_sha256']
        assert list(cut.size) == item['dimensions']
        bbox = original.getchannel('A').point(lambda a:255 if a>1 else 0).getbbox()
        assert list(bbox) == item['source_crop_bbox'] and item['padding'] == 8
        expected = Image.new('RGBA',(bbox[2]-bbox[0]+16,bbox[3]-bbox[1]+16))
        expected.paste(original.crop(bbox),(8,8))
        assert same(cut, expected), item['id']
        outside = original.getchannel('A')
        outside.paste(0,bbox)
        assert outside.getextrema()[1] <= 1
        x,y,w,h = item['bbox']
        assert same(cut,sheet.crop((x,y,x+w,y+h)))
        images[item['component_id']] = cut
    assert sum(i['slice']['type']=='9-slice' for i in manifest['items']) == 2
    assert sum(i['slice']['type']=='full' for i in manifest['items']) == 16
    by_component = {i['component_id']:i for i in manifest['items']}
    assert by_component['home-top-tile']['slice']['type']=='full'
    rank = by_component['home-rank-paper']
    assert rank['slice']['margins']==[820,400,115,125]
    assert rank['responsive_render_policy']['wide']['minimum_policy_width_at_source_height']==1575
    plan = load('contracts/home-slice-correction-v4.yaml')
    final_review = load('evidence/home-slice-correction-v4-visual-review.json')
    carried = load('evidence/home-slice-correction-visual-review.json')
    old = load('evidence/home-9slice-visual-review.json')
    assert final_review['complete'] and final_review['visual_acceptance_passed'] and not final_review['user_approved']
    assert final_review['inspected_count']==1 and not final_review['pending']
    assert final_review['current_turn_images_reopened']==0
    provenance=load('evidence/v4-review-reuse-provenance.json')
    assert provenance['preview_sha256']==final_review['inspected'][0]['sha256']
    assert provenance['current_turn_images_reopened']==0
    assert not any(i['acceptance_blocking'] for i in old['issues'])
    accepted = old['inspected'][:3]+[i for i in carried['inspected'] if i['result'] in {'visual-pass-at-review-scale','guide-placement-pass'}]+final_review['inspected']
    assert len(accepted)==7
    for record in accepted:
        assert sha(ROOT/record['path']) == record['sha256']
    preview_checks = []
    for test in plan['tests']:
        assert sha(ROOT/test['path']) == test['sha256']
        im = images[test['component_id']]
        actual = rgba(ROOT/test['path'])
        if test['mode'].startswith('full/Fit'):
            dw,dh = test['dimensions']
            scale = min(dw/im.width,dh/im.height)
            size = (round(im.width*scale),round(im.height*scale))
            origin = ((dw-size[0])//2,(dh-size[1])//2)
            assert list(size)==test['art_dimensions'] and list(origin)==test['art_origin']
            art = im if im.size==size else im.resize(size,Image.Resampling.LANCZOS)
            expected = Image.new('RGBA',(dw,dh))
            expected.paste(art,origin)
            assert same(actual,expected)
        else:
            expected = nine_x(im,test['margins_lrtb'],test['dimensions'][0])
            assert same(actual,expected)
            l,r,_,_ = test['margins_lrtb']
            assert same(im.crop((0,0,l,im.height)),actual.crop((0,0,l,im.height)))
            assert same(im.crop((im.width-r,0,im.width,im.height)),actual.crop((actual.width-r,0,actual.width,im.height)))
        preview_checks.append(test['path'])
    # Carry the reviewed track's three sizes and its height-locked container test.
    for test in load('contracts/home-9slice-review.yaml')['tests']:
        if test['component_id']!='home-bar-track':
            continue
        im = images['home-bar-track']
        actual = rgba(ROOT/test['path'])
        if test['variant']=='tall':
            expected = Image.new('RGBA',tuple(test['dimensions']))
            expected.paste(im,tuple(test['art_origin']))
        else:
            expected = nine_x(im,[260,260,60,60],test['dimensions'][0])
        assert same(actual,expected)
        preview_checks.append(test['path'])
    guide_checks=[]
    for cid,margins,path in [('home-bar-track',[260,260,60,60],'evidence/9slice-review-v2/home-bar-track-guides-v2.png'),
                             ('home-rank-paper',[820,400,115,125],'evidence/slice-correction-v3/home-rank-paper-guides-v3.png')]:
        guide=images[cid].copy();draw=ImageDraw.Draw(guide);l,r,t,b=margins
        for x in (l,guide.width-r):draw.line([(x,0),(x,guide.height-1)],fill=(34,191,237,255),width=3)
        for y in (t,guide.height-b):draw.line([(0,y),(guide.width-1,y)],fill=(34,191,237,255),width=3)
        assert same(guide,rgba(ROOT/path))
        guide_checks.append(path)
    package = ROOT/contract['output']['package_path']
    with zipfile.ZipFile(package) as archive:
        assert archive.testzip() is None
        names = archive.namelist()
        assert len(names)==len(set(names))
        checksums = json.loads(archive.read('file-checksums.json'))
        assert set(names)==set(checksums)|{'file-checksums.json'}
        for name,digest in checksums.items():
            assert hashlib.sha256(archive.read(name)).hexdigest()==digest,name
            assert not name.startswith(('/', '\\')) and '..' not in Path(name).parts
        portable = yaml.safe_load(archive.read('sprite-manifest.yaml'))
        assert portable['review']==manifest['review'] and portable['pack_id']==manifest['pack_id']
        portable_contract=yaml.safe_load(archive.read('sprite-contract.yaml'))
        assert portable_contract['items']==portable['items']
        assert portable_contract['review']==contract['review']
        assert portable_contract['policy_revision']==contract['policy_revision']
        assert len(portable['items'])==18
        for item in portable['items']:
            workspace_item=by_component[item['component_id']]
            restored=dict(item)
            assert restored.pop('workspace_path')==workspace_item['path']
            restored['path']=workspace_item['path']
            assert restored==workspace_item
            assert item['slice']==workspace_item['slice']
            assert item.get('responsive_render_policy')==workspace_item.get('responsive_render_policy')
            assert hashlib.sha256(archive.read(item['path'])).hexdigest()==workspace_item['sha256']
        assert len([n for n in names if n.startswith('items/') and n.endswith('.png')])==18
        package_preview_count=len([n for n in names if n.startswith('evidence/') and n.endswith('.png')])
        assert package_preview_count==12
        for name in names:
            if name.startswith(('evidence/','contracts/')):
                assert archive.read(name)==(ROOT/name).read_bytes(),name
    old_zip=ROOT/'D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-trim-v1-png.zip'
    assert sha(old_zip)=='22c5e48cfa6a51fe565289a013619a79bd59b8d196427db12c110e4b09544f0d'
    preserved=load('evidence/pre-finalization-preserved-files.json')['files']
    assert all((ROOT/rel).is_file() and sha(ROOT/rel)==digest for rel,digest in preserved.items())
    baseline=load('evidence/preservation-baseline.json')
    changed=[b['path'] for b in baseline if not (GAME/b['path']).is_file() or sha(GAME/b['path'])!=b['sha256']]
    assert not changed,changed
    assets={a['id']:a for a in load('contracts/asset-manifest.yaml')['assets']}
    assert sha(ROOT/assets['home-v2']['path'])==assets['home-v2']['sha256']
    assert assets['home-components-trim-v2-package']['sha256']==sha(package)
    status=load('evidence/delivery-status.json')
    if status.get('stage_complete'):
        assert status['active_contract']=='contracts/sprite-contract.yaml'
        assert status['active_manifest']==contract['output']['manifest_path']
        assert status['derivative_pack']==contract['pack_id'] and not status['derivative_user_approved']
        assert status['visual_review']['complete'] and not status['visual_review']['open_issues']
        assert status['package']['sha256']==sha(package) and status['package']['entries']==len(names)
        assert not status['remaining'] and not status['ue_import_performed'] and not status['runtime_files_written']
    strict=subprocess.run([sys.executable,r'C:\Users\zxxuh\.codex\skills\gameui\scripts\validate_project.py',str(ROOT),'--strict'],capture_output=True,text=True,encoding='utf-8')
    (ROOT/'evidence/validate-home-trim-final-strict.txt').write_text(strict.stdout+strict.stderr,encoding='utf-8')
    assert strict.returncode==0,strict.stdout+strict.stderr
    report=dict(passed=True,pack_id='home-components-trim-v2',policy_revision='v4',source_component_count=18,
                retained_rgba_checks=18,unchanged_original_source_hashes=18,unchanged_trimmed_png_hashes=18,source_sheet_region_checks=18,
                deterministic_preview_checks=len(preview_checks),guide_pixel_checks=len(guide_checks),
                accepted_visual_images=7,open_author_review_issues=0,derivative_user_approved=False,
                protected_files=dict(checked=len(baseline),unchanged=len(baseline),changed=[]),
                package=dict(path=package.relative_to(ROOT).as_posix(),sha256=sha(package),bytes=package.stat().st_size,
                             entries=len(names),component_png_count=18,preview_png_count=package_preview_count,integrity_passed=True,all_entry_hashes_verified=True),
                old_zip_unchanged=True,approved_home_hash_unchanged=True,
                existing_images_and_packages=dict(checked=len(preserved),unchanged=len(preserved),changed=[],
                    evidence='evidence/pre-finalization-preserved-files.json'),
                evidence_reuse=dict(current_turn_images_reopened=0,prior_reported_v4_pass_reused=True,
                    evidence='evidence/v4-review-reuse-provenance.json'),
                strict_validation=dict(exit_code=0,errors=0,warnings=0,evidence='evidence/validate-home-trim-final-strict.txt'),
                ue_import_performed=False,runtime_tests_run=False,atlas_created=False)
    (ROOT/'evidence/home-trim-final-checks.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False))


if __name__=='__main__':
    main()
