# v1.4 竞采、开采、活动和原型经济实施计划

**执行记录（2026-10-22）：** 已实现真实队伍竞采、五类UTC租约、签到/月度/周模板、青云初启、商店寄售和本地只读审计。两实际UE进程与两HTTP进程有独立通过证据；实施/边界见 `docs/2026-10-22-wuling-v14-implementation.md`。下方保留原细分验收步骤；开服原始30项仅9项有完整直接断言，未将整个矩阵标通过。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans after user review. Steps use checkbox (`- [ ]`) syntax for tracking. Do not dispatch agents without explicit authorization.

**Goal:** 实现真实多实例竞采、限时开采权、日周月及开服活动，并在封闭原型验证商店/白名单交易。

**Architecture:** 所有模块使用基础计划同一个事务库和权威事实，不以客户端计数或最终名次入账。竞采冻结模板和代表，租约按实际有效在线区间结算，活动订阅已提交的世界/互动事实，市场独立托管但不另建货币体系。

**Tech Stack:** Python/SQLite、unittest真实并发连接、两个独立UE实例、既有功能页。

**Spec:** 主计划；v1.4的02、03、05–08、14和对应CSV/JSON。

## Global Constraints

- 主计划约束全部适用；依赖基础T1–T3和物品T1。发布候选仍关闭竞采、商店、交易、新增跨日数值与开服活动。
- 竞采2–4主播，120准备+360挑战+120结算；最多10正式代表；共同600/主播200/成员200；最低600有租约。
- 不同分数组排名给30/20/10分钟，同分同档；单武灵同时仅一Active开采权。
- 自然日签到300修为一次；开服七任务一次性总780；1/3/6/12轮主播总200灵石+60破境石；榜分不兑换数值。
- 商店/市场仅封闭模拟身份；白名单只有MAT_HERB_T/MAT_IRON_T，新物品和付费来源禁售。

## Review Focus

- 无对手不补假主播，两个真实进程验证名单冻结，T1。
- 租约关播仍到期，零在线零产出，跨房间同人不双领，T2。
- 普通自然日与开服相对日分开，最后一轮必须完整落在活动窗内，T3–T4。
- 榜单相同分数用竞赛名次1/1/3，与竞采不同分数组排名区分，T4。
- 双买家和撤单并发只有一个终态、币物守恒，T5。

## T1：匹配、统一试炼与权威评分

**Files:** Create `Tools/CultivationService/races.py`、`matching.py`；Test `tests/test_races.py`、`test_matching.py`；Modify `schema.sql`、`authority.py`。

**Interfaces:** `Matching.queue(ctx, node_id: str) -> dict`；`Matching.cancel(ctx, queue_id: str) -> dict`；`Races.submit(ctx, race_id: str, task_id: str, action: str) -> dict`；`Races.advance(db: Connection, now: float) -> list[dict]`；`Races.settle(db: Connection, race_id: str, now: float) -> dict`。

- [ ] 先写失败测试：至少1正式成员，当前无世界/天劫/活跃租约，队伍按近3轮有效参与人数中位数分1–9/10–49/50+；新队伍标记；60秒可退出转世界，90秒不偷偷跨档。
- [ ] 测试2–4主播真实session，准备后冻结房间/规则hash/模板/代表，代表按本日次数再按稳定身份键；仅1人时分母2，不补机器人；临时助战不计代表分。
- [ ] 测试六个共同任务各100，两次主播时机各100，成员floor(200*responses/(2*代表数))，重复口令一次分、迟到拒绝；旧+200000礼物、境界、资源、装备均不改标准目标。
- [ ] 测试600分资格、全1000分并列30分钟、不同分数组30/20/10；600秒自动激活，不等待领取；共同任务成对完成才发对应三个阶段基础奖励，不叠一份世界轮。
- [ ] 运行测试确认失败；实施Queue/Matched/Ready/Running/Frozen/Settled及取消/掉线/审计终态，按09处理重连60秒宽限和全局服务故障，保留收据不伪造参与。
- [ ] 运行服务测试；随后用两个独立客户端进程完成同一race和重连，不以单进程模拟两个主播代替此验收。

## T2：五种开采权和有效在线结算

**Files:** Create `Tools/CultivationService/leases.py`；Test `tests/test_leases.py`；Modify `growth.py`、`races.py`。

**Interfaces:** `Leases.activate(ctx, race_id: str, guild_id: str, node_id: str, duration_seconds: int) -> dict`，仅赛事结算内部调用；`Leases.settle(db: Connection, lease_id: str, now: float) -> dict`；`Leases.benefit(ctx, now: float) -> dict`。

- [ ] 写失败测试：单武灵Active唯一，UTC start/end持久化，领取/重启/换页不重置；类型不能提前切换或预订第二份。
- [ ] 用60/599/600/1200秒和到期边界测试：石矿每600有效秒20石、草/铁每600秒2绑定、遗址每1200秒1绑定残页；不足整档无物品；暂停期间不推进在线秒数。
- [ ] 测试灵泉仅主播及同房正式成员在线+20%，临时/离线不享；资源贡献基础量与礼物放大后的最终修为分别记；窗口结束截断，不更改旧礼物时长。
- [ ] 测试A房撤销后旧心跳无收益，同一人不双领；掉线时间超过租约直接过期，不补离线资源；重启恢复原UTC。
- [ ] 运行测试确认失败后实施，复用基础参与游标，禁止接受客户端online_seconds。重复结算币物数一致，整轮资源产出满足nodes.csv上限。

## T3：签到、每日剑诀、月度与周模板

**Files:** Create `Tools/CultivationService/activities.py`；Test `tests/test_activities.py`；Modify `authority.py`、`schema.sql`、`world.py`。

**Interfaces:** `Activities.sign_in(ctx) -> dict`；`Activities.snapshot(ctx) -> dict`；`Activities.start(ctx, activity_id: str) -> dict`；`Activities.on_fact(ctx, fact: dict) -> None`。

- [ ] 写失败测试：香港00:00切日同身份300一次、两个UI入口同收据；每日剑诀固定文本轮换，月累计3/7/14/21只解锁外显，跨月不串历史领取。
- [ ] 测试七星护阵/玄铁破障复用世界预算或明确替换，不叠第三份奖励；未配置具体起止的限定活动disabled；静修只有修为，无世界材料。
- [ ] 测试通用活动开始时冻结规则，关闭只阻止新报名，已有轮按既定故障策略结算；不得将开服“完整轮落窗”特例套到所有通用活动。
- [ ] 运行测试确认失败后按activities.csv与05实施；日/周进度由权威事实推进，不由当前日期生成假已完成值。
- [ ] 运行跨日、跨周、跨月、断线重复领取测试；按功能计划接入签到/活动现有页面及真实未连接状态。

## T4：青云初启、称号、贡献榜与共修仪式

**Files:** Create `Tools/CultivationService/launch_event.py`；Test `tests/test_launch_event.py`；Modify `schema.sql`、`world.py`、`activities.py`、`growth.py`。

**Interfaces:** `LaunchEvent.start(ctx) -> dict`；`LaunchEvent.enroll(ctx) -> dict`；`LaunchEvent.on_fact(ctx, fact: dict) -> None`；`LaunchEvent.advance(db: Connection, now: float) -> list[dict]`；`LaunchEvent.leaderboard(ctx, cursor: str|None=None) -> dict`。

- [ ] 写失败测试：app/主播/launch_version只能开一次，start服务端产生，end=min(start+604800,window_end)，删建改名不重开；报名锁home_guild，加入前事实不追溯。
- [ ] 按launch_tasks测试L01–L07，个人三阶段事实才完整轮；4护阵/4破障各轮最多2；路线来自两个不同轮。总780仅观众一次，自动发放与按钮确认分开。
- [ ] 共同1/3/6/12轮需主播+至少1正式成员真实响应且完成；总200石/60破境石仅主播一次；第12轮自动1200秒仪式，修为加法0.10、剑伤0.10、主播防御0.10，同类取最高，临时/离线/竞采不享。
- [ ] 测试开服榜阶段2/3/5、正确响应2×3、路线2、免费参与2，每轮20，活动日最多前6轮120、7天840；错误但合法输入可算参与但不算正确响应。礼物/伤害/境界权重0。
- [ ] 测试≥30分且完整2轮入榜，1/1/3并列、并列第30全部荣誉；空缺不补假人；rank_numeric_rewards所有profile拒绝，disabled候选表不得进入发奖路径。
- [ ] 测试相对日按round_start桶、仅7桶；round.start≥activity.start且round.end≤activity.end；Closing60秒只接收已有可信事实；客户端回填occurred_at拒绝；称号全七任务且expires=min(grant+30天,window_end)。
- [ ] 运行失败测试后实施独立事实/任务/里程碑/榜分唯一表，权威事实提交与活动处理共事务或持久outbox防丢；重启、跨房、退会不增发。
- [ ] 执行策划launch-acceptance.csv30项，逐行生成运行状态与证据，不直接改原策划的not_run历史快照。

## T5：固定商店、白名单市场与币物守恒

**Files:** Create `Tools/CultivationService/shop.py`、`market.py`；Test `tests/test_shop.py`、`test_market.py`；Modify `schema.sql`、`authority.py`。

**Interfaces:** `Shop.purchase(ctx, sku: str) -> dict`；`Shop.recycle(ctx, instance_id: str, count: int) -> dict`；`Market.create_listing(ctx, instance_id: str, count: int, unit_price: int) -> dict`；`Market.buy(ctx, listing_id: str, expected_listing_version: int) -> dict`；`Market.cancel(ctx, listing_id: str, expected_listing_version: int) -> dict`；`Market.expire(db: Connection, now: float) -> None`。

- [ ] 先写profile深链接拒绝测试：release连SKU列表、挂单查询和写入均关闭；viewer不能买主播SKU或操作市场；仅两种免费交易草铁、付费污染及衍生禁售。
- [ ] 测试固定SKU日限购、同窗口徽记唯一、NPC回收日铸币50；贡献只用于明示外显，不能兑换战斗数值。购买失败物钱均不变。
- [ ] 市场测试每主播5单、数量1–100、草4–12/铁6–18整数单价、最长86400秒且不越资产到期、剩余不足3600不准挂牌；挂牌费1不退，税ceil(总价*0.05)，整单成交，不能自买。
- [ ] 并发测试两买家/买入对撤单仅一终态；提交后丢回包不重复扣款；卖家离线应收直接入账，领取不铸币；过期退货保留原期限，过期原物归档而不续期。
- [ ] 运行测试确认失败再实现托管账本及SQL版本检查；按07例子卖家10→9→85、买家100→20，系统余额110→105，物品总量不变。
- [ ] 运行真实数据库并发与事务故障测试，保存守恒明细；UE原型UI显式标本地模拟，不声称可公开启用。

## T6：审计与经营指标

**Files:** Create `Tools/CultivationService/metrics.py`；Test `tests/test_metrics.py`；Modify `schema.sql`、`authority.py`。

**Interfaces:** `Metrics.record(ctx, event_type: str, fact_id: str, dimensions: dict) -> None`；`Metrics.report(ctx, start_utc: float, end_utc: float) -> dict`。

- [ ] 测试参与率、完成率、回访、来源产出/消耗基于真实提交事实，重放不增；没有正式付费记录时金额字段unknown而非0收入或模拟变现。
- [ ] 实施最少必要本地审计导出，区分模拟/正式来源、规则和窗口，敏感身份不放直播画面；不发送任何外部分析。
- [ ] 测试免费/既有礼物分组只作记录，不改变排行榜和掉落；导出明确模拟规模，商业转化保持未验证。
