# panel-activities handoff

**团队状态：** 独立源文件与候选素材已准备，可交 `logic-integration` 排入受控引擎批次。没有写 Content，没有改公共路由文件；engine lease 当前仍未授予。

## 产物

- C++：`Source/BoundlessImmortality/Frontend/UiRollout20261024/Activities/InkActivityRolloutPanels.h/.cpp`
  - `UInkActivitiesRolloutPanel`
  - `UInkActivityCycleRolloutPanel`
  - `UInkDailyActivitiesRolloutPanel`
  - `UInkLimitedActivitiesRolloutPanel`
  - `UInkPermanentActivitiesRolloutPanel`
- C++：`Source/BoundlessImmortality/Frontend/UiRollout20261024/Activities/InkLaunchHudCountdownWidget.h/.cpp`
  - `UInkLaunchHudCountdownWidget::Configure(UInkFrontendWidget*)`
  - `FrameTexture` 从隔离路径 `/Game/UI/Ink/ComponentRollout20261024/Activities/T_HudLaunchCountdownFrame_v1` 读取；缺少资产时不会伪装为已加载。
- GameUI 候选：`panel-activities/assets/hud-launch-countdown-frame-neutral-v1.png`，提示词 `panel-activities/prompts/components/hud-launch-countdown-frame.md`。
- 清单/脚本：`panel-activities/component-contract.yaml`、`asset-manifest.yaml`、`import_activity_components.py`、`inspect_components.py`、`qa/component-measurements.json`、`qa/hud-launch-countdown-nine-slice-preview.jpg`。

## 逐页覆盖与接入状态

| 页面/区域 | 现有业务复用 | 新组件/类 | 公共接入点 | 状态 |
|---|---|---|---|---|
| 活动总览 | `WulingFrontendData::Activity()`；活动服务状态与当前各分类 | `UInkActivitiesRolloutPanel`：四个 88 高分类控件、权威状态卡、模板卡、单态空卡 | `UInkFeaturePanel::ClassForRoute(Activities)` 改为新类；保持 `Open_LaunchEvent` 路由 | source-ready；route-wired=false |
| 日常 | `activities.templates[]` 精确 `cycle=daily` | `UInkDailyActivitiesRolloutPanel`；共用 cycle renderer | `DailyActivities` route 映射新类；详情栈回退沿用 owner | source-ready；route-wired=false |
| 限定 | 精确 `cycle=limited`，只呈服务 `start_at/end_at` | `UInkLimitedActivitiesRolloutPanel` | `TimedActivities` route 映射新类 | source-ready；route-wired=false |
| 常驻 | 既有 `UInkPermanentActivitiesPanel` 的服务分类/参与/领取字段与状态展示 | `UInkPermanentActivitiesRolloutPanel` 只显示明确 `cycle=permanent` 模板 | `PermanentActivities` route 映射前需对比两类接口并保留旧实例态，不能丢参与/领取展示 | source-ready；route-wired=false；integration must compare |
| 开服总览 | `UInkLaunchEventPanel` 的活动/资格/期限/授权动作 | 活动总览内开服入口；开服页复用现有类 | 保留 `LaunchEvent` | existing source retained; import/build/PIE not reverified |
| 每日任务 | `UInkLaunchEventPanel` `tasks` tab，读取权威 `launch.tasks[]` | 复用现有任务卡/状态/请求逻辑，不做样例行 | LaunchEvent → tasks tab | existing source; this batch not runtime-verified |
| 贡献榜 | `UInkLaunchEventPanel` `honors` tab 和 `LaunchRank` 读取服务榜单 | 复用现有并列/未上榜/期限处理 | 保留 `LaunchRank` | existing source; this batch not runtime-verified |
| 特惠入口 | 未找到可用 SKU、价格或购买接口 | 活动总览开服分类下显示禁用控件与原因，不发请求 | 暂无商品详情路由；需 authority SKU/价格/确认/收据 contract 才能启用 | UI source-ready; intentionally disabled |
| HUD 倒计时 | `UWulingPrototypeSubsystem::Snapshot()` 的 `manifest.profile`, `server_utc`, `launch.status/end` | `UInkLaunchHudCountdownWidget` + 无字透明 PNG | HUD layout owner 创建小组件、设边界 200×80、`Configure(frontend)`、加到现有右上入口组并绑定纹理；挂载文件由 integration 独占编辑 | source/PNG ready; import and HUD mount pending |

## 实际接口

- `UInkActivitiesRolloutPanel` action IDs: `ActivityCycle_daily`, `ActivityCycle_limited`, `ActivityCycle_permanent`, `ActivityCycle_launch`, `Open_LaunchEvent`, `OfferUnavailable`。
- 周期面板从 `WulingFrontendData::Activity(this)` 读取 JSON `activities.templates[]`；字段只渲染 `name`, `activity_id`, `enabled`, `start_at`, `end_at`。分类不可用、连接中、读取中、错误与 ready-empty 按单一互斥状态顺序处理。没有本地进度、奖励、领取、期限判定或假行。
- `UInkLaunchHudCountdownWidget::NativeTick` 每秒只刷新 TextBlock；用新服务 `server_utc` 重新锚定单调时钟。显隐要求 profile 为 `prototype_full`、launch 状态为 `Active`、deadline 有效且未过期。断线时仅基于缓存快照标记“上次确认”；无红点。
- 特惠控件当前固定禁用，因为当前源码没有经确认的 SKU/价格/订单字段和支付入口。

## 集成建议与回读标准

1. integration 先审阅本 handoff/status 和目标路径，获 lease 后在公共路由文件记录前置哈希。
2. 运行 `panel-activities/import_activity_components.py --source <本队 PNG>` 于 UE Editor Python；导入目标唯一为 `/Game/UI/Ink/ComponentRollout20261024/Activities/T_HudLaunchCountdownFrame_v1`，存在目标时脚本拒绝覆盖。回读 Asset Registry/Texture2D 尺寸、压缩、sRGB、alpha 与实际引用。
3. 在独占阶段添加 `InkActivityRolloutPanels.h` include 与三类 route 映射；将 `Activities` route 指向 `UInkActivitiesRolloutPanel`。对 `PermanentActivities` 先保留旧类功能字段，再选新壳或组合两个读数，防止功能退化。
4. 创建/挂载 HUD 控件到既有右上活动组，尺寸遵循安全区和角色保护区；纹理设置为新候选图。不能移除或重排既有活动/签到/仙途。
5. 构建后分别验证 authority snapshot 状态切换、分类互斥、无配置空态、日期来源、活动失效、断线 last-confirmed 和 disabled offer；PIE 检查点击路由与返回栈。空态逻辑只能证明空态，不代表有数据业务通过。

## 已做验证 / 当前限制

- PNG：2172×724、RGBA、alpha extrema `(0,255)`、alpha bbox `(13,21)-(2162,698)`、SHA-256 `5ED2166752C6E891874CA9318D1BA74887D62A8DB32CBD15BCCD7541AE4B9985`。
- 源像素 9-slice margins `[140,140,90,90]`，按 200×80 / 280×80 逻辑尺寸生成对照预览；本地几何预览通过，UE UMG Border/Texture import preview 仍待 integration。
- 候选页面 `activity-state-correction-3` 仍 `approved:false`。本组件也 `approved:false`，只记录本轮用户授权的试作。
- 未运行 Unreal、UBT、自动化、PIE、Cook、设备或平台验收；`import_verified=false`、`route_wired=false`、`logic_verified=false`。
