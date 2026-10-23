from pathlib import Path
import hashlib, json, re, csv
import yaml
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT.parents[1]
def write(rel,text):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text.strip()+'\n',encoding='utf-8')
def dump(rel,obj):write(rel,yaml.safe_dump(obj,allow_unicode=True,sort_keys=False,width=110))

manifest={'schema_version':1,'project_id':ROOT.name,'assets':[]}
review=[]
for version,original in [(1,'exec-9ae5cdc6-9f12-4ed1-ac5d-ea17087f2016.png'),(2,'exec-358ef90f-5c87-431d-929b-000b3c054166.png')]:
    p=ROOT/'assets/pages'/f'home-v{version}.png'
    with Image.open(p) as im:
        size=list(im.size);mode=im.mode;alpha=im.getextrema()[-1] if 'A' in im.getbands() else None
    asset=dict(id=f'home-v{version}',kind='page',type='page',screen_id='home',path=p.relative_to(ROOT).as_posix(),
        dimensions=size,transparent_background=bool(alpha and alpha[0]<255),status='generated' if version==2 else 'stale',approved=False,
        prompt_path=f'prompts/pages/home-v{version}.txt',brief_path=f'prompts/pages/home-v{version}.md',
        source_tool='built-in-image_gen',source_file=original,version=version,
        source_assets=['home-v1'] if version==2 else [],
        sha256=hashlib.sha256(p.read_bytes()).hexdigest(),mode=mode,
        review_status='pending-user-approval' if version==2 else 'rejected-environment-drift')
    manifest['assets'].append(asset)
    review.append(dict(asset_id=asset['id'],dimensions=size,mode=mode,alpha_extrema=alpha,
        ratio=round(size[0]/size[1],6),target_ratio=9/16,pixel_resize_performed=False,
        preview_inputs={'display_name':'主播','realm':'炼气一阶','health':'1000/1000','cultivation':'5/60'},
        checks={'one_portrait_screen':True,'top_labels_readable':True,'bottom_order_preserved':True,'preview_label_visible':True,
        'side_rank_button_visible':True,'character_not_covered':True,'exact_scene_pixels_preserved':False,
        'no_invented_distant_mountains':version==2,'no_components_generated':True},
        notes='v2删除v1新增远山和建筑，保留近似场景构图。生图重绘并非像素不变，不能当成场景替换图或运行截图。中文可读但字体非引擎字体像素，动态字后续独立排版。'))
dump('contracts/asset-manifest.yaml',manifest)
contract=yaml.safe_load((ROOT/'contracts/screen-contract.yaml').read_text(encoding='utf-8'))
for s in contract['screens']:
    if s['id']=='home':
        s['status']='pending-approval'
        s['output'].update(prompt='prompts/pages/home-v2.md',image='assets/pages/home-v2.png')
        s['approval']={'approved':False,'candidate':'home-v2','supersedes':'home-v1','reason':'用户尚未批准；运行首页仍保留'}
dump('contracts/screen-contract.yaml',contract)
write('evidence/image-review.json',json.dumps(review,ensure_ascii=False,indent=2))

planning=yaml.safe_load((ROOT/'contracts/screen-contract.yaml').read_text(encoding='utf-8'))['screens']
by_id={s['id']:s for s in planning}
for s in planning:
    assert (ROOT/s['output']['brief']).is_file(),s['id']
    assert all(x in by_id or x=='application-start' for x in s['entry_points']),s['id']
    assert s['purpose'] and s['primary_actions'] and s['data_needs'] and s['edge_cases']
source_ids={s.get('legacy_screen_id') for s in planning if s.get('legacy_screen_id')}
with (GAME/'docs/design/2026-10-21-wuling-resource-v1.4/tables/ui-screens.csv').open(encoding='utf-8-sig') as f:
    expected={s['screen_id'] for s in csv.DictReader(f)}
assert source_ids==expected,source_ids.symmetric_difference(expected)
route_audit=json.loads((ROOT/'evidence/route-audit.json').read_text(encoding='utf-8'))
registered=re.findall(r'TEXT\("([A-Za-z]+)"\)',(GAME/'Source/BoundlessImmortality/Frontend/InkFeaturePanel.cpp').read_text(encoding='utf-8').split('void UInkFeaturePanel::Configure')[0])
known={r['route'].split('|')[0] for r in route_audit}
missing=sorted(set(registered)-known)
assert not missing,missing
base=json.loads((ROOT/'evidence/preservation-baseline.json').read_text(encoding='utf-8'))
changes=[]
for r in base:
    p=GAME/r['path']
    if not p.is_file() or hashlib.sha256(p.read_bytes()).hexdigest()!=r['sha256']:changes.append(r['path'])
preserve=dict(files_checked=len(base),unchanged=len(base)-len(changes),changed=changes,
              passed=not changes,scope='Frontend source, Content/UI/Ink, SourceAssets/UI',
              source_written_by_this_task=False,comparison='read-only SHA256; independent concurrent changes are reported')
write('evidence/preservation-check.json',json.dumps(preserve,ensure_ascii=False,indent=2))
cross=dict(passed=True,screen_count=len(planning),legacy_screens_covered=len(source_ids),
    current_registered_route_names=len(set(registered)),missing_current_routes=missing,
    brief_count=len(planning),all_entry_ids_resolved=True,components_generated=0,home_approved=False,runtime_tests_run=False)
write('evidence/crosscheck.json',json.dumps(cross,ensure_ascii=False,indent=2))

tasks=(ROOT/'tasks.md').read_text(encoding='utf-8').replace('- [ ] [page] 实际生成并检查home-v1，保存尺寸和提示词。','- [x] [page] 实际生成home-v1，检查发现背景漂移；定向生成home-v2并实测登记，两版均未批准。')
write('tasks.md',tasks)
for v in [1,2]:
    p=ROOT/f'prompts/pages/home-v{v}.md'
    text=p.read_text(encoding='utf-8')
    if v==1:
        text=text.replace('待实际输出后检查。需要用户批准，不能自动拆组件。','实际生成941×1672 RGB；发现额外远山/建筑，与场景保留要求不符，标记stale，修订见v2。需要用户批准，不能自动拆组件。')
    else:
        text=text.replace('待实际输出。v1保留为未批准探索稿，不能自动变成拆解源。','实际生成941×1672 RGB；已查看全图，上下导航、预览数值、侧榜与人物无覆盖，额外远山已去除。保持近似构图，但不是原截图像素复用。候选待用户批准；未生成组件。v1保留为stale探索稿。')
    write(f'prompts/pages/home-v{v}.md',text)

with (ROOT/'evidence/acceptance.csv').open(encoding='utf-8-sig',newline='') as f:rows=list(csv.reader(f))
for row in rows[1:]:
    if row[0]=='preserve':row[-1]='passed' if preserve['passed'] else 'external-changes-detected'
    elif row[0]=='home-visual':row[-1]='candidate-reviewed-pending-user-approval'
with (ROOT/'evidence/acceptance.csv').open('w',encoding='utf-8-sig',newline='') as f:csv.writer(f).writerows(rows)

write('README.md',f'''# 无极仙道 · 水墨UI设计项目
当前阶段：规范与屏幕地图已形成完整草案；首页候选v2已生成，待用户批准，尚未拆组件。

![首页候选v2](assets/pages/home-v2.png)

## 审阅入口
- [屏幕地图与44屏清单](screen-map.md)：39个v1.4规划屏幕+5个当前源码补充页面；4屏保留，40屏重制设计。
- [视觉规范契约](contracts/style-contract.yaml)：色彩、修仙字体、响应布局、状态、软边与动效。
- [玩法映射](gdd.md)、[需求与验收](prd.md)、[交互及逐屏流程](interaction.md)。
- [保留清单](preservation.md)、[参考分析](research.md)、[后续执行](plan.md)、[任务状态](tasks.md)。
- [首页v2最终提示词](prompts/pages/home-v2.txt)、[实际图像检查](evidence/image-review.json)。
- [交叉检查](evidence/crosscheck.json)、[保留哈希检查](evidence/preservation-check.json)、[校验日志](evidence/validate-project.txt)。

## 本轮范围
保留现有启动菜单、游戏HUD、角色属性和背包。其余页面本轮完成具体规范；按首页批准后的批次逐页生成视觉。没有改运行UI、相机、场景或玩法。
主审候选为home-v2（941×1672），不是运行截图。home-v1背景漂移，已标stale，不供拆解。生图字体仅供视觉审阅；后续动态字使用项目真实字体独立排版。
44份页面brief位于briefs/；契约包含目的、入口、主操作、状态、边界、数据与当前路由别名。旧出诊、抽奖和未开放仙缘已明确处置，不把参考图中的业务自动加进游戏。

## 批准与下一步
若方向合适，回复：`批准首页v2及规范和屏幕地图，开始拆解首页组件。`
该回复才会触发组件生成；本轮组件、切图ZIP、Atlas和引擎导入均未执行。

本轮检查：{len(planning)}屏、{len(source_ids)}个原规划ID全覆盖、当前注册路由均已映射或解释；保留文件{preserve['unchanged']}/{preserve['files_checked']}哈希一致。验证只是设计文档与文件检查，未执行UE/手机/生产平台测试。
''')
quick=(ROOT/'quickstart.md').read_text(encoding='utf-8')
quick=quick.replace('批准首页home-v1','批准首页home-v2')
quick+='''\n## 本轮实际产物与检查
44屏具体brief与屏幕契约；v1.4原39屏全覆盖，补5屏，当前注册路由均有映射或排除理由。最终首页候选home-v2为941×1672 RGB，图片实际无Alpha；v1标stale。保留文件670份已进行哈希对比，结果见preservation-check.json。
内置image_gen两次：首稿和一次定向背景修订，最终提示词分别保存于home-v1.txt/home-v2.txt。图片已复制到assets/pages，不依赖生成缓存。
文档均仍待审。默认校验预计保留“三文档未批准先生成视觉”的1条流程警告，原因是用户本次明确指定连续执行至首页；不篡改批准状态。最终资产包尚未生产，本轮不运行--strict来冒充最终交付。
'''
write('quickstart.md',quick)
print(json.dumps(dict(crosscheck=cross,preservation=preserve,images=[r['dimensions'] for r in review]),ensure_ascii=False))
