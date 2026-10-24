# logic-core adapter contract

试作授权来自本批 README；本工作不改变任何候选的 `approved:false`。源码只新增于 `Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/`。未执行 Editor、UBT、导入或公共文件改写。

## 已核实现状

| 领域 | 数据所有者 | 现有动作/约束 | 本批适配 |
|---|---|---|---|
| 角色 | `UCultivationSubsystem`；`WulingSnapshotProjection` 在权威模式镜像真实成长；`AInkMenuDirector::PublishGameplayData` 发布展示属性 | 不在 UI 计算属性或评分数值 | `InkCore::Read` 复用 frontend 已发布属性、功法；权威离线或 growth 缺失时不返回旧数据 |
| 背包 | `WulingInventoryProjection` + `UInkInventoryModel`；显式 legacy 本地模式保留 | 权威投影不本地扣除；服务按 item_id 最早到期实例结算 | 使用稳定 EntryId，点击时核对当前实例、预留、服务时间/过期与数量；拒绝超量，不自动缩量；装备/学习/使用/回收均走原命令 |
| 突破/天劫 | `UWulingPrototypeSubsystem` + `Tools/CultivationService/{breakthrough,tribulation}.py` | Prepare/Rework/Cancel/Reconcile；取消仅服务时间 `< trial_start`；Prepare/Trial/Reconcile 防二次准备 | `EvaluateQuote` 核对选择回显、槽位汇总、核心、报价类型和准备度；点击重新门控 |
| 三榜 | 房间：`PublishGameplayData`；全局：`UCultivationPrototypeClient::GetLeaderboardEntries` | `Anchor` / `CultivationSpeed` / `Damage`；Local/Global 不混用 | `ReadBoard` 只保留提供者名次、缺口、身份、指标；重复身份/名次或非法指标 -> Error；未发布房间响应 -> Loading |
| 武灵 | `UWulingPrototypeSubsystem::Snapshot` 的 guild/counts/members | 组织归主播创建；viewer 个人剑灵、主播成长、组织成长不同 | `Read` 返回 guild 原对象；CreateGuild/RequestMembers 通过现有服务；权限来自 identity.role=anchor，缺失时禁止 |

`FActionResult.Accepted` 只表示本地模型动作实际执行或权威请求已入队；`PendingAuthority=true` 不是已结算。不得播放到账/突破成功或本地增减资产。使用现有奖励 receipt 通道。

## 调用方式

头文件：`Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h`。

- `Read(Frontend, Active)` -> `FView`：CharacterState/InventoryState/GuildState、真实展示字段、只读 JSON 引用、CanAct。
- `ReadBoard(Context, RoomRows, Scope, Category, RoomDataProvided)`：调用方必须明确是否收到房间数据。全局自行从既有全局服务取数。
- `InventoryAction(Frontend, EntryId, Action, Quantity, Active)`：支持 Use/Sell/Decompose/Equip/Unequip；权威档案不支持 Decompose。
- `QuoteBreakthrough(Context, Selection, Rework, Active)`；`PrepareBreakthrough(...)`；`CancelOrReconcile(Context, Active)`。
- `CreateGuild(Context, Name, Active)`；`RequestMembers(Context, Cursor, Active)`。
- `EvaluateQuote(Snapshot, Selection, Rework)`：可直接给准备/取消/对账按钮门控，不发请求。

`Active` 必须取实际 frontend.CurrentPage 与宿主 PageId 是否相等，不是常量 true。业务入口仍用现有 Character/Inventory/Leaderboard/Wuling/WulingBreakthrough，不能增加重复顶层路由。

## 已绑定的 Player 槽位接口

`UInkCoreMaterialBinding` 已在本队新源码中引用并绑定面板队交付的 `UInkPlayerMaterialSlotWidget`，不修改 Player 文件。

宿主在自己的 UCLASS 中保留 `UPROPERTY(Transient) TObjectPtr<UInkCoreMaterialBinding> MaterialBinding`。创建时执行：

```cpp
MaterialBinding=NewObject<UInkCoreMaterialBinding>(this);
MaterialBinding->Configure(FrontendOwner,PageId,bRework);
MaterialBinding->BindSlot(MaterialWidget,SlotId,RealItemId,ItemTexture,FrameTexture);
```

MaterialWidget 必须是宿主实际 WidgetTree 中的 CreateWidget 实例。绑定者从当前服务 inventory 和 item_instances.definition 读取持有量/名称，缺字段显示 `—`，不编造需求数量。底部文字是本地草稿的“已选 n”，不是业务需求、奖励或进度。没有权威数据时不可选择。

`OnSlotActivated` 已动态绑定：验证确为已绑定槽位 -> 对照服务持有量 -> 新草稿数量 +1 -> 请求 QuoteBreakthrough/QuoteReworkQuality；入队成功后保存草稿。Busy 后重复点击被阻止。选择上限沿用服务每 item 100。

宿主刷新调用 `MaterialBinding->Refresh()`；清空调用 `ClearSelection()`；确认按钮门控 `CanSubmit()`、回调 `Submit()`；`OnFeedback` 显示请求状态。模式/页面重配调用 Configure，它会解除旧 delegate 并清空草稿。不要每次 tick Configure，也不要直接广播 delegate 绕过点击。

这完成原子控件到真实数据/动作的源码绑定；宿主实例化、加入布局、路由和运行回读仍由 logic-integration 完成。

## 状态与权限

- Unavailable：服务不存在/关闭；Offline：服务失联；Loading：未收到当前领域字段；Empty：明确返回空集合/null 组织；Data：真实数据；Error：字段类型/身份/名次非法。
- Busy 是独立门控：可保留已确认数据，但任何写入按钮不可用。
- 权威模式的动作需要 connected、active、not busy 及 identity.role=anchor；服务器仍最终裁决规则、名称、资金与所有权。
- 全局榜服务是另一既有 subsystem；其连接状态独立，不将武灵连接或本地榜当全局连接成功。

## 已实测的缺口（不可默补）

1. `real-quote-probe.json` 为本批真实 Authority 数据库执行 QuoteBreakthrough 的测试快照。当前报价提供 q/band/stage/selection/probability_basis_points，但缺少 `q_normalized/growth_min/growth_max/readiness/slots/sources`，且没有 UI 所需显式 target_stage 名称。本批不重算/添加这些字段，不把合成完整 fixture 当服务验收。Prepare 保持禁止，等待权威服务契约补齐后复验。
2. 现有 `ToolPrepare` 的 UI 禁用逻辑不等于处理函数重新鉴权；精确补丁改成适配器点击时再校验。
3. 房间 Anchor 榜现行指标是 gameplay swarm participant count + anchor，来自现有发布者；它不代表武灵集体进度或任意“战力”。全局从另外的服务 value/rank 读取。本批不改这个指标，不合并两种来源。
4. 武灵“贡献记录/排行周期/奖励/管理权限”没有本批确认的新契约；不把 WulingOrders/Offline/Rights 改名当贡献系统，也不创造新动作。
5. quote 没有可供客户端核对的独立报价版本/失效时间。客户端核对选择与最新快照，服务仍负责执行时库存、阶段及幂等校验。

## 精确公共补丁（尚未应用）

`integration.patch` 改四个旧公共文件：Frontend header/cpp 的只读房间榜 getter 和“已提供”标记；旧 ToolPrepare 转适配器；旧权威背包动作转适配器。原始 SHA-256 在 `integration-baseline.json`。

仅 logic-integration 可以应用。先对照当前 hash/并行变化；行尾需以下参数，已只读检查通过：

```powershell
git apply --check --ignore-space-change specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-core/integration.patch
```

确认版本后由 owner 顺序应用，检查 diff，不覆盖别队修改。普通 `git apply --check` 受旧 header 行尾影响不能匹配；不要全文件转换行尾。

补丁应用后榜单宿主：

```cpp
auto Board=InkCore::ReadBoard(this,FrontendOwner->GetPresentedRoomLeaderboard(),
    FrontendOwner->LeaderboardScope,Category,FrontendOwner->HasPresentedRoomLeaderboard());
```

新 RankWuling factory 仍由宿主构造/布局；绑定 Board.Entries 的 FText 到动态行，不使用 MakeRows 占位 rank 当真实上榜数据。
