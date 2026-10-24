# Bounded v7：37 个生产 FeatureHost、独立几何证据及新组件缺口

本轮只读取现有 Source、owner manifest、导入回执和 integration 报告，并写入本 review 目录。未运行 UE/UBT/测试、未导入迁移、未查看图片作视觉验收、未改生产文件。选定字节及哈希保存在 `selected-inputs-v7.json` / `selected-snapshot-v7/`；补充证据有独立捕获时间。后续七个选定输入发生变化，记录在 `source-drift-after-v7.json`，不追赶为同一批准 epoch。

## 证据结论

- **N05 Source 接入已从旧 7/0/37 更新为核心7 + Feature37 = 44。** Root `CreateRebuiltFeaturePage` 真正创建 `UInkRebuildFeatureHost`；Extensions 先走新 canonical resolver，保留独立 route/context，避免旧 alias 合并。Host 根据 PageId 创建 owner 的 UUserWidget，Player 七页则调用原生 builder 放入 FeatureHost scroll。`stage-to-runtime-v7.csv/.json` 每行记录实际 route、composition、context 与 Source hash。
- `WulingMembers` 为 Wuling 的辅助成员页，不多算第45屏。37 页为 **source-wired-lazy**，不等于已逐页运行通过；Daily/Timed cycle 的实际投影仍有下列缺口。
- build14 exit0，86 Source/ledger176，14:58:48 完成；build15 exit0，86/178，15:00:05 完成。记录是编译检查点；之后 Source 改动不自动获得其编译证明。
- PIE04 07:04:44–07:06:04 UTC，PID17164，**229 checks /17 captures，finished=false、passed=false**。核心7个 route 和 Feature Extensions/WorldCultivation 的实际 default WBP、Host、独立根、owner composition 已有通过断言，因此本轮观察为 **9个 route，其中新 Feature2个**。不宣称 all44、37页运行成功或完整页面验收。
- 失败为 `real Slate click OpenEquipmentFabao: None`。`character.json` 中按钮 enabled/visible，但 cached 0×0，独立排列 y1539..1635 已在 clip171..1244 之外。这支持“应先真实滚动到目标再点击并等待布局”的最小修复方向；不凭此断言 native action binding 已坏。保留 failed04 和其冻结副本，待单独记录的新通过尝试，禁止覆盖失败为成功。
- 已有 Native tests01 三项 Success、零失败继续成立，不要求重复执行。它们属于 build11 的 fixture epoch；Busy builder fixture 与真实服务 Busy 运输流程分别记录。

## 独立几何证据链

Editor producer 从有效 Slate 根 `GetPaintSpaceGeometry()` 出发，逐层 `ArrangeChildren`，用 `GetRenderBoundingRect` 与 `CalculateCullingAndClippingRules` 得到独立边界和裁剪区域。其已知/剔除判定不再依赖被检 UWidget 的 cached Position、PaintedEnd 或 finite-based `geometry_arranged`。有限且有面积的祖先完全在裁剪区外时，将该祖先 bounds/clip 作为后代剔除证据；缺失、非法或未遍历状态为 unknown。

`geometry_metric_validation.py` 对 visible 非有限值，仅在祖先名称存在、八个 proof 数值有限、祖先有正面积且与 clip 完全分离时允许剔除。unknown、重叠的伪剔除和非法 proof 均拒绝，原有循环过滤已在 Source 层修复。它是独立排列/裁剪证据，并非实际像素或 OnPaint 回调的证明。

PIE04 已有17次 strict geometry capture 检查及17次 deliberate expected-painted 非有限注入拒绝通过；landscape 的40项、末端8项均标为独立剔除，未以旧 finite gate 丢弃。五个 bad/unknown/culled/fake-cull/invalid-proof fixture 在 validator 源码存在，调度说明其执行已有记录；本次有限目录核对未找到单独持久执行回执，故不把本 reviewer 视为重跑执行者。**V5-VERIFY-01 改为 Source 已修复、局部证据通过、整体几何仍 provisional**，等下一轮完整通过记录。

## 每族只抽样一至两条关键链

| 族 | 本轮抽样链 | 静态结论/限制 |
|---|---|---|
| Player | Character→EquipmentFabao；突破确认→prepare/rework | 前者实际点击失败并有 offscreen 证据；后者 selection/rework revision 与 native 当前 quote 检查存在，但缓存 stage revision 未在最终提交前对最新阶段指纹重验，见 V7-CONFIRM-01 |
| Journey | MonthlySignIn 领取；World/Resource travel | handler 捕获 day/month/milestone 并经当前 adapter 重验日期、资格/claimed；travel 精确 EntryId 经当前 open/node/actor/Busy gate。只有 WorldCultivation 展示/导航有此次 PIE 证明，真实领取/旅行未验证 |
| Activities | StartLaunch 确认；Daily/Timed 路由分类 | captured anchor/guild/profile/rules 与当前比较，adapter 重验 known-absent、profile/window、所属主播；但 daily/timed hub 仍用同一未筛选 Categories，缺少 route cycle 语义投影 |
| Economy | Purchase；Buy/Cancel listing | captured SKU/price/snapshot version 和 listing ID/version/Listed 状态在提交前重取。通用 Submit 再验 active actor、Connected、!Busy、anchor；不据此宣称交易成功 |
| Rank/Wuling | RemoveMember；Build | 移除用当前 selected UserId、当前 members、非 guild anchor；建设重取 building ID/level/snapshot version。通用 gate 存在，实际服务权限、确认闭环和业务结果未验证 |

这是 bounded chain 记录，未扩展成各族所有动作的审计或 runtime 测试。上述 gates 的存在不等于已覆盖所有未知值、真实服务许可或传输结果。

## 交 integration 的精确缺口

1. **V7-ENTRY-01/P1**：failed04 的真实 Character 入口点击。先真实滚动、等待布局、再点击；新结果另存新 attempt。
2. **V7-CONFIRM-01/P1**：冻结 `InkRebuildFeaturePlayer.cpp:120–121,195–200` 的 stage fingerprint 仅在 RefreshPlayer 推进；HandlePlayerE 的 request-confirmation/prepare/rework 对比缓存 revisions。若同 actor 的新 stage+有效新 quote 在下一次刷新前到达，旧确认仍可能用于新阶段。Core adapter 重验的是当前 quote，而非用户已确认的 stage identity。最小修复是在两个确认/提交边界将最新 authoritative stage fingerprint 与已显示/确认指纹比较，变化则关闭确认并要求重确认。此为 integration 缓存闭环新缺口，不要求 owner 重做已完成的 stage revision overlay；不得把 display Complete、Q或来源字段加为 quote 提交前置。
3. **V7-CYCLE-01/P2**：冻结 `InkRebuildFeatureActivities.cpp:33` 的 `RefreshActivities` 对三个 hub class route 统一 `Vm.Categories=Categories`，DailyActivities/TimedActivities 未筛选 daily/limited，也未传 selected cycle；detail route 第35行已按 PageId 选择 cycle。补显式 cycle 投影，Activities 保留全量 hub。
4. **V7-ASSET-01/P2**：新运行组件13项中6项有导入及局部绑定回读，**7项缺失：Rank徽记1、Activities语义图标3、Player-c图集3**。Player-c 的共享 CharacterAttribute atlas 已存在但该 composition 未给字段赋值，直接复用即可。详见 `runtime-component-application-v7.md/.csv/.json`，不得将582迁移纹理算作全套新组件应用。

以上行号均对 `selected-snapshot-v7/Source/...` 中已保存字节，不能套用到后续改动的生产行号。资产赋值锚点另为冻结 `InkRebuildFeaturePlayer.cpp:54`、`InkRebuildFeatureRank.cpp:13`；独立几何锚点为冻结 EditorLibrary 第306行起。

旧 owner-v5 六组修正继续结案；旧 avatar backoff finding 本轮未重审，不因缺少本轮讨论而宣称关闭。服务禁用态可展示44路由仍只是展示接入目标；quote、购买、制作、武灵、竞赛等真实服务动作、真实 Busy、global server board、设备和最终视觉验收均另需证据。
