# 2026-10-23 全套 UI 集成管理员记录

> 2026-10-24 用户运行复看确认全部页面仍为旧样式。已确认先前主要完成路由/状态修复，候选图未接入、受保护页面继续沿用旧视觉；此前“重构”描述范围过大。本次反馈后新增了实际原生视觉源码、旧 WBP 主菜单子树迁移、共享功能页框架及签到/活动/地图层级。10 个相关 TU 的 MSVC `/Zs` 检查通过，随后完整 Editor target 构建成功：34/34 actions、退出码 0，日志 `Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261024-visible-native-v1.log`。新游戏 DLL 已核实含 `native-paper-v1`，哈希记录见 `Saved/Codex/NativeVisualRefactor/build-result.json`；新 PIE 尚待用户复看。下文 07:36 构建为历史记录，早于这批补丁。具体范围见 `plans/2026-10-24-visible-runtime-refactor.md`。

## 本轮已完成

- 活动分组包含日常、限时、常驻；日常页不再重复列出常驻模板。日常/限时明细按真实 `activity_id` 和 `cycle` 匹配，错误周期显示空态，不提供伪造领奖操作。
- 活动自动化验收新增日常与限时的 CTA 路由/context、交叉周期空态，以及当前 authority 没有 `permanent` 模板时的常驻空态断言。
- 突破报价提交按钮要求境界/品质/成长/准备度/来源/概率等合同字段齐全，并要求 quote `slots` 中每条 `item_id` 和合法数量与当前材料选择完全匹配；缺字段或不一致时禁用 CTA。
- `LaunchEvent` 与 `WulingActivity` 已拆分路由：开服活动分组进入开服活动页，武灵山门的“活动与签到”入口进入武灵活动工具页。
- `ExpeditionDetail` 的未开放“出诊”操作现为禁用态，避免点击后无事发生。
- 日常 `ACT_SIGN` 活动详情现从 authority 快照读取 `day/signed` 与签到占位奖励；只在模板启用、服务连通且不忙、状态完整且今日未签时开放“提交签到记录”，提交后仍等待服务确认。其他活动模板无相应 API 时明确禁用操作。
- `ACT_SIGN` 当前详情页按连接/忙碌/服务状态及签到日期和状态键刷新；请求和 authority 回执变化后会重建 CTA，且不轮询其他页面。
- 签到组按每日签到、每日剑诀、月签区分操作：每日签到仅调用真实 `SignIn`，月签奖励仅经真实 `ClaimSignInChest` 并校验 milestone `3/7/14/21` 为有限整数及 claimed/eligible；月签 footer 不会误触发每日签到。每日剑诀抽签 endpoint 尚不存在，故显示未接入并禁用 CTA，不伪造吉凶结果。
- 资源贡献榜不再伪装成普通空榜：按 authority/连接状态说明榜单数据边界，并提供跳转当前竞采的入口；单轮竞采分数不会代替周期榜数据。
- 地图路由拆为独立 `ResourceMap` 与 `ResourceContest`；资源地图、资源中心都能进入资源贡献榜，修复此前 ResourceContest 路由映射遮蔽榜单/资源争夺面板的问题。
- 资源地图现可进入 `ResourceContest` 中心；包验收代码改为从 `ResourceMap` 真实点击中心 CTA，再点击榜单 CTA 并检查两级返回、`CurrentPage` 与面板类型。合同 journey aliases 已统一为 `Extensions/WorldCultivation/ResourceMap`；这些是新增静态验收逻辑，尚未在 UE/PIE 中执行。
- `WulingRace` 的排队、取消与竞赛操作增加角色/阶段/队伍/资格/服务端时窗门控；权威快照缺少占用状态、允许节点、队伍资格与新鲜心跳信息时，相关 CTA 保持禁用并显示原因。
- 复审修复：新增代表限时响应 CTA，按本地 authority 的两个 20 秒窗口门控；突破取消仅在 `Prepare` 且服务端时间早于 `trial_start` 时开放，对账查询只在 `Reconcile` 且有 attempt id 时开放；武灵里程碑与奖励从 launch 快照和权限位显示；常驻活动验收断言 authority 首条标题。
- 突破报价现回显 authority 提供的完整 `stage.name` 与 `selection` 数量，并从快照目录查找材料名/库存；没有槽位分类或报价必需字段时仍禁用提交，不按材料 ID 推导类别。
- 突破材料选择器、quote `selection` 与 `slots` 校验统一使用服务已声明的 1–100 数量边界。
- GameUI 项目校验本轮通过：`validate_project.py specs/boundless-ink-ui` 返回 `OK`、0 warning。此前另一次复跑因错误 Python 运行时缺 PyYAML 未执行到项目校验；该失败不影响本轮隔离环境的实际校验结果。
- 已有只读 UE5.8 AssetRegistry 查询覆盖 `/Game` 1,507 个资产及 12 个旧 UI 候选包；6 个旧 WidgetBlueprint 当前直接引用者为 0，但报告不覆盖动态字符串路径、地图 Actor 实例值和启动地图；6 个 redirector 仍各指向 canonical 目标。审计不能判定任何候选可安全删除，0 项标记可删、12 项继续 retain/unknown。一次 redirector 清理尝试未完成；当前 6 个 redirector 与 6 个 canonical `.uasset` 的 SHA-256 均与预删备份清单匹配，Git 状态无这些文件的修改/删除。证据：`Saved/Codex/Legacy-UI-Audit/AssetRegistry/legacy-reference-audit.json` 与 `Backup/predelete-20261023-1148/backup-manifest.json`。
- 后续新增了只允许精确六个 `ObjectRedirector` 的 Editor commandlet 源码。较早 helper 构建以 `-532462766` 退出，没有 UBT 编译/链接结果；该次失败不代表当前 Editor target。清理没有开始：未运行 commandlet，也未删除资产。2026-10-24 的 Editor target 构建已包含 `-PreflightOnly`、全量同步 Asset Registry 扫描和逐项依赖校验版本；这只证明源码编译/链接，不表示已执行清理。

## 构建与运行验证

- 对 Safe PIE 运行图的独立只读目视复核：Main 仍是原菜单，符合 `preserved` 契约；Settings 可见分类分组和固定底栏，但保留现有纸面板与金边按钮语言，重构感较弱；所谓 Saves 截图实际仍显示 Main，因此没有存档页面视觉证据。运行报告未记录源码/二进制指纹，不能确认当时加载了哪版代码。该结果支持“玩家看不出明显 UI 变化”，不代表已批准的页面缺少渲染改动。
- 为产生安全、可见且不依赖候选图的具体变化，Settings 现把画质、帧率、动态三个分组各自包入既有墨纸卡片，并根据 `UGameUserSettings` 当前值显示选中状态；`RefreshPage` 完成共享样式刷新后重应用此状态。保持选项即时生效/保存、Apply、Main 与其他页面不变。源码已编译通过；由于本轮 Editor 启动停在 splash，720×1280 PIE 画面验收仍未完成。
- 2026-10-24 后续复核通过窗口枚举确认 PID 11172 显示本项目启动画面“虚幻编辑器 - BoundlessImmortality”及“无有效数据路径配置”对话框，未加载项目模块。向对话框与启动画面发出正常关闭消息后进程仍无窗口存活；按用户已确认编辑器关闭的状态，结束了这一精确启动进程，随后确认 PID 11172 已退出。当前快照无 `UnrealEditor`、`UnrealBuildTool`、`AutomationTool` 或 `dotnet`；仅有 session 0 的 `BuildService` PID 6944，系统拒绝查询其路径/服务归属。构建将使用 `-NoXGE -MaxParallelActions=1`，不依赖 XGE；该服务进程不属于已识别的 UE/UBT/Cook/PIE owner。
- 同日静态核验旧 UI 备份：`backup-hashes.json` 记录的 12 个源包与 12 个备份包 SHA-256 全部匹配。Commandlet 当前只有一处 `FixupReferencers` 调用，没有 `EditorAssetLibrary` 或 `delete_asset`；这只是源代码/文件哈希检查，不是 Asset Registry 新扫描、编译或清理执行证据。
- 2026-10-24 更新：UE5.8 Editor target 编译成功，命令为 `Build.bat BoundlessImmortalityEditor Win64 Development -Project=D:\demo\BoundlessImmortality\BoundlessImmortality.uproject -NoXGE -MaxParallelActions=1 -Log=D:\demo\BoundlessImmortality\Saved\UBT-Diagnostics\BoundlessImmortalityEditor-20261024-ubt-escalated.log`。30/30 actions，退出码 0；包含 Settings 样式、Safe PIE callback instrumentation 与 redirector commandlet 源码。日志：`Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261024-ubt-escalated.log`；未提供 Build ID。沙箱内首轮 UBT 因无法轮转 `%LOCALAPPDATA%\UnrealBuildTool\Trace.uba` 报 `UnauthorizedAccessException`，获准的 UBT 重试后编译成功。现有 float conversion 警告不影响退出码；此证据仅为 C++ 编译/链接。
- 2026-10-24 PIE 尝试：启动精确项目 Editor PID 33236 后约一分钟仍停在“虚幻编辑器 - BoundlessImmortality”启动画面；无项目模块、Editor 日志或新的 Safe PIE 报告。已关闭并结束该精确启动进程；因此没有 PIE、页面截图或运行态验收结果。当前进程快照无 Editor/UBT/UAT/dotnet，仅见 session 0 `BuildService` PID 6944；该服务不属于已识别的 UE/UBT/Cook/PIE owner。PID 11172 的早先启动尝试也已按用户确认的关闭状态结束。完整 UI 重构尚未达到运行验收。
- 时间线补充：`Saved/Logs/BoundlessImmortality.log` 创建于 2026-10-24 07:28；记录 Editor 启动耗时 60.132 秒，07:29 自动重复上次 PIE 命令并进入默认 `Level_Sky_Temple_Demo_01`，07:30 执行 `QUIT_EDITOR`。该日志早于本次 07:36 开始的成功构建，命令行字段为空，也没有 `BoundlessInkPIE.RunSafeRoutes`，因此是此前 Editor 会话的默认地图运行，不是新构建验收。它说明 splash 可能是正常的长启动阶段，但无法证明之后 PID 33236 的启动属于同一会话或加载了新二进制；Safe PIE 仍未验证。历史“无有效数据路径配置”可能对应 Zen 数据路径瞬态告警，但 PID 11172 无独立日志可建立关联，当前最新日志已正常启动 Zen，故不作为已确认根因。
- `specs/boundless-ink-ui/tasks.md` 中历史勾选的 PIE/Cook/分发包项对应既有交付记录；本轮管理员状态与新构建时间线不同，不应将这些勾选当作 2026-10-24 新版运行验收证据。

- 最终 UE5.8 Editor target 编译通过：`-NoXGE -MaxParallelActions=1`，2026-10-23 22:21:29（本地时间）完成，4 个动作，Editor DLL/lib 已链接，退出码 0。日志：`Saved/UBT-Diagnostics/BoundlessImmortalityEditor-20261023-222122-final.log`，配套 JSON 同名；该日志没有输出 Build ID，因此不沿用较早构建的 `776B5F1B-F93C-4D7E-920C-C2DB40FFC5D9`。本次成功构建包含 safe PIE runner 和 redirector helper 的 Editor 源码。该结果证明 C++ 编译与链接，不证明 PIE、界面布局或业务运行。
- 构建中间诊断：带 `-Session` 的一次尝试在 UBA 初始化退出（14 个动作规划后，exit 6）；Engine 源码表明该参数会关闭 UBT 默认 trace，而 UBA 要求全局 trace。移除 `-Session` 后正常启动，第一次编译在 runner 的两处 C++ 错误停止；修正 `FJsonSerializer::Serialize` 参数类型和 TCHAR 字符串比较后，最终构建成功。最终日志证据见上。
- 较早的一次交互运行验收为 0/7：UE `-game` 进程运行 90 秒后仍无窗口句柄或运行日志，截图 API 返回无效句柄；该验收任务停止了该进程且没有重试。此历史尝试与本轮后来编译的 safe PIE runner 分开记录。启动地图配置和静态路由矩阵都不算运行通过。
- 后续只读检查确认 `UnrealEditor-Cmd.exe` 文件存在，但没有覆盖目标路由、Widget 创建与真实空态/禁用态的无副作用自动化套件。现已编译并触发安全 PIE 导航 runner：只对白名单中精确可见的导航按钮执行 Slate 点击；StartGame、可能自动请求数据或有业务副作用的动作会阻塞记录。报告 run `20261023T145655Z-F139C3A3` 位于 `Saved/Codex/BoundlessInkPIE/20261023T145655Z-F139C3A3/report.json`，完成态 `completed_with_failures`；2 opened、41 blocked_entry、1 failed。PID 11172 仍显示 `BoundlessImmortality` splash/“无有效数据路径配置”模态框；PID 38472 的项目日志在 22:56:55 记录 runner 启动及 44-route PIE。报告结果及 failure/blocked 分组见下一节。
- 上一版 safe runner 已执行一次。Main/Settings 可达；Saves 页面未打开，但上一版只记录 Slate mouse-down handled，没有证明 `TriggerAction` 收到 callback，因此底层 route 是否失败尚未确认。当前 Editor-only runner 已加入 callback serial 观察，等待进程释放后重新编译/复验。StartGame 和依赖 unsafe/不可见入口的页面维持阻塞。旧 UI redirector 清理因 PID 11172 与 38472 两个 UnrealEditor 仍运行而未达到进程门槛；历史 Windows ZIP/PIE 不代表包含本轮源代码修改。
- 本机 authority Python 测试套件最终通过：在沙箱内的两次运行因 SQLite 临时数据库目录权限而未能执行到断言；随后用隔离临时数据库目录执行授权运行，`Ran 102 tests in 23.084s — OK`。这验证的是本地 authority 服务规则，不是 UI C++ 编译、UMG 集成或 PIE。

### Safe PIE runner 本轮结果

- 报告：`Saved/Codex/BoundlessInkPIE/20261023T145655Z-F139C3A3/report.json`；`status=completed_with_failures`，44/44 行已记录，`opened=2`（`main/Main`、`settings/Settings`），`expected_unavailable=0`，`blocked_entry=41`，`failed=1`。
- 报告唯一 `failed`：`saves/Saves`。报告 trace 将 `OpenSaves` 标为 `clicked`，但该 runner 版本只依据 Slate mouse-down 的 `Handled` 返回值，没有观察 `UInkFrontendWidget::TriggerAction` 回调；随后的页面检查为 `CurrentPage=Main`、预期 `Saves`。因此运行结果准确记录了“页面未切换”，但还不能断言 `OpenSaves` callback 或生产路由本身失败。
- 9 条 `side_effect_or_unknown` 被 deny-by-default 阻塞：`home/HUD`（`StartGame`）、`methods/CultivationMethods`、`shop/Shop`、`market/Market`、`breakthrough/BreakthroughPrepare`、`tribulation/Tribulation`、`item-sources/ItemSources`、`guild-build/GuildConstruction`、`craft/WulingCraft`。由于入口链含 StartGame、武灵自动请求或其他未审安全动作，没有尝试点击。
- 32 条 `navigation_only` 因当前可见状态中所需按钮不能通过 Slate hit-test 而阻塞：`character/CharacterProfile`、`inventory/Inventory`、`leaderboard/Leaderboard`、`wuling/Wuling`、`sword-spirit/SwordSpirit`、`gift-rules/GiftRules`、`race-lobby/RaceLobby`、`race-live/RaceLive`、`race-result/RaceSettlement`、`lease/ResourceLease`、`world-result/EventSettlement`、`offline/OfflineRewards`、`orders/TradeOrders`、`world/WorldCultivation`、`region/RegionDetail`、`resources/ResourceContest`、`resource-detail/ResourceDetail`、`sign-in/SignIn`、`monthly-sign-in/MonthlySignIn`、`daily-verse/DailyFortune`、`activities/Activities`、`daily-activities/DailyActivities`、`timed-activities/TimedActivities`、`daily-detail/DailyActivityDetail`、`timed-detail/TimedActivityDetail`、`launch/LaunchEvent`、`launch-rank/LaunchRank`、`equipment/EquipmentFabao`、`journey/Extensions`、`appearance/CharacterAppearance`、`resource-rank/ResourceRanking`、`resource-rewards/ResourceRewards`。由于 HUD 入口需要 StartGame，runner 留在 Main 并没有用直接 ShowPage 等捷径。
- 报告策略记载 `mutation_policy=deny-by-default`；仅进行安全导航/本地只读操作，没有签到领奖、购买、突破、建宗门、竞赛、存档读写、设置应用或 authority 请求。runner 生成了 Main、Saves、Settings 的视口与窗口截图，但没有在本任务中目视审图。
- 报告后的源码复核确认生产路由代码会将 `OpenSaves` 映射为 `Saves`，且 `ShowPage` 包含该页；没有找到足够证据证明当前是游戏代码路由错误。已修改 Editor-only runner：在 `UInkFrontendWidget::TriggerAction` 添加 `WITH_EDITOR` action+serial 观察值，runner 仅在观察到恰好一次期望 action callback 后才记录 action dispatch；mouse-down handled 但无 callback 会单独失败。此修正尚未构建/运行；当前 PIE 报告仍是上一版 harness 的证据，需待 Editor 进程退出后编译并复跑。
- 目前 Editor 进程 PID 11172 与 38472 仍活跃，旧 UI 清理未达到无 UE owner 的门槛；没有运行 redirector commandlet 或删除资产。完整逐页运行验收仍未完成，当前不能将 2/44 说成套件完成。

## 候选视觉稿盘点与复核

- 全套屏幕契约有 44 屏，其中 40 屏属于重设计扩展；根 manifest 中 11/40 屏有页面候选，29/40 屏没有已登记的页面候选。各并行 revision 目录还存有未晋升的候选文件，不能据此算作 active contract 交付。根 manifest 登记的扩展稿均保持 `approved:false`；未登记的 v2 变体不能视为已批准或已接入资产。
- 首轮对 15 张候选稿的复核发现：主菜单与天劫 v2 文件画面互换；突破页占位字段不够明确；活动中心把互斥的空态与加载态拼在一张图；开服榜单“未上榜”与“数据待同步”矛盾；剑灵榜出现名次占位同时又显示无数据；地图图钉是静态示意且原始 `{{...}}` 字段需要 UE 文本控件替换；资源地图缺少区域/列表入口；签到说明在窄屏偏小；月签未按星期偏移和月份天数生成；多个榜单缺少指标单位/周期，武灵加载候选没有失败重试态。
- 首轮逐图证据位于 `evidence/2026-10-23-suite-candidate-visual-review.md`。候选无论是否经过视觉复核都不是用户批准的最终贴图；候选 manifest 的 `approved` 保持 `false`。地图/签到与 player-core 修正还在独立只读核验中，活动候选由另一项 revision 修正。
- 活动状态修正 v3 已由独立视觉子任务完成，文件在 `revisions/2026-10-23-activity-state-correction-3/`：单一活动空态移除了互斥 loading skeleton；开服页改为无节点、无奖励占位的“服务端数据待接入”信息卡。两图均为 941×1672 RGB，SHA-256 与人工视觉检查写于该 revision 的 `manifest.yaml` / `validation.md`；均为 `generated-candidate`、`approved:false`。候选保留在独立 revision，未进入共享 asset manifest，也未导入 UE；目前没有 UMG/PIE/设备验证。
- 地图/签到独立视觉复核已完成：每日签与月签总体可读，但小字号及“示意而非真实日期/状态”的说明仍需强化；资源地图标记与世界地图路线/锁点可能被误读为真实位置或进度，应增加明确示意标注。四张候选仍待批准，未登记为生产资源、未导入 UE。
- `player-core-mapping-correction` v3 文件哈希复核显示，`main-menu-v3.png`（SHA-256 `D980A8339AC1304A31B6F608E23A5549001281A87EEB140C51B4FA8679690F21`）与内容正确的 `tribulation-live-v2.png` 字节一致；`tribulation-live-v3.png`（SHA-256 `0404447550AA3B0EA5B01288584E1B3839FAAA92DEF0490EE9E056C18105E922`）与内容正确的 `main-menu-v2.png` 一致，v3 文件语义映射已纠正但仍待批准。`breakthrough-preparation-v3.png` 尚未与源图作语义核对，也没有批准或集成。

## 仍待处理

- C++ 编译/链接已通过；safe PIE runner 已生成首份 44-row 运行报告，但仅 Main/Settings opened，Saves 页面未切换，其余页面均 blocked_entry。旧 runner 没有观察 action callback，因此不能据此确认 Saves 路由失败；callback serial 诊断补丁尚未编译，完整 44 屏视觉验收未完成。
- 根 manifest 中 40 个扩展屏有 29 个未登记视觉候选；只读源代码审计发现其中大多数已有原生控件或共享父页，候选缺失不等于没有 UI。`GiftRules` 已新增真实玩家入口和原生诚实不可用页；权威 `GiftEffectVm` / 礼物效果规则 API 仍缺失，因此不得展示规则或借用 GM/直播数据。其余屏的主要缺口是共享页不是独立页、演示/空态、部分 API 字段缺失，以及未获用户视觉批准的候选。
- 6 条 supplemental route 已完成源码状态复核；旧 `runtime-route-coverage.json` 仍是历史运行状态，需后续用实际 PIE 路由遍历更新，不能由静态代码审阅覆盖成运行通过。
- Android/iOS 真机与生产平台审核不在本轮 Windows 本地验证证据内。

## Supplemental route source review

- **44 屏是契约范围，不代表 44 屏已逐页可达或运行验证。** 部分入口复用父页/共享状态，类存在或解析器能返回类名都不等于 PIE 证明了资产加载和交互。
- `CharacterFate` 有路由类，但目前是“未开放”占位，没有实际配置动作。
- `Draw` 是本地演示，不消耗真实货币或发放物品，且不在正式导航中。
- `Expedition` 是旧委托预览，含虚构示例；没有真实队伍、行程或奖励结算。`ExpeditionDetail` 的未开放参与 CTA 已禁用。
- `WulingActivity` 有路由、快照读取和签到/开启活动请求，当前从武灵山门可达；里程碑从 launch 快照读取，并按 `guild_numeric_rewards` / `guild_numeric_buff` 权限显示条件状态。
- `ResourceContest`、`ResourceMap` 与 `ResourceRanking` 现在是独立路由；地图和资源中心均有资源贡献榜入口，榜页只用 authority 状态和数据边界说明，不伪造周期榜分数。
- `WulingRace` 有本机原型 API 调用代码，并按角色/阶段/资格/时窗门控。600/200/200 明确标为本地原型固定计分规则，不作为本局实时分数。由于快照缺少若干准入字段，相关 CTA 当前禁用。
- 武灵服务默认连接本机原型服务（`127.0.0.1:18770`，规则版本 `prototype_full`），不是生产服务。日常/限时活动模板来自 authority，但目标、资格、进度、奖励结算 API 尚未提供；活动按钮不能据此宣称已完成活动报名/领取。
- 抽奖、仙缘配置、每日剑诀、资源排行/奖励和通用详情参与/奖励仍有演示、空态或未接入边界；当前源码不能证明这些已经是完整业务功能。
- 旧 `runtime-route-coverage.json` 的 `runtime_implemented:false` 是旧验收状态，不能用来断定当前类/路由不存在，也不能被新源码静态盘点覆盖成 PIE 通过。逐页 PIE 与运行截图仍未完成。

Route evidence must distinguish native class presence, player reachability, local prototype API behavior, production authority, and executed PIE coverage. No route should be marked runtime verified until a fresh run captures it.

## Additional code review and static validation limits

- Source review found the current breakthrough quote API still lacks fields required for a complete truthful submission preview (`growth_min/max`, readiness, categorized slots, sources, normalized Q, and stage endpoints). UI now uses the available stage name and selection counts; submission remains disabled until the missing authority fields arrive. No formula or reward values were fabricated.
- 后续只读复核发现并修复了四项问题：ACT_SIGN 请求后 CTA 状态不刷新、资源地图/争夺中心路由不可达、突破材料选择上限不一致、月签里程碑可接受 authority 不支持的数值。路由/CTA/材料边界复审未发现具体剩余问题；月签修复有针对性静态检查，但未再单独复审。验收逻辑未在 UE/PIE 执行。最终 C++ 源码随后通过 UE Editor target 编译/链接（见上方日志路径；本次未记录 Build ID）；当前仍缺 PIE 证据。
- The package static validator `validate-wuling-pages.ps1` could not run because its declared baseline directory `Saved/Codex/PortraitUI20261023/Backup` is absent in this checkout. No result was produced; this is not a pass.
- Race service snapshot currently lacks `activity_busy`, allowed node list, eligible team, and fresh heartbeat data. The UI gates those actions off with a reason until those authoritative fields exist.

## 44-screen source route matrix

- A parallel source audit now records all 44 contract screens with player entry, `CurrentPage`, native class, state authority, and coverage limits in `evidence/2026-10-23-native-route-matrix.md`.
- The audit adds the `EquipmentFabao` alias to the authority-backed `CharacterEquipment` panel and package assertions for that mapping. It also records the `LaunchRank` honors route and resource map → contest → ranking route checks.
- `GiftRules` has a player path through HUD → Wuling → `UInkGiftRulesPanel`; it displays an explicit unavailable message and makes no request. The authoritative player-facing `GiftEffectVm` remains absent, so the screen does not show rules or use GM/live-session data. `WorldCultivation` resolves to JourneyMap by design: the journey-expansion contract explicitly aliases `Extensions`, `WorldCultivation`, and `ResourceMap`; a later `Routes` entry for the old live-cultivation panel is shadowed legacy residue. Several other contract screens resolve to shared parent/state panels. Route presence is not runtime validation.
- Follow-up source review found and closed three route-only UI gaps within the 44: character profile lacked entries to the existing equipment and appearance panels, and empty daily/limited template lists lacked an entry to their explicit detail-unavailable state. The new CTAs are local navigation only; empty detail context displays no-match copy and does not submit a service request. Per-screen player entry, `CurrentPage`, class, state source, and coverage limits are in the updated native route matrix.
- The matrix now includes exact ordered `player_entry_actions`, route-opening safety classification, canonical final `CurrentPage`, and group `SelectedRoute`/shared-alias state for the 44 rows. Wuling catalog routes that can auto-request data on activation are marked `side_effect_or_unknown`; navigation paths are not PIE or runtime acceptance.
- `CharacterFate`, `Draw`, and `Expedition` are legacy/preview routes outside the current 44 top-level contract ids; they are not counted as accepted screens. API/data gaps remain distinct from UI reachability: gift rules lack an authoritative player read model, resource period ranking/receipt data is absent, and race lifecycle aliases share one authority-backed panel. Candidate art remains unapproved pending user visual approval; no images were opened for this source review.
- The 44-row contract/report count, 11 targeted route assertions, and `git diff --check` passed statically. The audit itself did not run build or PIE; a later integrated Editor build is recorded above.
- Historical process checks found bundled `dotnet` PIDs 27752 (`AutomationTool`) and 38236 (`UnrealBuildTool`), plus PID 38944 (Editor UBT); those historical processes are absent. Current snapshots observed Editor PIDs 11172, 42244 and 38472, plus dotnet PIDs 23600/27768, with incomplete command-line ownership. PID 11172 has a splash/dialog window; PID 38472 logged the safe runner and latest report without a visible main window; PID 42244 has older ordinary PIE log entries. Do not run redirector cleanup until all owners are reconciled, Editor state is resolved, and a fresh no-UE/UBT/Cook/PIE check passes.

## Candidate registrations and latest build result

- Added 15 v2/v3 candidate entries to `contracts/asset-manifest.yaml`, each with measured pixel dimensions, SHA-256, actual RGB/RGBA mode, and a project-relative prompt/image path. All 15 have `status: generated` and `approved: false`; none is treated as an approved runtime asset. GameUI `validate_project.py specs/boundless-ink-ui` returned `OK` with 0 warnings after registration.
- Independent visual review confirmed `main-menu-v2.png` depicts tribulation content and `tribulation-live-v2.png` depicts a main menu. Both entries remain in the manifest for traceability and are quarantined as `content-mismatch-do-not-use`; their current hashes match their own files, so this is a semantic mapping problem rather than corruption. New v3 files were prepared as draft copies; until independent QA verifies hashes, dimensions, format, and visual identity, they are not treated as corrected replacements.
- Earlier build attempts exposed and fixed MSVC paging-file/compiler heap pressure, source type/shadowing diagnostics, ambiguous `Number(...)` calls, and a missing override definition. The latest `-NoXGE -MaxParallelActions=1` build completed all 4 planned actions and linked the Editor target successfully (no Build ID was emitted; see log path above). A 44-route safe PIE run now exists but completed with failures and blocked entries; full-screen runtime acceptance remains incomplete as described above.
- Candidate registration does not close the visual approval/runtime gap: 29/40 contract extensions lack a registered page candidate, but a read-only source audit found native/shared controls for most of those screens. `GiftRules` now has a route and unavailable-state panel; its required authority read model is still absent. Do not count every missing image as a missing UI, or every shared/preview panel as a complete independent business page. New candidate images remain unapproved, and runtime integration still requires a usable interactive UE session.

## 2026-10-24 新版全面接入与旧包清理

用户已复看 native-paper-v1 并确认可见，授权全面接入、旧UI清理和本地测试。本轮最新证据集中于 [新版UI接入与本地验证](2026-10-24-full-ui-integration-result.md) 与 `Saved/Codex/FullUI20261024/`。历史“未接入/未编译/清理尚未执行”的记录保留为当时状态，不应覆盖该新记录。

当前源码包含共享原生样式、保存WBP迁移及背包/角色/榜单/武灵/地图/签到/活动/工具页接入。精确6个旧重定向包已成功清理，正式6包哈希不变，12包备份与569条别名保留。候选整页图片仍保留各自待审状态。离线组件截图、受限Slate菜单检查、业务实机验收的边界分别记录，不能互换。
