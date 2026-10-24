# LogicJourney 契约（候选试作，approved:false）

用户已授权本批连续试作、组件接入与逻辑验证；无需重复流程批准。视觉仍由用户批准。

源码：`Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.{h,cpp}`。
不要求修改公共头，不创建第二套子系统。实际权威为 `UWulingPrototypeSubsystem` 与 `Tools/CultivationService`。

## 消费方式

包含 `Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h`。
用 `Capture(Service, bActivePage)` 获取上下文，再 `Read(Context)`。bActivePage 需同时核实前台顶层路由与分组内所选子页；嵌入 SignIn/Activities 组的子页不能只用 CurrentPage==PageId 判定。
`FView` 原样保留真实 Activities/Journey/Launch 对象。不得自行补价格、奖励、排行、日期、进度。
`FRows.bProvided=false` 表示缺失或损坏；true 且 Items 为空才是权威空态。Signed/MonthSignCount 使用 TOptional，不把缺失画成 false/0。
`ActivitiesForCycle` 只支持 daily/limited/permanent；显式分类优先，否则按模板 cycle 精确过滤，不把限时推断为常驻。

控件可用性使用 `Evaluate`，动作使用 `Submit` 再取一次当前快照。`FRequest.ExpectedDay` 在打开签详情时保存服务 day，跨日需重新查看；旅行保存 EntryId；确认后才置 bConfirmed。
页面关闭时 bActivePage=false；离线缓存可展示但不能提交。Submit=true 仅表示入队；现有服务 request_id/outbox/receipt 继续拥有幂等和到账确认。按钮动画不产生奖励。
响应错误文字消费 Service->GetStatus()；服务没有暴露结构化 error code 给当前 C++ 界面，适配器不猜测本地化字符串。UI 仍需在失败后展示真实状态并保留选择。

| 页面/路由 | 真实数据 | 动作与边界 |
|---|---|---|
| WorldMap / JourneyWorldDetail | journey.worlds；id/open/rewards | WorldTravel → StartWorldRound(ENC_SOLO,guard,id)，必须确认与真实开放行 |
| ResourceMap / JourneyResourceDetail | journey.resources；node_id/owners/quantity/interval_seconds/lease_max_seconds | ResourceTravel → QueueRace(node_id)，不能从离线示意图授权 |
| SignIn | activities.day/signed/templates/weekly_chests | SignIn → SignIn()；ACT_SIGN daily enabled；本周奖励是服务标注的占位，无经济发奖 |
| MonthlySignIn | day/signed_days/month_sign_count/monthly_chests | MonthlyChest → ClaimSignInChest；3/7/14/21，eligible=true 且 claimed=false，需先详情确认 |
| DailyFortune | activities.day/verse | 固定轮换剑诀；没有随机吉凶、改签、付费或领奖接口 |
| DailyActivities / TimedActivities / PermanentActivities | ActivitiesForCycle + 原始模板字段 | 沿用现有真实可用入口；缺进度/期限/领取接口时展示待同步/禁用，不新增万能 Claim |
| LaunchEvent / LaunchRank | launch/tasks/milestones/personal/leaderboard/permissions；server_utc | StartLaunch → StartLaunchEvent；prototype_full、所属武灵主播、launch 明确 null、未过授权期限、需确认；自动奖励不造领取动作 |

## 现状差异与接入待办

- interaction.md 签到段落仍写“每日300修为”，实际 `Activities.sign_in` 明确无经济发奖，snapshot sign_in_qi_reward=0；本批以服务为准，未修改旧规则文档或奖励规则。
- 本轮已将公共 `InkFeaturePanel.cpp::HandleAction(SignInClaim)` 改为本适配器统一门控，日/月签宝箱与旅行确认也重验当时选中的服务日期/地点。公共文件原哈希与逐文件备份、有效增量 diff 已登记。
- 源码路由/WidgetTree 已引用交付组件；引擎未运行，仍不能声称运行路由、新组件渲染或玩家入口已通过。逐屏实际范围见 screen-coverage.md，不把纸板替换当整页重制完成。
- 已保存 WBP 的父类/WidgetTree 与 ClassForRoute 回退需接收四面板队交接后逐一迁移、保存、回读；不能仅改 C++ fallback 就断言保存 WBP 生效。
- 日期/奖励/开服期限沿用服务时间；无数据时不生成本机日期或倒计时。

## 覆盖与续队登记

本次独立适配覆盖地图/签到/每日剑诀/月签与活动数据契约，未覆盖屏幕必须继续登记：world-result、race-lobby/live/result、lease、resource-rank/rewards、offline/shop/market/orders、saves/settings、item-sources/craft/appearance/guild-build/equipment/methods 等须按 screen-map 与其他队源码核对，不能由一张候选整页贴图宣称44屏完成。
即使本文件列出的路由也须完成新壳、真实导入引用、玩家入口、非空数据与失败动作验证后才能 route_wired/logic_verified=true。

## 验证证据与命令

- `run_authority_tests.py`：实际 Python 权威服务18项测试通过；含非空地图、租约过期、跨日、防重复签到、月签门槛/领取/重放、旧外观账本、开服贡献与奖励权限。
- 初次原命令因 TemporaryDirectory 0700 的 Windows ACL 导致 SQLite 建库前失败（authority-tests.log）；隔离 runner 只替换临时目录分配，继承本队目录 ACL；不替换业务、断言或服务实现，最终结果见 authority-test-result.json。
- UE 测试 `Boundless.InkUI.Rollout20261024.Journey.Guards` 与 `.Routes` 已编写，因 engine lease 尚未授予，未构建/执行。独立门控测试先于实现落盘，但没有红绿运行证据；不得声称 TDD/UE逻辑通过。
- `preflight_batch.py`：只读取依赖/status/lease/哈希，不启动引擎。运行结果不是 GameUI 资产严格校验，也不是 Unreal验收。

```powershell
& 'C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe' specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/run_authority_tests.py
& 'C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe' specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/preflight_batch.py
```
