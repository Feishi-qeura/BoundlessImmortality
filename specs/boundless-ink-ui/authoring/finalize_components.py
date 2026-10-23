from pathlib import Path
if (Path(__file__).resolve().parents[1]/'evidence/history-archive-index.json').exists():
    raise RuntimeError('Historical generator frozen after UE integration. Use the external baseline in a separate workspace to reproduce; never overwrite archived evidence.')

import json, hashlib, csv, html, subprocess, sys, zipfile
from PIL import Image
import yaml

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT.parents[1]
SKILL=Path(r'C:\Users\zxxuh\.codex\skills\gameui')
def write(rel,text):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text.strip()+'\n',encoding='utf-8')
def load(rel):return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))
def dump(rel,obj):write(rel,yaml.safe_dump(obj,allow_unicode=True,sort_keys=False,width=110))
def inspect(path):
    with Image.open(path) as im:
        assert im.mode=='RGBA',(path.name,im.mode)
        a=im.getchannel('A'); w,h=im.size
        bbox=a.point(lambda v:255 if v>8 else 0).getbbox()
        assert bbox and a.getextrema()[0]==0,path.name
        corners=[a.getpixel(p) for p in [(0,0),(w-1,0),(0,h-1),(w-1,h-1)]]
        assert max(corners)<=1,(path.name,corners)
        hist=a.histogram()
        return dict(dimensions=[w,h],mode=im.mode,alpha_extrema=list(a.getextrema()),corner_alpha=corners,
            content_bbox=list(bbox),content_size=[bbox[2]-bbox[0],bbox[3]-bbox[1]],
            content_uv=[bbox[0]/w,bbox[1]/h,bbox[2]/w,bbox[3]/h],
            alpha_zero_pixels=hist[0],alpha_partial_pixels=sum(hist[1:255]),
            center_alpha=a.getpixel((w//2,h//2)),sha256=hashlib.sha256(path.read_bytes()).hexdigest())

contract=load('contracts/component-contract.yaml');manifest=load('contracts/asset-manifest.yaml')
revisions={'home-qi-fill':2,'home-avatar-portrait':2}
records=json.loads((ROOT/'evidence/component-generation-records.json').read_text(encoding='utf-8'))
provenance={Path(r['path']).name:r['source'] for r in records}
assets_by_id={a['id']:a for a in manifest['assets']}
metrics=[]
for c in contract['components']:
    cid=c['id'];v=revisions.get(cid,1);assetid=f'{cid}-default-v{v}'
    p=ROOT/f'assets/components/{assetid}.png'; m=inspect(p)
    if cid in {'home-avatar-frame','home-icon-menu'}:assert m['center_alpha']==0,cid
    assert (ROOT/f'prompts/components/{assetid}.txt').is_file()
    if v>1:
        old=assets_by_id[f'{cid}-default-v1'];old.update(dimensions=inspect(ROOT/old['path'])['dimensions'],dimensions_basis='measured',status='stale',approved=False,
            review_status='replaced-by-v2-safe-padding' if cid=='home-avatar-portrait' else 'replaced-by-v2-excess-glow')
        if assetid not in assets_by_id:
            a=dict(id=assetid,kind='component',type='component',screen_id='home',component_id=cid,state='default',path=p.relative_to(ROOT).as_posix(),
                prompt_path=f'prompts/components/{assetid}.txt',source_assets=['home-v2',f'{cid}-default-v1'],version=v,approved=False)
            manifest['assets'].append(a);assets_by_id[assetid]=a
    a=assets_by_id[assetid]
    a.update(path=p.relative_to(ROOT).as_posix(),dimensions=m['dimensions'],dimensions_basis='measured',transparent_background=True,
        status='generated',approved=False,source_tool='built-in-image_gen',source_file=provenance.get(p.name),
        alpha_content_bbox=m['content_bbox'],content_uv=m['content_uv'],sha256=m['sha256'],mode=m['mode'],
        review_status='agent-reviewed-pending-user-review',text_free_visual_check=True)
    c.update(output=a['path'],prompt=a['prompt_path'],status='generated',review='agent-reviewed-pending-user-review',current_asset_id=assetid,
        actual_dimensions=m['dimensions'],content_bbox=m['content_bbox'],content_uv=m['content_uv'],
        source_to_logical_scale=[720/941,1280/1672],
        notes='原始PNG完整保留；content_uv只用于预览/后续UV裁切。不把目标尺寸冒充实际尺寸；Slice保持full，未批准9-slice。')
    if cid=='home-avatar-portrait':
        assert m['content_bbox'][0]>0 and m['content_bbox'][1]>0 and m['content_bbox'][2]<m['dimensions'][0] and m['content_bbox'][3]<m['dimensions'][1],m
        c['portrait_mask']='runtime circular aperture matching avatar-frame; portrait is a bust, not a full character'
    metrics.append(dict(component_id=cid,name=c['name'],asset_id=assetid,path=a['path'],dev_only=c['dev_only'],**m,
        visual_checks=dict(no_text=True,no_screenshot_background=True,single_component=True,complete_silhouette=True),user_approved=False))
    note=f"\n本轮实测：{m['dimensions'][0]}×{m['dimensions'][1]} RGBA，四角透明，Alpha内容框{m['content_bbox']}；当前版本{assetid}。已逐图查看无文字和截图背景。状态generated，等待用户审核组件本身；首页批准记录见approval-home-v2.json。\n"
    promptmd=ROOT/f'prompts/components/{cid}.md'
    t=promptmd.read_text(encoding='utf-8').replace('pending-generation；非背景组件必须实测透明角与干净边缘，源图不得在批准后被覆盖。','已生成；下述是当前版本实测结果，原图不覆写。')
    if '本轮实测：' not in t:write(promptmd.relative_to(ROOT),t+note)

contract['status']='generated-pending-component-review'
contract['review']=dict(complete=True,user_approved=False,scope='18图逐图视觉+Alpha/元数据检查；不含引擎或设备',evidence='evidence/component-image-review.json')
dump('contracts/component-contract.yaml',contract);dump('contracts/asset-manifest.yaml',manifest)
write('evidence/component-image-review.json',json.dumps(dict(passed=True,components=metrics,active_count=18,historical_revisions=2,
    current_art_user_approved=False,alpha_checks='all RGBA, nonempty, corner Alpha <=1/255; frame and gear center Alpha=0',
    no_raster_postprocessing=True,runtime_tests_run=False),ensure_ascii=False,indent=2))

state=load('contracts/home-state-recipes.yaml')
assert len(state['controls'])==9
for ctrl in state['controls']:
    assert len(ctrl['states'])==7 and all(x in {c['id'] for c in contract['components']} for x in ctrl['art'])
with (ROOT/'evidence/home-components.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['component_id','name','file','actual_width','actual_height','content_bbox','source_bbox_941x1672','text','slice','approval'])
    for c in contract['components']:w.writerow([c['id'],c['name'],c['output'],*c['actual_dimensions'],json.dumps(c['content_bbox']),json.dumps(c['source_bbox']),'no-text','full/Fit','pending-user-review'])

cards=[]
for c,m in zip(contract['components'],metrics):
    x0,y0,x1,y1=m['content_bbox'];w,h=m['dimensions']
    # SVG viewBox previews the source PNG without cropping, editing or rewriting it.
    cards.append(f'''<article><div class="sample"><svg viewBox="{x0} {y0} {x1-x0} {y1-y0}" role="img" aria-label="{c['name']}"><image href="{c['output']}" width="{w}" height="{h}"/></svg></div><h2>{c['name']}</h2><p>{html.escape(c['id'])}</p><p>{w}×{h} RGBA · {'仅预览' if c['dev_only'] else '无字独立素材'} · 未批准</p><a href="{c['output']}">查看原始 PNG ↗</a></article>''')
review='''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>首页组件审阅 · 无极仙道</title><style>
*{box-sizing:border-box}body{margin:0;background:#f3ebdd;color:#202823;font-family:Georgia,'Microsoft Yahei',serif;padding:32px;line-height:1.6}header{max-width:1120px;margin:0 auto 24px}h1{font-size:28px;margin:0 0 6px}header p{max-width:930px;margin:8px 0;font-size:14px}nav{display:flex;gap:8px;flex-wrap:wrap;margin-top:14px}button,select{font:inherit;background:#fbf7ee;border:1px solid #b88b45;border-radius:6px;padding:8px 14px;color:#202823}button{cursor:pointer}main{max-width:1120px;margin:auto;display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:16px}article{background:#fbf7ee;border:1px solid #cfc1a6;border-radius:9px;overflow:hidden}h2{font-size:17px;margin:10px 16px 0}article p{font-size:11px;margin:3px 16px;overflow-wrap:anywhere}article a{display:inline-block;font-size:12px;margin:8px 16px 12px;color:#3d5861}.sample{height:180px;display:flex;align-items:center;justify-content:center;padding:14px;background-color:#bbc2c4;background-image:linear-gradient(45deg,#d9dddd 25%,transparent 25%),linear-gradient(-45deg,#d9dddd 25%,transparent 25%),linear-gradient(45deg,transparent 75%,#d9dddd 75%),linear-gradient(-45deg,transparent 75%,#d9dddd 75%);background-size:20px 20px;background-position:0 0,0 10px,10px -10px,-10px 0}svg{width:100%;height:100%;overflow:visible;transition:filter .12s,transform .12s}.light .sample{background:#fff}.dark .sample{background:#182025}.hover svg{filter:sepia(.08) hue-rotate(145deg) saturate(.9);}.pressed svg{filter:brightness(.85);transform:translateY(2px)}.disabled svg{filter:grayscale(.7);opacity:.7}.focused article{outline:2px solid #b88b45}.selected article{border:2px solid #b88b45}.pending svg{opacity:.85}.pending article:after{content:'处理中示意 · 返回可用';font-size:11px;margin:0 16px 10px;display:block;color:#3d5861}footer{max-width:1120px;margin:22px auto 0;font-size:12px}a{color:#3d5861}@media(max-width:800px){main{grid-template-columns:repeat(2,minmax(0,1fr))}body{padding:16px}}@media(max-width:480px){main{grid-template-columns:1fr}}@media(prefers-reduced-motion:reduce){svg{transition:none}}
</style><header><h1>首页组件 · 审阅版</h1><p>18个透明无字基础素材 · 首页v2/规范/屏幕地图已获批 · 组件本身待审。原始PNG未做脚本裁切或修图；下方仅按实测Alpha内容范围展示。</p><p>文字与数值由引擎独立排版；9控件的63条状态规则见 <a href="contracts/home-state-recipes.yaml">状态契约</a>。切换状态只作共享素材渲染示意，不代表已接入UMG。</p><nav><button data-bg="checker">棋盘底</button><button data-bg="light">白底</button><button data-bg="dark">墨底</button><label>状态 <select id="state"><option value="normal">Normal</option><option value="hover">Hover 微蓝示意</option><option value="pressed">Pressed</option><option value="focused">Focused</option><option value="selected">Selected</option><option value="disabled">Disabled</option><option value="pending">Pending</option></select></label></nav></header><main>CARDS</main><footer>本页保留原始文件，视觉图只作为审阅；保留角色属性和背包不变。素材适配、9-slice、Atlas与UE导入尚未执行。<span id="qa">素材检查中</span></footer><script>
let bg=new URLSearchParams(location.search).get('bg')||'checker',state='normal';function apply(){document.body.className=bg+' '+state}apply();for(const b of document.querySelectorAll('[data-bg]'))b.onclick=()=>{bg=b.dataset.bg;apply()};document.querySelector('#state').onchange=e=>{state=e.target.value;apply()};let checked=0,failed=0;const files=[...document.querySelectorAll('svg image')].map(x=>x.getAttribute('href'));for(const src of files){const im=new Image;im.onload=()=>finish(false);im.onerror=()=>finish(true);im.src=src}function finish(bad){checked++;if(bad)failed++;if(checked===files.length){document.body.dataset.qa=failed?'failed':'passed';document.querySelector('#qa').textContent=' · 本地加载 '+(checked-failed)+'/'+files.length+' 张';}};
</script></html>'''.replace('CARDS','\n'.join(cards))
write('component-review.html',review)

base=json.loads((ROOT/'evidence/preservation-baseline.json').read_text(encoding='utf-8'));changed=[]
for b in base:
    p=GAME/b['path']
    if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=b['sha256']:changed.append(b['path'])
preserve=dict(files_checked=len(base),unchanged=len(base)-len(changed),changed=changed,passed=not changed,runtime_written_by_this_task=False)
write('evidence/component-preservation.json',json.dumps(preserve,ensure_ascii=False,indent=2))
result=subprocess.run([sys.executable,str(SKILL/'scripts/validate_project.py'),str(ROOT),'--strict'],capture_output=True,text=True,encoding='utf-8')
write('evidence/validate-components-strict.txt',result.stdout+'\n'+result.stderr)
assert result.returncode==0,result.stdout+result.stderr

write('component-handoff.md','''# 首页组件交接
状态：生成完成、作者已检查、组件待用户审阅。源首页home-v2以及规范/屏幕地图已由用户明确批准，审批证据保存在evidence/approval-home-v2.json。

## 文件
18个当前透明无字PNG在assets/components；组件表见evidence/home-components.csv；逐组件提示词在prompts/components；版本与实际尺寸在contracts/asset-manifest.yaml。修为填充和头像采用v2，旧v1保留并标stale，不供新装配。
图像已逐张看过无文字、无截屏矩形背景；Alpha数值检查见component-image-review.json。所有当前图RGBA、四角透明，头像框和齿轮中心透明；头像v2已补留边。纸底内部完整、不通过抠字留下孔洞。

## 装配步骤
1. 保留当前真实3D场景、主角、相机和运行HUD。只将本包作为获准后接入的独立UI素材，禁止覆盖保留角色/背包资产。
2. 以原始941×1672页面为对照，source_bbox提供大致区域；换算逻辑720×1280时约乘0.765，不把生图像素直接当UE坐标。源参考范围含装饰，按钮热点另按SafeZone与88×88逻辑单位核对。
3. 使用实际content_uv去除大幅透明画布的占位影响；这是供后续UV/切图的测量，不是已完成Atlas。等比Fit图标、头像框、题字区、牌匾与云带。当前slice=full，无一项9-slice获批。
4. 头像先圆形mask再叠透明框；血量/修为先共享轨道，填充在轨道内mask按真实fraction裁切，数字单独TextBlock。头像未知沿用现有占位，不将修士肖像冒充真人。
5. 顶部tile在下、独立图标在上、动态标签单行；底部云带和阅读渐变不接收点击。预览标签只在预览输入启用。
6. 九控件七态见home-state-recipes.yaml，63条各自登记；共享基础纹理+参数，不是63张已生图。pending不可模拟成功，返回保持可用；修炼/收功由实际快照决定。绑定ActionId前核查当前源码。

## UE导入建议（未执行）
新建独立目录导入，保留RGBA；检查UI压缩、sRGB与线性采样、直通Alpha/材质混合是否匹配。半透明云雾与发光用实际UI材质验证，不将背景黑色烘进PNG。低端尺寸、Mip与打包策略留待设备采样；本包保留工具原始尺寸，没有脚本缩图。
只交通用素材与契约，不声称有UMG、uasset、已接线按钮、Atlas、设备或直播验收。审阅包包含本页及当前PNG，不包含stale草稿。

## 下一步
用户确认组件后，才批准组件清单，并按要求继续独立切图/9-slice审核/Atlas或其他页面视觉。无需重复批准已获批首页与规范。
''')

t=(ROOT/'tasks.md').read_text(encoding='utf-8')
t=t.replace('完成三文档映射；状态draft，未替用户批准。','完成三文档映射；本轮随用户规范/屏幕地图批准确认。')
t=t.replace('两版均未批准。','v1停用；v2已于本轮由用户批准。')
t=t.replace('- [ ] [component] 生成18个透明无字基础素材，9控件×7态独立渲染规则；生成中。','- [x] [component] 18个透明无字当前素材生成并检查，头像/修为各修订一版；9控件×7态共63条共享图层渲染规则。')
t+='\n- [x] [verification] 本轮实测图片尺寸/Alpha、63条状态引用、保护哈希及--strict设计包校验；待组件本身审阅。\n- [ ] [component-approval] 用户批准当前18个组件，不能由首页批准代替。\n'
write('tasks.md',t)
write('README.md',f'''# 无极仙道 · 水墨UI设计项目
当前阶段：**首页组件已生成，待审阅**。首页v2、规范和屏幕地图已批准，无需重复确认。

## 本轮成果
- [首页组件总览](component-review.html)：18个独立透明无字素材，可切换棋盘/白/墨底与状态示意。
- [组件拆解说明](components.md)、[开发交接](component-handoff.md)、[组件CSV表](evidence/home-components.csv)。
- [组件契约](contracts/component-contract.yaml)、[九控件七态规则](contracts/home-state-recipes.yaml)。
- [透明PNG审阅包](D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-v1-review.zip)：当前18张素材、提示词与契约；组件仍待用户批准。
- [已批准首页v2](assets/pages/home-v2.png)、[视觉规范](contracts/style-contract.yaml)、[44屏地图](screen-map.md)。
- [图像检查](evidence/component-image-review.json)、[严格校验](evidence/validate-components-strict.txt)、[本轮保留核对](evidence/component-preservation.json)。

## 范围与状态
保留运行主页面、角色属性、背包、场景与相机。18个当前PNG都是真实RGBA透明素材，姓名/数值/按钮标签不烘焙；另有2张停用修订稿留档。9控件的63条状态通过共享图和参数表达，不冒充63张新状态PNG。
图片提示词均由内置image_gen执行，原始PNG未裁切、改色或缩图；HTML仅以测量的Alpha内容范围作无损预览。所有素材本身尚未获用户批准。
本轮保护文件{preserve['unchanged']}/{preserve['files_checked']}哈希一致。设计包--strict校验通过，0错误/0警告；这不证明UE、真机或平台运行。未生成Atlas或UMG/uasset，也未将审阅包标成最终批准资产包。

## 下一步文本
`使用 $gameui，读取本项目；批准首页组件当前版本，继续组件切图与9-slice审核。`
''')
write('quickstart.md',f'''# 使用与真实状态
项目：{ROOT}
Python：{sys.version.split()[0]}，PyYAML/Pillow可导入。图像生成使用内置image_gen，不用API key。

## 批准记录
用户本轮原文：“批准首页v2及规范和屏幕地图，开始拆解首页组件”。已记录首页v2、style、屏幕地图与支持文档批准；未把新生成组件自动标为approved。证据见evidence/approval-home-v2.json。
上一轮三文档待审提醒是历史记录；本轮获得批准后，以当前契约和validate-components-strict.txt为准。不要重跑build_project.py或finalize_review.py，它们是历史草案编写脚本，会回退批准状态与组件清单。

## 实际生成
18个当前基础PNG，20次内置工具生成：头像和修为填充分别修订一次。旧版标stale，其余素材generated/approved=false。真实尺寸、Alpha、内容UV、哈希和缓存来源均登记；原始文件已经复制进项目。
9个控件×7状态共63条独立渲染记录，共享图层，不是63张PNG。所有文字独立渲染；本轮无UE导入。component-review.html用于审阅，支持三种底色与状态示意。

## 可复现与校验
最终逐组件提示词见prompts/components；当前版本与源图关系见component-contract和asset-manifest。prepare_components.py是本轮初始化记录，不要覆盖后续用户改动。finalize_components.py只读取原PNG检查元数据并写契约/审阅页，没有资产修图。
```powershell
$env:PYTHONUTF8='1'
& 'C:\\Users\\zxxuh\\.codex\\skills\\gameui\\.venv\\Scripts\\python.exe' 'C:\\Users\\zxxuh\\.codex\\skills\\gameui\\scripts\\validate_project.py' '{ROOT}' --strict
```
实际检查：当前18图RGBA、非空、四角透明；两个环形图中心Alpha=0；头像v2无边界截断。真实文件尺寸和Alpha范围见component-image-review.json。63条状态引用均指向已存在基础组件。
严格设计包校验：0错误/0警告。保留文件{preserve['unchanged']}/{preserve['files_checked']}哈希一致。未做UE构建、PIE、Cook、设备或生产直播测试。

## 交付与下步
D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-v1-review.zip是组件审阅包，不是已批准Atlas或最终引擎包。当前素材保留完整工具画布，内容UV供未来切图；九宫格全部full/Fit，未伪造Slice Margin。
`使用 $gameui，批准首页组件当前版本，继续组件切图与9-slice审核。`
''')

zip_path=ROOT/'D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-v1-review.zip'
package_files=[ROOT/c['output'] for c in contract['components']]
package_files += [ROOT/c['prompt'] for c in contract['components']]
package_files += [ROOT/p for p in ['components.md','component-handoff.md','component-review.html','contracts/component-contract.yaml',
    'contracts/home-state-recipes.yaml','contracts/asset-manifest.yaml','contracts/style-contract.yaml',
    'evidence/home-components.csv','evidence/component-image-review.json','evidence/approval-home-v2.json']]
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
    for p in package_files:z.write(p,p.relative_to(ROOT).as_posix())
with zipfile.ZipFile(zip_path) as z:
    assert z.testzip() is None
    pngs=[p for p in z.namelist() if p.endswith('.png')];assert len(pngs)==18
package_metrics=dict(path=zip_path.relative_to(ROOT).as_posix(),files=len(package_files),png_count=18,bytes=zip_path.stat().st_size,
    sha256=hashlib.sha256(zip_path.read_bytes()).hexdigest(),status='review-package',user_approved=False,not_atlas=True)
write('evidence/component-package.json',json.dumps(package_metrics,ensure_ascii=False,indent=2))
write('evidence/component-delivery-status.json',json.dumps(dict(stage='home-component-review',home_approved=True,style_approved=True,screen_map_approved=True,
    active_components=18,generated_pngs_including_stale=20,state_records=63,components_user_approved=False,
    image_checks_passed=True,strict_validation_exit_code=result.returncode,preservation=preserve,package=package_metrics,runtime_tests_run=False),ensure_ascii=False,indent=2))
print(json.dumps(dict(active_components=18,generated_including_stale=20,state_records=63,strict_exit=result.returncode,preservation=preserve,
    package=package_metrics),ensure_ascii=False))
