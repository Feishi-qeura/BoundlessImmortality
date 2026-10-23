# v1.4 基础、组织与世界实施计划

**执行记录（2026-10-22）：** 已实现本机权威服务、组织、成长及实际飞剑完整世界。当前文件与证据见 `docs/2026-10-22-wuling-v14-implementation.md`；下方保留原细分验收步骤，不代表原场景已全测。最终服务75项、项目原生79项通过；原始场景覆盖逐行见 `acceptance-report.json`。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans after user review. Steps use checkbox (`- [ ]`) syntax for tracking. Do not dispatch agents without explicit authorization.

**Goal:** 建立可运行的隔离权威服务，使现有武灵客户端完成身份、成员、成长、主播突破及十分钟世界历练闭环。

**Architecture:** Python标准库HTTP入口只适配现有UE协议；SQLite写事务负责身份版本、命令幂等、账本和阶段推进。规则加载、组织、成长、世界分模块；可注入测试时钟，生产入口只能使用服务端时钟。

**Tech Stack:** Python、sqlite3、unittest、http.server；UE5.8现有HTTP/Json。

**Spec:** 主计划`2026-10-21-gameplay-implementation.md`及v1.4的01、03、08、09、tables。

## Global Constraints

- 主计划所有约束适用；本计划不调整礼物与UI视觉。
- 标准1修为/秒，观众自动连升，主播消耗单阶修为的50%主动突破；顶层不再扣费。
- 所有写命令具有request_id、expected_version、profile、rule_version；身份只来自当前token。
- 错误响应统一request_id/status/error_code/authoritative_version/receipt_id/result；未知命令明确拒绝。
- 新数据库禁止混用旧原型数据库或旧成长JSON。新规则下UE本地成长只镜像，不再次发奖。

## Review Focus

- 已提交命令重试携带旧版本仍取原收据，T1。
- 两进程同时加入不同武灵只能建立一份正式关系，T2。
- 无离场信号不能宣称真实在线；心跳过期停止收益，T2。
- 离线超过8小时后游标推进全部区间，不能分批套取余时，T2。
- 跨阶段、跨午夜、进出菜单不重复结算，T3。

## T1：服务协议、规则加载与事务基础

**Files:** Create `Tools/CultivationService/{__init__.py,__main__.py,http_server.py,authority.py,store.py,rules.py,contracts.py,schema.sql,README.md}`；Test `Tools/CultivationService/tests/test_authority.py`、`test_http.py`、`test_rules.py`。

**Interfaces:** `RuleBundle.load(path: Path) -> RuleBundle`；`Authority(db: Path, rules: RuleBundle, clock: Callable[[], float])`；`open_session(body: dict) -> dict`；`sync(token: str, body: dict) -> dict`；`snapshot(token: str, cursor: int=0) -> dict`；`execute(token: str, command: dict) -> dict`。HTTP适配`POST /v1/wuling/{session,sync,state,command}`，字段兼容现有客户端。

- [ ] 先写失败测试：缺表/重复ID/字符串False/错误规则hash拒绝；release profile及真实身份标记拒绝；同request同payload仅一份收据，不同payload返回REQUEST_ID_CONFLICT。
- [ ] 写事务测试：提交后丢响应再用原request和旧expected_version重试，`receipt_id`相等、余额只改变一次；两个独立数据库连接争同version只一个成功。
- [ ] 运行`python -m unittest discover -s Tools/CultivationService/tests -p test_authority.py -v`并记录真实失败；实施规则类型转换、profile检查、`BEGIN IMMEDIATE`、唯一约束和收据优先查找。
- [ ] 定义主表：identity/session/window、guild/membership、growth、wallet_ledger、item_instance、event_round/fact/reward_receipt、command_receipt、counter、feature_manifest；唯一键按09，不以昵称索引身份。每次SQL迁移有schema_version。
- [ ] 实施仅loopback HTTP，限制JSON大小、拒绝NaN/负数/非整数数额及未知字段；模拟session明确development_only/simulated/profile，生产身份不由body声明。
- [ ] 运行对应HTTP和规则测试；真实启动临时DB服务，session→state→无权限command，验证200业务错误与401认证错误符合现存UE回调策略。保存日志和保护校验，确认无旧档写入。

## T2：成员、单写会话和34境界成长

**Files:** Create `Tools/CultivationService/guilds.py`、`growth.py`；Test `tests/test_guilds.py`、`test_growth.py`；Modify `authority.py`、`schema.sql`。

**Interfaces:** `Guilds.apply(ctx: CommandContext, operation: str, args: dict) -> dict`；`Growth.settle(ctx: SessionContext, now: float) -> dict`；`Growth.breakthrough(ctx: CommandContext, args: dict) -> dict`。CommandContext由T1定义，含事务连接、认证身份、scope、server_now、request_id、rules，调用者不能替换owner。

- [ ] 先写成员测试：创建免费且一主播一武灵；名称2至12汉字等效长度（ASCII按半字，向上取整），模拟审核只用于本地；重复加入不增奖励；双武灵并发只一份active；退出/移除86400秒后才能再加入，个人物品不变；改名7天限次；Running赛事时拒绝解散。
- [ ] 先写成长测试：realm_costs相邻差值装载；观众零材料自动连续升阶；主播修为阈值50%、小阶旧成本；无顶层循环；旧规则档不迁移。未来大阶命令委托物品子计划，不叠收费用。
- [ ] 写会话测试：A→B切房撤销A租约，A旧心跳不复活；菜单/未认证无收益；离线仅观众，`0.1*min(seconds,28800)`，第二次领取不保留超8小时尾数；香港午夜和夏令时无关。
- [ ] 运行对应测试确认失败；实施组织、服务端游标、参与代次、正式/临时身份独立计数。名册每页20，不创建场景里的离线成员。
- [ ] 实施CreateGuild/JoinGuild/LeaveGuild/RenameGuild/RemoveMember/DissolveGuild/Breakthrough；移除和解散只改变组织关系，公库不转个人。
- [ ] 运行T1和T2测试；重启同一DB继续成长游标和退出限制，保存断线恢复证据。

## T3：十分钟世界历练和阶段权威结算

**Files:** Create `Tools/CultivationService/world.py`、`combat.py`；Test `tests/test_world.py`、`test_combat.py`；Modify `authority.py`、`schema.sql`；UE接入见功能计划T1–T2。

**Interfaces:** `World.start(ctx: CommandContext, preset: str, route: str, focus: str|None=None) -> dict`；`World.submit(ctx: CommandContext, round_id: str, action: str, target_id: str|None=None) -> dict`；`World.advance(db: Connection, now: float) -> list[dict]`；`World.interrupt(ctx: CommandContext, round_id: str) -> dict`；`Combat.resolve_hit(snapshot: dict, target: dict, action: dict) -> int`。

- [ ] 先写时钟测试：标准轮准备0–60、阶段60–240/240–420/420–540、结算540–600；开服预设另用60+160×3+60，不能改通用模板；中断保留已完成阶段。
- [ ] 写事实测试：服务重启跨多个阶段不自动制造命中或参与，迟到行动拒绝；客户端提交amount/damage/score不能充当事实；同目标死亡与同阶段奖励各唯一。
- [ ] 写固定供给测试：完整轮主播80灵石/12石/5绑定草+1交易草/3绑定铁+1交易铁/2残页；个人额外60修为日限6；公共资材每正式成员1、成员日6、武灵日100；声望需2名真实参与者，每轮10日60。
- [ ] 测试0/5/20/80固定预设，不因局中人数或礼物改变；完整轮个人参与必须覆盖三阶段；静修、世界、竞采、天劫互斥。世界失败恢复30秒，无付费复活消耗。
- [ ] 运行测试确认失败，按encounters与world_rewards装载固定目标、动作窗口和奖励；服务基于规则核算伤害、实际扣血统计，排除溢出与重复死亡。
- [ ] 实施StartWorldRound/SubmitAction/InterruptWorldRound/ClaimSettlement，领取只确认显示；Advance可由服务定时器和请求入口调用，均经同一事务防重。
- [ ] 运行T1–T3全套、真实HTTP完整轮及中断重启；再按功能计划完成渲染PIE，未经该检查不标P1完成。
