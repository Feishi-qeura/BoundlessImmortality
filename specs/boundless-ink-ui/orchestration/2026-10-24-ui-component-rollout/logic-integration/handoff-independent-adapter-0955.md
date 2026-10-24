# logic-integration · 独立适配里程碑交接

最后完成：LogicJourney C++ 适配器 + UE guard 测试源码、真实权威服务18项测试通过、磁盘依赖预检。尚无任何完整面板交接，engine-lease=not_granted，因此结束当前回合等待监督续派；未启动 Editor/UBT/导入，没有公共路由/保存 WBP 改动。

## 实际产物
- `Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h/.cpp`：Capture/Read/ActivitiesForCycle/Evaluate/Submit，缺字段可区分，签到跨日/月签3/7/14/21/真实地图确认/主播开服门控。Submit 直接调用真实 UWulingPrototypeSubsystem，不发模拟奖励。
- 同目录 `InkJourneyAdapterTests.cpp`：`Boundless.InkUI.Rollout20261024.Journey.Guards`，未构建/执行。
- 本目录 `adapter-contract.md`：字段、动作、UI调用方式、需求差异/续队登记与验证命令。
- `run_authority_tests.py` / `authority-test-result.json` / `authority-tests-isolated.log`：18 tests OK（实际 Python 权威层，非 UE验收）。原始临时目录 ACL 失败留在 authority-tests.log，隔离 runner 仅更换数据库临时目录分配。
- `preflight_batch.py` / `preflight-result.json`：四面板未 ready，无 handoff；公共文件原哈希一致；engine_started=false。
- `public-baseline-sha256.json`：8个可能接入公共文件的原哈希，尚未修改。

## 下一步由监督者续派
1. 至少一队完成源文件+组件资产+导入脚本+路由说明后，读其 handoff 并重新跑 preflight；四队缺失部分继续登记，不假装全批可用。
2. 面板源码停止写入后，请授予 **logic-integration 有界 engine lease**（指定首批目录/动作/期限），再运行 UBT。当前仅请求排队，不申请提前启动；未授予不可构建/导入。
3. 顺序消费 logic-core 公共 patch 建议及面板壳，接入唯一 ClassForRoute/保存 WBP 迁移，实际导入并回读 Texture2D/AssetRegistry/引用，完成玩家入口。
4. 接新壳到本适配器，公共 SignInClaim 直接调用需统一点击时门控；分组子页需正确 active 判定。非空业务/失败/断线/重复点击/跨日/月测试 + 本批 UE automation + PIE 另报，不能拿本次18项服务测试代替。
5. approved:false / visual_pending_user 保持。现有 native-paper 和脚本不是组件真实接入。

业务差异：旧 interaction 写每日300修为，现服务签到无经济奖励且七日为占位。本批遵循真实服务，不改奖励。每日一签是固定轮换剑诀，未提供抽签/领奖接口。详见 adapter-contract.md。

复跑（项目根）：
`& 'C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe' specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/run_authority_tests.py`
`& 'C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe' specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/preflight_batch.py`
