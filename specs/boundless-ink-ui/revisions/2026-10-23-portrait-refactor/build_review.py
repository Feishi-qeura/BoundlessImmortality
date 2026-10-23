"""Build a versioned design review and non-executing migration manifest."""
from pathlib import Path
import json, csv, hashlib, re, copy
from collections import Counter
import yaml
from PIL import Image

OUT=Path(__file__).resolve().parent
SPEC=OUT.parents[1]
ROOT=SPEC.parents[1]
def write(name,text): (OUT/name).write_text(text,encoding='utf-8')
def dump(name,data): write(name,json.dumps(data,ensure_ascii=False,indent=2))
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
audit=json.loads((OUT/'registry-audit.json').read_text('utf-8'))
texts=[]
for folder in ['Source','Config','Plugins','Tools']:
    for p in (ROOT/folder).rglob('*'):
        if p.suffix.lower() in {'.cpp','.h','.cs','.ini','.py','.json','.uplugin'} and not any(x in p.parts for x in ['Intermediate','Binaries','__pycache__']):
            try: texts.append((p.relative_to(ROOT).as_posix(),p.read_text('utf-8-sig')))
            except (UnicodeError,OSError): pass
types={'Texture2D':'Textures','Material':'Materials','MaterialInstanceConstant':'MaterialInstances','NiagaraSystem':'Niagara','WidgetBlueprint':'Widgets','Font':'Fonts','FontFace':'Fonts','InkUiThemeData':'Data'}
def family(pkg):
    n=pkg.split('/')[-1].lower()
    if any(x in n for x in ['character','profile','rating','dantian']):return 'Character'
    if any(x in n for x in ['rank','leaderboard','prestige']):return 'Leaderboard'
    if any(x in n for x in ['inventory','item','bag']):return 'Inventory'
    if any(x in n for x in ['signin','chest','fortune','reward']):return 'Rewards'
    if any(x in n for x in ['home','navigation','avatar','meter','namecard']):return 'HUD'
    if any(x in n for x in ['gift','wand','bomb']):return 'Gifts'
    if any(x in n for x in ['cultivation','meditation']):return 'Cultivation'
    if 'menu' in n:return 'Menu'
    return 'Shared'
rows=[]
destinations=set()
for a in sorted(audit['assets'],key=lambda a:a['package']):
    pkg=a['package'];name=pkg.rsplit('/',1)[-1];folder=types.get(a['asset_class'],'Other')
    dest=f'/Game/UI/Ink/{folder}/{family(pkg)}/{name}'
    if dest in destinations:dest=f'/Game/UI/Ink/{folder}/{family(pkg)}/Origin_{hashlib.sha256(pkg.encode()).hexdigest()[:8]}/{name}'
    destinations.add(dest)
    file=ROOT/'Content'/Path(pkg.removeprefix('/Game/')).with_suffix('.uasset')
    parent=pkg.rsplit('/',1)[0]
    refs=[f for f,t in texts if pkg in t or name in t or parent+'/' in t]
    rows.append(dict(old_package=pkg,proposed_package=dest,asset_class=a['asset_class'],family=family(pkg),decision='retain-and-relocate',executed=False,sha256=sha(file) if file.exists() else None,registry_referencers=a['referencers'],dependencies=a['dependencies'],source_or_parent_candidates=refs,classification='heuristic-review-required'))
dump('migration-manifest.json',dict(status='review-only',executed=False,delete_count=0,assets=rows))
with (OUT/'migration-manifest.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['旧路径','建议新路径','类型','分组需复核','决策','执行','源码候选数量','SHA256'])
    for r in rows:w.writerow([r['old_package'],r['proposed_package'],r['asset_class'],r['family'],r['decision'],False,len(r['source_or_parent_candidates']),r['sha256']])
protected=[]
for p in (ROOT/'Source/BoundlessImmortality/Frontend').rglob('*'):
    if p.is_file() and p.suffix in {'.cpp','.h','.inl'}:protected.append(dict(path=p.relative_to(ROOT).as_posix(),sha256=sha(p)))
dump('source-baseline.json',dict(note='Fingerprint for subsequent implementation; no claim of pre-session baseline',files=protected))

# Each page has a specific content arrangement. Existing business rules are inherited.
layouts={
'main':('启动','上部题字与版本；中部原场景人物；下部纵排开始/继续，设置/退出为次操作','左侧原场景，右侧四项导航；继续无档显示原因','开始游戏'),
'home':('HUD','上方身份与入口；中央角色保护区；右侧窄排行入口；底部五项导航','身份左上、活动签到仙途右上、底部五项居中','修炼/收功'),
'saves':('支持','存档槽竖向列表，选中槽展开时间/境界/操作；删除独立危险入口','左槽位列表、右存档详情与操作','读取所选存档'),
'settings':('支持','画质/帧率/声音/减少动态分组；每组单行名称与控件，底部应用','左设置分类、右参数组','应用设置'),
'wuling':('组织','顶部徽记身份；总览/成员/建设/记录分页；成员列表单独滚动','左组织摘要与页签、右当前分页','进入可用组织操作'),
'guild-build':('组织','设施分类；等级与收益；公库材料缺口；权限说明；底部建设确认','左设施列表、右升级前后对比与成本','确认建设'),
'sword-spirit':('组织','个人剑灵身份卡；成长与权益分组；来源/期限逐项列出','左身份、右成长与权益','查看权益来源'),
'gift-rules':('组织','先选择效果类别；列表展示对象、来源、期限和叠加规则','左效果目录、右完整规则','返回剑灵'),
'methods':('构筑','当前主修置顶；功法分类与列表；选中后下方详情；底部装备','左功法列表、右详情和装备槽','装备所选功法'),
'equipment':('构筑','装备/法宝页签；槽位摘要；候选物品对比；底部替换动作','左槽位与候选列表、右前后属性差异','确认装备'),
'breakthrough':('构筑','当前境界→目标；材料与品质Q；缺口来源；底部突破','左材料列表、右品质与结果预期','准备突破'),
'tribulation':('构筑','当前阶段与倒计时；概率组成；助力名单；结果恢复入口','左阶段进度、右助力与概率明细','发起/查看天劫'),
'item-sources':('构筑','目标物品与缺口置顶；免费可达来源优先；每行显示条件','左来源列表、右地点与开放条件','前往来源'),
'craft':('构筑','配方分类；选中配方；材料缺口；数量步进与结果预览','左配方、右材料与炼制设置','确认炼制'),
'appearance':('构筑','名片预览置顶；外观分类与3列网格；底部试用/应用','左实际预览、右外观网格','应用外观'),
'journey':('世界','世界历练大入口；武灵/资源/个人剑灵分组；锁定能力显示原因','左主历练区域、右系统入口2列','世界历练'),
'world':('世界','等比区域卷轴地图；下方地区卡；当前历练进度固定于底部','左等比地图、右地区列表和历练状态','进入区域'),
'region':('世界','区域山水头图；条件与风险；掉落来源；底部进入按钮','左区域图、右条件与掉落','开始历练'),
'world-result':('结算','结果印章；实际收据摘要；奖励清单；底部继续/返回','左结算摘要、右奖励与下一步','继续历练'),
'resources':('竞采','资源分类；单列资源卡含期限/占用/参与条件；下方规则入口','左资源列表、右选中资源概况','查看资源'),
'resource-detail':('竞采','资源头图；资格/配额/期限；得分规则；底部参与','左资源概况、右资格和规则','参与竞采'),
'race-lobby':('竞采','所选资源摘要；匹配状态；队伍与资格；底部取消/转历练','左匹配进度、右队伍和条件','匹配/取消'),
'race-live':('竞采','顶部阶段倒计时；中部双方进度与三类分数；底部允许行动','左现场状态、右分数与行动','执行当前允许行动'),
'race-result':('结算','实际排名与分数；配额结果；奖励收据；后续恢复路径','左结果、右收据与开采权','查看开采权'),
'lease':('竞采','资源与租期大卡；在线积累和受益；剩余条件与历练入口','左租约列表、右受益明细','查看资源收益'),
'resource-rank':('榜单子页','周期与规则置顶；三类得分列表；本人位置固定底部','左得分榜、右本人贡献说明','查看贡献明细'),
'resource-rewards':('结算','租约摘要；实际收益按来源分组；收据状态；底部确认已读','左收据列表、右来源明细','确认已读'),
'offline':('结算','实际离线期间；计算依据；奖励清单；限制说明','左时段与规则、右奖励','确认结果'),
'sign-in':('回流','三页签；7日奖励分2行4+3；今日卡突出；累计轨道与宝箱；固定领取','左7日奖励、右累计进度与详情','领取今日奖励'),
'monthly-sign-in':('回流','月份切换；7列日历；累计节点轨道；底部当日状态','左日历、右累计宝箱与奖励详情','领取可领月签奖励'),
'daily-verse':('回流','剑诀竖签主视觉；释义与真实奖励；下方当天状态','左竖签、右释义与结果','查看今日剑诀'),
'activities':('活动','日常/限时/开服页签；精选一张头图；单列活动卡','左分类、右两列活动卡','进入活动详情'),
'daily-activities':('活动','活跃摘要；按可完成优先的任务列表；领取按钮就近','左任务列表、右选中目标说明','前往/领取'),
'timed-activities':('活动','当前周期与剩余时间；按实际开放排序的活动卡','左活动列表、右期限和规则','查看限时活动'),
'daily-detail':('活动','活动标题与目标；真实进度；奖励与前置条件；底部动作','左目标进度、右奖励与来源','前往/领取'),
'timed-detail':('活动','期限置顶；阶段规则；奖励档位；固定参与动作','左阶段进度、右奖励详情','参与活动'),
'launch':('活动','开服倒计时；阶段页签；免费贡献与奖励；称号期限','左阶段列表、右奖励进度','查看可领取奖励'),
'launch-rank':('榜单子页','周期与贡献规则；排名列表；本人贡献固定底部','左榜单、右本人贡献与称号期限','查看贡献'),
'shop':('经济','分类页签；两列商品；详情抽屉；底部精确总价与确认','左商品3列、右选中详情与结算','确认购买'),
'market':('经济','筛选与搜索；寄售列表；详情；底部精确费用确认','左列表、右物品与费用','确认交易'),
'orders':('经济','进行中/已完成页签；订单状态列表；对账说明','左订单、右收据与处理动作','查看订单'),
}
contract=yaml.safe_load((SPEC/'contracts/screen-contract.yaml').read_text('utf-8'))
preserve={'character','inventory','leaderboard'}
pages=[]
for s in contract['screens']:
    p=copy.deepcopy(s);ident=p['id'];p['approval']={'approved':False};p['revision_status']='layout-review'
    if ident in preserve:
        p['treatment']='preserve-layout-and-behavior-background-only'
        p['proposed_layout']={'family':'保留','portrait':'保持现有控件、槽位、分页、榜单行结构，仅审查背景所有权','landscape':'沿用现有横屏布局；只验证背景无重复','primary':'沿用现有绑定'}
    else:
        fam,portrait,landscape,action=layouts[ident]
        p['treatment']='redesign'
        p['proposed_layout']={'family':fam,'portrait':portrait,'landscape':landscape,'primary':action}
    p['runtime_implemented_this_revision']=False
    pages.append(p)

# Runtime route inventory supplements the older design map rather than silently omitting subpages.
code=(ROOT/'Source/BoundlessImmortality/Frontend/InkFeaturePanel.cpp').read_text('utf-8-sig').split('void UInkFeaturePanel::Configure')[0]
routes=sorted(set(re.findall(r'Route==TEXT\("([^"]+)"\)',code)+re.findall(r'\{TEXT\("([^"]+)"\),UInk',code)))
covered={p.get('route') for p in pages}
for p in pages:covered.update(p.get('current_route_aliases',[]))
extra=[]
for route in routes:
    if route in covered:continue
    if route=='CharacterProfile': action='保留现有角色属性主面板，仅背景审查'
    elif route=='Draw':action='中央等比转盘；奖池/概率可查；次数与成本置底；先权威结果再播放；feature gate不变'
    elif route=='CharacterFate':action='仙缘身份摘要；已有仙缘条目与解锁条件；选中后展开来源与当前效果；横屏列表/详情双栏；沿用原可用动作'
    elif route=='WulingActivity':action='组织身份与活动周期；贡献目标、实际进度与奖励档位；底部当前可执行动作；横屏目标/奖励双栏'
    elif route=='WulingRace':action='组织资格与队伍；选中资源；匹配/竞采状态和三类分数；底部当前允许动作；横屏队伍/阶段双栏'
    elif route=='Expedition':action='保留旧行医出诊预览的开关限制；全部/紧急/常规/个人委托筛选，竖屏委托卡列表，横屏列表/详情双栏；不把其重新开放到正式武灵导航'
    elif route=='ExpeditionDetail':action='委托标题、紧急程度与条件；队伍槽及奖励；底部出诊/返回；横屏委托/队伍双栏；仍属于旧预览模式'
    else:action='顶部目标摘要；中部步骤与条件；下方结果/来源；固定主操作'
    extra.append({'route':route,'layout':action,'status':'layout-review','runtime_implemented':False})
dump('runtime-route-coverage.json',dict(runtime_routes=routes,covered_by_existing_contract=sorted(set(routes)&covered),supplemental_routes=extra,unaccounted=[]))
candidate={'schema_version':1,'project_id':'boundless-ink-ui','status':'draft','revision':'2026-10-23','supersedes_scope_only':True,'preserved':sorted(preserve),'screens':pages,'supplemental_runtime_routes':extra}
write('screen-contract-proposed.yaml',yaml.safe_dump(candidate,allow_unicode=True,sort_keys=False))
lines=['# 逐页布局审阅清单','', '状态：draft。旧玩法/数据/feature gate沿用；本文件的新布局未实施。角色属性、背包、主排行榜保留；主菜单和HUD纳入重设计。','', '| 页面 | 处理 | 竖屏布局 | 横屏重排 | 主操作 |','|---|---|---|---|---|']
for p in pages:
    l=p['proposed_layout'];lines.append(f"| {p['name']} `{p['id']}` | {'保留，仅背景' if p['id'] in preserve else '重设计'} | {l['portrait']} | {l['landscape']} | {l['primary']} |")
lines+=['','## 补充：源码存在、旧屏幕地图未独立列出的路由','','| 路由 | 布局与边界 |','|---|---|']
for p in extra:lines.append(f"| {p['route']} | {p['layout']} |")
write('screen-layouts.md','\n'.join(lines)+'\n')
style=yaml.safe_load((SPEC/'contracts/style-contract.yaml').read_text('utf-8'))
style['status']='draft';style['revision']='2026-10-23-portrait-refactor'
style['consistency_rules']=[x for x in style.get('consistency_rules',[]) if '先保留当前主菜单' not in x]
style['consistency_rules'].append('本轮保留角色属性、背包、排行榜；其他页面重设计。旧批准仅作为玩法和字体来源，不表示新布局已批准。')
style['surface_ownership']={'hud':'No fullscreen paper; localized ink backing only','standalone':'one backdrop + one paper surface','embedded':'no backdrop or paper; parent owns surface','preserved':'page-specific art owns paper; shared shell must not add another full paper','modal':'one dim and one dialog paper; do not dim each ancestor'}
style['adaptive_rules']={'logical_reference':[720,1280],'portrait_below_ratio':0.8,'landscape_from_ratio':1.25,'page_margin':24,'header_height':88,'footer_height':104,'min_touch':[88,88],'body':'remaining height; scroll only the body','wide':'two columns only when both remain readable','ultratall':'grow content/scene; do not stretch decoration','font_min_physical_px':14}
write('style-contract-proposed.yaml',yaml.safe_dump(style,allow_unicode=True,sort_keys=False))
images=[]
for name in ['home-v3-exploration.png','home-v4-review.png']:
    p=OUT/name
    if p.exists():
        with Image.open(p) as im:images.append(dict(path=name,dimensions=list(im.size),mode=im.mode,sha256=sha(p),approved=False,status='exploration-environment-drift' if 'v3' in name else 'review-only',source_tool='built-in-image_gen',runtime_screenshot=False))
dump('review-assets.json',dict(assets=images))
dump('review-validation.json',dict(registry_asset_count=len(rows),registry_classes=audit['classes'],migration_rows=len(rows),all_assets_hashed=all(r['sha256'] for r in rows),unique_destination_count=len(destinations),protected_pages=sorted(preserve),planned_screen_count=len(pages),runtime_routes=len(routes),supplemental_routes=len(extra),all_runtime_routes_accounted=True,layout_coverage_complete=set(layouts)==({p['id'] for p in pages}-preserve),asset_mutations_executed=False,native_build_run=False,pie_run=False,cook_run=False,device_test_run=False))
print(json.dumps(dict(assets=len(rows),pages=len(pages),runtime_routes=len(routes),supplemental=len(extra),images=len(images)),ensure_ascii=False))
