# 可重复 UI / PIE 验收方案 — 2026-10-23

## 目的与证据分层

为 44-screen 合同建立可重复的玩家入口验收，覆盖实际 Widget 创建、玩家可达路径、Slate 点击、返回路径、真实空态/禁用态、竖横屏截图。该方案不是已执行验收；本记录只读检查了代码、既有报告和当前可见进程，不启动 UE/UBT/PIE。

每次结果必须按层分别记录：

1. **源码静态**：route matrix、映射、CTA 代码与断言存在。不能证明资产加载或玩家可达。
2. **编译**：Editor target 完成并链接。本轮最新构建于 2026-10-23 22:21:29 完成，4 actions，exit 0；日志 `Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261023-222122-final.log` 未输出 Build ID。只证明该构建快照可编译。
3. **PIE/UI 运行**：在当前代码版本的实际 UE 窗口中，逐条从玩家入口触发、断言当前页/组件、记录交互与截图。未完成前不得填为 pass。
4. **Cook/包/设备/平台**：独立列结果。Windows PIE 不等于打包、Android/iOS、生产服务或平台验收。

## 只读现状检查

- 合同是 44 屏；静态行级记录见 `2026-10-23-native-route-matrix.md`。其中结构化 `player_entry_actions`、`player_entry_safety`、`final_CurrentPage`、`final_SelectedRoute_or_state` 是 runner 的唯一玩家入口来源。共享父页/别名、服务依赖态或缺 API 不能因“可解析类名”而算可达。
- `Saved/Codex/IndependentRunAcceptance20261023/acceptance-report.md` 的唯一独立启动尝试是 `-game` 运行 90 秒，无主窗口句柄、运行日志缺失，屏幕截图报“句柄无效”，最终覆盖 0/7。该截图无效，不可复用为视觉证据。
- 初次只读检查未看到 UE/UBT/AutomationTool。后续快照观察到 PID 11172 的 Editor splash 与“无有效数据路径配置”对话框；PID 38472 的项目日志确认 BoundlessImmortality AssetRegistry 加载但无可见窗口句柄。普通 PIE 在 22:46:16、22:53:52 启动。随后 runner 于 22:56:55 执行并写入报告 `Saved/Codex/BoundlessInkPIE/20261023T145655Z-F139C3A3/report.json`；完成状态为 `completed_with_failures`，详情见下方本轮记录。当前 PID 11172、38472 仍在运行，命令行与父进程查询 Access denied，dotnet PID 23600/27768 的 owner 也未确认。不要并发启动第二个 Editor/build。
- 最新 `Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261023.log` 以 `ExitCode=-532462766` 结束；对应 `diagnostic-summary.txt` 的非构建 `-Help` probe 记录了 Log.BackupLogFile 文件移动时的 `UnauthorizedAccessException`。关联性尚未证实，这不是 C++ 编译失败诊断，也不能单独证明可安全启动下一构建。
- 当前路径上 `UnrealEditor.exe`、`UnrealEditor-Cmd.exe`、UE5.8 `Build.bat` 和 `.uproject` 都存在。此前 admin report 写“没有 UnrealEditor-Cmd.exe”与当前文件系统状态不一致，应以当前存在检查为准。Headless executable 存在仍不提供可视 Slate 点击/窗口截图。
- `UWulingPackageAcceptance` 确有 Slate 点击、页面断言和截图，但它需要 `-WulingPackageAcceptance -WulingPrototype -GWLocalTest`。源码会调用 `CreateGuild`、启动/中断 `WorldRound`，并等待本地 authority 确认；后续步骤亦可能提交本地业务动作。因此这不是无副作用的验收方式，不能在玩家现有数据/服务上直接执行，也不能把 `GWLocalTest` 当成数据隔离证明。
- `UInkFrontendEditorLibrary` 已提供 `BeginFrontendPIE`、`CaptureFrontendViewport`、`ClickAction`、`DescribeWidget`。Editor-only 源码 `Source/BoundlessImmortalityEditor/Private/InkSafePieRouteAcceptance.cpp` 的 `BoundlessInkPIE.RunSafeRoutes` 已改为读取 44 行结构化玩家入口表，逐条执行**精确白名单 Slate 点击**，并记录 `action_id/from_page/to_page/outcome`。它没有 `ShowPage`、`OpenFeaturePanel` 或 `CloseFeaturePanel` 作为可达性捷径；矩阵行缺失、入口不可见、动态 id 缺失或任何安全字段不是 `navigation_only` 时写 `blocked_entry`，不会以 resolver 结果冒充玩家可达。动态地图/活动 id 只能从当前玩家可见且已启用的按钮中取；活动空态仅使用真实可见的无上下文 fallback。截图和结果写入 `Saved/Codex/BoundlessInkPIE/<run-id>/`。
- 点击白名单仅含矩阵入口涉及的 `OpenCharacter/OpenInventory/OpenLeaderboard/OpenSaves/OpenSettings/OpenWuling/OpenActivities/OpenSignIn/OpenExtensions/OpenLaunchEvent`、逐个审核过的 `Open_<Route>`、指定 group tabs、`AtlasWorld/AtlasResource`、开服荣誉 tab，以及用于恢复上一层的 extension `Back` 与非 HUD builtin `Back`。`StartGame` / `ContinueGame` / 读写存档、设置应用、退出、签到领奖、购买、突破、建宗门、赛事与任何非路由 action 一律拒绝。HUD 的 Back 会调用 `ReturnToMainMenu`，runner 不用它从 HUD 回到 Main。所有可能在打开时自动请求武灵目录的 route 标为 `side_effect_or_unknown`，整个 route row 在点击前就记 `blocked_entry`。GiftRules 已有本地原生页面和明确“暂未接入”态；目前缺的是 authority API，因此可在安全入口抵达后记 `expected_unavailable`，不得报成完整功能通过。
- 用户菜单 PIE 从 `Main` 开始时，`Main`、菜单里的保存/设置页是当前无需 gameplay 启动即可安全验收的范围。大多数 44 屏路线以 HUD 为入口；矩阵唯一 `Main → HUD` action 是 `StartGame`，它会创建/控制 gameplay pawn、初始化 gameplay systems、可能发布 gameplay data 并开始存档。因此在不触发这些副作用的约束下，HUD 及其后续路线应记 `blocked_entry`。如果 PIE 当前实际以 HUD 起始，runner 可以仅通过 visible allowlist 路线继续；它不会自行切换 page。
- 回退仅允许 extension 的 `Back` 与非 HUD builtin 页面中的 `Back`。HUD 的 `Back` 会调用 `ReturnToMainMenu`，因此在 HUD→Main 回退路径中禁止点击；如果不能在白名单内返回起点，该行留为失败/阻塞并继续保留当前页面状态。
- 本轮 Editor target 已编译，runner 已在真实 PIE 中运行并记录 44 行；证明了本次 parser、Slate hit-test/click、页面断言与截图输出链能执行，但只 Main 和 Settings 为 `opened`，Saves 为 `failed`，41 行为 `blocked_entry`。只有完成真实点击与断言的 `opened` / `expected_unavailable` 计路由证据；`blocked_entry` / `blocked` / `failed` 不算通过。没有独立 resolver probe；如之后增加，必须使用独立结果数组和 status，永不计入 player entry 通过数。
- 引擎内存在若干 `IMPLEMENT_SIMPLE_AUTOMATION_TEST`，覆盖数据规则、控件几何、部分布局或按钮逻辑；这些不能替代玩家入口路由、实际屏幕渲染和 Slate 窗口截图。

## 执行前门槛

1. 检查共享工作树当前源码与最终构建日志时间；本次日志未输出 Build ID。若验收前源码改变，重新编译后再运行。
2. 使用 `Get-Process` 检查 `UnrealEditor*`、`UnrealBuildTool*`、`AutomationTool*`、`dotnet` / `Build` 及其命令行；同时检查 `Build.bat is already running` / UBT mutex 状态。若 owner 不明确或有任何同项目 UBT/Editor/Cook/PIE，等待自然结束，不重复 Build、不停止别人的进程。
3. 确认可交互的 Windows 桌面、UE 主窗口和 PIE 窗口句柄有效；如果只有后台 `-game` 或 handle 为 0，停止运行验收，不重复无窗口启动。
4. 建立独立临时 user-data/service namespace 或只读测试 authority fixture；若不能证明 `Saved/` 数据、原型 DB 和服务副作用与玩家状态隔离，只执行安全白名单导航，不触发服务 CTA。
5. 建立一次运行目录，例如 `Saved/Codex/BoundlessInkPIE/<timestamp>/`，记录 commit/源码哈希、构建日志路径（Build ID 如可用）、引擎版本、命令行、分辨率和截图 SHA-256。不得写入资产目录、重存地图/蓝图或修改批准状态。

## 建议运行步骤

### A. 一次性 Editor target 编译

只能在确认无其他 UE/UBT/Cook/PIE 所有者后运行；不要并行开启第二个 Build：

```powershell
& 'F:\UNREAL\UE_5.8\UE_5.8\Engine\Build\BatchFiles\Build.bat' `
  BoundlessImmortalityEditor Win64 Development `
  '-Project=D:\demo\BoundlessImmortality\BoundlessImmortality.uproject' `
  -WaitMutex -NoXGE -MaxParallelActions=1
```

保留完整 stdout/stderr、结束码、实际 UBT 输出和 Build ID。无结束码/只有 Build.bat 启动行不算通过，也不算 C++ 编译失败；它是工具链启动阻塞，等待 owner/错误日志后再决定。

### B. 交互 PIE 手工冒烟与 44 屏遍历

当前建议使用 GUI Editor，不用之前失败的 `UnrealEditor.exe -game`。可打开项目的精确入口：

```powershell
& 'F:\UNREAL\UE_5.8\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' `
  'D:\demo\BoundlessImmortality\BoundlessImmortality.uproject' `
  -log='D:\demo\BoundlessImmortality\Saved\Codex\BoundlessInkPIE\editor.log'
```

UE GUI 成功打开后，从配置启动地图启动 PIE，采用独立窗口。若 `MainWindowHandle`/PIE 窗口仍为 0、Slate 未初始化或截图返回失败，立刻记录阻塞并停止，不用 headless screenshot 伪造结果。先在 `720×1280` 运行主路径，再至少以 `1280×720` 检查关键页重排；需要比较小屏时加 `540×960`。尺寸应为视口尺寸而非外窗尺寸。

按 `2026-10-23-native-route-matrix.md` 的结构化入口逐项操作，逐项登记 `requested route / visible entry / action trace / CurrentPage / expected panel class / click result / return path / authority state / screenshot / result`。没有安全入口或无法从当时页面直接到达的路线登记 `blocked_entry`，不绕路注入 page。至少覆盖以下链路：

- 菜单 → 开始 → HUD；角色资料/角色功能/装备与法宝；背包、排行榜、保存、设置及返回。
- HUD/业务入口 → Journey / World map → 区域详情及可开放/锁定节点；ResourceMap → ResourceContest → ResourceRanking / ResourceRewards，验证两级返回。
- SignIn → daily / monthly / daily-verse 分组；记录服务断开、忙碌、资格未知时 CTA 的文字和 disabled 状态，不提交签到/领取。
- Activities → daily/timed/permanent → matching detail；错 cycle / empty template → 明确空态；LaunchEvent → honor/rank。
- Wuling / Craft / Sources / Breakthrough / Tribulation / Race / Shop / Market / Orders / Offline / Rights / Construction 等共享工具/记录路由：记录合同 alias 对应的实际 `CurrentPage` 和 `SelectedEntry`；使用关闭/返回回到真实来源。
- `GiftRules` 已经有原生路由和明确“暂未接入”态；authority `GiftEffectVm` 仍缺失。安全入口可达后，应记为 `expected_unavailable`，不得将其记录为完整功能通过。

每屏执行同一观察规则：页面实际可见；目标区域无崩溃、截字和明显遮挡；点击后 CurrentPage/共享页状态与合同一致；返回回到上一层；网络/authority 未连接时展示明确空态/禁用原因；不开启会消耗资源、签到、创建/修改 guild、排队/竞赛、突破/领奖等 CTA。重要页面保留视口截图和完整窗口原图；截图须来自当前运行，文件名带 route 和尺寸。

### C. Editor-only route harness（旧版本已运行；2026-10-24 新增 callback instrumentation 与 Settings 样式补丁已编译，尚待可见 Editor/PIE 复验）

`Source/BoundlessImmortalityEditor/Private/InkSafePieRouteAcceptance.cpp` 注册 `BoundlessInkPIE.RunSafeRoutes`。它调用 `UInkFrontendEditorLibrary::BeginFrontendPIE(720,1280)`，等待当前 PIE 的 `AInkMenuDirector::FrontendWidget` 和 Slate 窗口，再读取 route matrix 的 44 行 `player_entry_actions`。仅逐个通过 `ClickAction` 对当前可见的精确 allowlist action 做真实 Slate hit-test/click。action trace 记录 action id、起始/落点 page 和结果。多步路线每步点击之间留出 0.35 秒让 Slate 更新几何；完成后截图，再通过安全的 extension Back 或非 HUD builtin Back 尝试返回原始页面，HUD Back 不会调用。报告和截图写到 `Saved/Codex/BoundlessInkPIE/<run-id>/`。

最近一轮后发现旧 runner 将 `ProcessMouseButtonDownEvent` 返回 `Handled` 当作 action 已触发；这无法证明 `UInkFrontendActionButton::HandleClicked` 到达 `UInkFrontendWidget::TriggerAction`。当前源码已增加 `WITH_EDITOR` action serial 与最近 action 观察值；runner 现在会等待匹配 action serial 恰好增加一次后才记录 `action_dispatched`，只 handled mouse input 没有 callback 会记为单独 failure。该诊断补丁于 2026-10-24 编译通过；尚无新 PIE 运行验证。上一轮 Saves 失败只证明 `CurrentPage` 未变，是否是生产路由故障仍未确认。

安全字段不是 `navigation_only`、矩阵读取失败、动作未获精确 allowlist、点击按钮当前不可见、动态 id 不存在，全部是 `blocked_entry`；没有任何 `ShowPage` / `OpenFeaturePanel` / `CloseFeaturePanel` fallback。`StartGame` 是 side-effect/unknown，会阻止 Main→HUD；可能自动发起目录请求的 WulingTools 路线也整体阻止。完成真实入口点击和 CurrentPage、native panel class、group SelectedRoute 断言后 `player_entry_verified:true`；页面返回结果独立记录在 `return_verified`。GiftRules 页面可通过纯导航时要求显示显式“暂未接入”态，并因缺 authority API 标 `expected_unavailable`。报告的 `blocked_entry`、`blocked`、`failed` 都不能算通过。上一轮源码编译过并生成报告；Main 和 Settings 可见，Saves 页面未切换但旧 runner 未观测 callback，其余路线被安全门控或入口 hit-test 阻塞。截图也不自动批准候选设计。

### 本轮 runner 结果摘要

- Run `20261023T145655Z-F139C3A3`，44/44 rows；2 opened（Main、Settings）、41 blocked_entry、1 failed（Saves 页面未切换），0 expected_unavailable。旧 harness 未观测 `TriggerAction` callback，不能据此断言生产路由失败。详细路由和动作 trace 以 `Saved/Codex/BoundlessInkPIE/20261023T145655Z-F139C3A3/report.json` 为准。
- Main→HUD 的 StartGame 被 `side_effect_or_unknown` 策略阻塞；依赖 HUD 或未审武灵调用的路线没有继续尝试。Saves 点击后页面仍为 Main，是唯一 failed；该报告只能证明页面未切换，不能将其算成通过或确认路由故障。
- 运行器保存了 Main、Saves、Settings 的 view/window screenshot 文件；本次只读 JSON，不做图像审查。旧 UI 清理仍需等所有 UE/UBT/Cook/PIE 进程结束并完成精确哈希预检。

## 当前推荐结论

2026-10-23 的 Safe PIE runner 在 22:56:55 执行并生成 `completed_with_failures` 报告，计数见上方。2026-10-24 已成功编译包含 callback serial 诊断补丁与 Settings 样式变更的 Editor target（30/30 actions，退出码 0；日志 `Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261024-ubt-escalated.log`，无 Build ID）。构建后启动 PID 33236 的观察停在 Editor splash，该精确进程已关闭；没有与该新构建对应的 PIE 报告。另有 07:28 的 `Saved/Logs/BoundlessImmortality.log` 记录一段更早 Editor 会话：60.132 秒后完成启动，自动重复旧 PIE 命令并进入默认 Demo 地图，之后正常退出。该日志早于 07:36 的新构建，命令行字段为空且没有 `BoundlessInkPIE.RunSafeRoutes`，所以不能证明新构建通过 UI 验收；它表明 splash 可能是正常长启动的一部分，但与 PID 33236 是否同一会话没有证据关联。HUD 入口因 StartGame 会改变 gameplay/authority 状态而阻塞；不为扩大 coverage 而旁路。Saves 旧截图仍停留在 Main，旧 runner 未证明按钮 callback 是否触发；新诊断代码已编译，但尚未在运行中复验。历史 PID 11172 已关闭；PID 38472 等先前进程记录不应当作当前快照。redirector 清理仍需新一轮无 UE/UBT/Cook/PIE owner 检查、精确哈希预检与 commandlet 前置条件。现有 package harness 因真实本地 authority 写操作，不满足“无副作用交互验收”。`UnrealEditor-Cmd.exe` 当前存在，可以另行运行隔离的数据/几何自动化测试，但这仍不能覆盖 PIE 页面可见性、Slate 点击或版式截图。
