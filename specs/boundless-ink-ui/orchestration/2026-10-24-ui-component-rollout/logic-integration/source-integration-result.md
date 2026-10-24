# 源码接入批次结果

> 2026-10-24 12:17 更新：下文保留为引擎执行前的历史源码阶段记录，原文另备份至 historical-source-integration-result.md。当前首批已实际构建、导入、迁移及验证，最终结果以 [engine-acceptance.md](engine-acceptance.md) 和 [engine-final-verification.json](engine-final-verification.json) 为准。后续授权的诊断修复有精确链记录；HUD 九宫格未通过，最终采用已验证整图模式。下文“仍未导入/未构建/9-slice”等描述不代表当前状态。

本轮读取了四面板与 LogicCore handoff/contract。LogicCore 的4个公共基线哈希全部匹配；先做逐文件备份，再应用 `integration.patch --ignore-space-change` 并精确合并宿主接线。未修改其他队的冻结源码。所有新增集成类仍在 LogicJourney；所有工具/文档在本目录。

## 已做的实际源码修改

- 玩家入口的 `ClassForRoute`：Activities 指向交付活动壳的组合子类，Daily/Timed/Permanent 各有实际保留页；移除原来强制折回 Activities 的 remap，让详情与返回栈可达。
- 新周期组合层保留旧常驻页的显式分类、参与/领取资格、目标/进度/目标值、奖励摘要、来源和结束时间字段，避免交付简版壳造成字段退化。未提供字段仍待同步。没有通用 Claim 动作；已确认的签到/剑诀/日常与限时详情入口保留。
- `InkJourneyMapPanel` 实际 pin 控件内创建交付 badge 的组合子类，保留动态名称及原节点动作。world available/locked 才使用真实语义；资源四种状态没有权威字段，因此纹理保持未知。旅行确认通过当前权威行/主播身份/忙碌/连接/资源武灵资料门控。
- 签到外壳固定 CTA 改为真实 `UJourneyClaimStateWidget`，动态回调进入适配器，回填 Pending/Error。已选择 SignIn 子页才允许提交；服务禁用/断线状态可刷新。
- 月历日格实际创建 `UJourneyCalendarDayWidget` 并绑定 OnDayPressed，只查看已选记录；signed_days/服务day决定已签/今日可签/未来/未知，不把未知当漏签。月箱先详情再领取，提交核对打开详情时的服务day。
- 突破宿主在实际材料目录行创建 Player 材料槽的集成子类，用 UPROPERTY 保留 `UInkCoreMaterialBinding`。OnFeedback 回写本地草稿，刷新/清空/切模式/报价/提交均接到该绑定。2026-10-24报价修正批次已更正两层门控：缺展示字段不阻止发送服务校验请求，阶段/选择/回显/类型/连接/权限/Busy仍保留。已接恢复的材料槽框消费者，full图保持正方形，不做9-slice；仍未导入。
- 原权威背包动作改走 LogicCore，保留原背包UI。三榜实际消费 ReadBoard，保留发布者身份/名次与原行布局；未发布/空/离线/错误用真实状态区分，首次空响应也刷新。新的 RankWuling paper factory 在主榜/HUD榜框与武灵统计卡挂载；这只完成部分组件替换，未完成整个榜单/武灵页面重制。
- HUDPage 实际创建交付 countdown 的组合子类，保留上方既有入口；透明按钮层不挡住新纹理，按源像素做9-slice。collapsed控件不tick，因此服务发布时也 Configure，以便发现新活动。位置/缩放/点击/遮挡仍需PIE。

完整44屏逐项状态见 `screen-coverage.md/json`；不能把本批部分覆盖扩称全套重制。

## 首个有界引擎批次（尚未执行）

`engine-batch.json` 包含源文件SHA256、原/后公共哈希、5张真实PNG尺寸/alpha/hash和2个保存WBP前置哈希；原冻结文件未改写，两处授权LogicCore修改由精确前/后hash台账核验。

1. 世界 available/locked 两枚 glyph → Journey。
2. RankWuling paper → RankWuling。
3. HUD countdown frame → Activities。
4. 恢复的1254×1254材料槽框 → Player；full切片、approved:false，极低alpha1像素待用户视觉审阅。

其他 copied HUD、explored、resource、fortune 图章没有本批明确消费者/权威状态，不导入无引用纹理。新组件运行前资源缺失时，原业务仍保留/显示未知；这不代表新资源已接入通过。

`run_unreal_batch.py` 已准备实际 AssetImportTask/Texture2D设置/AssetRegistry/PNG导出回读，随后调用现有 **保留其他页面** 的 RefreshLeaderboardFrontendAsset，对 Shared/WBP_InkFrontend 与 Leaderboard/WBP_InkLeaderboard 做保存迁移。脚本先校验 lease 与源/资源/原WBP哈希、备份WBP、拒绝覆盖候选，要求已重建原生类存在；导入后刷新首次导入前可能为空的 HUD CDO引用。`verify_unreal_readback.py` 在正常Python下比较实际导出PNG的尺寸和逐像素alpha，不拿源PNG测量冒充导入验证。

尚未运行任何上述引擎步骤。map/sect保存WBP的native parent未更换，使用同名native BuildPanelContent的新增子控件；必须通过实际玩家入口确认保存资产未覆盖/绕过它，不能仅凭源码推断生效。

## 当前证据

- `core-patch-precheck.json`：4/4原哈希匹配。
- `source-before/`、`source-integration-baseline.json`、`source-integration.diff`：只对本轮实际改动生成可审增量，区别于已有dirty worktree。
- `engine-batch.json`：其他队冻结源码 drift=[]。
- `source-inspection.json`：静态括号/生成头顺序/引用/Python解析与无lease拒绝检查，无错误。这不是 C++/UHT编译。
- 上一独立里程碑 `authority-test-result.json`：18项实际Python服务测试通过；不能代替本轮新C++/UMG集成验收。
- 未执行：Editor、UBT、Live Coding、Content写入、保存WBP、UE automation、PIE、Cook、设备/平台验收。

## 明确阻塞

`engine-lease.json=not_granted`；本项目 Editor PID35164 仍运行，已从其已加载模块确认项目归属，10:20的只读快照在 `engine-occupancy.json`。未重试CIM、未停止或重启进程，也未判断它空闲。请监督者在用户保存关闭后授予有界批次；预计构建、导入/迁移、自动化与PIE合计约60–120分钟，编译修复/环境问题可更久，非承诺。

候选保持 approved:false，visual_pending_user=true；source_compiled/import_verified/route_wired/logic_verified=false。
