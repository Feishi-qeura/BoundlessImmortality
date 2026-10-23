"""Validate the delivered design package, not the game implementation.

Python + Pillow; optional --zip creates a sibling archive and compares every byte.
"""
from pathlib import Path
from datetime import datetime, timezone
from urllib.parse import unquote
import csv
import hashlib
import json
import math
import re
import struct
import sys
import zipfile
from PIL import Image, ImageFont

ROOT = Path(__file__).resolve().parents[1]
checks = []


def check(name, passed, detail=None):
    checks.append({'name': name, 'passed': bool(passed), 'detail': detail})


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def font_characters(path):
    """Read Unicode SFNT cmap format 4/12 without an optional fontTools install."""
    b = path.read_bytes()
    u16 = lambda o: struct.unpack_from('>H',b,o)[0]
    u32 = lambda o: struct.unpack_from('>I',b,o)[0]
    tables={}
    for i in range(u16(4)):
        pos=12+16*i
        tables[b[pos:pos+4].decode('ascii')]=(u32(pos+8),u32(pos+12))
    cmap=tables['cmap'][0]
    chars=set()
    for i in range(u16(cmap+2)):
        pos=cmap+4+8*i
        platform,encoding=u16(pos),u16(pos+2)
        if not (platform==0 or (platform==3 and encoding in (1,10))):
            continue
        sub=cmap+u32(pos+4)
        fmt=u16(sub)
        if fmt==12:
            for g in range(u32(sub+12)):
                a,z,glyph=struct.unpack_from('>III',b,sub+16+12*g)
                chars.update(c for c in range(a,z+1) if glyph+c-a!=0)
        elif fmt==4:
            n=u16(sub+6)//2
            end=sub+14
            start=end+2*n+2
            delta=start+2*n
            offset=delta+2*n
            for j in range(n):
                a,z=u16(start+2*j),u16(end+2*j)
                if a==0xffff:
                    continue
                d=u16(delta+2*j)
                r=u16(offset+2*j)
                for c in range(a,z+1):
                    if r:
                        glyph=u16(offset+2*j+r+2*(c-a))
                        glyph=(glyph+d)&0xffff if glyph else 0
                    else:
                        glyph=(c+d)&0xffff
                    if glyph:
                        chars.add(c)
    return chars


def main():
    required=['README.md','UI开发规范.md','动效与转盘商店专项.md','交给开发AI的执行约束.md','联网参考与取舍.md','设计令牌.json','验收矩阵.csv','fonts/README.md','fonts/font-manifest.json','assets/00-用户选定HUD方向.png','assets/01-转盘与商店风格参考.png','assets/02-主HUD横竖屏参考.png','assets/03-字体试排.png','assets/04-360短边文字样张.png','evidence/visual-review.md']
    for name in required:
        check('required:'+name,(ROOT/name).is_file() and (ROOT/name).stat().st_size>0)
    for path in ROOT.rglob('*.json'):
        if path.name=='delivery-validation.json':
            continue
        read_json(path)
        check('json:'+path.relative_to(ROOT).as_posix(),True)
    links=0
    for path in ROOT.rglob('*.md'):
        text=path.read_text(encoding='utf-8-sig')
        for target in re.findall(r'\[[^\]]*\]\(([^)]+)\)',text):
            if target.startswith(('https:','http:','#','mailto:')):
                continue
            target=unquote(target.split('#')[0].strip('<>'))
            check('link:'+path.relative_to(ROOT).as_posix()+':'+target,(path.parent/target).exists())
            links+=1
    font_report=[]
    for row in read_json(ROOT/'fonts/font-manifest.json'):
        path=ROOT/row['path']
        content=path.read_bytes()
        check('font_hash:'+row['path'],hashlib.sha256(content).hexdigest()==row['sha256'])
        check('font_size:'+row['path'],len(content)==row['bytes'])
        check('font_commit:'+row['path'],bool(re.fullmatch('[a-f0-9]{40}',row['commit'])))
        if path.suffix in ('.ttf','.otf'):
            face=ImageFont.truetype(str(path),28)
            font_report.append({'path':row['path'],'name':face.getname(),'role':row['role']})
        else:
            check('license:'+row['path'],'SIL OPEN FONT LICENSE' in content.decode('utf-8-sig'))
    samples={
        'fonts/MaShanZheng/MaShanZheng-Regular.ttf':'无极仙道 字体资源试排 签到有礼 云山坊市 仙缘寻宝',
        'fonts/LXGWWenKai/LXGWWenKai-Regular.ttf':'角色 背包 修炼 排行 菜单 活动 签到 仙途 开始游戏 继续游戏 存档 设置 退出游戏 确认购买 领取奖励 云气连接山水，文字留在安静的纸面。 血量 修为 单价 数量 总价 余额不足 请求确认中 已领取 暂未开放 0123456789,./%×+-()',
        'fonts/NotoSansSC/NotoSansCJKsc-Regular.otf':'·',
    }
    coverage=[]
    for rel,sample in samples.items():
        available=font_characters(ROOT/rel)
        missing=sorted({c for c in sample if not c.isspace() and ord(c) not in available})
        coverage.append({'path':rel,'sample':sample,'unicode_codepoints':len(available),'missing':missing})
        check('glyph_coverage:'+rel,not missing,missing)
    # Document the known title-font punctuation gap, rather than claiming universal coverage.
    title_chars=font_characters(ROOT/'fonts/MaShanZheng/MaShanZheng-Regular.ttf')
    fallback_chars=font_characters(ROOT/'fonts/NotoSansSC/NotoSansCJKsc-Regular.otf')
    check('title_middle_dot_fallback',ord('·') in title_chars or ord('·') in fallback_chars)
    for path in list(ROOT.rglob('*.png'))+list(ROOT.rglob('*.jpg')):
        with Image.open(path) as im:
            size=im.size
            im.verify()
        check('decode:'+path.relative_to(ROOT).as_posix(),min(size)>0,list(size))
    with (ROOT/'验收矩阵.csv').open(encoding='utf-8-sig',newline='') as f:
        rows=list(csv.DictReader(f))
    check('acceptance_unique_ids',len({r['id'] for r in rows})==len(rows),len(rows))
    check('acceptance_all_pending',all(r['status']=='pending' and not r['evidence_path'] for r in rows),len(rows))
    tokens=read_json(ROOT/'设计令牌.json')
    check('five_navigation_actions',tokens['layout']['hud']['bottom_actions']==['角色','背包','修炼','排行','菜单'])
    check('one_cultivation_action',not tokens['layout']['hud']['cultivate_action_is_navigation'])
    check('subnav_hit_height',tokens['layout']['feature_page']['subnav_min_hit_height']>=tokens['layout']['min_hit_size'][1])
    check('dpi_curve_base',all(abs(p/720-scale)<1e-8 for p,scale in tokens['layout']['dpi_curve']))
    check('font_roles_exist',all((ROOT/v['file']).is_file() for v in tokens['typography'].values() if isinstance(v,dict) and 'file' in v))
    check('xianxia_all_body_and_numeric_roles',all(tokens['typography'][role]['file']=='fonts/LXGWWenKai/LXGWWenKai-Regular.ttf' for role in ['section','button','body','data','hud_label','secondary']))
    check('health_display_label',tokens['typography']['health_display_label']=='血量')
    # Verify the written equal-sector formula, not any future UE implementation.
    cases=0
    for n in (3,6,8,12,16):
        delta=360/n
        for i in range(n):
            for start in (-721,-1,0,359,360,720,12345.5):
                for eps_fraction in (-0.15,0,0.15):
                    for turns in (2,4,6):
                        target=(-((i+0.5)*delta)-eps_fraction*delta)%360
                        final=start+360*turns+(target-start%360)%360
                        landed=math.floor((-final%360)/delta)
                        assert landed==i and final>start, (n,i,start,final,landed)
                        cases+=1
    check('written_equal_sector_formula',True,{'cases':cases,'scope':'offline formula only; no UE rendering or authority test'})
    values=[6*(i/100)**5-15*(i/100)**4+10*(i/100)**3 for i in range(101)]
    check('written_easing_monotonic_endpoints',values[0]==0 and values[-1]==1 and all(a<=b for a,b in zip(values,values[1:])))
    files=[]
    for path in sorted(ROOT.rglob('*')):
        if path.is_file() and path.name!='delivery-validation.json' and '__pycache__' not in path.parts:
            files.append({'path':path.relative_to(ROOT).as_posix(),'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    report={
        'checked_utc':datetime.now(timezone.utc).isoformat(),
        'scope':'design_package_integrity_and_written_formula_only',
        'passed':all(c['passed'] for c in checks),'checks_count':len(checks),
        'acceptance_runtime_rows':len(rows),'runtime_rows_passed':0,
        'local_markdown_links_checked':links,'fonts':font_report,'glyph_samples':coverage,
        'checks':checks,'files':files,
        'not_run':['Unreal build','PIE','Cook','Android/iOS packaging','device performance','Niagara runtime','commerce authority','actual wheel renderer'],
    }
    (ROOT/'evidence/delivery-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    failed=[c for c in checks if not c['passed']]
    print(json.dumps({'passed':report['passed'],'checks':len(checks),'runtime_pending':len(rows),'formula_cases':cases,'failed':failed},ensure_ascii=False))
    if failed:
        raise SystemExit(1)
    if '--zip' in sys.argv:
        archive=ROOT.parent/'无极仙道_UI开发规范_v1.0_含参考图与字体.zip'
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
            for path in sorted(ROOT.rglob('*')):
                if path.is_file() and '__pycache__' not in path.parts:
                    z.write(path,ROOT.name+'/'+path.relative_to(ROOT).as_posix())
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            count=0
            for name in z.namelist():
                rel=Path(name).relative_to(ROOT.name)
                assert z.read(name)==(ROOT/rel).read_bytes(),name
                count+=1
        summary={'archive':str(archive),'bytes':archive.stat().st_size,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'entries_compared':count,'crc_and_bytes_match':True}
        archive.with_suffix('.validation.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        print(json.dumps(summary,ensure_ascii=False))


if __name__=='__main__':
    main()
