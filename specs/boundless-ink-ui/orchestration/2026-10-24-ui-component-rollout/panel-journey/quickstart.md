# Journey 组件候选 v1

**状态：** `approved:false`；候选源页面和新组件都等用户视觉审核。源图被本轮总控 README 明确授权为加工输入，这项授权不是视觉批准。PNG/ZIP/JSON 也不代表 Unreal 导入。

## 包含内容

- `packages/journey-components-v1-png.zip`：11 个独立 RGBA PNG、像素实测 manifest、组件/切图/导入映射、逐页映射及生成 prompt。
- `assets/source-pages/`：本次四张源候选的副本与原 hash；未变更根目录候选或共享资产清单。
- `component-contract.yaml`：11 个图像状态加两个代码控件契约；地图点用 full/contain，无 9-slice；可伸缩按钮沿用现有 native ink theme。
- `screen-coverage.md`：逐页面壳、复用业务、数据源和待做项。
- C++ 源码：[JourneyRolloutWidgets.h](../../../../../Source/BoundlessImmortality/Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.h) 与对应 `.cpp`。

## 重建 PNG/ZIP

在项目根目录使用 GameUI 专用 Python：

```powershell
$gameUiPython = 'C:\Users\zxxuh\.codex\skills\gameui\.venv\Scripts\python.exe'
$journey = 'specs\boundless-ink-ui\orchestration\2026-10-24-ui-component-rollout\panel-journey'
& $gameUiPython "$journey\tools\split_journey_sprite_sheet.py"
& $gameUiPython "$journey\tools\package_journey_assets.py"
& $gameUiPython "$journey\tools\verify_journey_delivery.py"
```

切图基于 1536×1024 RGBA 雪碧图，以 alpha≥8 的连通区按 3/4/4 顺序语义命名，只保留各自连通标记，导出边界加 8px 间距。每张 PNG 的实际 RGBA 模式、尺寸、alpha、hash 在 `assets/journey-sprite-manifest-v1.json`。

## 真正导入引擎

由唯一 `logic-integration` 持有 engine lease 后，在 Unreal Editor Python 执行 `tools/import_journey_assets.py`。目标目录 `/Game/UI/Ink/ComponentRollout20261024/Journey`。脚本对资产重名采取失败保护，并应用 UI texture / no-mip / sRGB / alpha 设置。随后仍要从 AssetRegistry 和 Texture2D 实际回读验证；`import_verified` 当前为 false。

接入类名、字段、事件、互斥状态和 route 复用方法见 `screen-coverage.md`。图标当前有 11 张候选 PNG；月历日期和签到 CTA 是 UMG/Slate 组件，不导出成烘焙文字图片。

## 当前验证边界

- 本包生成、源图复核、PNG 像素属性、YAML 与静态接口门控由 `tools/verify_journey_delivery.py` 检查。
- 尚未运行 UBT、UE Editor 导入、AssetRegistry 回读、真实 route、PIE、设备或视觉验收。
- 目前 `WulingFrontendData::Activity()` 使用 `day/signed/month_sign_count/signed_days/weekly_chests/monthly_chests/templates`。尚未看到 `daily_fortune`、月签漏签或可领日期字段；不能凭本机日期、缺少记录或 `verse` 生成这些状态。
