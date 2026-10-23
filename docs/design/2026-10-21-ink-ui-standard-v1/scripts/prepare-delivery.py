"""Render typography and reference contact sheets; reconcile font provenance.

Run with Python + Pillow. This does not load Unreal or alter game assets.
"""
from pathlib import Path
import hashlib
import json
from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[1]


def write_json(path, data):
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def git_blob(data):
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()


def reconcile_manifest():
    path = ROOT / 'fonts/font-manifest.json'
    rows = json.loads(path.read_text(encoding='utf-8-sig'))
    provenance = json.loads((ROOT / 'evidence/noto-provenance.json').read_text(encoding='utf-8-sig'))
    for row in rows:
        if row['path'].endswith('LXGWWenKai-Regular.ttf'):
            row['role'] = 'body_and_all_numeric'
        if row['path'] in ['fonts/NotoSansSC/NotoSansSC[wght].ttf', 'fonts/NotoSansSC/OFL.txt']:
            row['role'] = 'legacy_variable_reference'
    rows = [r for r in rows if r['path'] not in ['fonts/NotoSansSC/NotoSansCJKsc-Regular.otf', 'fonts/NotoSansSC/LICENSE']]
    for rel, key, role in [
        ('Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf', 'font_git_blob', 'missing_glyph_fallback_only'),
        ('Sans/LICENSE', 'license_git_blob', 'data_fallback_license'),
    ]:
        target = ROOT / 'fonts/NotoSansSC' / Path(rel).name
        data = target.read_bytes()
        assert git_blob(data) == provenance[key], f'Official git blob mismatch: {target.name}'
        commit = provenance['commit']
        rows.append({
            'path': target.relative_to(ROOT).as_posix(),
            'source': f'https://raw.githubusercontent.com/notofonts/noto-cjk/{commit}/{rel}',
            'transfer_url': f'https://api.github.com/repos/notofonts/noto-cjk/contents/{rel}?ref={commit}',
            'repository': 'https://github.com/notofonts/noto-cjk',
            'commit': commit, 'role': role, 'bytes': len(data),
            'sha256': hashlib.sha256(data).hexdigest(), 'git_blob': git_blob(data),
            'downloaded_utc': None if key == 'font_git_blob' else provenance['checked_utc'],
            'verified_utc': provenance['checked_utc'],
            'verification': 'local bytes match official GitHub blob at pinned commit',
        })
    write_json(path, rows)


def specimen():
    def font(name, size):
        return ImageFont.truetype(str(ROOT / 'fonts' / name), size)
    body = lambda size: font('LXGWWenKai/LXGWWenKai-Regular.ttf', size)
    data = body
    title = lambda size: font('MaShanZheng/MaShanZheng-Regular.ttf', size)
    paper, ink, muted, gold = '#F3EBDD', '#202823', '#3D5861', '#B88B45'
    im = Image.new('RGB', (1440, 1200), paper)
    d = ImageDraw.Draw(im)
    d.text((48, 32), '无极仙道　字体资源试排', font=title(58), fill=ink)
    d.text((48, 112), '实际字体本地渲染 / 非 UE 截图 / 颜色与字重按角色分工', font=body(26), fill=muted)
    roles = [
        ('标题 · 马善政 / 48', title(48), '签到有礼　云山坊市　仙缘寻宝'),
        ('分区 · 霞鹜文楷 / 32', body(32), '角色属性　今日奖励　修炼进度'),
        ('按钮 · 霞鹜文楷 / 30', body(30), '开始游戏　领取奖励　确认购买'),
        ('正文 · 霞鹜文楷 / 28', body(28), '云气连接山水，文字留在安静的纸面。'),
        ('数值 · 霞鹜文楷 / 28', data(28), '血量 175 / 175　修为 346 / 449'),
        ('价格 · 霞鹜文楷 / 28', data(28), '单价 1,200　数量 × 10　总价 12,000'),
        ('说明 · 霞鹜文楷 / 24', data(24), '余额不足　请求确认中　已领取　暂未开放'),
    ]
    y = 186
    for label, face, text in roles:
        d.text((48, y), label, font=data(22), fill=muted)
        d.text((48, y+38), text, font=face, fill=ink)
        d.line((48,y+100,1392,y+100), fill=gold, width=1)
        y += 126
    d.text((48, 1100), '字号为720短边逻辑单位起点；字形像素高度仍须在UE与目标设备测量。', font=data(25), fill=ink)
    d.text((48, 1142), '全部正文、数字、提示均使用文楷；标题用马善政。Noto仅补缺字，不作默认数字字体。', font=data(23), fill=muted)
    im.save(ROOT / 'assets/03-字体试排.png')
    # A 360px-wide actual raster sample, not an engine FontDPI measurement.
    small = Image.new('RGB', (360, 220), paper)
    s = ImageDraw.Draw(small)
    s.text((12,12),'签到有礼',font=title(24),fill=ink)
    s.text((12,54),'云气连接山水，正文保持清楚。',font=body(14),fill=ink)
    s.text((12,82),'血量 175 / 175　修为 346 / 449',font=data(14),fill=ink)
    s.text((12,110),'总价 12,000　数量 × 10',font=data(14),fill=ink)
    s.text((12,145),'角色　背包　修炼　排行　菜单',font=body(13),fill=ink)
    s.text((12,183),'360px 本地栅格样张 · 非UE字体测试',font=data(12),fill=muted)
    small.save(ROOT / 'assets/04-360短边文字样张.png')


def contact_sheet(paths, name, columns, cell_size):
    w,h = cell_size
    sheet = Image.new('RGB', (columns*w, ((len(paths)+columns-1)//columns)*h), '#F3EBDD')
    d = ImageDraw.Draw(sheet)
    label_font = ImageFont.truetype(str(ROOT/'fonts/LXGWWenKai/LXGWWenKai-Regular.ttf'),18)
    records=[]
    for index,path in enumerate(paths):
        data=path.read_bytes()
        with Image.open(path) as source:
            fitted = ImageOps.contain(source.convert('RGB'), (w-16,h-42))
            x=(index%columns)*w+(w-fitted.width)//2
            y=(index//columns)*h+30
            sheet.paste(fitted,(x,y))
            records.append({'index':index+1,'source':str(path),'size':list(source.size),'sha256':hashlib.sha256(data).hexdigest()})
        d.text(((index%columns)*w+8,(index//columns)*h+4),f'{index+1:02d} / {path.stem[-12:]}',font=label_font,fill='#202823')
    sheet.save(ROOT/'evidence'/name,quality=90)
    return records


if __name__ == '__main__':
    reconcile_manifest()
    specimen()
    # Original supplied screenshots are read only; sheets support documentation review.
    qq = Path('C:/Users/zxxuh/Documents/Tencent Files/732263357/nt_qq/nt_data/Pic/2026-10/Ori')
    ids = ['426f70ba911a02b4aff6a6ef7413e7ba','7ce0ef83fe5da9544676d1ade6c71fe3','e3dd01b6bbf0cca04b308f0332101582','2acf8927d674baa1f947dc94961ff927','78af88f1d8c31c7b9cbcaab8a9fbf481','21e9a6b6f799aa1314e08c64e639c4ad','6d971720e0656ea53eddf06d3bf42b69','2679aeae6ce5cd78725894fb045fe7cf','d0c931824b6e72e415fc5f23b49eafb3','aece5ce7f6848e24af990ca60990ca71','c0f98fc5738c5f81a44a3bb35864174c','dab24bd465a9d49454ad992559916050']
    paths=[qq/(i+'.png') for i in ids]
    evidence={}
    if all(p.is_file() for p in paths):
        evidence['supplied_screenshots']=contact_sheet(paths,'01-原12屏核对总览.jpg',4,(400,570))
    desktop=Path('C:/Users/zxxuh/Desktop/UI资源/2')
    refs=sorted(desktop.glob('*.png')) if desktop.exists() else []
    if refs:
        evidence['supplementary_concepts']=contact_sheet(refs,'02-补充概念图核对总览.jpg',2,(836,540))
    if evidence:
        write_json(ROOT/'evidence/reference-review.json',evidence)
    print(json.dumps({'prepared':True,'reference_groups':list(evidence)},ensure_ascii=False))
