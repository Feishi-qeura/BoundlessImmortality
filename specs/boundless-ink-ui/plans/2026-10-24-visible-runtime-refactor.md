# 运行界面可见改版实施计划

> 使用 subagent-driven-development 分文件实施；当前状态以本计划和对应运行证据为准。

**目标：**修复用户在真实项目中看不到新版界面的问题，交付实际运行的原生布局与样式变更。

**架构：**沿用 UInkFrontendWidget、共享功能页、现有路由和权威数据。原生 Slate/UMG 样式承载新视觉；不把候选整页 PNG 作为可交互界面。已有 WBP WidgetTree 必须得到针对性迁移，不能仅修改只对空树生效的 BuildDefaultTree。

**技术：**UE5.8 C++ / UMG / Slate；既有中文字体与已批准水墨颜色规范。

**规格：**`contracts/style-contract.yaml`、`revisions/2026-10-23-full-portrait-suite-design.md`。

## 已确认原因

- 2026-10-24 用户确认所有页面仍旧。最新项目日志记录 Live Coding 成功但没有代码变化。
- 先前大部分修改属于路由、缺失数据门控、空态和验证工具；候选图片保留在 specs，未导入运行资产。
- Main/Character/Inventory 等原合同写明 preserved；现有素材与入口继续使用旧样式。因此编译通过并不能交付全套视觉重构。
- BuildDefaultTree 对已有 RootWidget 返回；只在该函数加入布局不足以更新已保存的 WBP。

## 并行实施

- [x] 主菜单、设置、存档：`InkFrontendWidget.cpp/.h`；更新实际实例，保留中央角色和现有动作；加入运行日志版本证据（源码已实现，待运行生成日志）。
- [x] 共享功能页：`InkPresentationStyle.cpp/.h`、`InkFeaturePanel.cpp/.h`、`InkFeatureGroupPanel.cpp`；替换旧按钮与纸面框架，强化页头/标签/内容层级。
- [x] 签到、活动、地图：`InkSignInShell.cpp`、`InkSignInSample.cpp`、`InkRewardsPanels.cpp`、`UI/InkJourneyMapPanel.cpp`、`UI/InkLaunchEventPanel.cpp`；实际布局更新，保持真实状态来源。
- [x] 合并源码复核：保留已有修改与批准资产；修复旧 WBP 缺少 `Main_OpenSaves` 时的迁移失效，新建对应导航按钮并重绑。实际蓝图实例与画面仍需 PIE 验证。
- [x] 本批 Editor target 完整编译与链接：34/34 actions，退出码 0。
- [ ] 使用真实新运行日志及页面截图验收；无运行证据不标记完成。

## 交付边界

当前不是全套视觉完成记录。角色、背包、排行榜等原合同保留的独立构图，以及未接入业务的数据缺口，仍需逐项说明，不以共享样式改动冒充所有页面完整重做。旧 tasks.md 勾选的 PIE/Cook/包运行是历史基线，不能作为本批证据。

## 语法检查证据

使用本机 MSVC 14.50、上次 UBT 生成的各 TU response file 及 `/Zs` 检查上述九个 UI TU 和 `InkMenuDirector.cpp`。隔离脚本移除 object、SARIF、dependency 输出参数，不替换运行 DLL；脚本及逐文件日志位于 `Saved/Codex/NativeVisualRefactor/`。首次发现并修复 Slot 名称遮蔽、FSlateColor Alpha 访问、地图样式头文件路径三处错误；复跑后 10 个 TU 均退出 0。该步骤仅为语法检查；后续完整构建证据如下。

## 完整构建证据（2026-10-24）

- 用户保存关闭 Editor 后，其 PID 23164 已退出；仅剩本任务早先启动且停在初始化的 PID 12716。核实路径、启动时间、无项目模块、日志不再更新后先正常关闭请求，再结束该精确进程；随后确认没有 Editor/UBT/UAT/dotnet owner 才启动构建。
- `BoundlessImmortalityEditor Win64 Development -NoXGE -MaxParallelActions=1`：34/34 actions，`Result: Succeeded`，退出码 0，用时 158.88 秒。游戏模块与 Editor 模块均已重新链接；有既有 C4305 浮点转换警告。
- 日志：`Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261024-visible-native-v1.log`。
- 新游戏 DLL 已确认含 `native-paper-v1` 字符串；它证明布局迁移代码进入二进制，不证明实际运行已执行该迁移。
- DLL SHA-256、修改时间、13 个 UI 源文件哈希及模块清单 BuildId 记录在 `Saved/Codex/NativeVisualRefactor/build-result.json`。清单 BuildId 为 `55116800`，不作为本批代码的唯一指纹。
- 当前没有本批 PIE/截图通过证据；下一步由用户正常打开项目并启动新的 PIE，先复看主菜单及设置，再检查签到/地图/活动。
