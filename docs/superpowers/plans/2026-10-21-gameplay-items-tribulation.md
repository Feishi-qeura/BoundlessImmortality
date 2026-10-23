# v1.4 物品、功法与天劫实施计划

**执行记录（2026-10-22）：** 已实现物品导入、来源/制作、公库、装备法宝道具、七功法、确定品质重炼及完整天劫状态机，并接入UE。下方保留原细分验收步骤；实施/证据见 `docs/2026-10-22-wuling-v14-implementation.md`。物品原始30项仅10项有完整直接断言，其余保持not_run，不将相关模块测试当作整个原场景通过。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans after user review. Steps use checkbox (`- [ ]`) syntax for tracking. Do not dispatch agents without explicit authorization.

**Goal:** 实现v1.4物品和确定配方、主播构筑、品质突破与完整助力天劫事务。

**Architecture:** 复用基础服务的事务、scope和收据；物品实例记录来源、绑定、paid_origin及原到期。品质和概率分离，天劫单次服务端随机值持久化，UE只显示结果并按attempt维护预警租约。

**Tech Stack:** Python/SQLite，UE C++战斗与已有剑灵表现。

**Spec:** 主计划及v1.4的04、08、15、16、`tables/items-v1.4/`。

## Global Constraints

- 主计划全部约束适用；依赖基础T1–T3。
- 总索引730，640材料变体、14功法物品、16装备、12法宝、20新道具、4公库材料；不重复导入旧24项。
- 核心1必需，辅材全可选；大境界不重复收核心配方已支付的破境石/灵石，小阶不改。
- 品质确定，十个大境界各取最高快照后加法合计最多10%；不累计相乘。
- 天劫默认BR04起，5000基础+每位不同观众100+每次失败1000基点，最高10000；失败只扣min(1500,未花费修为)，不降阶不消耗材料。

## Review Focus

- 数量相同但绑定、paid_origin、到期不同不能混栈，T1。
- 失败和重放不复制原材料，T5。
- 公库制作个人扣料、公库入账要同事务，T3。
- 卸装不重置法宝冷却，无目标不消耗冷却，T4。
- Trial后掉线仍结算，预留跨故障过期进入对账而不重抽，T5。

## T1：来源实例和确定性制作

**Files:** Create `Tools/CultivationService/inventory.py`、`crafting.py`；Test `tests/test_inventory.py`、`test_crafting.py`；Modify `rules.py`、`schema.sql`。

**Interfaces:** `Inventory.grant(ctx: CommandContext, item_id: str, count: int, source: dict) -> list[str]`；`Inventory.reserve(ctx, requirements: list[dict], reservation_id: str) -> list[dict]`；`Inventory.release(ctx, reservation_id: str) -> None`；`Inventory.consume(ctx, reservation_id: str) -> None`；`Crafting.craft(ctx, recipe_id: str, count: int=1) -> dict`。grant仅内部奖励调用，不开放HTTP增币/增物命令。

- [ ] 写失败测试：索引唯一、CSV True/False严格解析、556配方引用完整；最早到期优先扣；绑定/来源污染/到期不混栈；缺1份材料整个事务无变化。
- [ ] 测试2同种同品质→1下一品，到极品停止；输出到期等于最早输入和窗口上限，paid_origin为输入OR；高品质不接受客户端指定产量。
- [ ] 运行对应测试确认失败；实现Craft/BindMaterial：两种交易草铁1:1单向绑定，原来源和期限保留；同request重复只一份产物。
- [ ] 测试到期清理不删除收据、不扫描旧礼物权益文件；过期预留按attempt状态处理，不擅自延时。
- [ ] 运行独立连接争同一份库存测试，确认只有一笔消费成功，保存币物变化流水。

## T2：定向来源与突破品质

**Files:** Create `Tools/CultivationService/focus.py`、`breakthrough.py`；Test `tests/test_focus.py`、`test_quality.py`；Modify `world.py`、`growth.py`。

**Interfaces:** `Focus.freeze(ctx, round_id: str, source_id: str, item_id: str) -> dict`；`Focus.settle(ctx, round_id: str) -> dict`；`Breakthrough.quote(ctx, selection: list[dict]) -> dict`；`Breakthrough.rework(ctx, selection: list[dict]) -> dict`。

- [ ] 写专注失败测试：每天前6个完整世界轮全渠道共享，一轮只推进冻结目标；核心/护道1、精华/原典2、奇珍6轮产1所选下品，切目标保留已确认余数；竞采不再发专注奖励。
- [ ] 写品质测试：BR04极品满料Q100；上品核心26；Q39/40/59/60/74/75/89/90/97/98/100档位准确；除数用阶段上限而非当前投入数。
- [ ] 测试十次最高快照总增益≤0.10，后续普通突破不抹旧快照；重炼只允许更高Q，完整替换辅材，不退款旧料、不扣整笔突破修为、不再天劫。
- [ ] 运行测试确认失败，实施源表境界门槛、可达安全区配方与未开放章节过滤；核心专注所得不补收旧成本。
- [ ] 运行基础世界+专注测试，核对六轮预算互斥，确保不会同时发6核心、3精华和1奇珍。

## T3：公库材料与设施

**Files:** Create `Tools/CultivationService/construction.py`；Test `tests/test_construction.py`；Modify `crafting.py`、`guilds.py`。

**Interfaces:** `Construction.craft_public(ctx, recipe_id: str) -> dict`；`Construction.build(ctx, building_id: str, target_level: int) -> dict`。

- [ ] 先写测试：四类公库配方每日合计5份；主播个人绑定输入扣除后直接入本武灵公库，跨武灵owner伪造拒绝。
- [ ] 测试8设施×3级；增量资材20/60/120，两类公库材料各2/6/12；大殿基础0免费；同目标级并发仅成功一次，不能跳级。
- [ ] 运行测试确认失败，实施支出审计及外显解锁；设施不加战斗/离线产出，不限制基础安全区炼制。
- [ ] 验证退会、解散不向个人退款/提款，组织改名不刷新限次；按事务前后检查全部所有者余额。

## T4：七功法、装备法宝与道具实际效果

**Files:** Create `Tools/CultivationService/loadouts.py`、`effects.py`；Create `Source/BoundlessImmortality/Wuling/WulingCombatRules.h/.cpp`；Modify `LiveWeapons/LiveWeaponCombatModel.h/.cpp`、`LiveWeapons/LiveWeaponSwarm.cpp`、`Cultivation/CultivationHealthComponent.h/.cpp`；Test `tests/test_loadouts.py`、`test_effects.py`、`Wuling/Tests/WulingCombatRulesTests.cpp`。

**Interfaces:** `Loadouts.equip(ctx, slot: str, item_instance_id: str) -> dict`；`Loadouts.study(ctx, method_id: str, progress_instance_id: str) -> dict`；`Effects.activate(ctx, effect_key: str, target_id: str|None) -> dict`；C++ `WulingCombatRules::Resolve(const FWulingCombatInput&) -> FWulingCombatResult`，新结构在WulingCombatRules.h定义，只在新规则模式应用。

- [ ] 写测试：练气1主2辅，其后按08境界解锁至6辅，同功法不可重复；研究升级成本1/2/3/4/6；3/6级主系数×1.10/1.20，持续/次数/耗血不同比放大。
- [ ] 参数测试覆盖疾锋第5击1.8、星罗20秒/6秒+20%、玄罡30秒/6秒减伤20%、回响第4击0.5延迟、合契标记20秒+15%、燃脉当前血10%换6秒+25%、百蚀0.05×最多5层5秒。追加伤害标签不得递归触发自身。
- [ ] 测试四非武器槽+一法宝，冻结开局配置，安全准备才能换；新属性攻击20%/防御25%/生命25%/移速10%上限，旧付费权益不截断；标准竞采不使用这些加成。
- [ ] 测试12法宝和20道具按各effect_id真实生效、无目标不扣物/冷却，法宝卸装冷却持久；道具单种日2、总日6；负面tag未实现时明确不可用，不能返回已成功。
- [ ] 运行失败测试后实现服务语义及C++解析应用，不允许只改变面板数字；回归实际扣血、屏障吸收、延迟伤害和攻击0.2秒下限。方法/法宝冷却跨重连不刷新。
- [ ] 运行服务测试及原生自动化；渲染PIE采样每种机制至少一次，若某effect缺前置tag，交付明确该项未完成，不能将配置加载当效果实现。

## T5：天劫全状态与唯一随机结算

**Files:** Create `Tools/CultivationService/tribulation.py`；Test `tests/test_tribulation.py`；Modify `schema.sql`、`authority.py`、`growth.py`、`inventory.py`。

**Interfaces:** `Tribulation.prepare(ctx, selection: list[dict], readiness: dict) -> dict`；`Tribulation.assist(ctx, attempt_id: str, event_id: str) -> dict`；`Tribulation.cancel(ctx, attempt_id: str) -> dict`；`Tribulation.advance(db: Connection, now: float) -> list[dict]`。readiness来自权威战斗状态；不接受UE自报健康/安全条件为既定事实。

- [ ] 先写测试：修为足够、≥50%血量、安全区、战斗后15秒、无其他活动、材料至少剩83秒；准备15、试炼60（三段各20）、揭示8；仅Prepare可无损取消。
- [ ] 测试相同app/attempt/user助力只一次，重复event去重；正式/临时均可，主播不可自助；第51人有反馈但不涨概率；准备前/截止后/旧代次拒绝，离场不扣已确认概率。
- [ ] 整数边界测试：5000基点时roll4999成功/5000失败；10000时roll9999成功；连续5失败后第6次保证；礼物、品质、道具不改变P；随机只由服务生成，客户端roll/helpers字段拒绝。
- [ ] 测试失败释放全部成功修为预留后扣min(1500,unspent)，境界不变、原实例数不增加；成功一次扣修/扣材、提升境界、写Q、清保底；同attempt重试/restart不重抽，retry至少60秒且修为重新满足。
- [ ] 运行失败测试后实现attempt/helpers/pity持久表、冻结roll与结果事务、故障对账。Trial掉线不停表、不产生世界奖励或专注进度，基础在线修为照常且不重叠离线。
- [ ] 用实际数据库提交失败和提交后丢响应注入验证恢复；材料跨故障到期进入Reconcile，待原事务核查，不延时铸物。原礼物收据/期限哈希不变。
- [ ] 服务测试通过后执行功能计划T3预警租约及渲染PIE，覆盖取消、成功、失败、切房、卸载、实例销毁和对账完成。
