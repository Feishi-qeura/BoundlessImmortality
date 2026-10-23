from pathlib import Path
import csv, json, hashlib, shutil, sys
import yaml
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT.parents[1]
DESIGN = GAME / 'docs/design/2026-10-21-wuling-resource-v1.4'
PID = ROOT.name

def write(name, text):
    p = ROOT / name
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text.strip() + '\n', encoding='utf-8')

def dump(name, obj):
    write(name, yaml.safe_dump(obj, allow_unicode=True, sort_keys=False, width=110))

def header(**kw):
    return dict(schema_version=1, project_id=PID, **kw)

# This authoring script is confined to this newly created design project.
for d in ['evidence', 'references', 'briefs', 'prompts/pages', 'assets/pages']:
    (ROOT / d).mkdir(parents=True, exist_ok=True)

refdir = Path(r'C:\Users\zxxuh\Desktop\UI资源\3')
refs = []
for i, p in enumerate(sorted(refdir.iterdir()), 1):
    if p.suffix.lower() not in {'.png', '.jpg', '.jpeg', '.webp'}:
        continue
    dest = ROOT / 'references' / f'reference-{i:02d}{p.suffix.lower()}'
    shutil.copy2(p, dest)
    with Image.open(dest) as im:
        size, mode = list(im.size), im.mode
    refs.append(dict(source=str(p), copy=dest.relative_to(ROOT).as_posix(), dimensions=size,
                     mode=mode, sha256=hashlib.sha256(p.read_bytes()).hexdigest(), inspected=True))
write('evidence/references.json', json.dumps(refs, ensure_ascii=False, indent=2))

baseline = {
 'home': 'Saved/Codex/VisualRefactor20261022/Rendered/hud-720x1280.png',
 'main': 'Saved/Codex/NiagaraUiRevision20261022/RenderedVerified/main-provided-normal.png',
 'character': 'Saved/Codex/CharacterRating20261022/PIE/final/portrait.png',
 'inventory': 'Saved/Codex/VisualRefactor20261022/Rendered/Inventory-720x1280.png',
 'home-landscape': 'Saved/Codex/SignInRank20261022/PIE/hud-entry.png',
}
for sid, rel in baseline.items():
    shutil.copy2(GAME / rel, ROOT / 'references' / f'current-{sid}.png')
write('evidence/baseline-screens.json', json.dumps(baseline, ensure_ascii=False, indent=2))

protected = []
for base in ['Source/BoundlessImmortality/Frontend', 'Content/UI/Ink', 'SourceAssets/UI']:
    for p in sorted((GAME / base).rglob('*')):
        if p.is_file():
            protected.append(dict(path=p.relative_to(GAME).as_posix(), size=p.stat().st_size,
                                  sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
write('evidence/preservation-baseline.json', json.dumps(protected, ensure_ascii=False, indent=2))

source_rows = list(csv.DictReader((DESIGN / 'tables/ui-screens.csv').open(encoding='utf-8-sig')))
ids = {'UI_MAIN':'main', 'UI_HUD':'home', 'UI_CHARACTER':'character', 'UI_INVENTORY':'inventory',
       'UI_RANK':'leaderboard', 'UI_SAVES':'saves', 'UI_SETTINGS':'settings', 'UI_WULING':'wuling',
       'UI_SWORD':'sword-spirit', 'UI_GIFT_RULES':'gift-rules', 'UI_METHODS':'methods',
       'UI_RACE_LOBBY':'race-lobby', 'UI_RACE_LIVE':'race-live', 'UI_RACE_RESULT':'race-result',
       'UI_LEASE':'lease', 'UI_ROUND_RESULT':'world-result', 'UI_OFFLINE':'offline',
       'UI_SHOP':'shop', 'UI_MARKET':'market', 'UI_ORDERS':'orders', 'UI_WORLD':'world',
       'UI_REGION':'region', 'UI_RESOURCES':'resources', 'UI_RESOURCE_DETAIL':'resource-detail',
       'UI_SIGN':'sign-in', 'UI_MONTH':'monthly-sign-in', 'UI_VERSE':'daily-verse',
       'UI_ACTIVITIES':'activities', 'UI_DAILY':'daily-activities', 'UI_LIMITED':'timed-activities',
       'UI_ACTIVITY_DETAIL':'daily-detail', 'UI_TIMED_DETAIL':'timed-detail', 'UI_LAUNCH':'launch',
       'UI_LAUNCH_RANK':'launch-rank', 'UI_BREAK_PREPARE':'breakthrough', 'UI_TRIBULATION':'tribulation',
       'UI_ITEM_SOURCES':'item-sources', 'UI_GUILD_BUILD':'guild-build', 'UI_EQUIPMENT':'equipment'}

# Per-page content, action and boundary. These are design contracts, not claims of implementation.
details = {
'main': ('保留启动页的水墨题字、人物与按钮构图', '开始、继续、存档、设置、退出；沿用当前可达路径', '存档为空、继续不可用、退出确认；不为参考图增设选服登录', 'FrontendVm', '进入HUD', '保留现有启动页；导航清单见当前路由，缺项只登记'),
'home': ('保留游戏首页HUD与相机，作为新页面的风格衔接基准', '左上身份、血量、修为；右上活动/签到/仙途；中央场景；底部角色/背包/修炼/排行/菜单；侧边查看排行', '无目标则隐藏目标；断线不加收益；修炼/收功使用实际状态', 'AnchorVm|WorldVm|LeaseVm', '修炼或收功', '不得改镜头、山石、人物尺寸和导航顺序；概念图不是运行截图'),
'character': ('保留当前丹田与功法装备槽及属性评级面板', '功法行、丹田、六侧槽、下方已配槽与实际属性；保持当前品质框和减弱动态选项', '空槽、锁槽、长属性、未配置；不套用参考图旧立绘属性卡或虚构总战力', 'AnchorVm|MethodVm|EquipmentVm', '查看当前属性及进入获准子页', '保留当前角色评级系统，不恢复旧稿；用户保留优先'),
'inventory': ('保留背包全套UI、模型与操作', '分类/搜索/列表与网格/新标/数量/滚动/多选/分页/详情/出售分解整理', '200物理槽、默认20解锁、40每页；hover蓝、selected金；绑定到期托管动作权限', 'InventoryVm', '选物品查看允许动作', '不改格数、材质、框架；新增来源页从允许的详情入口进入'),
'leaderboard': ('重制榜单外壳与阅读层级', '本房/全服、剑灵数/修炼速度/伤害；窄榜8行、我的位置、规则与周期', '未上榜、空榜、头像失败、周期切换、并列；不提供假好友榜', 'LeaderboardVm', '切换榜单', '保留持久行/头像更新逻辑与8行边界；重制视觉不能造成头像闪烁'),
'saves': ('重制存档列表，明确本地范围', '槽位、时间、模式、摘要、覆盖或载入', '空槽、坏档、版本不兼容、覆盖和删除二次确认', 'SaveVm', '载入或保存', '不可把本地存档覆盖在线成长或服务账本'),
'settings': ('重制设置页，降低调整成本', '声音/图像/控制/文字可读性/减弱动态；仅列现有能力', '应用失败恢复原值；未保存退出提示；未知能力不绘可用开关', 'SettingsVm', '应用设置', '键鼠与触控焦点可达；返回不中断待对账事务'),
'wuling': ('重制武灵组织总览与名册', '总览/成员/建设/记录四子栏；正式、参与、临时人数分开；公库与当前租约', '未创建、名称2–12等效汉字、审核驳回、改名冷却；成员20一页', 'GuildVm|WorldVm', '创建或管理武灵', '武灵是主播创建的组织；移除与解散说明个人权益保留'),
'sword-spirit': ('重制个人剑灵权益页', '查看身份、归属、境界修为、称号、已解锁剑形、有效增益与来源', '未加入、未解锁、到期、其他房间范围、非本人权限', 'SwordVm|GiftEffectVm', '查看个人剑灵与选择称号', '观众无生命防御死亡栏；解锁剑形不增加观众人数'),
'gift-rules': ('重制效果规则说明页', '来源/对象/原期限/到期/叠加/当前模式；旧权益与候选规则分列', '测试/正式、过期、暂不适用、断线未确认', 'GiftEffectVm', '查看效果说明', '送礼跳官方路径；说明点击不购买，不生成付费加概率入口'),
'methods': ('重制功法构筑页', '七路线、1主修、当前辅修槽、锁定条件、主辅差异和草稿预览', '事件中禁换并显示原因；不足材料跳世界来源；确认失败留旧配置', 'MethodVm', '确认准备期配置', '第3/6级强化从规则表读取；不在UI推导另一倍率'),
'world': ('重制世界历练主页', '首发三区与后续锁区、独行/小/中/大预设、路线、10分钟阶段、免费固定供给', '准备/进行/30秒恢复/结算中/完成/中断；运行中锁预设', 'WorldVm|AnchorVm', '选择路线并开始历练', '非送礼补给入口可达；同一轮在武灵与世界共用round_id'),
'region': ('重制区域详情', '一幅等比区域山水、难度、阶段、固定奖励、开放条件与参与说明', '境界锁定、事件互斥、未开放、奖励未同步', 'WorldVm', '按条件进入历练', '锁定区域保留条件说明，不画虚假的开始按钮'),
'world-result': ('重制历练结算', '基础阶段、个人修为、个人贡献、公库、额外核心分别列归属和收据', '中断保留已完成阶段；待对账/已到账/已展示', 'RewardVm', '确认展示或再去历练', '确认只标已读，不能再次发奖励；满包转待入账收据'),
'resources': ('重制五类资源选择', '灵泉/灵石矿/灵草园/玄铁脉/残卷遗址；对象、产出、间隔、租期、当前开采权', '未开放、已有租约、正式成员不足、网络断开', 'LeaseVm|RaceVm', '查看资源并报名', '资源是临时配额；不渲染为永久独占矿点'),
'resource-detail': ('重制资源规则和报名说明', '收益对象、单位产出、统一竞采规则、人数档、资格', '正式成员不足、活跃租约、profile关闭', 'LeaseVm|RaceVm', '进入竞采匹配', '说明世界历练替代路线；不把礼物额作为分数'),
'race-lobby': ('重制匹配大厅', '资源、人数档、2–4真实武灵、等待时间、统一规则和取消', '60秒无对手转世界历练；取消与匹配成功竞态按服务结果', 'RaceVm|GuildVm', '报名或取消', '没有真实对手只留空位，不画假姓名或头像'),
'race-live': ('重制竞采现场轻量HUD', '120秒准备/360秒挑战/120秒结算；600共同/200主播/200成员；最多10冻结代表', '未响应/有效响应/断线/阶段切换；礼物表现与计分分开', 'RaceVm', '响应当前阶段', '中央保护角色；准确分母；直播观看者通过允许指令响应'),
'race-result': ('重制竞采结算', '总分与三项来源、600资格线、并列组、30/20/10分钟租期', '未达标原因、并列、对账中、租约已开始', 'RaceVm|RewardVm', '查看开采权或回世界', '第600秒租约自动开始；确认不延后计时'),
'lease': ('重制当前开采权', '服务器start/end、剩余时间、受益身份、在线积累、已入账和下一档', '到期/断线暂停更新/无租约/跨房不适用', 'LeaseVm', '查看收益来源并回历练', '离线不自行计数；允许并列配额，不假装唯一控制者'),
'sign-in': ('重制签到组及七日页', '七日宝箱、今日详情、累计轨道、已签形态、固定CTA；月历和每日剑诀二级页', '可签/请求中/已签/跨日；宝箱先详情后领取；未达和断线可看明细', 'ActivityVm', '查看奖励并签到', '沿用现有服务账本；每日300修为依当前规则，动画不触发发奖'),
'monthly-sign-in': ('重制月签日历', '服务器月份、七列日历、今天/已签/未到、3/7/14/21累计宝箱和数量', '跨月、月末、节点未达/可领/已领、断线；节点后轨道与箱中心对齐', 'ActivityVm', '选日或查看累计奖励', '不能沿用参考图/旧稿7/14/21/28；复用现有已领取账本'),
'daily-verse': ('重制每日剑诀', '卷轴诗签、固定轮换剑诀与行动提示、对应免费玩法入口', '已读/未读、固定轮换未知、无网络', 'ActivityVm', '查看剑诀并去行动', '不做随机吉凶、改签付费；不展示抽奖入口'),
'activities': ('重制活动总入口', '日常/限时/开服；首屏一个主题头图、目标卡、实际时间和状态', '空活动、未开、进行、已结束、可领取、feature关闭', 'ActivityVm|LaunchVm', '选择活动', '不用两张大海报撑空白；红点仅真实未读/可领'),
'daily-activities': ('重制日常活动列表', '实际目标、当前/需求、预算、固定奖励、前往和已领取', '未解锁/进行/达成/待对账/已领/周期切换', 'ActivityVm', '前往或查看奖励', '不能以页面打开产生进度；领取沿用原request_id'),
'timed-activities': ('重制限时活动列表', '等比主题卡、start/end与Asia/Hong_Kong时区、免费参与与期限', '未开始/进行/已结束/数据未知', 'ActivityVm', '查看限时活动', '没有服务起止时间就显示待同步，不生成倒计时'),
'daily-detail': ('重制日常详情', '主题头图、条件、任务、奖励归属、预算、主操作', '预算耗尽/条件不足/已完成/断线', 'ActivityVm|RewardVm', '参与允许活动', '标明是否替换基础预算；不足给来源入口'),
'timed-detail': ('重制限时详情', '剩余时间、真实规则、固定产出、截止、有效期与入口', '未开/过期/满包/处理中/活动版本变化', 'ActivityVm|RewardVm', '参与或确认结果', '不新增随机抽奖或未规划付费档位'),
'launch': ('重制青云初启', '总览/个人任务/武灵目标/规则四内栏、本人报名、七任务、1/3/6/12轮次、称号到期', '未报名/进行/达成/自动入账/过期/profile禁用', 'LaunchVm|RewardVm', '查看目标或确认展示', '四内栏不重复压路由；发布候选隐藏并拒绝直达'),
'launch-rank': ('重制开服贡献榜', '免费贡献四来源、轮数、资格、并列名次和荣誉', '未达资格/并列/周期结算/未上榜/禁用', 'LaunchVm', '查看贡献明细', '不展示消费排名或已关闭的修为奖励'),
'breakthrough': ('重制突破准备', '四类材料实例槽、数量/替换、Q评分来源、确定品质、失败扣修与保底规则', '不足材料跳来源；准备取消；与其他事件互斥；预留过期', 'BreakthroughVm|InventoryVm', '核对成本后确认突破', '确定Q与天劫成功率分开展示；v1.4覆盖旧突破成本语义'),
'tribulation': ('重制渡劫现场', '15秒准备、60秒三重天劫、8秒揭示；不同助力人数、基础/助力/保底/最终概率', '准备/天劫/冻结/提交/对账/成功/失败/清理；已助力和截止可见', 'TribulationVm', '观众发送助力；主播查看结果', '50%基础+每唯一助力1百分点(最多50)+每失败10百分点(最多50)，封顶100%；失败扣1500修为保留材料和境界'),
'item-sources': ('重制物品来源追踪', '材料分型品质、具体区域/副本、专注进度、配方与共享额度', '未知物品/区域锁定/额度耗尽/可制作/版本变更', 'ItemSourceVm', '选择免费来源并前往', '来源沿v1.4表；不得导入旧表叠加奖励；回背包保留选中项'),
'guild-build': ('重制武灵建设', '公库余额、建设项目、材料、级别、成本、外显预览、版本与记录', '权限不足/材料不足/提交中/版本冲突/已完成', 'GuildBuildVm', '预览并原子提交建设', '个人与公库分开；v1.4建设表覆盖旧100资材示意，不生造永久倍率'),
'equipment': ('重制装备法宝子页', '主播四个非武器装备槽和一个法宝槽、属性上限、归属、替换前后', '安全准备期/事件冻结/锁定/冷却/绑定/过期', 'EquipmentVm|InventoryVm', '在准备期确认配置', '不重画被保留角色页；切装不重置法宝冷却'),
'offline': ('重制离线收益页', '上次结算、本次有效时长、8小时上限、基础10%、修为结果、原收据', 'profile禁用/无收益/待对账/已入账', 'RewardVm', '确认已入账展示', '不受灵脉礼物放大；确认不重复领取；仅获准profile显示'),
'shop': ('重制原型商店', 'SKU、成本、绑定材料、产物、限购、唯一窗口、期限、数量与总价', '低余额/售罄/旧价过期/事务中/feature禁用', 'EconomyVm|InventoryVm', '核对完整成本后制作或购买', '仅prototype_full；发布候选入口隐藏且直达请求拒绝'),
'market': ('重制原型寄售', '白名单原料、单价区间、数量、税费、总价、到期、托管', '旧价失效/订单已售/背包满/超白名单/禁用', 'EconomyVm|InventoryVm', '整单买入或挂牌', '完整费用二次确认；无服务不假扣币或假成交'),
'orders': ('重制我的订单', '挂牌/成交/取消/到期/应收；时间、数量、手续费与唯一收据', '空订单/撤单竞态/应收待对账/禁用', 'EconomyVm', '撤单或查看应收', '按订单版本原子请求；不能因关闭重开发一次应收'),
}

states = ['default', 'loading', 'ready', 'empty', 'locked', 'pending', 'disconnected', 'error', 'expired', 'feature-disabled']
screens = []
for r in source_rows:
    sid = ids[r['screen_id']]
    purpose, content, edge, vm, action, constraint = details[sid]
    parent = ids.get(r['parent'], 'home') if sid not in {'main', 'home'} else ('main' if sid == 'home' else None)
    if sid in {'world', 'resources', 'wuling', 'sword-spirit', 'methods'}:
        parent = 'journey'
    preserved = sid in {'main','home','character','inventory'}
    screen = dict(id=sid, legacy_screen_id=r['screen_id'], name=r['title'], category='core' if r['phase']=='UI-P1' else 'system',
        priority='optional' if sid in {'offline','shop','market','orders'} else 'must-have',
        status='preserved' if preserved else 'specified', purpose=purpose, entry_points=[parent] if parent else ['application-start'],
        primary_actions=[action], dependencies=[parent] if parent else [],
        route=r['route'], current_status=r['current_status'], route_verification='planning-csv; inspect runtime alias before integration',
        delivery='preserve-runtime' if preserved else 'redesign-planned',
        profile_gate=r['feature_gate'], source_phase=r['phase'],
        layout=dict(top=['标题、返回、作用对象与真实状态'], center=[content], bottom=['固定主操作与禁用理由'], floating=['必要详情与确认；装饰不接收点击']),
        responsive=dict(portrait='720×1280单列列表→详情，标题与CTA固定，中段独立滚动', landscape='1280×720列表+详情；常规页右侧抽屉，密集页居中宽页'),
        components=[], component_planning=['功能页外壳','带理由按钮','状态条','按业务选择奖励/成员/来源块'],
        states=states, edge_cases=edge.split('；'), data_needs=vm.split('|'), rules=[constraint],
        output=dict(brief=f'briefs/{sid}.md', prompt='prompts/pages/home-v1.md' if sid=='home' else '', image=''),
        approval=dict(approved=False, reason='用户仅指定保留范围，未批准本轮新视觉'))
    screens.append(screen)

# The current project has a real 仙途 hub, omitted by the legacy UI CSV.
screens.append(dict(id='journey', name='仙途菜单', category='core', priority='must-have', status='specified', purpose='重制系统总入口且保持现有导航位置',
    entry_points=['home'], primary_actions=['进入世界、武灵或资源系统'], dependencies=['home'], route='Extensions',
    current_status='source-present; InkFeaturePanels.cpp lists six routes', delivery='redesign-planned', profile_gate='base',
    layout=dict(top=['仙途标题与返回'],center=['世界历练/武灵/我的剑灵/功法/资源五组入口，按能力隐藏；保留现有签到活动角色捷径'],bottom=['返回HUD'],floating=[]),
    responsive=dict(portrait='高频入口上置，纵向滚动',landscape='两列入口与主题山水，真实文字独立'),
    components=[], component_planning=['软边入口卡','状态条'], states=states,
    edge_cases=['开关关闭不显示可用入口','世界事件进行中展示当前状态'],data_needs=['WorldVm','GuildVm'],rules=['不在HUD新增第二排入口'],
    output=dict(brief='briefs/journey.md',prompt='',image=''),approval=dict(approved=False)))

extra_pages = [
    ('craft','物品炼制','WulingCraft','inventory','RecipeVm|InventoryVm','配方、材料分型与品质、共享额度、产物、缺口与来源；完整成本确认后原子制作','缺料、额度耗尽、版本冲突、满包待入账、重复提交','item_extension'),
    ('appearance','名片外观','CharacterAppearance','character','AppearanceVm|GiftEffectVm','已获头像框与称号、等比试戴、来源、期限和应用；未获只预览','未解锁、过期、当前规则不适用；不得试戴即取得权益','base'),
    ('resource-rank','资源贡献榜','ResourceRanking','resources','RaceVm','周期、规则、三类分数、并列与自己位置；与竞采结算同一数据','旧周期、对账中、未上榜、feature关闭','cross_host_race'),
    ('resource-rewards','资源奖励明细','ResourceRewards','resource-detail','LeaseVm|RewardVm','租期、实际受益、在线积累、收据与来源明细；确认只已读','无租约、过期、断线、未达资格','cross_host_race'),
]
for sid,name,route,parent,vm,content,edge,gate in extra_pages:
    screens.append(dict(id=sid,name=name,category='system',priority='must-have',status='specified',purpose='重制'+name,
        entry_points=[parent],primary_actions=['查看条件后确认允许动作'],dependencies=[parent],route=route,
        current_status='source-present-not-runtime-tested-this-run',delivery='redesign-planned',profile_gate=gate,
        layout=dict(top=['标题、返回与当前身份'],center=[content],bottom=['固定主操作与禁用原因'],floating=['详情与必要确认']),
        responsive=dict(portrait='单列内容滚动，主操作固定',landscape='列表与详情双栏'),components=[],component_planning=['功能页外壳','状态条','带理由按钮'],
        states=states,edge_cases=[edge],data_needs=vm.split('|'),rules=['当前接口能力优先，不画假成功；不覆盖保留页'],
        output=dict(brief=f'briefs/{sid}.md',prompt='',image=''),approval=dict(approved=False)))

aliases={
 'main':['Main'], 'home':['HUD'], 'character':['Character','CharacterProfile'], 'inventory':['Inventory'],
 'leaderboard':['Leaderboard'], 'saves':['Saves'], 'settings':['Settings'], 'journey':['Extensions'],
 'wuling':['Wuling','WulingGuild'], 'sword-spirit':['SwordSpirit','WulingRights'],
 'gift-rules':['GiftRules'], 'methods':['CultivationMethods','CharacterSkills','WulingEquipment|methods'],
 'equipment':['EquipmentFabao','CharacterEquipment','CharacterArtifacts','WulingEquipment'],
 'breakthrough':['BreakthroughPrepare','WulingBreakthrough'], 'tribulation':['Tribulation','WulingBreakthrough|trial'],
 'item-sources':['ItemSources','WulingSources'], 'guild-build':['GuildConstruction','WulingConstruction'],
 'shop':['Shop','WulingShop'], 'market':['Market','WulingMarket'], 'orders':['TradeOrders','WulingOrders'],
 'offline':['OfflineRewards','WulingOffline'], 'race-lobby':['RaceLobby','WulingRace|lobby'],
 'race-live':['RaceLive','WulingRace|live'], 'race-result':['RaceSettlement','WulingRace|result'],
 'lease':['ResourceLease','WulingRace|lease'], 'launch':['LaunchEvent','WulingActivity|launch'],
}
route_audit=[]
for s in screens:
    s['current_route_aliases']=aliases.get(s['id'],[s['route']])
    route_audit.extend(dict(route=a,screen_id=s['id'],disposition=s['delivery'],notes='语义映射；不是已创建的新原生类') for a in s['current_route_aliases'])
route_audit.extend([
 dict(route='WulingActivity',screen_id='activities',disposition='merge',notes='现有签到与开服合并承载路由；保留事务，再分视图设计'),
 dict(route='CharacterFate',screen_id=None,disposition='excluded-feature-disabled',notes='当前规则未开放额外仙缘配置，不新增获得或付费系统'),
 dict(route='Draw',screen_id=None,disposition='excluded-from-formal-navigation',notes='旧抽奖开发预览；现有v1.4正式业务导航明确排除'),
 dict(route='Expedition',screen_id=None,disposition='excluded-from-formal-navigation',notes='旧行医出诊，与项目当前核心玩法不符；不能误作仙途主入口'),
 dict(route='ExpeditionDetail',screen_id=None,disposition='excluded-from-formal-navigation',notes='旧出诊详情同上，保留开发资产不删除'),
])
write('evidence/route-audit.json',json.dumps(route_audit,ensure_ascii=False,indent=2))
for screen in screens:
    sid=screen['id']
    if sid=='character':
        screen['runtime_route_note']='当前主要属性面板为CharacterProfile；Character为旧表入口名，保留二者现有路由映射。'
    if sid=='home':
        screen['layout']=dict(top=['左上主播头像、血量与修为条；右上活动/签到/仙途'],center=['原场景与角色，侧边现有查看排行；无虚假任务'],bottom=['角色/背包/修炼/排行/菜单'],floating=['仅确认过的短播报；最多3条'])
        screen['responsive']=dict(portrait='保留现有720×1280 HUD槽位，概念只作风格对照',landscape='保留左上/右上锚点与底中五导航，不整体拉伸')

dump('contracts/screen-contract.yaml', header(status='draft', screens=screens))
dump('contracts/component-contract.yaml', header(source_screen_id='home', source_screen_status='pending-approval',
     status='deferred-until-home-approval', components=[], notes=['首页未批准；未拆解、未生组件、未指定九宫格数值']))
dump('contracts/asset-manifest.yaml', header(assets=[]))
sprite=yaml.safe_load((ROOT/'contracts/sprite-contract.yaml').read_text(encoding='utf-8'))
sprite['review']['notes']=['阶段未执行；无组件合图、无切图、无PNG ZIP。']
dump('contracts/sprite-contract.yaml',sprite)
atlas=yaml.safe_load((ROOT/'contracts/atlas-contract.yaml').read_text(encoding='utf-8'))
atlas['review']['notes']=['阶段未执行；需要首页批准、组件审核、实测9-slice后才能打包。']
dump('contracts/atlas-contract.yaml',atlas)
export=yaml.safe_load((ROOT/'contracts/export-contract.yaml').read_text(encoding='utf-8'))
export['targets']=[dict(engine='generic',format='json-manifest',manifest_path='exports/generic/ui-main.json',native_project_files=False)]
export['review']['notes']=['UE5.8只规划通用JSON交接；未生成UMG、uasset或原生工程。']
dump('contracts/export-contract.yaml',export)

style=header(status='draft', platform=dict(target='UE5.8主播端；移动比例作为设计适配目标',orientation='responsive',aspect_ratio='9:16',
    reference_size=[720,1280],alternate_size=[1280,720],safe_area=dict(top=24,right=24,bottom=24,left=24),
    safe_area_rule='根SafeZone处理设备切口后再加24逻辑单位；此处不是硬编码系统刘海',
    dpi='ShortestSide：360=.5,540=.75,720=1,1080=1.5；不叠加整页ScaleBox',
    breakpoints=dict(portrait='w/h<0.8',compact='0.8<=w/h<1.25',landscape='w/h>=1.25')),
    visual=dict(theme='修仙水墨传奇；宣纸承字、墨山融边、旧金点睛',keywords=['宣纸','留白','墨山','云气','旧金','青玉','书法'],mood='清远、庄重、可读',complexity='medium',
        materials=['低对比宣纸','干湿墨迹','细旧金边','玉石点缀'],lighting=dict(direction='top-left',softness='soft'),
        page_families=dict(rewards='小山水头图+奖励与累计节点',world='等比路线图+阶段详情',organization='组织徽记+总览与名册',competition='阶段与分数主导',economy='商品网格/订单表+精确费用'),
        decoration='普通功能页金色静态覆盖建议<=5%；边框最多两层；装饰图与动态字分层'),
    colors=dict(primary='#202823',secondary='#3D5861',accent='#B88B45',background='#F3EBDD',warning='#AA4937',reward='#E9CA84',disabled='#8D8C82',paper_light='#FBF7EE',jade='#3A8D83',hover='#4A91AF'),
    typography=dict(language='zh-CN',title=dict(family='MaShanZheng',size=44,weight=400),section=dict(family='LXGWWenKai',size=32,weight=400),body=dict(family='LXGWWenKai',size=28,weight=400),
        number=dict(family='LXGWWenKai',size=28,weight=400),button=dict(family='LXGWWenKai',size=30,weight=400),hud=dict(family='LXGWWenKai',size=26,weight=400),
        font_sources='docs/design/2026-10-21-ink-ui-standard-v1/fonts；中文数字标点一致，Noto仅缺字回退',
        contrast='正文4.5:1；360短边目标正文14px、关键数字13px；不以缩小整页通过溢出验收',
        dynamic_text='动态值必须TextBlock；生图文字只是预览，使用时重新排版'),
    geometry=dict(radius=dict(small=4,card=8,modal=12),stroke=dict(thin=1,regular=2,strong=3),button_height=dict(small=64,primary=72),icon_size=dict(utility=56,navigation=80,currency=32),
        touch_min=[88,88],paper_shell='有机墨边不能被普通圆角矩形替代',slice='边框可伸；人物/题字/图标/山水等比Fit；9-slice待实际资产测量'),
    effects=dict(shadow='墨色低透明软影；正文用稳定阅读底',highlight='仅当前主动作或可领取焦点，禁止全屏闪金',selected='金线+选中标记',disabled='降饱和+原因文字，保持可读',
        motion='hover120ms、切页220ms、弹层180ms；减少动态关闭循环粒子并保留静态状态'),
    tokens=dict(color={'action-primary':'#B88B45','action-danger':'#AA4937','surface-panel':'#F3EBDD','text-primary':'#202823','interaction-hover':'#4A91AF'},
        spacing=dict(xs=4,sm=8,md=16,lg=24,xl=32,xxl=48),motion={'fast-ms':120,'normal-ms':220}),
    control_states=dict(normal='清晰轮廓',hover='蓝墨微染，仅桌面',pressed='下沉1–2单位',focused='细线焦点框',selected='金色并保持',disabled='可读且有原因',pending='仅锁该提交，保留返回',claimable='金色小光环',claimed='开箱形态+已领取字',locked='锁符+开放条件'),
    consistency_rules=['先保留当前主菜单、HUD、角色与背包；本规范不得自动覆盖保留页。','所有新页共享纸色、字体、左上柔光与图标视角；不把所有卡片涂金。',
        '中央保护区只约束HUD原态；打开密集功能页时可占据阅读区域。','P列表→详情，L列表+详情；切换方向保留筛选、选中和request_id。','来源文档冲突优先：本轮用户保留要求→当前保留页→v1.4专章→旧稿→参考图装饰。'])
dump('contracts/style-contract.yaml',style)

write('spec.md', '''# 无极仙道 · 水墨 UI 项目
## 文档状态
- status: draft
- 日期：2026-10-22；版本：UI设计v1

## 用户目标与授权边界
依据全部两张参考图，建立完整修仙水墨传奇UI设计项目。用户指定：“先完成规范和屏幕地图，再生成首页视觉，批准后拆解首页组件”。本轮连续执行到首页视觉候选，停在首页批准点。
保留现有主页面、角色属性面板、背包。将“主页面”保守解释为启动主菜单和玩法HUD均保留；首页视觉采用玩法HUD的对照候选，不直接替换任何现有页面、相机、角色、地图、蓝图或运行代码。
其余页面重制设计并覆盖既有策划功能。完整项目是设计目录与逐屏契约，不等于所有页面已生图或已在UE完成。

## 平台、比例与语言
沿用UE5.8主播端；主审阅9:16，逻辑720×1280，横屏1280×720重排，中文。移动真机和平台观众面板另需验证。用户未授权改核心玩法或服务。

## 规则来源及冲突处理
docs/design/2026-10-21-wuling-resource-v1.4 的11/13/14/15/16及tables是玩法依据；最新保留页、源码与截图优先用于现状描述。UI开发规范沿用2026-10-21-ink-ui-standard-v1，强调修仙字体和真实业务数据。
v1.4表中旧页面状态只是历史状态，不能据此断言当前仍未实现。当前源码和保存截图已发现后续角色评级与签到宝箱升级；本轮不重跑其运行验收。
参考目录完整路径与哈希见evidence/references.json，已逐张查看。参考图片用于风格，不自动引入宠物召唤、抽奖、付费改签或虚构总战力。

## 验收
全部既有规划屏幕都有稳定ID、入口、退出、主操作、数据、异常及页面brief；保留/重制标记明确。新视觉符合单一style-contract。首页仅一张竖屏候选，实际尺寸和Alpha登记，用户批准前不拆组件。数据缺失用可见“视觉预览”输入，不读本地日期生成成长。不能把文档校验当作UE、设备、直播或平台验收。
''')
write('research.md', '''# 全参考分析与现状核对
## reference-01：四分区组件合辑
全部四分区均检查：签到与活动、出游与修行、门派与武灵、角色属性。
共同语言：白宣纸、毛笔短标题、灰青远山、旧金细框、玉石奖励、山脚羽化入纸；主动作金、次动作浅纸、辅助动作青；朱砂仅印记与提醒。密集功能组共用母纸，不为每行重复粗框。
签到提取日历、已领勾、累计轨道、活动主题卡；出游提取地图、路线和阶段，而不沿用“出诊”业务；门派提取组织徽记、成员行、建设入口。参考中的宠物武灵召唤与本项目组织武灵冲突，明确不采用。角色框与境界条只作下游装饰语法，当前保留角色页优先。
合集是组件设计板，原字号不能按整图比例直接用于手机；完整合图不能作为一张功能页导入。

## reference-02：横竖屏HUD规范参考
查看横屏与竖屏两区：左上身份条、右上活动/签到/仙途、底部五入口、中央角色与高山场景。横屏签到为右侧纸卷，竖屏未强塞同一宽面板。
采用它的导航层级、黑墨保护底和局部金云；不把高饱和仙宫替换当前岩石场景。现有相机、角色大小、装饰资产保留。图上“生命”在新页统一为当前规范“血量”。累计奖励按策划3/7/14/21，而非机械照抄图中数字。新首页无弹窗原态，以便批准HUD基准；签到重制列后续页面。

## 实际项目核对
查看了references/current-home.png、current-home-landscape.png、current-main.png、current-character.png、current-inventory.png五张既有截图。它们是历史保存的Windows PIE证据，不是本轮新截图。
首页是青空岩石平台上的小比例人物；不能按参考图变成近景大立绘。角色页是丹田、功法和装备槽、评级属性；不是参考图旧人物属性卡。背包有独立墨边外壳、搜索、格子、分类、多选、分页与底操作。
源码InkInventoryModel.h确认200槽/20默认解锁，InkInventoryWidget.cpp确认40每页。InkHudLayout.cpp确认上下导航。InkFeaturePanel.cpp确认仙途Extensions以及CharacterProfile路由；Expedition为旧出诊业务，不能误映射仙途。旧策划表漏列仙途、制作与外观等，已补入屏幕地图。
历史交付记录显示签到服务已有宝箱账本与排行榜头像持久行；本轮设计要求保留业务行为，新视觉不能造成重复发奖或头像闪烁。任何后续实现仍需重新检查真实控件树与接口版本。

## 方向取舍
推荐并采用“保留页为锚，其他系统页重制”：与当前项目最兼容，审核聚焦。全UI统一重画会破坏保留要求；逐张照搬组件合辑会带入错误武灵语义和未规划商业系统，因此不采用。字体、配色、软边标准沿用当前规范。
''')
write('gdd.md', '''# 游戏策划映射
## 文档状态
- status: draft
- 来源：现有v1.4策划；本文件是UI映射，不新订经济参数。

## 定位与用户
主播操作修士与武灵组织，观众通过允许互动成为个人剑灵，以修炼、世界历练、组织协作和临时资源配额构成修仙循环。传奇式成长反馈采用境界、功法、装备与奖励，不擅加职业、PK、攻城、元宝充值或战力付费榜。
卖点：水墨修仙表达；主播与剑灵协作；免费历练补给与可恢复的资源竞争。

| 循环 | 行为 | UI承接 |
|---|---|---|
| 回流 | 看实际进度、签到与当前活动 | home、sign-in、activities |
| 准备 | 看角色、选功法装备、补足来源 | character、inventory、methods、equipment、item-sources |
| 核心 | 修炼、世界历练、竞采 | home、world、race-live |
| 成长 | 确认收据、突破与公库建设 | world-result、breakthrough、tribulation、guild-build |
| 持续参与 | 查看开采权、剑灵权益、下一目标 | lease、sword-spirit、wuling |
| 可选经济 | 原型商店、寄售、订单 | 仅prototype_full的shop、market、orders |

## 系统与优先级
完整分类见screen-contract。must-have覆盖当前主流程与每个既定规划页面；原型经济与离线为optional，功能开关优先于页面存在。竞采/天劫是已策划系统，其设计不表示正式能力已核准。
主播成长、观众个人剑灵成长、组织公库分别显示，不把组织等级当个人境界。武灵是组织，不是召唤宠物。观众个人页没有生命、防御或死亡。

## 经济与成长约束
材料分型、品质、配方、来源与建设成本以v1.4专表为准；UI不重算一套公式。免费世界来源始终可达，资源租约临时且失去后可恢复。材料不足展示具体缺口与来源。背包200总槽、20默认解锁、40每页，目录数量不等于同时拥有。
突破确定品质Q与天劫成功概率分离。天劫从配置允许的金丹六阶→元婴开始，基础50%，每有效不同助力+1个百分点最多50，连续失败每次+10个百分点最多50，最终封顶100%。失败扣1500修为、保留境界和材料；不提供付费加概率。具体行为以16与当前服务规则版本为准。
礼物权益列清对象、期限、来源与当前模式，不把金额当竞采得分；确认展示不再次发奖。发布候选对未核准功能隐藏入口并拒绝直达。
''')
write('prd.md', '''# 产品需求与交付范围
## 文档状态
- status: draft

## 成功指标
规划表全部页面覆盖，无无出口孤岛；每个必需页都有可读的主动作、数据归属、禁用理由和异常恢复。保留页的现有源文件/资产哈希不因本轮写入变化。首页只交一个竖屏候选，用户批准后才开始组件拆解。

| ID | 优先级 | 需求 | 屏幕ID | 验收 |
|---|---|---|---|---|
| P0-01 | P0 | 保留主页面、角色和背包 | main/home/character/inventory | 保留清单与哈希；不覆盖运行工程 |
| P0-02 | P0 | 修炼、世界历练到结算闭环 | home/journey/world/region/world-result | 开始、运行、恢复、结算和返回路径完整 |
| P0-03 | P0 | 武灵与剑灵归属明确 | wuling/sword-spirit/gift-rules | 三类人数分开；无观众血量；来源期限可查看 |
| P0-04 | P0 | 资源竞采闭环 | resources/resource-detail/race-lobby/race-live/race-result/lease | 真实对手、超时转历练、配额归属与自动租期 |
| P0-05 | P0 | 成长构筑、来源与渡劫 | methods/equipment/breakthrough/tribulation/item-sources/guild-build | 冻结配置、成本、Q、概率、免费助力与保底均可见 |
| P0-06 | P0 | 签到活动及排行重制 | sign-in/monthly-sign-in/daily-verse/activities/daily-activities/timed-activities/daily-detail/timed-detail/leaderboard | 日期与目标权威；宝箱详情；3/7/14/21；窄榜8行 |
| P0-07 | P0 | 安全返回与设置存档 | saves/settings | 覆盖删除确认；在线状态不由本地档覆盖 |
| P1-01 | P1 | 开服活动与贡献榜 | launch/launch-rank | 真期限、免费贡献、并列、自动到账与称号到期 |
| P1-02 | P1 | 原型经济与离线 | offline/shop/market/orders | 仅获准profile；价格版本与完整费用确认 |

## 本轮具体交付
策划映射三文档、参考分析、UI规范、逐屏地图/契约/brief、保留清单、首页提示词与一张实际视觉、真实资产清单、验证与后续任务。其他页面本轮完成规格和地图；其逐页视觉与组件按批准批次后续执行。
不新增游戏核心系统，不改客户端逻辑，不生成已实施的UMG/uasset，不把原型权限说成生产权限。不拆未批准首页，不交空壳PNG包。

## 数据与请求
所有正式VM含profile/app/room/session_generation/rule_version/server_utc/snapshot_version/is_authoritative。预览输入显式标记；未知用“待同步”，不能填假0冒充余额。服务权威控制归属、权限、支付、奖励、概率、UTC与事务。
通用Ready→Submitting→PendingReconcile→Confirmed/Rejected；一次操作一个request_id，重试保持原ID；版本变化重确认。关闭页保留请求对账，切房丢弃旧回调。

## 验收矩阵
详见evidence/acceptance.csv。检查540×960、720×1280、1280×720、960×720和超长屏：文本、热点、SafeZone、筛选/选中/请求状态保留。新视觉只证明构图候选，功能必须后续在UE和真实设备验证。
''')

inter = '''# UI交互契约
## 文档状态
- status: draft

## 全局导航与状态
启动主菜单→HUD；顶部活动/签到/仙途，底部角色/背包/修炼/排行/菜单。菜单包含现有存档、设置、返回主菜单。普通页面一层，详情压导航栈；Esc先关确认，再退详情，再回HUD。
页面层级：场景0、HUD10、短播报20、页面100、确认/详情200、连接状态300。连接层不能挡返回或取消。所有详情返回保留选择、筛选、页码、滚动锚点。横竖屏不重复发请求。
所有页面共享loading/ready/empty/locked/pending/disconnected/error/expired/feature-disabled。网络未知时显示最后确认快照与“更新暂停”。pending只锁本次主提交。红点只读真实未读或可领状态。

## 控件与反馈
主按钮Normal/Hover/Pressed/Focused/Disabled/Pending；hover蓝与selected金区分，触控不依赖hover。点击热点至少88×88逻辑单位，视觉可小于热点，邻近区域不重叠。禁用显示具体原因，可跳免费来源。破坏性或精确成本操作先确认，收到权威结果再播放成功。
装饰、烟云、粒子HitTestInvisible；动态文字独立TextBlock；人物与图标等比，不能被纸框拉伸。减少动态时停循环粒子，保留静态选中和可领取标记。

## 跨页闭环
home→journey→world→region→world-result→character/inventory/home。
home→journey→resources→resource-detail→race-lobby→race-live→race-result→lease→world。
character→breakthrough→item-sources→world（回页保持原材料选择）；breakthrough→tribulation→character。
home→journey→wuling→guild-build→wuling；wuling→sword-spirit→gift-rules→sword-spirit。
home→sign-in→monthly-sign-in/daily-verse→home；home→activities→日常/限时/开服详情→home。
inventory→item-sources→world；shop/market→orders→inventory仅在prototype_full。

## 与首页视觉的约束
首屏必露主播身份、实际血量与修为、现有上下导航；本轮样例使用可见“视觉预览”标签。无服务目标就不生成任务。HUD保护区x=.22W..78W/y=.30H..78H，侧榜不挡主角。原场景只作概念参考，不由UI位图替换。首页无签到弹层，避免将单页审核扩为第二张页面。

## 逐屏流程、控件与边界
'''
for s in screens:
    sid=s['id']
    content='；'.join(s['layout']['center'])
    edge='；'.join(s['edge_cases'])
    rules='；'.join(s['rules'])
    flow=f"从{' / '.join(s['entry_points'])}进入→加载权威快照→查看状态与条件→{s['primary_actions'][0]}→待对账→结果或原因→返回原入口"
    if s['delivery']=='preserve-runtime':
        flow='保留当前运行流程，本轮只记录约束；'+flow
    section=f'''\n### {sid} · {s['name']}
目的：{s['purpose']}。\n入口/退出：{' / '.join(s['entry_points'])}；返回按导航栈，不默认回主菜单。\n主流程：{flow}。\n控件：标题/返回、{content}；底部固定主动作；需要时弹出成本确认与详情。\n状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：{edge}。\n数据：{' / '.join(s['data_needs'])}。\n规则：{rules}。\n'''
    inter+=section
    write(f'briefs/{sid}.md',f"# {s['name']} · {sid}\n\n交付：{s['delivery']}；视觉状态：未批准。\n"+section+f"\n竖屏：{s['responsive']['portrait']}。\n横屏：{s['responsive']['landscape']}。\n\n使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：{s['current_status']}。\n")
write('interaction.md',inter)

table=['| ID | 页面 | 处理 | 入口 | Gate | 数据 |','|---|---|---|---|---|---|']
for s in screens:
    table.append(f"| {s['id']} | {s['name']} | {'保留' if s['delivery']=='preserve-runtime' else '重制'} | {', '.join(s['entry_points'])} | {s['profile_gate']} | {', '.join(s['data_needs'])} |")
write('screen-map.md', '''# 屏幕地图与重制清单
所有表内页面已做具体规格；仅首页进入本轮视觉生成。角色外观、装备、功法等子页重制时不能覆盖已保留的角色属性主面板。组件与功能确认层为页面内局部状态，不重复增设顶层路由。

```mermaid
flowchart TD
 M[主菜单 保留] --> H[首页HUD 保留]
 H --> C[角色属性 保留]
 H --> I[背包 保留]
 H --> R[排行 重制]
 H --> Menu[菜单]
 Menu --> Save[存档 重制]
 Menu --> Settings[设置 重制]
 H --> J[仙途 重制]
 J --> W[世界历练与区域]
 W --> WR[历练结算]
 WR --> C
 J --> G[武灵 总览/成员/记录]
 G --> Build[武灵建设]
 G --> Sword[个人剑灵]
 Sword --> Gift[效果说明]
 J --> Res[资源列表与详情]
 Res --> Lobby[匹配]
 Lobby --> Live[竞采]
 Live --> Result[竞采结算]
 Result --> Lease[开采权]
 Lease --> W
 C --> Method[功法构筑]
 C --> Equip[装备法宝]
 C --> Break[突破准备]
 Break --> Trial[天劫]
 Trial --> C
 Break --> Source[物品来源]
 I --> Source
 I --> Craft[物品炼制]
 Craft --> Source
 C --> Appearance[名片外观]
 Res --> ResRank[资源贡献榜]
 Res --> ResRewards[资源奖励明细]
 Source --> W
 H --> Sign[七日签到]
 Sign --> Month[月签]
 Sign --> Verse[每日剑诀]
 H --> Act[活动]
 Act --> Daily[日常列表与详情]
 Act --> Timed[限时列表与详情]
 Act --> Launch[开服活动与贡献榜]
 J -.获准profile.-> Off[离线收益]
 J -.prototype_full.-> Shop[商店]
 J -.prototype_full.-> Market[寄售]
 Market --> Orders[订单]
```

'''+'\n'.join(table)+'''

## 保留与新增的边界
本轮把主菜单和HUD同时列为保留，避免“主页面”歧义造成覆盖。地图中的方法、装备、突破等连接是子页接线要求，不授权修改角色现有布局。
旧表没有仙途页，已根据当前源码补journey以及craft、appearance、resource-rank、resource-rewards，共44屏。所有当前路由别名与合并/排除理由见evidence/route-audit.json；仙途是Extensions，旧Expedition/ExpeditionDetail与Draw均退出正式业务导航，保留开发资产不删除。CharacterFate因当前规则未开放而禁用；CharacterEquipment/Skills/Artifacts分别并入装备或功法视图，不重复造功能。旧route Character与当前CharacterProfile映射须集成时核查。材料与天劫五页以15/16和ui-extension.csv为准，旧突破弹窗只作为兼容入口。
''')

write('plan.md', '''# 分阶段执行计划
1. 参考与现状：两张参考全部查看，五张现有截图核对；以哈希固化保留范围。
2. 规范与地图：补齐GDD/PRD/交互映射；style-contract单一视觉事实来源；所有既有规划屏幕加漏列的仙途都有逐页brief。
3. 首页候选：按用户本轮顺序，完成规范/地图后生成home-v1竖屏视觉；保留运行工程，标注视觉预览。三文档和新视觉均保持draft/未批准，不能默认为用户已批准。
4. 人工审核点：用户批准首页方向和保留范围后，才拆首页组件。若用户要求改稿，新建home-v2并标记依赖stale。
5. 后续批次A：仙途、世界/区域/结算、武灵/成员/建设、剑灵/说明；共享纸壳但按阅读任务重排。
6. 后续批次B：功法、装备、来源、突破/天劫；资源/匹配/竞采/结算/开采权。
7. 后续批次C：排行、签到/月签/剑诀、活动/开服；原型经济和离线只在profile允许时。
8. 资产交付：已批准页面逐组件/状态生成透明无字图，人工语义与9-slice审核后切图/ZIP/Atlas/generic JSON。UE导入绑定、PIE和设备测试另行执行。

## 审核条件
文档待审与用户授权执行范围分开记录。首页候选可用于审阅，不作为已批准基准向其余页面批量生图。没有生成组件图时不填坐标、切片和已完成包。
''')
write('tasks.md', '''# 实际任务状态
- [x] [research] 查看全部两张参考与五张当前页面截图。
- [x] [preserve] 记录现有UI源文件/资产哈希与保留范围。
- [x] [product] 完成三文档映射；状态draft，未替用户批准。
- [x] [specification] 完成配色/字体/布局/状态/软边/适配规范草案。
- [x] [extension] 完成全页面地图、契约与逐页brief。
- [ ] [page] 实际生成并检查home-v1，保存尺寸和提示词。
- [ ] [approval] 用户批准首页；当前未批准。
- [ ] [component] 批准后拆首页组件，逐状态出透明无字图。
- [ ] [page] 按批次扩展其余重制页面视觉。
- [ ] [asset] 实测九宫格、切图、ZIP、Atlas、generic交接。
- [ ] [runtime] UE集成、PIE、设备与平台验证，均不属于本轮完成声明。
''')

with (ROOT/'evidence/acceptance.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['id','scope','criterion','method','status'])
    for s in screens:
        w.writerow([s['id'],s['delivery'],'入口/主动作/数据/状态/规则/返回齐全','contract-crosscheck','specified'])
    for row in [
        ['preserve','runtime','保护文件哈希一致','sha256','pending-check'],
        ['home-visual','design','9:16导航清晰、主角不遮挡、标注预览','image-review','pending-generation'],
        ['touch','runtime','有效热点>=88且不重叠','UE-widget-bounds','not-run'],
        ['responsive','runtime','540/720竖屏、1280横屏、960方屏与长屏重排','PIE/device','not-run'],
        ['transactions','runtime','重试不重复发奖、旧回调隔离、版本冲突不暗扣','service-integration','not-run']]:w.writerow(row)

write('preservation.md', '''# 保留清单与实施优先级
本轮仅写specs/boundless-ink-ui，不写Source、Content、Config、Tools或既有SourceAssets。
保留主菜单（题字/角色/按钮）、游戏首页HUD（相机/场景/头像条/导航/查看排行）、角色属性（丹田/槽位/评级/品质框）、背包（图框/格子/分页/操作）与其现有数据流程。
原始保留文件哈希见evidence/preservation-baseline.json；交付后按相同路径复核。保留页旁接的新页面可以新建独立资源，不以统一风格为由覆盖旧材质或字体。
旧v1.4历史实现状态与当前不一致时，先查当前源码/控件树。当前角色评级优先；已有签到3/7/14/21账本与榜行持久更新行为继续保留。新设计改外观不改变事务或权限。
首页生图只作审阅候选，保留场景构图与导航；生成背景不能导入后直接覆盖真实3D场景。截图中测试文字不代表正式业务规则。
''')

prompt='''Use case: ui-mockup
Asset type: one portrait game home HUD review image, 9:16, target 1080x1920. This is a design preview, not a runtime screenshot.
Input image 1 (current-home.png) is the preserved content and composition reference. Input image 2 (reference-02.png) supplies ink-paper/aged-gold UI material language only. Input image 3 (reference-01.jpg) supplies delicate cloud and mountain motifs, not its collage composition.
Primary request: make ONE coherent full-screen portrait home HUD baseline for the Chinese cultivation game 无极仙道. Respect the existing home screen structure, environment and small character scale from image 1. Preserve the existing portrait camera composition: open turquoise-blue sky, rock pillars covered in yellow-green foliage, small black-robed swordsman on the central rock ledge in the lower middle, foreground bamboo at right. Do not replace the environment with the grand palace from image 2. Do not turn the small in-game character into a huge illustrated portrait. This is a faithful visual review of the existing home, not a new navigation design.
At upper left keep a circular male cultivator avatar, player name 主播, a compact red health bar with exact text 血量 1000/1000, and a green cultivation bar with exact text 修为 5/60. Beneath avatar, realm 炼气一阶. Keep these legible on a localized dark ink reading surface. Upper right keep exactly three compact icons with the labels 活动, 签到, 仙途; use ornate but restrained gold and ink material treatment. All top controls fit within the safe margin without colliding. Do not add false notification dots. Do not add quests when no actual target is supplied.
At lower right, above bottom navigation, preserve the small parchment button 查看排行. Bottom navigation has EXACTLY five entries in this left-to-right order: 角色, 背包, 修炼, 排行, 菜单. Character uses the existing portrait style, bag uses an antique pouch, cultivation uses a gold meditating silhouette with restrained amber aura and is the largest focal control, ranking uses crossed swords, menu uses an antique gear motif. Maintain these individual functions and silhouettes from image 1. Labels are readable, single-line, Chinese ink calligraphy. The bottom row sits above a dark soft gradient for readability; small faded cloud strokes can connect it, but no giant white dock, no excessive gilded shell.
Style: refined Chinese xianxia ink-wash game UI, low-contrast warm xuan paper #F3EBDD and light paper #FBF7EE where needed, readable ink #202823, blue-gray #3D5861, aged-gold accent #B88B45 with limited #E9CA84 highlights, jade #3A8D83, soft top-left light, consistent delicate silhouettes. Functional readability first, generous open scene. Use brush-like title language and elegant readable handwritten Chinese body/digits inspired by MaShanZheng and LXGW WenKai; no generic modern sans UI text. Straight-on UI, fine 1–3 logic-unit edges, no distorted perspective. UI references only; do not add a giant page title or marketing frame.
Place the small visible label 视觉预览 in an unobtrusive safe location below the top group; the numeric values here are explicit preview inputs. Keep central character protection region clear. Keep all controls within about 36px side and bottom safety margins at 1080-wide output. No modal or sign-in calendar in this home image, since sign-in is a separate future page. Do not draw component callouts or annotations. Output exactly one portrait screen without a device frame.
Avoid: a collage, landscape sheet, side-by-side mobile/desktop layouts, copied large title 无极仙道, giant protagonist, palace replacement, extra tabs, pets, random draw, shop promotion, paid controls, fake loot, invented combat joystick, skill hotbar, fake rankings, unreadable pseudo-Chinese, garbled digits, clipped labels, distorted buttons, overglow, gold covering the scene, white rectangular card UI, watermarks, additional brand names.
'''
write('prompts/pages/home-v1.txt',prompt)
write('prompts/pages/home-v1.md', '''# 首页HUD · home-v1
## 页面定位
保留现有首页结构的竖屏视觉对照候选，待批准；不覆盖当前UI。
## 视觉约束
读取style-contract；以current-home保持场景、人物尺度、上下导航。reference-02仅供材质语法，reference-01仅供局部水墨语法。
## 布局与组件
左上身份条；右上活动/签到/仙途；中央真实场景参考；侧边查看排行；底部角色/背包/修炼/排行/菜单。无弹窗，无假目标。
## 状态
默认未入定预览；血量1000/1000、修为5/60、炼气一阶为明确预览输入。正式状态由AnchorVm驱动。
## 图像生成提示词
最终完整提示词逐字保存于home-v1.txt，工具调用读取该文本；仅调用内置image_gen。
## 反向限制
不重构场景相机，不把人物放大成海报，不添加商业页、召唤、摇杆、抽奖或假红点。乱码与裁断不能作为可直接使用的UI。
## 验收记录
待实际输出后检查。需要用户批准，不能自动拆组件。生成中文仅用于审阅，后续组件无文字并用引擎排字。
''')

write('quickstart.md', f'''# 使用与真实状态
项目路径：{ROOT}
本轮读取gameui与brainstorming梳理设计；按用户授权先完成具体规范/地图，再执行首页候选视觉。imagegen用于内置生图，不需要API key。

## 批准记录
用户原文：“先完成规范和屏幕地图，再生成首页视觉，批准后拆解首页组件”。本轮据此连续推进到首页候选；不另插前置批准。GDD/PRD/interaction/style均保持draft，不将用户授权生图写成已批准新文档。首页approved=false；组件/切图/ZIP/Atlas/UE集成尚未执行。
gameui默认先审三文档；本轮以用户明确指定的执行顺序为准。校验若提示三文档未批准，应保留并解释，不为消警改成approved。

## 可复现命令
```powershell
$env:PYTHONUTF8 = '1'
& 'C:\\Users\\zxxuh\\.codex\\skills\\gameui\\.venv\\Scripts\\python.exe' '{ROOT}\\authoring\\build_project.py'
& 'C:\\Users\\zxxuh\\.codex\\skills\\gameui\\.venv\\Scripts\\python.exe' 'C:\\Users\\zxxuh\\.codex\\skills\\gameui\\scripts\\validate_project.py' '{ROOT}'
```
Python已验证：{sys.version.split()[0]}；PyYAML/Pillow可导入。build_project.py为首次编写脚本，会重建本轮草案及基线；后续有用户修改/批准后不要直接重跑，应编辑现有文档与新建版本。
最终提示词：prompts/pages/home-v1.txt。图片输出路径、尺寸、Alpha以asset-manifest与evidence/image-review.json实测记录为准。

## 验证边界
文档/路径/图片元数据与保留哈希校验，只证明设计包自身一致性。本轮未执行UE构建、PIE、Cook、手机或生产直播测试；使用的current截图为历史证据。完整设计目录不代表所有重制页已生成视觉。

## 下一步文本
`使用 $gameui，读取 D:\\demo\\BoundlessImmortality\\specs\\boundless-ink-ui；批准首页home-v1及本轮规范与屏幕地图，保留主页面/角色属性/背包，开始拆解首页组件。`
''')

write('evidence/build-summary.json',json.dumps(dict(screen_count=len(screens),legacy_rows=len(source_rows),references=len(refs),protected_files=len(protected),python=sys.version.split()[0],
    approved=False,ue_modified=False,runtime_tests_run=False),ensure_ascii=False,indent=2))
print(json.dumps(dict(project=str(ROOT),screens=len(screens),references=len(refs),protected_files=len(protected)),ensure_ascii=False))
