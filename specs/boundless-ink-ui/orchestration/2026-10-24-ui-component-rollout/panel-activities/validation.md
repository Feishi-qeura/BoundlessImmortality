# 活动小队验证记录

## 已执行

- GameUI 绑定 Python 3.12 + Pillow：`python inspect_components.py`。
  - 通过：`hud-launch-countdown-frame-neutral-v1.png` 读取为 2172×724 RGBA。
  - 通过：alpha extrema `[0,255]`；透明像素 601278、完全不透明像素 1920、半透明像素 969330。
  - 通过：SHA-256 `5ED2166752C6E891874CA9318D1BA74887D62A8DB32CBD15BCCD7541AE4B9985`。
  - 通过：输出 `qa/hud-launch-countdown-nine-slice-preview.jpg`，按 200×80、280×80 逻辑目标测试。
- GameUI 绑定 Python：YAML `safe_load` 解析 `asset-manifest.yaml` 与 `component-contract.yaml`。
  - 通过：清单资产/源文件/提示词路径存在，manifest 4项、component contract 10项。
  - 通过：清单引用的资产仍全部 `approved:false`，清单 SHA-256 与 PNG 文件匹配。
- 对照项目源码，核实 `WulingFrontendData::Activity()`、`UWulingPrototypeSubsystem::{IsEnabled,IsConnected,IsBusy,Snapshot}`、`launch.status/end`、`server_utc`、`manifest.profile`、`UInkFrontendWidget::OpenFeaturePanel` 的实际接口。
- 人眼复核新组件 PNG：透明底、无内嵌文字、单独横向面板、中心保留给运行时文本；9-slice 几何预览的边框与山形角落未被中心拉伸破坏。

## 未执行 / 不能宣称

- 没有运行 Unreal Editor Python 导入脚本；Texture2D / Asset Registry 参数回读：未验证。
- 没有改 `ClassForRoute`、HUD shared layout 或 WBP；route_wired=false。
- 没有运行 UBT、Automation、PIE、Cook、设备或平台验证；native source 尚未编译。
- GameUI 全项目 strict validator 没有针对该隔离目录执行；项目共享资产清单含历史外部归档路径，单项候选 schema 检查不替代全项目 strict validator。
- 开服特惠接口未发现；组件保持禁用。周期活动有数据业务的运行验证必须用真实服务快照执行；空配置仅可验证空态。
