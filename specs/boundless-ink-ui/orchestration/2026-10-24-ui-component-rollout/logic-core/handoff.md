# logic-core handoff

独立工作已完成，源码停止写入，可交唯一执行者构建/接入。新源码仅 `Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/`，文档/工具仅本目录；未编辑公共文件、Player/RankWuling 文件或资源。候选仍 `approved:false`，试作已获本批授权。

- `InkCorePolicy.h`：真实适配器共用的离线/Busy/数量/报价/取消/背包门控。
- `InkCoreAdapter.h/.cpp`：角色/背包/三榜/突破天劫/武灵的数据与动作薄适配，复用现有 subsystem/model/projection。
- `InkCoreMaterialBinding.h/.cpp`：已引用并绑定交付的 Player 槽位 atom 的动态 delegate、真实持有量/名称、草稿选择和报价动作。宿主实例化接线见 `contract.md`。
- `InkCoreAdapterTests.cpp`：4 个新增 Unreal 自动化测试，前缀 `BoundlessImmortality.Frontend.Rollout20261024.Core`。**未构建/未运行**，需 engine lease。
- `integration.patch` + `integration-baseline.json`：四个旧公共文件的精确补丁，未应用。`git apply --check --ignore-space-change .../integration.patch` exit 0。正常无参数检查受旧 header 行尾影响失败，不全文件改行尾。

本批实际验证：

1. `run_policy_tests.cmd`：MSVC C++17 /W4 /WX；红阶段 41 checks/12 failed，绿阶段 **41 checks/0 failed**。日志 `policy-red.txt` / `policy-result.txt`。不涉及 UE 引擎。
2. 依赖运行时 Python `-B run_service_tests.py`：真实 Authority 的 foundation/world_inventory/tribulation/focus_loadouts/metrics + 本批报价探针，**40 tests/0 failures/0 errors**。结果 `service-test-result.json`，详细 `service-tests.log`。数据库在本目录 test-databases，未改生产存档/服务源码。Windows sandbox 的 tempfile 0700 ACL 初次不可读，现用普通 mkdir 的隔离目录，仅 runner 改动；通过后未扩大测试。

阻塞/缺口：`real-quote-probe.json` 证实当前服务未返回 normalized Q、成长区间、readiness、slots、sources 等完整展示字段；Prepare 正确保持禁止，不能称有数据业务 UI 已验收。三榜的既有 Anchor 指标是剑灵数量，不是武灵贡献/集体成长。缺少报价版本字段。详见 `contract.md`，未擅改玩法/数值。

下一步（监督者转交 logic-integration）：

1. 对照公共文件 hash 和本批 panel-player/panel-rank handoff，应用或手工合并精确 patch；保持现有 routes，不开重复页面。
2. 在实际突破宿主 WidgetTree 中创建槽位并保留 `UInkCoreMaterialBinding`，绑定 OnFeedback/Submit/Refresh；导入纹理由唯一执行者落实。三榜传入既有发布者数据，只读 getter 在 patch 中。
3. engine lease 后统一 UBT；执行新增 4 个 Core 自动化测试及原 Inventory/Leaderboard/Wuling 受影响测试；验证真实玩家入口、Busy、离线、空态、至少一个非空库存/榜单、权威拒绝和天劫状态互斥。原生编译或运行发现问题可续派本队修复。
4. 服务展示字段缺口需权威 owner 给正式契约后补齐并复验；不得填造数值。维持 import_verified=false/route_wired=false/logic_verified=false，直到执行者记录本批证据。视觉始终待用户。

## 2026-10-24 只读契约续派：重要更正

最新结论以 `quote-contract-reconciliation.md` 为准：此前将 normalized Q/readiness/slots/sources/成长区间缺失作为必须阻止 Prepare 的服务缺口不准确；真实服务合法 Prepare/Rework 不需要这些展示字段。当前 adapter 和 panel 两层门控构成退化。21个窄探针断言、13个真实Authority请求确认；未重复上一轮完整测试集。

三文件修正建议位于 `quote-submission-minimal-unapplied.patch`，`git apply --check --ignore-space-change` 通过，**没有应用**。Source/服务仍冻结，9个读取输入hash前后一致。监督者将该报告/patch转交logic-integration；解除冻结后统一审阅、构建与原生/实际入口验证。本次仅文档/工具新增，不发送跨聊天消息。
