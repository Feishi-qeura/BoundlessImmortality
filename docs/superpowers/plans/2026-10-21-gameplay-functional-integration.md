# v1.4 UE功能页面、表现与综合验收实施计划

**执行记录（2026-10-22）：** 已完成权威接入、现有背包/角色/签到数据投影、13个内部玩法/记录路由、attempt预警租约及独立Niagara道具表现。Editor/Game构建和项目79项原生测试通过，实际渲染世界、功能页面与最后交互序列通过。下方保留原细分验收步骤；284条完整状态与指定分辨率组合仍为not_run，部分渲染证据不等于全矩阵通过。详情见 `docs/2026-10-22-wuling-v14-implementation.md`。

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans after user review. Steps use checkbox (`- [ ]`) syntax for tracking. Do not dispatch agents without explicit authorization.

**Goal:** 让权威玩法在现有UE场景和功能页面中可操作、可核对，并验证完整预警生命周期及原有玩法保护。

**Architecture:** 继续使用UWulingPrototypeSubsystem及其持久命令队列，补规则协商、会话代次、委托和快照投影。现有面板承载现有功能，新物品/天劫等作为内部子页；复用既有水墨样式，不执行独立UI视觉规范。

**Tech Stack:** UE5.8 C++、UMG/Slate、UE Python、原生自动化、渲染PIE。

**Spec:** 主计划；v1.4的09、11–16和功能/交互验收表；不采用独立UI开发规范中的视觉重构要求。

## Global Constraints

- 主计划全部约束适用；此计划分段穿插执行，不能延后全部UE接入再只交服务。
- 不创建第二套导航、库存账本、成长存档或剑灵飞行模型。显示和操作使用现有ActionId、页面结构和样式资源。
- 原有七WBP及礼物、武器数据资产路径和手工默认值保护；新增WBP按需创建，不覆盖现有二进制。
- UI不授予修为、不扣币、不自己计算阶段胜负；处理中/断线/对账/已确认明确区分，缺业务数据显示未连接。
- 保持横竖可用、文字可读和输入恢复，但不在本轮执行新字体、字号规范、五底栏重排、转盘美术、全站动效等独立视觉重构。

## Review Focus

- 旧session HTTP回包和停用后的命令队列不能恢复旧房状态，T1。
- 新模式本地成长与旧共享客户端不得和权威服务双写，T1。
- 用户连续点击/进退页面复用待确认request_id，不能重复消费，T2。
- 剑在STRIKE/RETURNING时助力不瞬移，结束只释放attempt自身租约，T3。
- 构建、渲染和退出状态独立判断，不用退出码证明清洁关闭或测试通过，T4。

## T1：现有客户端增量接入

**Files:** Modify `Source/BoundlessImmortality/Wuling/WulingPrototypeSubsystem.h/.cpp`、`Cultivation/CultivationSubsystem.h/.cpp`、`Cultivation/CultivationPrototypeClient.cpp`、`Frontend/InkGameplayBridge.cpp`；Create `Wuling/WulingSnapshotProjection.h/.cpp`、`Wuling/Tests/WulingPrototypeTests.cpp`。

**Interfaces:** 保留现有Blueprint接口；补`RefreshState()`、`SubmitPrototypeCommand(const FString& Operation,const FString& ArgsJson)`仅限开发profile内部调用；类型化公开接口分别为Craft/Equip/PrepareBreakthrough/AssistTribulation/QueueRace/SignIn/Purchase/CreateListing，参数对应服务契约，禁止开放任意余额写入。新增`OnAuthoritativeStateChanged`动态委托；`WulingSnapshotProjection::Apply(UCultivationSubsystem&, const FString& SnapshotJson, int64 Generation) -> bool`。

- [ ] 先写原生失败测试：manifest版本/hash不匹配拒绝；session generation过期回包不更新State；bEnding/切房/回菜单时取消请求并保留有scope的待确认命令。
- [ ] 测试outbox按app/profile/window/room/user/rule隔离；已有v1.1队列不静默改版本重发，转为可核对隔离记录；STALE_VERSION先刷新权威版本，同request继续；已提交命令取原收据。
- [ ] 测试prototype模式成长仅权威快照应用，UCultivationSubsystem本地Tick/命中/击杀不再给该规则档重复成长；旧CultivationPrototypeClient不得向同一新档提交本地record，旧模式不变。
- [ ] 运行测试确认失败后实现代次保护、规则协商和快照字段验证；ImportAuthoritativeRecords只作为受约束的兼容投影，不允许新profile覆盖旧GiftReceipts。新权益投影与旧礼物记录独立。
- [ ] 测试发出SubmitAction只携带动作、目标、round，不能注入最终伤害；世界目标仅镜像服务health，切菜单/断线销毁，恢复后重建当前目标。
- [ ] 构建Editor/Game并运行Wuling原生自动化，真实服务session→建武灵→模拟观众加入→完整历练→突破；记录服务与UE一致的版本、收据、成长及材料。

## T2：复用页面与新增功能子页

**Files:** Modify `Frontend/InkWulingPanel.cpp`、`InkFeaturePanel.h/.cpp`、`InkCharacterPanels.cpp`、`InkInventoryModel.h/.cpp`、`InkInventoryWidget.cpp`、`InkRewardsPanels.cpp`、`InkFeaturePanels.cpp`、`InkFeatureGroupPanel.cpp`、`InkLeaderboardWidget.cpp`、`UI/InkUiViewModels.h/.cpp`；Create `Frontend/UI/InkUiBreakthroughPanels.h/.cpp`、`InkUiItemSourcePanel.h/.cpp`、`InkUiGuildBuildPanel.h/.cpp`、`InkUiEquipmentPanel.h/.cpp`及所需内部注册代码。

**Interfaces:** `FInkWulingViewState`在InkUiViewModels.h定义，以authoritative_version和receipt_id关联UI。每个新Panel使用既有`BuildPanelContent()`和`RefreshLiveData()`模式，数据来自现有子系统；订阅/解绑与面板生命周期一致。复用`UInkFeaturePanel::ClassForRoute(FName)`、`UInkFeatureGroupPanel::SelectRoute(FName)`和`HandlePanelAction(const FString&)`注册内部路由，不创造并行顶层菜单。

- [ ] 先写有意义的交互测试：点击待确认购买/制作后关闭重开仍同request；回包丢失显示待确认；再次查询原receipt后才显示到账；失败保留真实余额和原因。
- [ ] 接现有武灵名册、阶段、结算、资源租约；加入/正式/临时计数区别；角色/背包显示真实主播物品和实例到期；排行不混旧新规则或测试/正式。
- [ ] 按ui-extension增加内部功能：四类突破材料与Q/概率说明、天劫现场、具体来源/配方与专注、8设施、公库/个人成本、四装备与一法宝。复用既有纸卡/图标/字体，640材料共享分类图标加中文文字，不替换礼物图标。
- [ ] 接活动签到/月度/周活动、青云初启七任务/共同目标/荣誉榜；贡献不可兑换数值，称号选择保留已有剑灵被动；首次未接服务显示未连接，不用日期填假进度。
- [ ] 接竞采报名/准备/统一试炼提示/结算、五种租约和到期；原型商店/市场在服务确认profile后显示，release深链接也拒绝；客户端不自报代表数、分数、币数。
- [ ] 对照11–13页面清单及284交互用例分类记录：功能/数据保留并验收；纯视觉重构项标excluded_by_user；依赖后续P4项标deferred_by_spec，不能统称通过。新增开服30、物品30用例分别追踪。
- [ ] 渲染PIE检查720×1280、1280×720和1080×1920；覆盖中文长昵称、空库存、跨页选择、倒计时、超时错误和输入焦点。仅作为Windows布局/交互验证，不声称移动设备验证。

## T3：attempt预警租约与现有飞行安全衔接

**Files:** Create `Wuling/WulingTribulationPresentation.h/.cpp`；Modify `LiveWeapons/LiveWeaponSwarm.h/.cpp`及必要的`LiveWeaponTrajectory.cpp`调用点（不得改轨迹数学）；Test `Wuling/Tests/WulingTribulationPresentationTests.cpp`、`LiveWeapons/Tests/LiveWeaponAlertTests.cpp`。

**Interfaces:** `ALiveWeaponSwarm::AcquireAlertLease(const FString& ParticipantId,const FString& SourceId,double ExpiresUtc,int64 SessionGeneration) -> bool`；`ReleaseAlertLease(const FString& ParticipantId,const FString& SourceId)`；`ReleaseAlertLeasesForGeneration(int64 SessionGeneration)`。WulingTribulationPresentation将权威helper列表与当前attempt投影到上述接口。

- [ ] 先写失败测试：同attempt重复助力只一租约；旧generation拒绝；第51人仍有表现；服务拒绝或未确认不假装已计概率。
- [ ] 测试STRIKE/DRIFT/RETURNING完成安全返回后进入ALERT，不直接改Transform；新普通战斗停止但既有礼物履约继续；租约不更改gift expiry或武器类型。
- [ ] 测试天劫Source释放后：另一有效警戒保持ALERT；有当前目标按现状态重新处理；无目标回IDLE，不恢复已过期旧快照。
- [ ] 覆盖成功、失败、Prepare取消、服务拒绝、对账完成、切房、卸载、断线、剑灵销毁全部释放路径；参与者离场仅销毁表现、不删除服务已确认助力。
- [ ] 运行失败测试后实现按participant/source/generation索引租约，复用AdvanceReturn/AdvanceAlert；重连剩余Trial只恢复剩余租约，已结束不补播60秒。
- [ ] 原生测试通过后实际渲染PIE各路径取状态证据，保留礼物时长和轨迹回归；涉及轨迹文件的改动必须逐项解释，否则回退。

## T4：综合验证、保护与交付

**Files:** Create `Tools/WulingQA/verify_gameplay_pie.py`、`verify_two_process.py`、`check_preservation.py`、`README.md`；证据统一写入当段`Saved/Codex/WulingGameplayV14/<stage>/`。

**Interfaces:** QA脚本通过命令行路径/独立Saved目录接入，不写默认旧档；输出每条case_id/status/evidence_path，状态只允许passed/failed/not_run/excluded_by_user/deferred_by_spec。

- [ ] 执行前核实本机UE安装和运行进程，必要时按正常关闭接口退出编辑器再native rebuild；不强杀含未保存内容的编辑器。
- [ ] 运行服务unittest全套、两个独立DB连接竞态、真实HTTP恢复和双进程竞采；保存每种失败注入结果，不用配置算术代替事务测试。
- [ ] 运行Editor/Game目标编译及Cultivation/LiveInteraction/LiveWeapons/Wuling相关原生自动化；新失败按实际日志定位；已有失败必须独立复现后才标baseline。
- [ ] 实际渲染PIE闭环：建武灵→加入→完整世界→阶段奖励→来源/制作→主动突破→天劫助力/失败/保底成功→建设/装备→竞采→租约→活动；长时间等待可用只在开发构建中的权威测试时钟，报告中说明，不能用客户端改时间。
- [ ] 原礼物八种映射/五特殊剑、重复份数、剩余时长、主菜单回到游戏、C修炼与鼠标蓄力、剑灵待机/预警/攻击/返回、现有相机及地图参数回归；配置和资产前后哈希核对。
- [ ] 用本轮Backup和每段变更清单核对：不在允许修改列表中的保护文件必须零变化；允许改动的源文件必须有准确diff和必要性说明，不把“哈希不同”当自动通过。
- [ ] 单独检查LogExit之后的崩溃/heap/ensure，报告渲染成功与清洁退出各自状态；未复现或未修复的退出问题继续列未验证。
- [ ] 交付源码、配置导入、启动命令、Blueprint调参所在类/Category/属性、完整/未完成清单、回退步骤。设备、真实Douyin、生产服务、平台审批没有证据则not_run，不宣称完成。
