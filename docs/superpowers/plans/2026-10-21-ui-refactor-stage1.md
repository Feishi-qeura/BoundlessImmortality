# 水墨 UI 重构与策划落地：首段实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Do not dispatch agents unless the user chooses that execution method.

**Goal:** 按已经交付的 UI 规范完成真实场景 HUD 与签到样板，在视觉验收后持续推进 v1.4 功能与内容。

**Architecture:** 增量改造现有 UInkFrontendWidget、UInkAnchorStatusWidget、UInkFeaturePanel 与原型客户端。既有七个 WBP 保留路径和人工默认配置；原生功能面板继续独占 WidgetTree。共享主题、响应式计算和数据状态独立成小单元，不创建第二套导航或客户端奖励账本。

**Tech Stack:** Unreal Engine 5.8.4，C++，UMG/Slate，UE Python，现有 Cohesion/Portrait/Provided 材质；后续本机 Python/SQLite 原型服务，正式平台不在本轮发布范围。

**Spec:**
- `docs/design/2026-10-21-ink-ui-standard-v1/交给开发AI的执行约束.md`
- `docs/design/2026-10-21-ink-ui-standard-v1/UI开发规范.md`
- `docs/design/2026-10-21-ink-ui-standard-v1/动效与转盘商店专项.md`
- `docs/design/2026-10-21-ink-ui-standard-v1/设计令牌.json`
- `docs/design/2026-10-21-wuling-resource-v1.4/README.md` 及 09、11–16、tables。

**Status:** 2026-10-21 用户已批准并开始 Native 实施。主题/五入口HUD/签到开发预览已实施，本轮补齐昵称触控、焦点、选日保留、背包搜索和4/8列。Editor构建成功，当前原生77/77，渲染313/313（40图），几何合同14/14。用户要求暂不读取参考图片，因此参考视觉验收尚未进行；边界补测及启动期错误归因仍未全部闭合。详见 `Saved/Codex/UIRefactor20261021/Resume20261021/handoff.md` 和独立验收矩阵，不把首段标成全项目完成。

## Global Constraints

- 本轮用户文字 → 最新 UI 规范 → v1.4 增量规则 → 旧基础策划 → 概念图。旧 UI 文档中的“四入口/右下修炼/生命/旧字号”由最新规范覆盖；旧 100 资材建设由 v1.4 的 20/60/120 覆盖。
- 启动菜单：开始游戏、继续游戏、存档、设置、退出游戏。玩法底栏：角色、背包、修炼、排行、菜单；顶部：活动、签到、仙途。不得新增第二个修炼入口。
- 主场景、相机、本命剑、旧礼物映射及已配置参数保护；不从历史备份还原覆盖当前工作。
- `UIScaleRule=ShortestSide`：360→0.5、540→0.75、720→1、1080→1.5、1440→2、2160→3；SafeZone 后加 24 逻辑单位；热点至少 88×88。不对整页再用 ScaleBox 缩字。
- P：宽高比 <0.80；C：0.80–1.25；L：≥1.25。横屏常规抽屉宽 `min(0.46W,620)`，数据页上限 1180；C 上限 900。
- 全部可见文本和数字使用风格字体。标题马善政；其余文楷；Noto 仅缺字回退。显示词统一“血量”，内部 Health 不改名。新字体入口，不覆盖共享旧字体二进制。
- 200 物理槽、默认解锁 20、每页 40、竖屏 4 列/横屏 8 列；窄榜与每屏八行保持。
- UI 不发奖励、不扣币、不用 NativeTick 做业务。未接服务显示未连接/开发预览；提交携带唯一 request_id，重试复用，旧代次回调丢弃。
- `prototype_full` 隔离新规则与新档；旧礼物 `legacy_preserve`。`douyin_release_candidate` 的交易、跨主播、物品扩展与天劫等受限能力保持关闭。
- 分段交付按执行约束：第一段 HUD+签到/活动样板；视觉确认后第二段页面扩展；第三段动效/完整交易表现。更大范围玩法改造在视觉确认后展开。不能用本计划冒充已通过此门槛。
- 当前目录不是 Git 仓库；使用逐文件备份、SHA256、变更清单与恢复说明，不初始化仓库、不生成虚构提交记录。

## Review Focus

1. 安全宽度不足 720：身份与顶栏分行，热点保持可点，不压小字体。
2. 横竖切换和连续进退页：保留页面状态和同一请求，不重复构建业务、不丢输入焦点。
3. 长中英文昵称、缺字、超大数字：名字省略且可查看全文；成本不省略，数字仍是文楷。
4. 服务缺失/超时/旧代次回包：保留最后确认状态并标明时间，不显示领取成功或假榜。
5. 七个 WBP 与运行时动态树：仅迁移明确的显示引用及 HUD 布局，不清空整棵人工树；关闭后停止委托、定时器与特效。

---

## 现状与差距（本轮实查）

| 范围 | 当前证据 | 实施决定 |
|---|---|---|
| 设计包 | v1.4：39 屏、284 条 UI 验收、384 条资产清单；全为设计/待实施状态 | 按页面表追踪，不把 CSV 存在算作功能完成 |
| 主 HUD | `InkFrontendExtensions.cpp`：4 底部按钮，独立右下 `HUD_ToggleCultivation`；顶栏热点 64×88 | 复用 ActionId，将同一修炼按钮移到五入口中心 |
| HUD 身份 | `InkAnchorStatusWidget.cpp`：自绘血条和数字，名字 17、数据 12–15；使用旧字体 | 调整实际绘制字号和条轨，不能只改 TextBlock |
| 功能页 | `InkFeaturePanel.cpp`：固定 720×1220/1180×760，再 ScaleToFit | 首段对签到组增量启用真实可用区布局；旧页逐页迁移 |
| 签到 | `InkRewardsPanels.cpp`：占位预览，无权威签到；月累计仍 7/14/21/28 | 样板保持未接服务状态；不得把预览按钮改成假领取；节点按 3/7/14/21 |
| 已有组织 | `InkWulingPanel` 调用 `UWulingPrototypeSubsystem`；后者有创建、名册、历练、突破接口 | 完善既有客户端，不新建第二套武灵系统 |
| 服务依赖 | 当前 Tools 仅 BlenderMCP；`Tools/CultivationService` 缺失 | 后续按源码现有协议重建隔离服务并做事务验收，不声称当前服务可运行 |
| 资产与字体 | 七 WBP 实际可加载，默认仍 `F_LiveNames`；源文楷/马善政/Noto 文件存在 | 逐引用迁移；共享原字体保留 |
| 样式链 | 控件 MI_Cohesion、条轨 MI_Portrait、Provided 图集与榜单专用材质 | 优先复用；补软边/底栏云带前记录现有素材缺口 |
| UI 粒子 | 本机 Epic `NiagaraUIRenderer` 1.0，Experimental；项目未显式启用 | 第三段单独验证内置 API；不套第三方 ribbon/MaterialRemap 接口 |

保护备份：`Saved/Codex/UIRefactor20261021/Backup/`；索引：`preservation-baseline.json`。初次备份 516 文件约 75.2 MiB，另记录 52 文件保护哈希。七 WBP 默认配置记录在 `Baseline/baseline-report.json`。PIE 图像只有实际生成并检查后才能列入证据。

本轮已实际取得并查看 `01-main-720x1280.png`、`02-hud-720x1280.png`、`03-signin-720x1280.png`、`04-hud-1280x720.png`、`05-signin-1280x720.png`。横屏签到图可见第二排奖励标签与底部预览按钮重叠；HUD 身份区、亮草地上的白色入口标签存在阅读层级问题。截图中的场景和默认测试目标按现状记录，不以 UI 工作擅改。它们只证明当前基线采集，未证明新设计完成，也不是手机实机验证。

采集后 `preservation-check.json` 确认568条源码/配置/UI及保护资产记录无变化。`editor-rendered.log` 有引擎启动阶段的 `LogAutomationTest: Error: Condition failed`；本轮未运行完整自动化套件，不能将编辑器退出或成功截图宣称为测试通过。后续实施前需按失败上下文归因。

## Task 1：补齐首段基线与回退

**Files:** `Saved/Codex/UIRefactor20261021/`；新增 `Tools/InkFrontend/verify_ui_refactor_pie.py`，验证清单 `Tools/InkFrontend/ui-refactor-acceptance.json`。

**Interfaces:** 消费现有 `UInkFrontendEditorLibrary::BeginFrontendPIE/SetPIEViewportSize/CaptureFrontendViewport/DescribeWidget/ClickAction`；输出当前源码/资产哈希、默认配置、截图与机器可读报告。

- [x] 查明工程、真实 UE 路径与 Git 状态；检查现有源码、WBP、设计配置、字体和插件。
- [x] 复制 Source、Config、UI 资产、共享字体、项目文件；记录 LiveWeapons、地图、角色 BP 和插件源码哈希；备份现有本地存档。
- [x] 在同一关卡、未修改机位的现有运行状态采集720×1280主菜单及横竖屏HUD/签到五张截图；横屏主菜单截图留作首段完整回归补充。
- [x] 验证七个 WBP 的默认配置和依赖；导出主题材质参数与原字体引用，过滤认证或平台原始 ID。
- [x] 以备份和现有文件 SHA256 完整比对：568条保护记录变化为0；检查边界不包含编辑器缓存或用户偏好。

## Task 2：单一主题与新字体入口

**Files:** 新增 `Frontend/UI/InkUiTheme.h/.cpp`、`Tools/InkFrontend/import_ui_theme.py`；修改 `InkPresentationStyle.h/.cpp`、`InkFrontendWidget.h/.cpp`；新增 `Content/UI/Ink/Refactor20261021/Fonts/` 与 `Data/DA_InkUITheme_v1`。

**Interfaces:** `UInkUiThemeData` 提供 DisplayFont/BodyFont/FallbackFont 与字号/颜色/间距；`EInkUiTextRole` 为 Display/Section/Body/Button/Data/Secondary/HudLabel，`FSlateFontInfo InkUiTypography::MakeFont(EInkUiTextRole Role)` 返回对应角色字体；设计令牌由导入工具映射至运行 DataAsset，运行代码只保留缺省回退。

- [ ] 写字体导入与角色引用检查：标题使用马善政、正文/按钮/数字文楷、Noto 只缺字回退。运行旧界面证明当前不满足新字体合同。
- [ ] 导入为新的 FontFace/Composite Font；保存源文件哈希、OFL、回退配置及精确资产路径；不覆写 `F_LiveNames`。
- [ ] 在 `Ink UI|Typography` 暴露 DisplayFont/BodyFont/DataFont/FallbackFont 及角色字号；在 `Ink UI|Surface` 暴露纸/墨/金色与 FeatherWidth。HEX 显式转线性空间。
- [ ] 对主 HUD、签到样板和现有前端字体入口迁移引用；七个 WBP 若覆盖原值，只修改已核对的字体字段并保存前后差异，不重建整棵树。
- [x] 输出剩余非 TextBlock 字体位置（Slate/Canvas、WidgetComponent/TextRender、GM）清单；后续迁移逐项验收，首段不把未迁移项目标 pass。

## Task 3：五入口 HUD 与真实身份信息

**Files:** 新增 `Frontend/UI/InkUiResponsiveLayout.h/.cpp`、`Frontend/Tests/InkUiLayoutTests.cpp`；修改 `InkFrontendExtensions.cpp`、`InkFrontendWidget.h/.cpp`、`InkAnchorStatusWidget.h/.cpp` 及相应现有测试。

**Interfaces:** `FInkHudLayoutRects InkUiLayout::ResolveHud(FVector2D SafeSize, float ExtraStreamBottomInset)` 返回身份、三顶栏、五底栏与菜单矩形；`void UInkFrontendWidget::UpdateAdaptiveHudLayout()` 仅在尺寸/配置变化时应用；既有 `ToggleCultivation` 继续调用 `RequestCultivationToggle()`。

- [ ] 写并运行失败用例：720×1280 底栏五项次序准确、修炼唯一；1280×720 底栏宽 560；安全宽度不足 720 时顶部两行；所有热点≥88 且不交叠。
- [ ] 将同一修炼按钮移入底部中心，视觉 112，相邻视觉 80；一组五入口中心对齐，保留既有导航 ActionId。
- [ ] `Ink UI|Layout` 暴露 ExtraStreamBottomInset（默认 0）、OuterPadding（24）、中心视觉尺寸（112）、MinHitSize（88）；布局只使用 SafeZone 后逻辑尺寸。
- [ ] 身份区在 368×132 预算内清晰显示真实姓名、境界、血量和修为；名字可展开全文，数字字号 26–30，取消默认占位的无护盾空行；不改变 Health 数据含义。
- [ ] 目标摘要只绑定实际世界目标；没有目标数据即隐藏，不把参考图的任务/80级抄进业务。
- [ ] 菜单打开与关闭恢复焦点；中心修炼动作不因打开菜单被隐藏；打开整页时清除被遮挡 HUD 热点。
- [ ] 通过实际 Slate 点击验证五个入口、修炼/收功与返回；对既有动作有限进度/循环不定进度测试回归。

## Task 4：签到样板真实尺寸布局与软边

**Files:** 修改 `InkFeaturePanel.h/.cpp`、`InkFeatureGroupPanel.cpp`、`InkRewardsPanels.cpp`；必要新 UI 材质放 `Content/UI/Ink/Refactor20261021/Materials/`，新源图与 UV/切片说明放 `SourceAssets/UI/Refactor20261021/`。

**Interfaces:** `ResolveFeatureFrame(SafeSize, PageFamily)` 返回页面矩形；签到组首段使用响应式壳；嵌入内容继续经 `ConfigureEmbedded`，业务/选中标签状态与重排分离。

- [ ] 用 PIE 验证旧页 ScaleBox 导致字号变化；新增“页面打开不覆盖另一组有效按钮”的断言。
- [ ] P 页面外边 24，C 居中上限 900，L 右抽屉 `min(0.46W,620)`；移除样板整页 ScaleBox；标题88、标签热点88、固定底区104，只有中部滚动。
- [ ] 日签采用横幅→七日奖励4+3→今日详情→累计节点→固定操作；月签节点3/7/14/21。不得添加没有配置来源的实发七日奖品或使用本机日期冒充服务日期。
- [ ] 无服务时明确“签到服务未连接/开发预览”，奖励量未知显示待同步；真正领奖按钮禁用并有理由，返回可用。样板本身不编造到账。
- [ ] 复用 Cohesion 场景、Provided/Portrait 纸卡；普通内区用留白/细线，最多页面壳+选中卡两层边框。
- [ ] 新增软边 UI 材质时仅让山脚/图像边缘24–48单位羽化，正文和数量在独立层；头像/图标保持比例。每项补图给出原素材不能复用的原因。
- [ ] 反复切标签、横竖切换与关闭后重开；选中标签保持，无重复提交、残留回调或第二个顶层页面。

## Task 5：请求状态与后续功能接入接口

**Files:** 新增 `Frontend/UI/InkUiViewModels.h/.cpp`、`InkUiCommonWidgets.h/.cpp` 和 `Frontend/Tests/InkUiRequestStateTests.cpp`；首段仅接现有快照与明确缺失状态。

**Interfaces:** `EInkUiPageState` 为 Loading/Ready/Empty/Pending/Disconnected/Error/Expired/FeatureDisabled；`FInkUiSnapshotContext` 包含 profile/app/room/session_generation/rule_version/server_utc/snapshot_version/is_authoritative；`FInkUiRequestContext` 保留 request_id/receipt_id/status/expected_version。

- [ ] 写失败用例覆盖重复提交、同 ID 重试、旧代次回调、服务时间缺失与禁用 profile；明确 UI 的锁按钮不等同服务事务幂等。
- [ ] 接现有 Cultivation 和 Wuling 客户端为展示适配器，标明唯一数据源，禁止合并两个独立成长账本。
- [ ] 未提交返回拒绝；超时进入 PendingReconcile；页关闭只停止表现和观察者，不取消权威已提交事务。
- [ ] 首段完成状态控件接口与未连接样板。真正签到服务、跨日及奖励收据留到后续服务阶段，不通过本地发300修为假装完成。

## Task 6：首段验证与视觉评审交付

**Files:** `Tools/InkFrontend/verify_ui_refactor_pie.py`；`Saved/Codex/UIRefactor20261021/Verification/`；独立复制的验收矩阵，不覆盖设计包原 pending 清单。

- [ ] 关闭本次启动的 Unreal Editor 后执行 Editor 构建：`F:/UNREAL/UE_5.8/UE_5.8/Engine/Build/BatchFiles/Build.bat BoundlessImmortalityEditor Win64 Development -Project=D:/demo/BoundlessImmortality/BoundlessImmortality.uproject -WaitMutex -NoHotReloadFromIDE`。必须 exit 0 且无编译错误。
- [ ] 原生自动化先运行新增 layout/request 用例，再运行 `BoundlessImmortality.Frontend` 与必要 Cultivation 回归；保存真实 Tests/Failed/日志，不沿用旧成绩。
- [ ] 首轮必测720×1280、1280×720，再扩至360×640、540×960、1080×1920、1080×2340、960×720、1920×1080、2340×1080；保存像素截图和逻辑几何。
- [ ] 覆盖长昵称、中英文、缺字、极大数字、亮草地、无头像、空数据、断线、连续切页、窗口旋转、SafeZone 模拟、推流额外避让；测实际字高和热点，不只看控件存在。
- [ ] 回归启动五菜单、背包200/20/40与选择/搜索、窄榜八行、存档确认、设置、角色与修炼输入；检查基线保护哈希及 WBP 属性差异。
- [x] 验收逐项填写 pass/fail/blocked/not_run 与本轮证据。首段无粒子性能、移动实机、生产服务或正式平台测试，不能扩张结论。
- [ ] 交付前后截图、实际改动文件、资产路径、Blueprint 参数分类、恢复步骤和未过项；按 UI 规范请用户确认视觉后再展开后续全系统重构。

## 后续持续实施路线（保留完整请求，不以首段样板当最终完成）

| 顺序 | 工作包与现有接入点 | 可验收结果与关键约束 |
|---|---|---|
| S2 | 共用壳/路由注册/主题；39屏按 `ui-screens.csv` 增量迁移，角色、背包、排行、存档、设置和旧功能优先 | 全页面字体/状态/输入/横竖适配；新增薄 WBP；原生树唯一所有者；逐屏功能覆盖，不套同一厚壳 |
| G0 | `Tools/CultivationService/` 重建本机权威服务；先解析 `WulingPrototypeSubsystem` 的当前 HTTP 命令和现存 outbox 协议，再做兼容版本 | SQLite 事务、身份/profile/窗口隔离、幂等收据、绑定/到期来源链；只监听127.0.0.1；旧档无静默迁移 |
| G1 | 武灵创建/改名/成员/记录；一个世界历练和主播突破；补真实签到 | 名称审核、20条游标成员页、单正式归属/24小时冷却；600秒固定阶段，断线不双结算；观众自动突破，主播扣料；签到日唯一收据、服务日历 |
| G2 | 青云初启活动、个人剑灵/礼物说明、世界结算 | 七任务780修为归观众；1/3/6/12轮共同目标归主播灵石200/破境石60；青云踏飞雁无数值加成；荣誉不按礼物金额，名次修为奖励保持关闭 |
| G3 | v1.4 物品目录、来源、专注、配方、装备法宝、公库 | 730项索引按配置导入；160种材料四品质；确定Q与十次加法预算封顶10%；核心不重复扣旧石币；专注每日6完整轮共享；四装备/一法宝，冷却跨换装保留；公库20/60/120 |
| G4 | 突破准备与天劫/观众助力 | 服务单次attempt结果；5000bp基础+每唯一身份100bp、至多50人；每失败1000bp保底，最多10000；失败最多扣未用修为1500、不消耗材料、不掉境界；83秒有效期；所有退出路径释放Alert租约并恢复当前实际状态 |
| G5 | 两至四主播竞采、开采权与功法扩展 | 两个独立UE进程+服务完成比赛；600/200/200计分；单武灵一个租约，30/20/10分钟服务器时钟；迟到/并列/掉线/重复结算；七功法按配置分期，不能受礼物金额决定排名 |
| S3 | UI动效/软边材质/内置Niagara UI Renderer/ReducedMotion | 真实状态驱动动效；关闭≤0.25秒残留清理；PSO/异步延迟测试；预算按令牌，设备性能实测单独报告 |
| G6 | 原型商店/制作/白名单寄售与转盘视觉 | 价格确认→唯一请求→服务收据→表现；跳过/关闭/重连结果不变；仅免费草铁旧白名单；真实随机经济缺规则与服务时只做明确开发预览，发布候选路由和服务均拒绝 |
| G7 | 内容扩展、平台/设备专项 | 八区域按真实章节开放，不伪造完成；Android/iOS包装、设备、官方身份/ACK与生产部署分别验收。生产服务/账号购买/公开发布不由本计划自动授权 |

每个后续工作包执行前在现有设计约束下写具体文件/接口/事务/验收计划；只对新增的产品决策或外部发布权限提问，不反复要求重新批准已完成策划。

## 回退

停止编辑器与本机测试服务，按 `preservation-baseline.json` 选择恢复本轮实际改动的文件；先列清单后复制，不递归删除 Content，不覆盖本轮以外的新工作。新增资产先确认引用再移出项目；原共享字体和原 WBP 路径一直保留。隔离新服务数据库/outbox，旧礼物与旧存档不迁移。恢复后重查保护哈希、依赖加载与启动五菜单。

## 评审结论与执行方式

用户已有策划/视觉规范，本计划不要求重新设计。待确认本计划准确后，建议**当前会话直接实施（Native）**，先做 Task 1–6 再交视觉样板；可选逐任务多代理实施与独立复核，但仅在用户选择后派发。后续完整需求保留在上表，不把视觉样板标成整个项目功能已完成。
