# panel-journey handoff

独立交付已完成，等 `logic-integration` engine lease 做实际导入/路由/逻辑接入。全部候选 `approved:false`。

## 页面覆盖

| 页面 | 现有业务壳复用 | 新组件 | 接入点 | 余项 |
|---|---|---|---|---|
| 世界地图 | `UInkJourneyMapPanel` / `UInkJourneyDestinationPanel`；`InkJourneyData`；`StartWorldRound` | 三个世界节点状态 PNG；`UJourneyStateBadgeWidget` | `WorldVm` 或 `journey.worlds[]` 的位置、id、显式状态；`OnGlyphPressed(State)` 回选中节点 | 导入、控件替换/绑定、实际路由/PIE |
| 资源地图 | 同一地图/详情类；`LeaseVm` / `RaceVm`；已有 `QueueRace` | 四类资源 pin PNG；`UJourneyStateBadgeWidget` | 资源类型、临时点有效期、竞赛权限/开关；事件只选中节点，服务端继续决定资格 | 资源类型/期限映射，导入接入和真实竞赛验证 |
| 每日签到 | `UInkSignInGroupPanel` → `UInkSignInPanel`；当前 `SignIn()` 与 chest 权限门控 | `UJourneyClaimStateWidget` | 设置单一 `EDailySignInClaimState`；`OnClaimRequested` 接回现有权威 action | 仅 Claimable 可发一次事件；实际订阅、异步回写与重复点击 PIE |
| 每日一签 | `UInkDailyFortunePanel` 现只读 `verse/day/signed`，不读取签运 | 大吉/吉/平/凶四张章；`UJourneyStateBadgeWidget` FortuneSeal 家族禁点 | 新逻辑只能按服务返回 `daily_fortune` 映射 glyph；缺值保留未同步 | 当前路由叫“每日剑诀”，与“一签”不一致；当前字段未见，须 adapter/route 协调 |
| 月签到 | `UInkMonthlySignInPanel`、`BuildSignInChestStrip(true)`、3/7/14/21 服务节点 | `UJourneyCalendarDayWidget` | 服务 month/day/signed/claimable records → `SetDayState`；`OnDayPressed` 只开详情 | 显式漏签/可领字段未消费；unknown 不可推成 missed；月历和领取运行验证 |

## 接口与交付路径

- 新源码：`Source/BoundlessImmortality/Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.h/.cpp`。仅新增于本队目录，直接包含现有公共 `Frontend/InkFeaturePanel.h`，没有编辑公共头/路由文件。
- `UJourneyStateBadgeWidget::SetGlyphState(EJourneyGlyphFamily,FName)` / `SetCaption(FText)` / `SetBadgeEnabled(bool)`；地图选择回调 `OnGlyphPressed(FName)`；FortuneSeal 一律只读。
- `UJourneyCalendarDayWidget::SetDayState(int32,EJourneyCalendarState,bool)`、`SetDayEnabled(bool)`、`OnDayPressed(int32)`；日号为运行时文本。
- `UJourneyClaimStateWidget::SetClaimState(EDailySignInClaimState,FText)`、`OnClaimRequested()`；状态是单枚 enum，只有 Claimable 发一次请求，接入层必须等 authority 回填 Pending/Claimed/Error。
- PNG + contracts: `packages/journey-components-v1-png.zip`; measured source/outputs: `assets/`; UE Editor importer: `tools/import_journey_assets.py`；目标 `/Game/UI/Ink/ComponentRollout20261024/Journey`。
- 详细契约：[screen-coverage.md](screen-coverage.md)、[component-contract.yaml](component-contract.yaml)、[asset-import-map.yaml](asset-import-map.yaml)。

## 验证

- 运行命令：`& 'C:\Users\zxxuh\.codex\skills\gameui\.venv\Scripts\python.exe' 'specs\boundless-ink-ui\orchestration\2026-10-24-ui-component-rollout\panel-journey\tools\verify_journey_delivery.py'`
- PNG：预期 11 个 RGBA 图标；真实尺寸/alpha/hashes 在 `assets/journey-sprite-manifest-v1.json`；ZIP CRC/条目也由同一脚本核验。
- 逻辑门控：源静态检查保证每日领取 enum 互斥与一次 callback、fortune 不可点。不是 Unreal 编译、自动化、PIE 或服务端集成验收。
- 未执行：UBT、Editor import、AssetRegistry 回读、route/WidgetTree 替换、PIE/设备。

## 下一步

唯一接入者先确认每日一签 route/field 映射，再导入11枚贴图、回读 Texture2D 设置、把三类代码组件接到现有业务壳并适配真实 service snapshot。无服务字段的 `daily_fortune`、`claimable_days`、`missed` 维持未知。用户最后审阅全部视觉；状态不可自动转为 approved。

Package SHA-256: `98D8D2E905AEF4E4E20C225ADFCE9A6E05610C95385AC01471C6EC9C4EA2A816`.
