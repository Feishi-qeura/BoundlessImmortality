# logic-integration · 报价修正与材料框源码已交接

**已撤回旧“缺展示字段必须禁用Prepare”的结论。** 两层门控现按原生stage/当前境界、真实selection与回显、报价类型和无活跃尝试核验；缺展示字段保持unknown，不阻止向服务发送校验请求。连接/Busy/角色/active、版本/幂等与服务最终资格未绕过。请求入队不是突破或重炼成功。

- 已精确合并授权3文件：`LogicCore/InkCoreAdapter.cpp`、`InkCoreAdapterTests.cpp`、`UI/InkWulingToolsPanel.cpp`。原hash匹配，备份与增量见 `quote-source-before/`、`quote-source-correction.diff`。
- 当前审阅结果：`quote-source-correction-result.md`。补丁额外最小修正包括未知kind拒绝、重炼不借旧天劫概率；新增/调整UE测试源码**未运行**。
- 新增实际材料框消费者：`LogicJourney/InkRolloutMaterialSlotWidget.h/.cpp`；真实宿主BindSlot传FrameTexture，full图正方形，不做9-slice。恢复PNG1254×1254 RGBA及SHA256已核对，alpha1微点保留视觉待用户；没改原ZIP/manifest，没新生图。
- `prepare_engine_batch.py` 对原freeze哈希保持核验；只有 `quote-source-correction-ledger.json` 中授权前/后hash精确一致的两处LogicCore漂移可通过，其余漂移阻塞。原freeze未改，授权三文件集合必须完全相等，当前未知漂移=[]。
- 首批 `engine-batch.json` / `engine-batch-request.json` 已从4增到**5张**有实际消费者的纹理，仍只有2个保留其它页的榜单WBP保存迁移；无需导入未消费的其它候选。
- 本轮证据：41源码文件、8脚本静态检查无错误，git diff --check exit0；LogicCore真实协议探针21断言/13请求passed仅只读消费，见 `authority-reconciliation-consumed.json`，未在本聊天重跑，不能当UE验收。

**阻塞**：11:13仍有项目Editor35164和LiveCodingConsole33572，`engine-lease=not_granted`；未停止/重启任何进程，未运行Editor/UBT/Live Coding/导入/Content写入。

**监督续派**：用户保存关闭并授予有界lease后，重建→更新后的Core/Journey及受影响UE自动化→5张实际导入/AssetRegistry/alpha回读→2个WBP保存迁移→PIE真实玩家入口与合法正常/重炼请求、拒绝、Pending/幂等验证。展示unknown不应变成默认值或资格授权。逐页缺项仍见44屏screen-coverage.md/json。

source_ready/assets_ready/source_route_wired=true仅表示源码/待导入资产准备；source_compiled/import_verified/actual_wbp_migrated/route_wired/logic_verified=false；approved:false、visual_pending_user=true。旧交接已备份 handoff-before-quote-correction.md，不能继续沿用其错误门控判断。
