# 活动组件分组：独立工作与接入契约

## 本批边界

- 候选参考只使用 `revisions/2026-10-23-activity-state-correction-3` 和 `revisions/2026-10-23-parallel-activities`，不把任何页面图当运行时整页贴图。
- 活动页模板数据来自 `WulingFrontendData::Activity(UUserWidget*)`，其实现只在活动服务启用且连接时读取 `UWulingPrototypeSubsystem::Snapshot().activities`。
- 活动周期只按模板 `cycle` 精确值匹配 `daily`、`limited`、`permanent`。未知周期不归类，不混在空态里。
- 开服状态、任务、贡献榜、截止、报名/开启和既有主操作继续由 `UInkLaunchEventPanel` 处理。活动小队没有改动其公共源码。
- 当前检索未找到开服特惠 SKU/价格消费合同；候选活动壳显示“SKU 与价格快照未提供”，CTA 为禁用态，没有购买请求。

## 新类和调用接口

- `Frontend/UiRollout20261024/Activities/InkActivityRolloutPanels.h`
  - `UInkActivitiesRolloutPanel`: 四分类总览壳。内置 `SelectedCycle`；UI action IDs 为 `ActivityCycle_daily`、`ActivityCycle_limited`、`ActivityCycle_permanent`、`ActivityCycle_launch`、`Open_LaunchEvent`、`OfferUnavailable`。
  - `UInkActivityCycleRolloutPanel`: 单周期只读卡片 renderer；抽象 `ActivityCycle()` 由三个子类固定周期。
  - `UInkDailyActivitiesRolloutPanel`、`UInkLimitedActivitiesRolloutPanel`、`UInkPermanentActivitiesRolloutPanel`: 指定周期分类面板。
- `Frontend/UiRollout20261024/Activities/InkLaunchHudCountdownWidget.h`
  - `UInkLaunchHudCountdownWidget::Configure(UInkFrontendWidget*)` 注入宿主路由。
  - 可在 Widget Blueprint 设置 `FrameTexture`；`NativeTick` 每秒读取服务快照，不重建 HUD。只在 `prototype_full`、状态 `Active`、服务时间和活动截止有效且未过期时显示。断线时标记“上次确认”；不造红点。
  - 点击调用 `UInkFrontendWidget::OpenFeaturePanel(LaunchEvent, "")`。
- 组件使用既有 `UInkFeaturePanel::Surface/Label/Action/Open/RebuildPanel`，由 UE `TextBlock` 实时绘字，无新增公共基类修改。

## 页面覆盖

| 页面/区域 | 现有业务复用 | 本批新组件/类 | 接入点 | 状态 |
|---|---|---|---|---|
| 活动总览 | `WulingFrontendData::Activity`; 现有活动组路由与子页 | 四分类选择器、权威状态卡、活动模板卡、单独空态；`UInkActivitiesRolloutPanel` | `UInkFeaturePanel::ClassForRoute(Activities)` 需由唯一接入队改为该类；源码路径 `Frontend/UiRollout20261024/Activities` | source candidate; route unwired |
| 日常活动 | `cycle=daily` 权威模板，现有详情 route 可保留 | `UInkDailyActivitiesRolloutPanel` / 共用配置卡与空态 | `DailyActivities` 路由类映射；如详情动作需保留由 integration 再接原 detail panel | source candidate; route unwired |
| 限定活动 | `cycle=limited` 权威模板；截止只显示 `start_at/end_at` 服务字段 | `UInkLimitedActivitiesRolloutPanel` | `TimedActivities` 路由类映射 | source candidate; route unwired |
| 常驻活动 | 现有 `UInkPermanentActivitiesPanel` 只列明确 permanent 项、权限和服务状态 | 新 `UInkPermanentActivitiesRolloutPanel` 配置外壳；原面板更丰富的活动实例展示继续作为既有实现 | 唯一接入队核对保留字段后选择新壳或旧面板；不得覆写或丢失已有参与/领取字段 | source candidate; route unwired; existing class retained |
| 开服活动 | `UInkLaunchEventPanel` 的状态、服务器时间、任务、目标、报名/开启 action 与已有测试 | 复用其 4 标签；活动总览增加开服入口和明确禁用的特惠入口 | LaunchEvent/LaunchRank 原路由保持；特惠入口暂禁用，须先有合法 SKU/价格、订单/收据接口再启用 | existing logic; offer entry disabled; integration pending |
| 每日任务 | `UInkLaunchEventPanel` `tasks` tab 的权威 `launch.tasks[]` | 复用现有任务卡，不重复写一份模拟数据 | `LaunchEvent` → `tasks` tab | existing source; runtime integration not reverified |
| 贡献榜 | `UInkLaunchEventPanel` `honors` tab 与 `LaunchRank` 复用真实 `leaderboard` 快照 | 复用权威行、并列和未上榜状态 | `LaunchEvent` → `honors`; `LaunchRank` route | existing source; runtime integration not reverified |
| 特惠入口 | 无 SKU/价格/购买协议字段发现 | 单独特惠说明与禁用控件，不发起动作 | 活动总览“开服”分类底部；只有 integration 后续核实采购 contract 才能改 | source candidate; disabled by design |
| HUD 开服倒计时 | `UWulingPrototypeSubsystem::Snapshot` 的 `launch.status/end` 和 `server_utc` | `UInkLaunchHudCountdownWidget` + 无字透明倒计时底板 | HUD Owner 创建控件、设置 FrameTexture、Configure(owner)、按原入口组尺寸挂载；HUD 公共编辑仅唯一接入队进行 | source + PNG candidate; texture import and HUD mount pending |

## 资产与 9-slice

- `hud-launch-countdown-frame-neutral-v1.png`：2172×724 RGBA，alpha extrema `[0,255]`，transparent pixel count 601278，opaque 1920，partial 969330；SHA-256=`5ED2166752C6E891874CA9318D1BA74887D62A8DB32CBD15BCCD7541AE4B9985`。
- 9-slice margins（源像素，顺序 left/right/top/bottom）=`[140,140,90,90]`。隔离脚本按 200×80 与 280×80 目标进行了算法预览；轮廓和中心可拉伸区均保持连续。Unreal Texture 导入、UMG Border/Brush margins 的实机拉伸仍需 integration 检查。视觉整体留待用户审批，`approved:false`。
- 内置 GameUI exporter 不支持 Unreal。`import_activity_components.py` 是 Editor Python 导入任务，不等于已导入。本队没有运行 Editor、UBT 或导入脚本。

## 验证边界

- 已检查 PNG 像素尺寸、模式、alpha、alpha bbox、sha256 和 9-slice 预览。
- 新 UMG 类依赖真实 subsystem snapshot；源码接口已接入，但无 lease 时不能编译、注册到公共路由、挂载 HUD 或运行逻辑验收。
- `UInkLaunchEventPanel` 当前源代码含状态/任务/贡献榜/服务时间处理，现有自动化断言仅是既有源码证据，不代表本批构建/PIE 通过；本批没有新增或运行测试。
