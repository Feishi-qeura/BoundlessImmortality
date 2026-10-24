# 有界增量审阅 v8

截止以 `selected-inputs-v8.json` 为准：消费15:37 family Source hash、build19结果及新增七组件导入回执，写入本review目录。未运行UE/UBT/测试、未改生产、未看图签视觉。build19记录88 Source/ledger212、15:37:42 exit0；Source快照和运行attempt分别记录，不把此编译结果当运行验收。后续 verifier/status hash变化列在 `source-drift-after-v8.json`。

## 本轮已解决的静态项

- **V7-ASSET-01-import：关闭静态缺导入。** 13候选全部存在，新增七项PNG/uasset哈希逐项匹配导入回执，旧六项uasset哈希未变。**reference：关闭所选Source缺引用。** Rank Insignia、Activities三分类图标和Player-c三图集均有canonical加载/赋值；共享CharacterAttribute图集也补入Art。实际brush回读仍为新增7项+共享复用1处pending。见 `runtime-component-application-v8.md/.csv/.json`。
- **V7-CONFIRM-01：Source修复，运行pending。** 冻结 `InkRebuildFeaturePlayer.cpp:200–201` 在request-confirmation、prepare/rework共同入口对最新 `StageSubjectIdentity(Snapshot)` 与显示的 `StageFingerprint` 比较，不同即清确认、刷新并return；第213行确认时捕获最新指纹，第216行提交前另查 `ConfirmedStageMatches`。第132行显示指纹使用同一helper。helper比较stage_id/from/to；现有native quote gate仍负责缺失/非法stage、selection、kind等。没有添加display Complete/Q/来源字段为提交条件。
- **V7-CYCLE-01：Source修复，运行pending。** 冻结 `InkRebuildFeatureActivities.cpp:33` 设置 `RouteCycle=daily/limited` 并过滤authoritative Categories；完整Activities保留全量。consumer `RebuildActivityWidgets.cpp:227,253` 使用RouteCycle选择分类和标题，三图标由实际分类卡调用CategoryBadge。尚无本epoch真实daily/timed内容回读。
- **商店/listing稳定subject：有界Source链通过，运行pending。** `RebuildEconomyWidgets.cpp:188–201`、`418–440`、`659–680` 将SKU/listing选择回传给host；`InkRebuildFeatureActivities.cpp:77` 保存SelectedEntry/SecondaryEntry、清SemanticKey后return，避免选择事件里同步重挂。刷新VM再用保存的ID投影。Shop `ApplyViewModel:172` 对SKU消失、notallowed、报价version/price改变撤销确认；Consignment第375行起对listing ID/version/price、可买状态变化撤销；Orders第659行起对ID/version/可取消撤销。最终handler仍重取当前SKU/version/price或listing/version/Listed。这里只确认这些改动链，未重做旧owner API审计，也未执行刷新/服务回归。

以上行号属于 `selected-snapshot-v8/Source/...` 保存字节。Selection稳定不表示旧报价确认应永久保留：version/price/资格变化使确认失效正是当前代码的保护。

## 实际入口与几何证据边界

新消费的 **PIE05** 07:19:22–07:22:31 UTC（本地15:19–15:22），**479 checks /31 captures，finished=false、passed=false**。观察到22个不同route：原核心7加Feature15。Character四个原生入口均先通过实际scroll_action，再click_after_scroll，再观察相应子页，故V7-ENTRY-01这一具体offscreen入口路径有后续通过证据；失败04报告及冻结副本继续保留。

PIE05随后在 `typed back OfflineRewards` 失败：`real Slate typed-button click 返回: None`。它是旧attempt的实际失败证据，不据此断言15:37生产源码仍有同一缺陷；integration需用修复后的独立新attempt提供返回闭环。05同样不可覆盖为成功。

PIE05 strictgeometry只适用于其实际captures和其source_epoch，不扩推44路由、两个方向全覆盖，更不扩推后续build19。新增七组件的导入时间晚于05，故22路由观察不能填补新增组件binding证明。四个实际Character入口与后面router遍历也分开：直接调用default frontend router的route观察不冒充每页真实玩家点击入口。

已有三个native fixture Success继续作为native局部证据；本轮读到native-tests-03同样3 Success/0失败/0notRun，未由reviewer执行。它不验证真实服务Busy、freshstage确认、shop刷新业务或新增七组件绘制。ServiceDisabled的展示/路由可用也不等于购买、制作、领取、活动启停或竞赛业务成功。

## 给唯一production integration的短剩余清单

1. 另存新PIE attempt，保留failed04/05；验证真实滚动后的Character四入口和OfflineRewards真实返回闭环，继续区分玩家点击、router观察和整体attempt结果。
2. 对新增七canonical资源及Appearance共享属性图集，记录实际可见consumer/brush回读；import/Source引用不再作为缺口重做。未取得回读均pending。
3. 在新epoch记录freshstage两边界的阶段变化拒绝、daily/limited真实投影、SKU/listing刷新保留同ID及version/price/移除变化撤销确认。已有Source修复不重复派回owner API修正。
4. 严格分开当前attempt几何、all44覆盖、native Busy fixture、真实service Busy/业务和ServiceDisabled展示。真实服务业务与最终用户视觉仍待独立验收。

本轮没有新增生产修复建议超出上述证据缺口。当前screen表为 `stage-to-runtime-v8.csv`：仍为核心7+Feature37的Source接入，当前已消费attempt观察22，不是all44或完整业务验收。未选取的旧Source hash沿用v7并明确标注，不能冒充本轮重审。
