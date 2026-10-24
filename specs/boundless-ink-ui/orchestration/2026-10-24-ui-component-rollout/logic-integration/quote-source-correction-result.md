# 报价门控与新增材料框：有界源码修正

> 2026-10-24 12:17 更新：下文为引擎执行前的历史阶段记录，原文备份 historical-quote-source-correction-result.md。当前 native QuoteContract/相关套件已执行，最终 build11 automation03 共21项通过；实际正常/重炼/拒绝/版本恢复等证据见 [engine-acceptance.md](engine-acceptance.md)。材料框已实际导入并通过 alpha 回读。下文“仅源码/未运行”等不代表当前状态。

2026-10-24，Asia/Hong_Kong。结论：旧“缺展示字段必须禁止Prepare”的判断已撤回。当前改动仅为源码，尚未构建/执行UE测试、导入或运行PIE。

## 授权与实际变更

`../source-correction-grant-quote.json` 明确授权三文件：LogicCore/InkCoreAdapter.cpp、InkCoreAdapterTests.cpp 与 UI/InkWulingToolsPanel.cpp。全部前置SHA256匹配；备份在 `quote-source-before/`。未修改LogicCore其它源文件、服务、原first-source-freeze、原Player ZIP或manifest。

- Adapter把 CanSubmit 与展示完整性分开：原生stage身份、当前境界关系、合法真实选择、selection回显、报价类型、无进行中尝试仍必需；缺失normalized Q/growth/readiness/slots/sources不是请求拒绝。若服务明确给readiness=false或给损坏的readiness字段则仍拒绝。连接/角色/active/Busy继续由原DispatchReady与binder控制。
- Panel移除自己的独立展示AND门控，使用同一 EvaluateQuote.CanSubmit；footer仍同时经过binding与现有请求门控。未知展示保持待同步，不默认为0或Ready。
- 原outbox携带request_id/expected_version/profile/rule_version和服务器事务/库存/安全校验未改。CanSubmit仅表示可发送校验请求，入队不是突破/重炼成功。
- 对建议补丁作两点精确修正：正常原生报价无kind，只有明确rework才可发重炼；未知/损坏kind拒绝。重炼展示不再借用上一轮tribulation概率。
- UE QuoteContract测试源码扩展：缺展示仍可发送正常/重炼校验、缺失字段不变成完整/Ready、错误阶段与小数阶段/错误kind/回显和明确拒绝等。保留既有连接/库存/取消/对账测试。**本轮均未编译或运行，不能称已通过。**

## 新增框消费者

`LogicJourney/InkRolloutMaterialSlotWidget.h/.cpp` 继承已交付Player槽位。实际突破材料目录创建该类，BindSlot最后一参读取隔离目标 `Player/T_BreakthroughMaterialSlotFrame_Default_v1`。图按full Image绘制，框放入144×144尺寸容器居中，未改Player原控件，未使用9-slice。

恢复PNG1254×1254 RGBA、SHA256 `8b93fa72780f31737ca5fc7926b5c79f7f796bcaf14c773cb2ba47b62193d9e3` 与 additive-frame-manifest.json匹配；微弱alpha1像素维持原样，approved:false、视觉待用户。未新生图、未覆盖旧ZIP/manifest、未写Content。

## 精确冻结漂移核验

`quote-source-correction-ledger.json` 记录授权grant哈希、未变的原freeze哈希，以及三文件的before/after。prepare_engine_batch.py逐次核验grant/备份/当前after值，且授权文件集合必须完全相等。原freeze中两处LogicCore变动只有在其原值与grant.before、当前值与ledger.after精确一致时归入authorized_frozen_source_corrections；其他漂移仍阻塞。不是忽略整个LogicCore目录。

完整修改可审见 `quote-source-correction.diff`；首批现为5张纹理，engine-batch与request均已更新。保存WBP仍限原两项保留式榜单迁移。

## 证据边界与下一步

- 只读消费LogicCore真实协议探针：21断言/13请求passed，包含合法Prepare/Rework、幂等重放、过时版本/无库存/不安全/viewer拒绝。证据 `../logic-core/quote-reconciliation-evidence.json`；本聊天未重跑这些探针，也未把它当UE验收。
- 当前静态检查：41源码文件、8个Python脚本无错误，未知冻结漂移为空，精确授权漂移2处；git diff --check exit0。静态检查不是C++/UHT编译。
- 11:13只读快照仍有项目Editor35164和LiveCodingConsole33572，见 engine-occupancy-quote.json。lease=not_granted，无引擎操作，无停止/重启进程。
- 监督在用户保存关闭后授予有界lease，再构建、5张导入/AssetRegistry/alpha回读、2个WBP保存迁移、更新后的UE测试和真实玩家入口/正常突破与重炼请求/拒绝/Pending逻辑验证。source_compiled/import_verified/route_wired/logic_verified保持false。
