from pathlib import Path
import json, hashlib, yaml, csv, math, zipfile, subprocess, sys
from PIL import Image, ImageDraw, ImageFont, ImageChops

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT.parents[1]
PACK='home-components-trim-v1'
OUT=ROOT/'assets/sprites'/PACK
PAD=8
ALPHA_THRESHOLD=1
PREVIEW_DIR='evidence/9slice-review-v2'
def write(rel,text):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text.strip()+'\n',encoding='utf-8')
def load(rel):return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))
def dump(rel,obj):write(rel,yaml.safe_dump(obj,allow_unicode=True,sort_keys=False,width=110))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save_new(im,p):
    p.parent.mkdir(parents=True,exist_ok=True)
    if p.exists():
        with Image.open(p) as prior:
            if prior.mode!=im.mode or prior.size!=im.size or prior.tobytes()!=im.tobytes():
                raise RuntimeError('Refuse to overwrite differing existing pixels: '+str(p))
    else:im.save(p)

components=load('contracts/component-contract.yaml')
assets=load('contracts/asset-manifest.yaml')
lookup={a['id']:a for a in assets['assets']}
approval=dict(date='2026-10-22',timezone='Asia/Hong_Kong',source='direct-user-message',quote='确认',
    resolves_previous_offer='确认18个首页组件，并继续切图与九宫格审核',
    approved_assets=[c['current_asset_id'] for c in components['components']],
    source_page='home-v2',authorized_stage='lossless-individual-trimming-and-9slice-review',
    runtime_integration_authorized=False,atlas_generated=False)
write('evidence/approval-home-components.json',json.dumps(approval,ensure_ascii=False,indent=2))
for c in components['components']:
    a=lookup[c['current_asset_id']]
    assert sha(ROOT/a['path'])==a['sha256'],a['id']
    a.update(status='approved',approved=True,review_status='user-approved',approval_evidence='evidence/approval-home-components.json')
    c.update(status='approved',review='user-approved',approval_evidence='evidence/approval-home-components.json')
components['status']='approved'
components['review'].update(user_approved=True,approval_evidence='evidence/approval-home-components.json')

# All cut sources are already separate approved images. Explicit bounds preserve each
# semantic component, including disconnected hair/cloud/glow details. No color editing.
items=[];source_total=0;trim_total=0
for c in components['components']:
    a=lookup[c['current_asset_id']]
    with Image.open(ROOT/a['path']) as im:
        im=im.convert('RGBA');original_size=list(im.size);alpha=im.getchannel('A')
        bbox=alpha.point(lambda x:255 if x>ALPHA_THRESHOLD else 0).getbbox()
        assert bbox
        cut=im.crop(bbox)
        result=Image.new('RGBA',(cut.width+2*PAD,cut.height+2*PAD),(0,0,0,0))
        result.paste(cut,(PAD,PAD)) # No alpha mask: preserve every retained RGBA byte.
        sid=c['id']+'-default-trim-v1'
        p=OUT/'items'/f'{sid}.png';save_new(result,p)
        assert result.crop((PAD,PAD,PAD+cut.width,PAD+cut.height)).tobytes()==cut.tobytes()
        # The discarded data has no pixel above 1/255 Alpha, including faint glows.
        outside=alpha.copy();outside.paste(0,bbox);assert outside.getextrema()[1]<=ALPHA_THRESHOLD
        source_total+=im.width*im.height;trim_total+=result.width*result.height
        item=dict(id=sid,semantic_name=sid,component_id=c['id'],category=c['category'],state='default',
            source_asset_id=a['id'],source_path=a['path'],source_dimensions=original_size,source_sha256=a['sha256'],
            source_crop_bbox=list(bbox),source_crop_format='left-top-right-bottom-exclusive',padding=PAD,
            dimensions=list(result.size),transparent_background=True,path=p.relative_to(ROOT).as_posix(),
            sha256=sha(p),retained_pixels_exact=True,max_discarded_alpha=ALPHA_THRESHOLD,
            pivot=[0.5,0.5],slice=dict(type='full',margins=[0,0,0,0]),
            slice_review='full/Fit preserves original silhouette',dev_only=c['dev_only'])
        items.append(item)
        c['derived_sprite_id']=sid;c['derived_sprite_path']=item['path']

# Build an actual, losslessly assembled provenance sheet for the sprite contract.
# This is not an Atlas: there are no optimized regions/UV exports or engine bindings.
sheet_width=4096;gap=48;x=gap;y=gap;row_h=0
for item in items:
    w,h=item['dimensions']
    if x+w+gap>sheet_width:x=gap;y+=row_h+gap;row_h=0
    item['bbox']=[x,y,w,h]
    x+=w+gap;row_h=max(row_h,h)
sheet_height=y+row_h+gap
sheet=Image.new('RGBA',(sheet_width,sheet_height),(0,0,0,0))
for item in items:
    with Image.open(ROOT/item['path']) as im:
        x,y,w,h=item['bbox'];sheet.paste(im,(x,y))
        assert sheet.crop((x,y,x+w,y+h)).tobytes()==im.tobytes()
sheet_path=OUT/'source-sheet-v1.png';save_new(sheet,sheet_path)

# Coordinates measured in the new trimmed PNGs. Only stable straight edge regions
# can stretch; source ornamented caps are kept outside the stretch rectangle.
candidate_margins={
    'home-bar-track':([260,260,60,60],'左右完整菱形端帽与卷角固定，上下金线在60px以内；中心为平直空轨道。'),
    'home-top-tile':([180,180,200,200],'四角金色卷纹固定；伸缩区只穿过平直边与无文字深墨内底。'),
    'home-rank-paper':([600,350,115,125],'左云尾与下侧云头留在600px端区，右云尾与卷角留在350px端区；中心纸面及直边伸缩。'),
}
full_reasons={
    'home-realm-plaque':'上下中央有尖饰；普通九宫格会把中央尖饰横向拉宽，因此保持full/Fit。',
    'home-identity-ink':'墨迹收笔与右端干刷没有重复平直伸缩区，保持等比。',
    'home-preview-paper':'不规则纸边和右侧渐细长尾必须保持轮廓，full/Fit。',
    'home-bottom-cloud':'云团位置与细云尾是整体构图，非无缝纹理；full/Fit且不重复铺贴。',
    'home-health-fill':'只在轨道内按fraction遮罩，不用九宫格或拉伸改变边缘。',
    'home-qi-fill':'只在轨道内按fraction遮罩，与血量共享轨道几何。',
}
def nine(im,margins,dest):
    l,r,t,b=margins;w,h=im.size;dw,dh=dest
    assert l+r<w and t+b<h and l+r<dw and t+b<dh
    sx=[0,l,w-r,w];sy=[0,t,h-b,h];dx=[0,l,dw-r,dw];dy=[0,t,dh-b,dh]
    out=Image.new('RGBA',dest,(0,0,0,0))
    for row in range(3):
        for col in range(3):
            part=im.crop((sx[col],sy[row],sx[col+1],sy[row+1]))
            size=(dx[col+1]-dx[col],dy[row+1]-dy[row])
            if part.size!=size:part=part.resize(size,Image.Resampling.LANCZOS)
            out.paste(part,(dx[col],dy[row]))
    corners=[((0,0,l,t),(0,0,l,t)),((w-r,0,w,t),(dw-r,0,dw,t)),
             ((0,h-b,l,h),(0,dh-b,l,dh)),((w-r,h-b,w,h),(dw-r,dh-b,dw,dh))]
    assert all(im.crop(src).tobytes()==out.crop(dst).tobytes() for src,dst in corners)
    return out

tests=[];guides=[]
for item in items:
    cid=item['component_id']
    if cid not in candidate_margins:
        item['slice_review']=full_reasons.get(cid,'头像、镂空框、图标或光效保持完整轮廓，等比Fit，不作九宫格。')
        continue
    margins,why=candidate_margins[cid];w,h=item['dimensions'];l,r,t,b=margins
    assert l+r<w and t+b<h
    item['slice']=dict(type='9-slice',margins=margins)
    item['slice_review']=why
    item['normalized_margins_lrtb']=[l/w,r/w,t/h,b/h]
    item['minimum_source_pixel_size']=[l+r+1,t+b+1]
    item['allowed_stretch_axes']=['x','y'] if cid=='home-top-tile' else ['x']
    item['height_policy']='free within minimum' if cid=='home-top-tile' else 'uniform-scale to intended control height, then lock height and stretch X only'
    with Image.open(ROOT/item['path']) as im:
        guide=im.copy();draw=ImageDraw.Draw(guide)
        for vx in [l,w-r]:draw.line([(vx,0),(vx,h-1)],fill=(34,191,237,255),width=3)
        for vy in [t,h-b]:draw.line([(0,vy),(w-1,vy)],fill=(34,191,237,255),width=3)
        path=ROOT/PREVIEW_DIR/f'{cid}-guides-v2.png';save_new(guide,path)
        guides.append(dict(component_id=cid,path=path.relative_to(ROOT).as_posix(),margins=margins,dimensions=[w,h],reason=why))
        for label,dest in [('native',(w,h)),('wide',(math.ceil(w*1.4),h)),('narrow',(max(l+r+32,math.ceil(w*.8)),h)),('tall',(w,math.ceil(h*1.35)))]:
            origin=[0,0]
            mode='nine-slice'
            if label=='tall' and cid!='home-top-tile':
                out=Image.new('RGBA',dest,(0,0,0,0));origin=[0,(dest[1]-h)//2]
                out.paste(im,tuple(origin));mode='height-locked-centered-in-taller-container'
                assert out.crop((0,origin[1],w,origin[1]+h)).tobytes()==im.tobytes()
            else:out=nine(im,margins,dest)
            endcaps=None
            if cid!='home-top-tile':
                ox,oy=origin;dw,dh=dest
                assert out.crop((ox,oy,ox+l,oy+h)).tobytes()==im.crop((0,0,l,h)).tobytes()
                assert out.crop((dw-r,oy,dw,oy+h)).tobytes()==im.crop((w-r,0,w,h)).tobytes()
                endcaps=True
            path=ROOT/PREVIEW_DIR/f'{cid}-{label}-v2.png';save_new(out,path)
            if label=='native':assert im.tobytes()==out.tobytes()
            tests.append(dict(component_id=cid,variant=label,dimensions=list(dest),path=path.relative_to(ROOT).as_posix(),
                fixed_corner_pixels_identical=True,full_endcaps_pixels_identical=endcaps,valid_stretch_center=True,
                mode=mode,art_origin=origin,native_reconstruction_exact=label=='native'))

write('prompts/pipeline/home-components-trim-v1.md','''# 首页组件切图复现
输入为18个已批准独立RGBA素材，非带文字的UI截图。
运行authoring/build_home_slices.py：以Alpha>1/255测量边界，保留框内RGBA字节，丢弃框外最多1/255的透明噪点，四周增加8px全透明留边；不改色、不缩图、不覆盖原文件。
独立素材再以已知坐标无损汇集为真实source-sheet-v1.png供溯源；按显式bbox输出，避免发丝/镂空/多团云的连通域被误拆。该汇集图不是优化Atlas。
九宫格参数按实际切图像素测量，测试native/wide/narrow/tall四种尺寸；预览允许确定性9-slice缩放，原始批准图和正式切图不重采样。
''')
source=dict(screen_id='home',sheet_asset_id='home-components-source-sheet-v1',sheet_path=sheet_path.relative_to(ROOT).as_posix(),
    sheet_dimensions=list(sheet.size),sheet_kind='lossless-provenance-assembly-not-atlas',
    original_inputs=[dict(asset_id=i['source_asset_id'],path=i['source_path'],sha256=i['source_sha256']) for i in items])
manifest=dict(schema_version=1,project_id=ROOT.name,pack_id=PACK,status='generated',source=source,
    coordinate_order=dict(bbox='x-y-width-height on source-sheet',source_crop='left-top-right-bottom exclusive on original PNG',margins='left-right-top-bottom'),
    trim_policy=dict(alpha_threshold=ALPHA_THRESHOLD,padding=PAD,color_resampling=False,retained_rgba_exact=True),items=items,
    review=dict(complete=False,text_free=True,user_approved=False,notes=['组件源图已获用户批准；派生切图和九宫格测试待本轮像素与视觉核对。']))
dump(OUT.relative_to(ROOT)/'sprite-manifest.yaml',manifest)
sprite=dict(schema_version=1,project_id=ROOT.name,status='generated',pack_id=PACK,text_policy='remove-all',source=source,
    detection=dict(mode='alpha',alpha_threshold=ALPHA_THRESHOLD,background_tolerance=0,connect_gap=0,min_area=0,padding=PAD,
        method='explicit-per-component-bounds; no connected-component detection'),
    output=dict(items_dir=(OUT/'items').relative_to(ROOT).as_posix(),manifest_path=(OUT/'sprite-manifest.yaml').relative_to(ROOT).as_posix(),
        package_path='packages/'+PACK+'-png.zip'),
    semantic_mapping=dict(required=True,naming_pattern='{component-id}-default-trim-v1'),items=items,review=manifest['review'])
dump('contracts/sprite-contract.yaml',sprite)
dump('contracts/home-9slice-review.yaml',dict(schema_version=1,project_id=ROOT.name,status='generated-pending-visual-review',
    candidate_count=3,full_fit_count=15,source_pack=PACK,coordinate_order='left-right-top-bottom',guides=guides,tests=tests,
    engine_status='metadata and deterministic previews only; no UE import or Box brush verification',
    note='Margins are native trimmed-source pixels, not logical button padding. Preserve border scaling consistently when importing high-DPI art.'))
dump('mappings/home-components-trim-v1.yaml',dict(items=[dict(semantic_name=i['semantic_name'],component_id=i['component_id'],category=i['category'],state=i['state'],slice=i['slice'],bbox=i['bbox']) for i in items]))
components['derivative_pack']=PACK
dump('contracts/component-contract.yaml',components)

for aid,kind,path,dim in [('home-components-source-sheet-v1','sprite-sheet',sheet_path,list(sheet.size)),('home-components-trim-v1-package','package',ROOT/('packages/'+PACK+'-png.zip'),None)]:
    if any(a['id']==aid for a in assets['assets']):continue
    a=dict(id=aid,kind=kind,screen_id='home',path=path.relative_to(ROOT).as_posix(),status='generated',approved=False,
        prompt_path='prompts/pipeline/home-components-trim-v1.md',source_tool='deterministic-lossless-trim-pipeline',
        source_assets=[c['current_asset_id'] for c in components['components']])
    if dim:a.update(dimensions=dim,transparent_background=True)
    assets['assets'].append(a)
dump('contracts/asset-manifest.yaml',assets)

package=ROOT/('packages/'+PACK+'-png.zip')
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as z:
    z.write(OUT/'sprite-manifest.yaml','sprite-manifest.yaml')
    for item in items:z.write(ROOT/item['path'],'items/'+Path(item['path']).name)
    z.write(ROOT/'contracts/home-9slice-review.yaml','home-9slice-review.yaml')
    z.writestr('README.txt','18 separate, losslessly trimmed RGBA PNGs. Keep all originals. Crop threshold Alpha > 1/255, padding 8px. Native-source 9-slice margin order: left,right,top,bottom. 3 candidates have 9-slice previews; 15 use full/Fit. No Atlas or UE integration. Source sheet and original inputs are traceable in the full project.\n')
with zipfile.ZipFile(package) as z:
    assert z.testzip() is None
    for item in items:assert hashlib.sha256(z.read('items/'+Path(item['path']).name)).hexdigest()==item['sha256']
stats=dict(component_count=18,source_pixels=source_total,trimmed_pixels=trim_total,pixel_area_reduction=1-trim_total/source_total,
    retained_pixel_comparisons_passed=18,max_discarded_alpha=ALPHA_THRESHOLD,padding=PAD,
    source_hashes_unchanged=True,source_sheet_dimensions=list(sheet.size),nine_slice_candidates=3,full_fit=15,stretch_tests=len(tests),
    corner_preservation_checks=len(tests)*4,package=str(package.relative_to(ROOT)),package_sha256=sha(package),
    atlas_created=False,ue_files_created=False)
write('evidence/home-trim-pixel-checks.json',json.dumps(stats,ensure_ascii=False,indent=2))
with (ROOT/'evidence/home-trim-slices.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['id','source_asset','source_crop_ltrb','trimmed_size','padding','slice_type','margins_lrtb','retained_pixels_exact'])
    for item in items:w.writerow([item['id'],item['source_asset_id'],json.dumps(item['source_crop_bbox']),json.dumps(item['dimensions']),PAD,item['slice']['type'],json.dumps(item['slice']['margins']),True])
print(json.dumps(stats,ensure_ascii=False))
