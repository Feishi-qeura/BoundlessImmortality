# Journey 页面覆盖与拼装接口

状态：候选壳和组件已准备；视觉审核待用户；引擎导入、路由接入、PIE 尚未完成。当前主 checkout 存在其他队已授权修改，本队未改共享源码、根级契约或 Content 目录。

| 页面 | 现有业务/页面复用 | 本队新组件/类 | 接入点与数据 | 剩余状态 |
|---|---|---|---|---|
| 世界地图 `world` | `UInkJourneyMapPanel`、`UInkJourneyDestinationPanel`、`InkJourneyData::Rows/Find/Ready`；世界路线仍调用现有 `StartWorldRound` | `world-node-explored/available/locked` + `UJourneyStateBadgeWidget` | `WorldVm` / `journey.worlds[]` 的区域 id、位置与显式服务状态；节点点选转为 `OnGlyphPressed(State)`，外部按已选区域进入已有详情页 | importer 与组件替换要 engine lease；横屏截图及真实路由待接入执行者 |
| 资源地图 `resources` | 同一地图/详情类；复用 `journey.resources[]`、`LeaseVm`、`RaceVm` 与现有 `QueueRace` 路径 | `resource-pin-free/temporary/contest-open/expired` + `UJourneyStateBadgeWidget` | 服务节点类型/有效期/竞赛可用性；选中发 `OnGlyphPressed(State)`；参与操作继续由已有服务权限和队列规则判定 | 缺少类型/过期/竞赛字段时显示 unknown/禁用；真实引用和竞赛端到端待接入队 |
| 每日签到 `sign-in` | `UInkSignInGroupPanel` → `UInkSignInPanel`；复用 `activities.signed/day`、`templates[]`、`weekly_chests[]`，领取仍调用现有 `SignIn()`/`HandleSignInChestAction` 门控 | `UJourneyClaimStateWidget`：`Unknown/Claimable/Pending/Claimed/Disconnected/Unavailable/Error` 互斥状态 | 外层 adapter 设置单一 claim state；`OnClaimRequested` 由 adapter 调现有 SignIn 路径；本控件只准许 Claimable 发一次事件，事件后即禁用，等服务回填 | 不连接服务时不得预置 Claimable；实际壳迁移/重复提交运行验证待 lease |
| 每日一签 `daily-fortune` | 源图的一签区域拆成独立只读语义；现有 `UInkDailyFortunePanel` 当前标题为“每日剑诀”，只读 `activities.verse/day/signed`，**尚未消费 `daily_fortune`** | 四枚 `fortune-great/good/neutral/bad` + `UJourneyStateBadgeWidget`；FortuneSeal 家族强制禁用点击，不提供抽签/重抽 callback | 必须由服务 fortune 值精确映射；缺值只显示未同步，不随机、不拿 verse/signed 推导吉凶 | 当前 route 语义与候选签运页不同，logic-integration 需把“每日剑诀”与“一签”路由/标签对齐；真实 fortune provider 缺失/待核验 |
| 月签到 `monthly-sign-in` | `UInkMonthlySignInPanel`、`BuildSignInChestStrip(true,...)`、现有里程碑 `3/7/14/21` 与 `ClaimSignInChest` 权限检查 | `UJourneyCalendarDayWidget`：服务器状态 `Unknown/Signed/TodayClaimable/Future/Missed`，数字与状态字均是运行时 TextBlock | Adapter 只用服务月份、day、`signed_days` 与明确 claimable days 设置日期；点击仅 `OnDayPressed(day)` 打开详情，不自动领取 | 现有代码用服务日期生成月份网格、缺少 `missed` 和 `claimable_days`；须由服务字段或业务 adapter 明确映射，unknown 不可猜漏签；没做月份/闰年 PIE |

## 组件 API

- 头文件：`Source/BoundlessImmortality/Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.h`
- 源文件：`Source/BoundlessImmortality/Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.cpp`
- `UJourneyStateBadgeWidget::SetGlyphState(EJourneyGlyphFamily, FName)` 只接受 manifest 列出的状态名；`SetCaption(FText)` 单独控制动态文字；`OnGlyphPressed(FName)` 用于地图选点。`FortuneSeal` 不可点。
- `UJourneyCalendarDayWidget::SetDayState(int32, EJourneyCalendarState, bool)` 由服务 VM 设置单格状态；`OnDayPressed(int32)` 仅选日/开详情。
- `UJourneyClaimStateWidget::SetClaimState(EDailySignInClaimState, FText)` 接受一个互斥状态；`OnClaimRequested()` 只在 Claimable 发一次。控件不持有 GameInstance、不调用网络/子系统、不自行确认成功。
- 所有图像路径期待资产落在 `/Game/UI/Ink/ComponentRollout20261024/Journey/`；父级现有 route panel 保持文本和业务行为所有权。

## 数据字段审计

- 现有 `WulingFrontendData::Activity()` 只读取已启用、已连接且有 Snapshot 的 `activities` 对象；已看到使用的字段有 `day`、`signed`、`month_sign_count`、`signed_days`、`weekly_chests[]`、`monthly_chests[]`、以及活动 `templates[]` 中 `activity_id=ACT_SIGN/cycle=daily/enabled`。
- 月签到 chest 使用 `milestone/claimed/eligible/reward/placeholder`；UI 已限制支持 3、7、14、21，权威结果经 `SubmitPrototypeCommand("ClaimSignInChest", ...)`。
- 当前活动 snapshot 代码没有读取 `daily_fortune`、每日日签结果解释、`claimable_days[]` 或月签 `missed`。这些只能等真实适配字段，不能从本机日期/签文/缺失记录推导。
- `InkJourneyData` 离线 atlas 仅提供地理浏览样例；离线、缺区域行或字段缺失均不能变成开放、可前往、租约或产出。

## 契约边界

- 候选设计 `daily-sign-in-v2-candidate.png` 混排每日签到、每日一签、七日节点与月签入口；它只做布局/绘制参考，不作为整页运行图导入。拆为每日签到、只读每日一签、月签到三个独立功能面板。
- 原图与本次新图均保持 `approved:false`；游戏 UI 的动态文案不从 PNG 读。
- 8 个图像按钮/图标都是 `full/contain`，没有 9-slice 拉伸区域；可伸缩纸底/卡片复用现有 `InkPresentationStyle` 主题，不从新图拼版。
