# UI交互契约
## 文档状态
- status: approved

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

### main · 主菜单
目的：保留启动页的水墨题字、人物与按钮构图。
入口/退出：application-start；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从application-start进入→加载权威快照→查看状态与条件→进入HUD→待对账→结果或原因→返回原入口。
控件：标题/返回、开始、继续、存档、设置、退出；沿用当前可达路径；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：存档为空、继续不可用、退出确认；不为参考图增设选服登录。
数据：FrontendVm。
规则：保留现有启动页；导航清单见当前路由，缺项只登记。

### home · 常驻HUD
目的：保留游戏首页HUD与相机，作为新页面的风格衔接基准。
入口/退出：main；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从main进入→加载权威快照→查看状态与条件→修炼或收功→待对账→结果或原因→返回原入口。
控件：标题/返回、原场景与角色，侧边现有查看排行；无虚假任务；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：无目标则隐藏目标；断线不加收益；修炼/收功使用实际状态。
数据：AnchorVm / WorldVm / LeaseVm。
规则：不得改镜头、山石、人物尺寸和导航顺序；概念图不是运行截图。

### character · 角色
目的：保留当前丹田与功法装备槽及属性评级面板。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从home进入→加载权威快照→查看状态与条件→查看当前属性及进入获准子页→待对账→结果或原因→返回原入口。
控件：标题/返回、功法行、丹田、六侧槽、下方已配槽与实际属性；保持当前品质框和减弱动态选项；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空槽、锁槽、长属性、未配置；不套用参考图旧立绘属性卡或虚构总战力。
数据：AnchorVm / MethodVm / EquipmentVm。
规则：保留当前角色评级系统，不恢复旧稿；用户保留优先。

### inventory · 物品
目的：保留背包全套UI、模型与操作。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从home进入→加载权威快照→查看状态与条件→选物品查看允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、分类/搜索/列表与网格/新标/数量/滚动/多选/分页/详情/出售分解整理；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：200物理槽、默认20解锁、40每页；hover蓝、selected金；绑定到期托管动作权限。
数据：InventoryVm。
规则：不改格数、材质、框架；新增来源页从允许的详情入口进入。

### leaderboard · 排行
目的：重制榜单外壳与阅读层级。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→切换榜单→待对账→结果或原因→返回原入口。
控件：标题/返回、本房/全服、剑灵数/修炼速度/伤害；窄榜8行、我的位置、规则与周期；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未上榜、空榜、头像失败、周期切换、并列；不提供假好友榜。
数据：LeaderboardVm。
规则：保留持久行/头像更新逻辑与8行边界；重制视觉不能造成头像闪烁。

### saves · 存档
目的：重制存档列表，明确本地范围。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→载入或保存→待对账→结果或原因→返回原入口。
控件：标题/返回、槽位、时间、模式、摘要、覆盖或载入；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空槽、坏档、版本不兼容、覆盖和删除二次确认。
数据：SaveVm。
规则：不可把本地存档覆盖在线成长或服务账本。

### settings · 设置
目的：重制设置页，降低调整成本。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→应用设置→待对账→结果或原因→返回原入口。
控件：标题/返回、声音/图像/控制/文字可读性/减弱动态；仅列现有能力；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：应用失败恢复原值；未保存退出提示；未知能力不绘可用开关。
数据：SettingsVm。
规则：键鼠与触控焦点可达；返回不中断待对账事务。

### wuling · 武灵总览成员建设记录
目的：重制武灵组织总览与名册。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→创建或管理武灵→待对账→结果或原因→返回原入口。
控件：标题/返回、总览/成员/建设/记录四子栏；正式、参与、临时人数分开；公库与当前租约；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未创建、名称2–12等效汉字、审核驳回、改名冷却；成员20一页。
数据：GuildVm / WorldVm。
规则：武灵是主播创建的组织；移除与解散说明个人权益保留。

### sword-spirit · 我的剑灵
目的：重制个人剑灵权益页。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→查看个人剑灵与选择称号→待对账→结果或原因→返回原入口。
控件：标题/返回、查看身份、归属、境界修为、称号、已解锁剑形、有效增益与来源；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未加入、未解锁、到期、其他房间范围、非本人权限。
数据：SwordVm / GiftEffectVm。
规则：观众无生命防御死亡栏；解锁剑形不增加观众人数。

### gift-rules · 礼物效果说明
目的：重制效果规则说明页。
入口/退出：sword-spirit；返回按导航栈，不默认回主菜单。
主流程：从sword-spirit进入→加载权威快照→查看状态与条件→查看效果说明→待对账→结果或原因→返回原入口。
控件：标题/返回、来源/对象/原期限/到期/叠加/当前模式；旧权益与候选规则分列；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：测试/正式、过期、暂不适用、断线未确认。
数据：GiftEffectVm。
规则：送礼跳官方路径；说明点击不购买，不生成付费加概率入口。

### methods · 功法构筑
目的：重制功法构筑页。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→确认准备期配置→待对账→结果或原因→返回原入口。
控件：标题/返回、七路线、1主修、当前辅修槽、锁定条件、主辅差异和草稿预览；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：事件中禁换并显示原因；不足材料跳世界来源；确认失败留旧配置。
数据：MethodVm。
规则：第3/6级强化从规则表读取；不在UI推导另一倍率。

### race-lobby · 竞采匹配
目的：重制匹配大厅。
入口/退出：resource-detail；返回按导航栈，不默认回主菜单。
主流程：从resource-detail进入→加载权威快照→查看状态与条件→报名或取消→待对账→结果或原因→返回原入口。
控件：标题/返回、资源、人数档、2–4真实武灵、等待时间、统一规则和取消；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：60秒无对手转世界历练；取消与匹配成功竞态按服务结果。
数据：RaceVm / GuildVm。
规则：没有真实对手只留空位，不画假姓名或头像。

### race-live · 竞采现场
目的：重制竞采现场轻量HUD。
入口/退出：race-lobby；返回按导航栈，不默认回主菜单。
主流程：从race-lobby进入→加载权威快照→查看状态与条件→响应当前阶段→待对账→结果或原因→返回原入口。
控件：标题/返回、120秒准备/360秒挑战/120秒结算；600共同/200主播/200成员；最多10冻结代表；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未响应/有效响应/断线/阶段切换；礼物表现与计分分开。
数据：RaceVm。
规则：中央保护角色；准确分母；直播观看者通过允许指令响应。

### race-result · 竞采结算
目的：重制竞采结算。
入口/退出：race-live；返回按导航栈，不默认回主菜单。
主流程：从race-live进入→加载权威快照→查看状态与条件→查看开采权或回世界→待对账→结果或原因→返回原入口。
控件：标题/返回、总分与三项来源、600资格线、并列组、30/20/10分钟租期；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未达标原因、并列、对账中、租约已开始。
数据：RaceVm / RewardVm。
规则：第600秒租约自动开始；确认不延后计时。

### lease · 当前开采权
目的：重制当前开采权。
入口/退出：race-result；返回按导航栈，不默认回主菜单。
主流程：从race-result进入→加载权威快照→查看状态与条件→查看收益来源并回历练→待对账→结果或原因→返回原入口。
控件：标题/返回、服务器start/end、剩余时间、受益身份、在线积累、已入账和下一档；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：到期/断线暂停更新/无租约/跨房不适用。
数据：LeaseVm。
规则：离线不自行计数；允许并列配额，不假装唯一控制者。

### world-result · 历练结算
目的：重制历练结算。
入口/退出：world；返回按导航栈，不默认回主菜单。
主流程：从world进入→加载权威快照→查看状态与条件→确认展示或再去历练→待对账→结果或原因→返回原入口。
控件：标题/返回、基础阶段、个人修为、个人贡献、公库、额外核心分别列归属和收据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：中断保留已完成阶段；待对账/已到账/已展示。
数据：RewardVm。
规则：确认只标已读，不能再次发奖励；满包转待入账收据。

### offline · 离线收益
目的：重制离线收益页。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→确认已入账展示→待对账→结果或原因→返回原入口。
控件：标题/返回、上次结算、本次有效时长、8小时上限、基础10%、修为结果、原收据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：profile禁用/无收益/待对账/已入账。
数据：RewardVm。
规则：不受灵脉礼物放大；确认不重复领取；仅获准profile显示。

### shop · 原型商店
目的：重制原型商店。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→核对完整成本后制作或购买→待对账→结果或原因→返回原入口。
控件：标题/返回、SKU、成本、绑定材料、产物、限购、唯一窗口、期限、数量与总价；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：低余额/售罄/旧价过期/事务中/feature禁用。
数据：EconomyVm / InventoryVm。
规则：仅prototype_full；发布候选入口隐藏且直达请求拒绝。

### market · 原型寄售
目的：重制原型寄售。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→整单买入或挂牌→待对账→结果或原因→返回原入口。
控件：标题/返回、白名单原料、单价区间、数量、税费、总价、到期、托管；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：旧价失效/订单已售/背包满/超白名单/禁用。
数据：EconomyVm / InventoryVm。
规则：完整费用二次确认；无服务不假扣币或假成交。

### orders · 我的订单
目的：重制我的订单。
入口/退出：market；返回按导航栈，不默认回主菜单。
主流程：从market进入→加载权威快照→查看状态与条件→撤单或查看应收→待对账→结果或原因→返回原入口。
控件：标题/返回、挂牌/成交/取消/到期/应收；时间、数量、手续费与唯一收据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空订单/撤单竞态/应收待对账/禁用。
数据：EconomyVm。
规则：按订单版本原子请求；不能因关闭重开发一次应收。

### world · 世界历练
目的：重制世界历练主页。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→选择路线并开始历练→待对账→结果或原因→返回原入口。
控件：标题/返回、首发三区与后续锁区、独行/小/中/大预设、路线、10分钟阶段、免费固定供给；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：准备/进行/30秒恢复/结算中/完成/中断；运行中锁预设。
数据：WorldVm / AnchorVm。
规则：非送礼补给入口可达；同一轮在武灵与世界共用round_id。

### region · 区域详情
目的：重制区域详情。
入口/退出：world；返回按导航栈，不默认回主菜单。
主流程：从world进入→加载权威快照→查看状态与条件→按条件进入历练→待对账→结果或原因→返回原入口。
控件：标题/返回、一幅等比区域山水、难度、阶段、固定奖励、开放条件与参与说明；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：境界锁定、事件互斥、未开放、奖励未同步。
数据：WorldVm。
规则：锁定区域保留条件说明，不画虚假的开始按钮。

### resources · 资源争夺
目的：重制五类资源选择。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→查看资源并报名→待对账→结果或原因→返回原入口。
控件：标题/返回、灵泉/灵石矿/灵草园/玄铁脉/残卷遗址；对象、产出、间隔、租期、当前开采权；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未开放、已有租约、正式成员不足、网络断开。
数据：LeaseVm / RaceVm。
规则：资源是临时配额；不渲染为永久独占矿点。

### resource-detail · 资源详情
目的：重制资源规则和报名说明。
入口/退出：resources；返回按导航栈，不默认回主菜单。
主流程：从resources进入→加载权威快照→查看状态与条件→进入竞采匹配→待对账→结果或原因→返回原入口。
控件：标题/返回、收益对象、单位产出、统一竞采规则、人数档、资格；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：正式成员不足、活跃租约、profile关闭。
数据：LeaseVm / RaceVm。
规则：说明世界历练替代路线；不把礼物额作为分数。

### sign-in · 签到组及日签到
目的：重制签到组及七日页。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→查看奖励并签到→待对账→结果或原因→返回原入口。
控件：标题/返回、七日宝箱、今日详情、累计轨道、已签形态、固定CTA；月历和每日剑诀二级页；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：可签/请求中/已签/跨日；宝箱先详情后领取；未达和断线可看明细。
数据：ActivityVm。
规则：沿用现有服务账本；每日300修为依当前规则，动画不触发发奖。

### monthly-sign-in · 月签到
目的：重制月签日历。
入口/退出：sign-in；返回按导航栈，不默认回主菜单。
主流程：从sign-in进入→加载权威快照→查看状态与条件→选日或查看累计奖励→待对账→结果或原因→返回原入口。
控件：标题/返回、服务器月份、七列日历、今天/已签/未到、3/7/14/21累计宝箱和数量；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：跨月、月末、节点未达/可领/已领、断线；节点后轨道与箱中心对齐。
数据：ActivityVm。
规则：不能沿用参考图/旧稿7/14/21/28；复用现有已领取账本。

### daily-verse · 每日剑诀
目的：重制每日剑诀。
入口/退出：sign-in；返回按导航栈，不默认回主菜单。
主流程：从sign-in进入→加载权威快照→查看状态与条件→查看剑诀并去行动→待对账→结果或原因→返回原入口。
控件：标题/返回、卷轴诗签、固定轮换剑诀与行动提示、对应免费玩法入口；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：已读/未读、固定轮换未知、无网络。
数据：ActivityVm。
规则：不做随机吉凶、改签付费；不展示抽奖入口。

### activities · 活动组
目的：重制活动总入口。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→选择活动→待对账→结果或原因→返回原入口。
控件：标题/返回、日常/限时/开服；首屏一个主题头图、目标卡、实际时间和状态；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空活动、未开、进行、已结束、可领取、feature关闭。
数据：ActivityVm / LaunchVm。
规则：不用两张大海报撑空白；红点仅真实未读/可领。

### daily-activities · 日常活动
目的：重制日常活动列表。
入口/退出：activities；返回按导航栈，不默认回主菜单。
主流程：从activities进入→加载权威快照→查看状态与条件→前往或查看奖励→待对账→结果或原因→返回原入口。
控件：标题/返回、实际目标、当前/需求、预算、固定奖励、前往和已领取；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未解锁/进行/达成/待对账/已领/周期切换。
数据：ActivityVm。
规则：不能以页面打开产生进度；领取沿用原request_id。

### timed-activities · 限时活动
目的：重制限时活动列表。
入口/退出：activities；返回按导航栈，不默认回主菜单。
主流程：从activities进入→加载权威快照→查看状态与条件→查看限时活动→待对账→结果或原因→返回原入口。
控件：标题/返回、等比主题卡、start/end与Asia/Hong_Kong时区、免费参与与期限；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未开始/进行/已结束/数据未知。
数据：ActivityVm。
规则：没有服务起止时间就显示待同步，不生成倒计时。

### daily-detail · 日常详情
目的：重制日常详情。
入口/退出：daily-activities；返回按导航栈，不默认回主菜单。
主流程：从daily-activities进入→加载权威快照→查看状态与条件→参与允许活动→待对账→结果或原因→返回原入口。
控件：标题/返回、主题头图、条件、任务、奖励归属、预算、主操作；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：预算耗尽/条件不足/已完成/断线。
数据：ActivityVm / RewardVm。
规则：标明是否替换基础预算；不足给来源入口。

### timed-detail · 限时详情
目的：重制限时详情。
入口/退出：timed-activities；返回按导航栈，不默认回主菜单。
主流程：从timed-activities进入→加载权威快照→查看状态与条件→参与或确认结果→待对账→结果或原因→返回原入口。
控件：标题/返回、剩余时间、真实规则、固定产出、截止、有效期与入口；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未开/过期/满包/处理中/活动版本变化。
数据：ActivityVm / RewardVm。
规则：不新增随机抽奖或未规划付费档位。

### launch · 青云初启开服活动
目的：重制青云初启。
入口/退出：activities；返回按导航栈，不默认回主菜单。
主流程：从activities进入→加载权威快照→查看状态与条件→查看目标或确认展示→待对账→结果或原因→返回原入口。
控件：标题/返回、总览/个人任务/武灵目标/规则四内栏、本人报名、七任务、1/3/6/12轮次、称号到期；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未报名/进行/达成/自动入账/过期/profile禁用。
数据：LaunchVm / RewardVm。
规则：四内栏不重复压路由；发布候选隐藏并拒绝直达。

### launch-rank · 开服玩法贡献榜
目的：重制开服贡献榜。
入口/退出：launch；返回按导航栈，不默认回主菜单。
主流程：从launch进入→加载权威快照→查看状态与条件→查看贡献明细→待对账→结果或原因→返回原入口。
控件：标题/返回、免费贡献四来源、轮数、资格、并列名次和荣誉；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未达资格/并列/周期结算/未上榜/禁用。
数据：LaunchVm。
规则：不展示消费排名或已关闭的修为奖励。

### breakthrough · 突破准备
目的：重制突破准备。
入口/退出：character；返回按导航栈，不默认回主菜单。
主流程：从character进入→加载权威快照→查看状态与条件→核对成本后确认突破→待对账→结果或原因→返回原入口。
控件：标题/返回、四类材料实例槽、数量/替换、Q评分来源、确定品质、失败扣修与保底规则；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：不足材料跳来源；准备取消；与其他事件互斥；预留过期。
数据：BreakthroughVm / InventoryVm。
规则：确定Q与天劫成功率分开展示；v1.4覆盖旧突破成本语义。

### tribulation · 天劫助力
目的：重制渡劫现场。
入口/退出：breakthrough；返回按导航栈，不默认回主菜单。
主流程：从breakthrough进入→加载权威快照→查看状态与条件→观众发送助力；主播查看结果→待对账→结果或原因→返回原入口。
控件：标题/返回、15秒准备、60秒三重天劫、8秒揭示；不同助力人数、基础/助力/保底/最终概率；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：准备/天劫/冻结/提交/对账/成功/失败/清理；已助力和截止可见。
数据：TribulationVm。
规则：50%基础+每唯一助力1百分点(最多50)+每失败10百分点(最多50)，封顶100%；失败扣1500修为保留材料和境界。

### item-sources · 物品来源
目的：重制物品来源追踪。
入口/退出：inventory；返回按导航栈，不默认回主菜单。
主流程：从inventory进入→加载权威快照→查看状态与条件→选择免费来源并前往→待对账→结果或原因→返回原入口。
控件：标题/返回、材料分型品质、具体区域/副本、专注进度、配方与共享额度；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未知物品/区域锁定/额度耗尽/可制作/版本变更。
数据：ItemSourceVm。
规则：来源沿v1.4表；不得导入旧表叠加奖励；回背包保留选中项。

### guild-build · 武灵建设
目的：重制武灵建设。
入口/退出：wuling；返回按导航栈，不默认回主菜单。
主流程：从wuling进入→加载权威快照→查看状态与条件→预览并原子提交建设→待对账→结果或原因→返回原入口。
控件：标题/返回、公库余额、建设项目、材料、级别、成本、外显预览、版本与记录；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：权限不足/材料不足/提交中/版本冲突/已完成。
数据：GuildBuildVm。
规则：个人与公库分开；v1.4建设表覆盖旧100资材示意，不生造永久倍率。

### equipment · 装备与法宝
目的：重制装备法宝子页。
入口/退出：character；返回按导航栈，不默认回主菜单。
主流程：从character进入→加载权威快照→查看状态与条件→在准备期确认配置→待对账→结果或原因→返回原入口。
控件：标题/返回、主播四个非武器装备槽和一个法宝槽、属性上限、归属、替换前后；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：安全准备期/事件冻结/锁定/冷却/绑定/过期。
数据：EquipmentVm / InventoryVm。
规则：不重画被保留角色页；切装不重置法宝冷却。

### journey · 仙途菜单
目的：重制系统总入口且保持现有导航位置。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→进入世界、武灵或资源系统→待对账→结果或原因→返回原入口。
控件：标题/返回、世界历练/武灵/我的剑灵/功法/资源五组入口，按能力隐藏；保留现有签到活动角色捷径；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：开关关闭不显示可用入口；世界事件进行中展示当前状态。
数据：WorldVm / GuildVm。
规则：不在HUD新增第二排入口。

### craft · 物品炼制
目的：重制物品炼制。
入口/退出：inventory；返回按导航栈，不默认回主菜单。
主流程：从inventory进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、配方、材料分型与品质、共享额度、产物、缺口与来源；完整成本确认后原子制作；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：缺料、额度耗尽、版本冲突、满包待入账、重复提交。
数据：RecipeVm / InventoryVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

### appearance · 名片外观
目的：重制名片外观。
入口/退出：character；返回按导航栈，不默认回主菜单。
主流程：从character进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、已获头像框与称号、等比试戴、来源、期限和应用；未获只预览；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未解锁、过期、当前规则不适用；不得试戴即取得权益。
数据：AppearanceVm / GiftEffectVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

### resource-rank · 资源贡献榜
目的：重制资源贡献榜。
入口/退出：resources；返回按导航栈，不默认回主菜单。
主流程：从resources进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、周期、规则、三类分数、并列与自己位置；与竞采结算同一数据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：旧周期、对账中、未上榜、feature关闭。
数据：RaceVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

### resource-rewards · 资源奖励明细
目的：重制资源奖励明细。
入口/退出：resource-detail；返回按导航栈，不默认回主菜单。
主流程：从resource-detail进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、租期、实际受益、在线积累、收据与来源明细；确认只已读；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：无租约、过期、断线、未达资格。
数据：LeaseVm / RewardVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

批准记录：用户于2026-10-22批准本轮规范、屏幕地图和首页v2：批准首页v2及规范和屏幕地图，开始拆解首页组件。批准仅针对当前UI设计基线，不新增玩法或发布权限。
