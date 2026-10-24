# 突破报价：真实提交契约与展示契约核对

日期：2026-10-24，Asia/Hong_Kong。范围：只读冻结的 Source/服务实现，隔离 Authority 探针；新增文件仅本 logic-core 目录。没有启动 UE/UBT/HTTP listener，没有改动 Source、服务或生产数据。本文更正本目录上一轮 contract/handoff 中“缺完整展示字段所以必须阻止 Prepare”的判断；旧文档保留作为历史，不再作为该判断的依据。

## 结论

**当前适配器会持续禁用现有服务本来允许的正常突破与重炼请求。根因是把展示完整性当成了提交必要条件。** 不需要先向服务补出 normalized Q、成长区间、readiness、slots、sources，才能恢复合法请求。这些字段缺失属于展示能力缺口；缺失本身不是权威拒绝。

必须区分：Quote 成功也不是允许突破的授权。服务在 Prepare/Rework 执行时重新报价、核实修为/阶段/角色/库存/活动状态并作事务结算。客户端可以保留最新报价与本地选择匹配的防误点门控，但应称“可以发送校验请求”，不能称“服务确认准备完成”或“已突破”。

现有旧面板本身已有严格的 `BreakSubmissionReady` 展示门控；不能声称旧 GUI 曾实际通过。本批又把它下沉到 `EvaluateQuote -> CanSubmit -> PrepareBreakthrough`，新 binder 及实际宿主的两层门控均会阻止服务合法请求。只修其中一层仍会残留禁用。

## 一条真实请求链

1. `InkCoreMaterialBinding.cpp:62-72` 增加真实库存范围内的本地选择，调用 `InkCore::QuoteBreakthrough`。`InkCoreAdapter.cpp:216-221` 通过 connected/not busy/active/anchor 门控，提交 `{selection:[{item_id,count}]}`；没有提交 Q、roll、helpers 或奖励。
2. `WulingPrototypeSubsystem.cpp:90-98,124-128,169-174` 将请求写入 outbox，携带 request_id/profile/rule_version，在发送时绑定 expected_version。`http_server.py:47-53` 将 `/v1/wuling/command` 原样交给 `Authority.execute`。
3. `authority.py:227-273` 校验身份会话、幂等、规则档和版本，在事务中执行 handler。QuoteBreakthrough -> `Breakthrough.preview`；PrepareBreakthrough -> `Tribulation.prepare`；QuoteReworkQuality/ReworkQuality -> `Breakthrough.rework_preview/rework`。handler 映射在 `authority.py:37-51`。
4. `breakthrough.py:12-16,19-46` 真实正常报价只保存 stage/q/band/core_score/weakness_seconds/selection，加上 probability_basis_points。`authority.py:212` 将已保存对象放到 result.breakthrough_quote，未添加或重命名上述展示字段。
5. `WulingPrototypeSubsystem.cpp:196-220` 对 command 取 **Data.result**；检查 manifest/rules_hash，成功且非旧版本才把该对象直接赋给 State。`Snapshot()` 返回 State。它没有在解包时丢失 normalized/readiness/slots/sources，也没有把它们放在其它包装层。
6. `InkCoreAdapter.cpp:134-174` 直接读取 State.breakthrough_quote。当前 line172 的 Complete 和 line144 的 ServiceReady、line165 的 SlotsMatch/CoreSlotsValid，因为真实响应没有展示字段而为 false；`InkCorePolicy.h:CanSubmitQuote` 将这些值全部 AND，于是 CanSubmit 恒 false。binder line75-83 及已接入 `InkWulingToolsPanel.cpp:406,602,632` 又据此禁用和拒绝提交。

以上是源码逐层追踪；本轮实跑是相同 Authority JSON 协议的进程内调用，不是 UE HTTP/PIE 执行证据。

## 字段逐项核对

原始证据目录：`quote-reconciliation-probe/run-899e1b2af19b4a47b7e891af55fc2967/`。下表 Q 指 `00-normal-QuoteBreakthrough-response.json` 的 **result.breakthrough_quote**，该原始对象在 line559-600；R 指 `04-normal-QuoteReworkQuality-response.json` 的同名对象 line622-664。所有“缺失”均由原始对象的键集合确认，非数值为零。

| 字段/实际位置 | 实际权威实现及原始位置 | 分类与处置 |
|---|---|---|
| request_id / expected_version / rule_version / profile | `authority.py:231-245`；每个 `*-request.json` 顶层；正常请求 `00-normal-QuoteBreakthrough-request.json` | **required-for-submit（协议）**。保留 outbox、版本和 receipt 幂等；不得客户端跳过或自行结算 |
| identity.role / 会话权限 | `tribulation.py:14`、`breakthrough.py:20,63`、`contracts.py:anchor_only`；响应 result.identity.role | **required-for-submit（服务器授权）**。客户端继续需要 anchor；服务仍最终鉴权，viewer 实测 FORBIDDEN |
| args.selection[].item_id / count | `breakthrough.py:24-36` 分类、阶段、整数/数量；`inventory.py:38-63` 当前库存/过期/预留；Q.selection line571-576 | **required-for-submit（真实请求）**。保留非空合法选择、回显匹配、重复项/数量防错。服务的 core/guard/essence/treasure 规则在执行时重验 |
| Q.selection | `breakthrough.py:46`，Q line571-576 | 本客户端可保留的报价一致性门控；**服务协议不要求先持有 Quote**。不可把“存在匹配 Quote”解释为库存或安全资格已获批准 |
| Q.stage.stage_id / from_realm_index / to_realm_index | `breakthrough.py:22-23,46`；Q.stage line577-600，from line582、to line594；result.growth.realm_index 是当前阶段 | 服务重算所需阶段来自权威表/成长。客户端用真实嵌套字段作当前/目标身份核对，**不需要不存在的 target_stage 文本** |
| current_stage / target_stage 文本 | Q 中两者均没有；当前名可用 result.growth.name（`growth.py:46-53`）；目标只有 Q.stage.to_realm_index 和 stage.name | **display-only**。目标不是从一个被漏解包的 target_stage 字符串获取。可按现有权威 realm 名查询，未知保持未知，不解析箭头文案来作资格判断 |
| Q.q | `breakthrough.py:44-46`；Q line570 = 10，R line633 = 35 | **display-only（真实计算结果）**。显示返回值；服务器执行重新计算。它不是客户端提交参数，不应作为准备度授权 |
| Q.q_normalized | 正常/重炼原始对象均缺失；服务全文无写入该 key 的代码 | **unavailable + display-only**。归一化定义 unknown；不私自定义为 q/100，也不默认 0；移出提交门控 |
| Q.band / quality / quality_band | 实际有 band，Q line560-568，包括 band_id/name/q_min/q_max/live_snapshot_factor/source_indie_multiplier；`breakthrough.py:45-46` | **display-only，部分是命名/层级差异**。真实品质名在 band.name，不能把没有 quality_band 顶层字段视为服务拒绝 |
| Q.growth_min / growth_max | 原始 Q/R 均缺失；band 中是两个 multiplier，不是区间 | **unavailable + display-only**。增长预测区间与单位 unknown；不得把 multiplier 拼成上下界或把它当 submit 条件 |
| Q.readiness / Q.readiness.ready | 原始 Q/R 均缺失；preview 只计算材料 Q，不核实背包、安全状态或活动锁 | **unavailable（并非当前授权字段）**。不能设成“必须 true”。如未来明示 false，保守拒绝该请求；缺失时显示“服务未提供，提交时校验”，不假装获得 Ready 授权 |
| Q.slots / category / slot_category | 原始 Q/R 均缺失；服务内部 `breakthrough.py:26-38` 查 breakthrough_material_items 表并验证四类数量，返回的是 selection 而不是 slots | **unavailable + display-only**。不是 parser 读错嵌套路径。保留 selection 的真材料/数量；分类未知，不编造 slots；不能用 slots 缺失/展示结构错阻断合法原生请求 |
| Q.sources | Q/R 均缺失；另一个契约是 `Catalog.snapshot` kind=sources 的 catalog.rows[].source，`catalog.py:22-24,37` | **display-only；报价未提供，另有独立目录来源查询**。本轮没有执行 sources 查询，不能声称原始 Q 已有来源。获取方式不构成持有资格，不作提交门控 |
| Q.probability_basis_points | 正常 Q line569 = 5000，`breakthrough.py:15`；重炼 R 中没有该字段 | **display-only**。正常预览来自权威概率函数；不从 q 推算概率。重炼没有随机天劫概率契约，不应借用上一次 tribulation.probability_basis_points 来显示重炼概率 |
| tribulation.status / id / trial_start / server_utc | `tribulation.py:30-35,61-65`；正常 Prepare 响应 result.tribulation | **required-for-submit（取消/对账的动作相关状态）**。保留 Prepare/Trial/Reconcile 防重复、取消时间窗、有效尝试编号；不让展示字段改变这些动作 |
| growth.qi / qi_to_next / combat / activity / 实例库存 | `tribulation.py:15-28`、`breakthrough.py:49-54,62-70`、`inventory.py:38-63` | **required-for-submit（服务最终资格）**。Quote 不替代这些检查。客户端不伪造 readiness，不跳过服务，不提前消耗、发成功奖励或更改阶段 |

## 本轮窄探针结果

工具：[probe_quote_reconciliation.py](probe_quote_reconciliation.py)。汇总：[quote-reconciliation-evidence.json](quote-reconciliation-evidence.json)。**21 项断言、13 个真实协议请求通过**；未重跑上一轮 40 项完整测试集。夹具只在本目录数据库显式设置境界/材料/安全状态；这些数据不用于 UI 默认值或生产账本。

| 真实请求/反例 | 原始响应文件 | 已观测结果 |
|---|---|---|
| 正常报价，6 个展示字段缺失 | `00-normal-QuoteBreakthrough-response.json:3-4,559-600` | success；真实 Q/阶段/回显齐全 |
| 同材料旧版本 Prepare | `01-normal-PrepareBreakthrough-response.json:3-4` | STALE_VERSION，未预留材料 |
| 同材料合法 Prepare | `02-normal-PrepareBreakthrough-response.json:3-4,41-100` | success，进入 Prepare；仍没提供那 6 个展示字段 |
| 同 request_id 重放 | `03-normal-PrepareBreakthrough-response.json` | 与上次响应完全一致，只有原尝试 id |
| 完成后的真实重炼报价 | `04-normal-QuoteReworkQuality-response.json:622-664` | success，kind=rework，未提供 readiness/slots/来源/预测，也未提供重炼天劫概率 |
| 结算后清理期内重炼 | `05-normal-ReworkQuality-response.json:3-4` | ACTIVITY_BUSY；证明 Quote 成功不等于动作获批 |
| 原 cleanup_at 后重炼 | `06-normal-ReworkQuality-response.json:3-4` | success；未补展示字段也可执行 |
| 无库存先报价，再 Prepare | `07-unowned-QuoteBreakthrough-response.json` / `08-unowned-PrepareBreakthrough-response.json` | 报价 success；执行 INSUFFICIENT_FUNDS |
| 不安全状态先报价，再 Prepare | `09-unsafe-QuoteBreakthrough-response.json` / `10-unsafe-PrepareBreakthrough-response.json` | 报价 success；执行 NOT_SAFE |
| 没有先请求 Quote 的合法 Prepare | `11-direct-no-preview-PrepareBreakthrough-response.json` | success；服务不要求 quote token/readiness |
| viewer 准备突破 | `12-direct-no-preview-PrepareBreakthrough-response.json` | FORBIDDEN |

重炼探针最初在结算+76秒就期待成功，真实服务返回 ACTIVITY_BUSY；跟踪 `tribulation.py:127-136` 的清理状态后，改为先断言这个拒绝，再在原 cleanup_at 后请求成功。没有调整规则或服务时间窗。

## 最小修正建议：未应用

产物：[quote-submission-minimal-unapplied.patch](quote-submission-minimal-unapplied.patch)，基线 [quote-submission-patch-baseline.json](quote-submission-patch-baseline.json)。仅建议改三个 Source 文件，由 logic-integration 在解除冻结后审阅、应用并统一验证：

1. Adapter 将 `CanSubmit` 与 `Facts.Complete/ServiceReady/SlotsMatch/CoreSlotsValid` 的展示诊断分开。请求仍要求现有服务连接/角色/active/not busy、合法选择、成功报价的原生 stage 身份/当前境界关系、选择回显、kind 和无活跃尝试；未来明确 readiness=false 仍拒绝。不计算 Q/预测/概率，不把 missing readiness 填 true，不把 slots/来源作为持有凭证。选择数量限制对齐服务的最大64条、每项整数1..100。
2. Panel 的 `BreakSubmissionReady` 改读同一适配器决策；旧长 AND 仅命名为 BreakDisplayComplete，显示未知资料提示。避免 adapter 已放行而 UI 仍永久禁用，以及错误文案“服务未确认准备度所以不可提交”。
3. 原生测试改用真实存在的 stage 索引形状；新增“展示字段缺失可发送请求”“明确拒绝仍禁止”“实际报价 stage 身份缺失仍禁止”断言。原测试把 missing normalized Q 当成必须禁用的断言应纠正。

`FQuoteFacts.ServiceReady` 不会被伪造为 true；它仍如实表示该字段没有提供。`FQuoteDecision.CanSubmit` 的含义收窄为“允许发起权威执行校验请求”，不是所有业务条件已满足。原纯策略的 Complete AND 只继续作旧展示诊断测试；上一轮41项门控结果不能证明这个建议补丁运行通过。

补丁只读检查：`git apply --check --ignore-space-change .../logic-core/quote-submission-minimal-unapplied.patch` exit 0。普通 check 的行尾差异沿用本项目处理方式。不应用 patch，不生成任何新的服务字段。

尚待执行者验证：统一构建、更新后的原生自动化、已保存 WBP/实际入口、Busy/断线/版本冲突反馈和至少一次真实库存充足的 Prepare。重炼 UI 仍可能从旧 tribulation 对象显示历史概率；这属于另一个展示问题，不能将本建议补丁表述为已修正该文案。新的服务准备度接口、归一化定义、成长区间定义均继续 unknown。

冻结证明：probe 汇总存有9个实际读取的 Source/服务文件 before/after SHA-256，均一致；可选补丁生成器只读取冻结文件并向本目录写 diff。没有进行生产数据变更或引擎操作。
