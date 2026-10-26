# v12 独立审核 checkpoint（部分交付，尚不签机器全完）

本文件先持久化已完成判定；延续 `selected-inputs-v12.json`，不重跑历史。reviewer仅写model/review，无UE/UBT/服务/生产操作，无视觉批准。

## 已证范围，不再重复列pending

- Source26正式14：44路render、37typedBack、56张精确720×1280、strictgeometry及bad-value拒绝已通过。仅竖屏9:16，横屏不再是验收门槛。
- actual-ui-entry03保留overall=false及其87成功真实edges；04整体通过，补23edges。110物理viewnavigation支持44页可达，不等于五页真实business subject/result触发。03旧失败没有被删除。
- native07是当前26的正式49：48Success＋1SuccessWithWarning，0fail、0notRun；warning仍为transient automation world DestroyActor无context。不是49项NewUI独立业务测试。
- merchant02通过45checks：真实刷新使旧Shop确认脱离Slate路径；旧version/另一实际SKU价格经native Host seam拒绝，零Purchase、钱包/确认库存不变。actual gesture与seam输入不混记。
- 13组件保留，12竖屏适用组件paint通过；共享Appearance当前展开9图paint已证；横地图仅保留、无需强制横屏paint。
- `final-owned-process-shutdown.json`证明17:27旧测试批次正常关闭，无force kill。后续functional批次的服务/引擎生命周期必须另记，不能沿用旧退出证明。

## Race active判定已更新

`authority.py`已补真实 `active=bool(ctx.session['active'])`，不是default true；after SHA256为 `ca51514499201236b9e77622401f42be08bb589306f24746e7b314edd27190de`。ledger/backup编号224的before/after与该hash一致；`backend-provider-projection-epoch.json`记录此新增投影，C++未改。7项red→green及102原service tests通过，覆盖真实flag/active eligible成功、inactive拒绝、stale heartbeat仍拒绝等。

因此不再沿用“当前authority缺active字段”的旧结论。Host unknown字段仍deny，资格/heartbeat规则不降级；本轮unknown分支为保留Source证，不能伪装已跑完整UI负例。**Source/authority修复与实际Race UI可用链分开**：functional01在StartGame选择器空失败，不能证明后续业务；最新02仍由integration定位，未读到通过即pending。

## 五个subject/result缺口：分类与闭合标准

| 页面 | 当前证据与真实Source可实现性 | 精确闭合标准 |
|---|---|---|
| RegionDetail | 已有World地图pin→EntryId→Host.SelectedEntry→真实related menu携带context→详情Find(id)链；03只打开空context。属于真实选点/context测试缺口，不是缺backend或要新造菜单 | 真实点一个authority现有开放world pin，打开详情，核当前owned host selected ID与实际region一致；Back正确。结合适用WorldTravel记录真实request/receipt及round.region_id，不伪造ID |
| ResourceDetail | 已有resource pin/group→实际resource ID→node_id→详情→QueueRace链；03同样context为空。属于选点/主体测试缺口 | 实际选资源pin，核resource ID/node_id和详情；适用时真实QueueRace→Lobby→Cancel，核同一queue及权威回执/Back。无资格则准确记录真实拒绝 |
| EventSettlement | backend已有last_world→snapshot.round，Source结果页读真实round；当前没有已完成round样本。不是backend未提供 | 用真正WorldTravel产生的对应round，在真实backend Completed/Interrupted后查看结果，核round ID/region/status及已有结果/回执；可复用既有真实结果。声明的时间/场景前置不算自然earned流程，不手填奖励 |
| RaceSettlement | backend已有last_race→snapshot.race及原生settle，Source读该race；当前没有对应Settled样本。属于可构造的真实赛程/结果测试条件，不能默认删出机器范围 | 在隔离、明确声明前置的native赛程中取得实际Settled race，核race ID/team/result及结算审计，再真实UI查看/Back；不以空related menu导航替代结果，不发明奖励数值 |
| ResourceRewards | 当前Host读取未输出的 `resource_rewards.rows`；authority已输出 `reward_receipts.entries`，包含真实source_id、owner、sequence、items；原生race/lease发放已接该回执系统。属于现有provider投影未接通，不应简单称外部backend缺失 | 将当前实际race/lease的真实source关联回执投影到只读页，或输出同源只读projection；按真实owner/source过滤、保留receipt ID/quantity。正例来自native已确认grant/receipt，空态/无关购买回执不得冒作资源奖励，禁止用钱包差值或新增claim制造结果 |

## 尚未结案的有界项

1. 五页以上主体/真实结果标准及ResourceRewards投影；不能因44 view可达而自动排除。
2. 修复后实际Race/UI适用链，以及WorldTravel、resource queue/cancel、eligible StartLaunch等本批尚未取得的对应UI+权威回执；已证业务成功不重做，不测试未知offer/价格/fortune/global榜。
3. 新functional失败与后续attempt保持独立；新进程收尾另外留证。已有native07、merchant02、14和旧进程退出不再重跑。

机器完成判定为false。后续仅在这些明确机器项闭合后，才可只剩用户视觉与实体手机/平台等已分开的外部scope。
