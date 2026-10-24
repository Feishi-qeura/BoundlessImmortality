# v11：竖屏正式render关闭，玩家入口与动作证据分列

最新人类范围已按根 `scope-override-portrait-1705.json` 核实：userMessage `01a16d26-2364-7de1-81e8-c690503a517e` 原文“只考虑竖屏，不是横屏，最终9比16比例上线手机直播互动玩法”。当前仅9:16竖屏，基准720×1280；横屏资产/代码保留，**横屏paint/验收不再是阻断**。这是人类变更，非worker自行降scope。实体手机/平台与用户视觉仍分别验收。

本review未运行UE/UBT/服务/测试、未改生产、未看图签视觉。输入、Source supplement、初始checkpoint和晚到正式结果分别保存，未将失败attempt改写为成功。

## 可以关闭的正式范围

| 证据层 | 结论 |
|---|---|
| Source26 | frozen build26 epoch hash与14一致；只按该manifest精确核88个文件，**88/88当前hash匹配**，没有以build25覆盖新26。见 `current-source26-hash-proof-v11.json` |
| attempt13 | finished/pass=true，905checks/56captures；44路render通过。PNG metadata核19张初始核心capture为720×1282、37张为720×1280；保留此差异，不把13写成全精确尺寸 |
| attempt14 | **finished/pass=true，906checks/56captures**；独立核56张PNG及snapshot尺寸均720×1280。44不同页面render、37 Feature router probe及37 typedBack→HUD均通过 |
| 严格几何 | `current44-formal-audit.json` passed、errors=[]；重核56captures和56 deliberate bad-metric rejection，与Source26 epoch一致。未知/坏expected-painted不能用finite-based过滤排除；预布局generation等待不是paint通过证据 |
| 当前组件 | `current-runtime-component-paint-audit.json` passed：**导入总13，竖屏适用12，12/12实际expected-painted UImage**。横地图仍保留/已导入，但按人类范围不要求本轮横图paint。既有Appearance共享属性9可见等scope证明保留，不重复旧pending |
| 地图pin纠正 | 11/13/14真实collector读到8个World pin，8个唯一object、8个enabled/expected-painted、8个正paint尺寸。原10的pin0失败保留；差异来自漏数nested inner button的旧collector，不代表凭空豁免pin |
| 本轮进程 | exact PID/log：13/14均已有normal shutdown markers且PID不在；authority40168/avatar28596仍在。不能声称所有owned process已退出 |
| 晚到merchant02 | **finished/pass=true，45checks**；真实UI刷新后旧确认无Slate路径，旧version及另一实际SKU价格通过native Host seam输入均零Purchase，钱包/确认库存不变。最新trace入站Purchase仍1、SQLite对应audit仍1；原正向receipt唯一。见 `merchant-zero-write-proof-v11.json`。同时补当前26展开Appearance共享属性paint证明 |

14这整轮render/typedBack/strictgeometry已完成其scope，**不要求重跑**。已保存的15request/14类命令、Stage/Avatar/Appearance独立成功继续有效；不会因这次范围变更重新列为未执行。

## 地图wrapper→button→callback核对

实际对象为 `InkJourneyMapPinButton_*.WidgetTree.JourneyPinHitButton`，widget_class为真实UMG.Button。Editor `Visit` 穿过每个nested UUserWidget自己的WidgetTree，并用Visited避免重复；不是拿外层wrapper假充button。地图Source `BuildMapPins` 以真实EntryId配置wrapper，内部OnClicked→HandleClicked→OnEntryPressed(EntryId)，page的AddUniqueDynamic→HandlePinSelected→SelectedEntryId/OnEntrySelected。独立排列提供expected-paint，八个object不重复。

以上证明存在、paint和静态回调链；**不把“8个pin被收集”写成“8个pin均实际点击/前往成功”**。后者和真实selected context属于轻量UI-entry检查。详见 `nested-pin-proof-v11.json`。

## 44页render不等于44条玩家入口，更不等于全功能

13/14的37页loop明确调用 `FRONTEND.open_feature_panel(route,'')`。它们证明真实默认root的route/class/composition、数据布局和typed Back；不是每页从玩家UI点击进入。前置核心7、HUD/Journey→Extensions/World、Character四按钮有真实Slate路径，合计 **13屏已有这种入口证据；另外31屏在本次正式证据中未证明真实玩家入口**。

`stage-to-runtime-v11.csv/.json` 将route-render、direct-router、typedBack和player_UI_entry分列。余31页由integration已派的轻量导航补足：真实popup、滚到实际目标、等paint、原生Slate点、核准确route/context与Back；无需重做56截图或重跑业务成功链。详情/结果页必须用真实上级选择/业务结果或诚实记当前state/provider缺口，不能造任意菜单和空ID替代真实触发。

`functional-44-evidence-v11.csv/.json` 另逐页列canonical actions、实际backend、当前批证据与边界，类别包含：read-only/合法空态；本批实际业务正负例；适用native adapter/Source证；backend存在但新Host暂无适用动作运行证；真实provider/状态环境缺口。不同类别不合并成“功能全部通过”。

## 当前动作层Source缺口与校正

1. **V11-RACE-ACTIVE-01 / P1**：冻结 `InkRebuildFeatureJourney.cpp:28` 的RaceDenied要求存在且为真的 `identity.active`；当前authority `_snapshot`（`authority.py:189`）只输出user/role/room/generation。最新持久真实receipt也确认没有active。Native `races.py:85` 正确用server session.active/heartbeat判权限，但这些事实没有传到Host额外gate，因此有效live action也无法通过。当前没有race是合法空态，不能掩盖这个schema/provider接线缺口。最小修复为接通真实活跃资格，unknown仍deny，**不得直接默认true**；补一组适用eligible/unknown/refused证明即可，不要求重新跑整轮44或发明比赛结果。
2. **V11-ACTIVITY-CARD-01撤回**：本review曾漏读第37行对已选详情VM的最终覆写。该行已给ACT_SHARED/WEEK_GUARD/WEEK_IRON设置AllowedAction/ActionLabel/bActionAllowed，consumer使用此最终VM；与当前Source同hash，不是后来代码修复。无需生产修复。只保留真实选详情→Start/End权威回执这一尚未观察到的动作链缺口。

准确Source/hash和最小修复在 `functional-source-findings-v11.json`。这里只审当前可用backend和新增Host链，不重新派发旧owner已定案API修正。

## 有界必要剩余

| 剩余 | 收尾边界 |
|---|---|
| 一项Source provider修复 | Race活跃资格provider；最小适用逻辑证明，不重跑已完成render整轮。Activity card intent已有绑定，不再要求修复 |
| 31条真实UI-entry/context | 与已派integration轻量检查合并；真实详情/结果触发不足就记gap，不以router代替点击 |
| 当前backend可用但本批无动作链 | Journey实际pin选择/WorldTravel、Resource Queue→Cancel、支持的ACT_SHARED Start/End、StartLaunch未出现在当前trace；与上述真实entry组合各取一条必要链。当前own guild/prototype_full/window有效且launch=null，不应把StartLaunch写作无backend；若实时条件已不允许，记录真实拒绝/状态，不造fixture结果 |
| Merchant边界已补 | 当前merchant02已补原Shop版本/价格拒绝风险，无需再列空白pending；价格/旧version两项是实际native Host seam负例，不冒充用户改价UI。Market正向买/上架/撤单与已有Source确认比较分别有效；当前无Listed记录为合法空态，不发明不存在的可编辑listing-price API。native06 preflight本身仍不算业务执行 |
| 当前native适用性 | Native05是build24的Frontend49＝48Success+1带warning成功；native06 preflight并非formal49执行。当前26相关formal回执在本轮选定证据中未找到，旧结果不自动覆盖新Source。reviewer不运行测试 |
| owned服务退出 | 最后由唯一owner正常结束仍在的authority/avatar服务并留证；本review不控制进程 |

已知合法边界不作为无穷新测试：当前signed=true、月签仅1次且无eligible胸盒、offline_reward=null、race/queue/round/launch为空时，对应已领/未达成/无结果页可合法disabled或只读；DailyFortune/全服榜/资源榜等真实provider不足就明示不可用，不发明fortune、排名、offer或claim。已有适用native/Source证和正向业务不等于每个action实测，但也不会无差别要求重跑整个旧backend。

机器可做项仍有上述具体缺口，尚不能给“只剩用户视觉”的结案结论。完成这些有界项后只保留用户视觉、实体手机/平台等明确外部scope；当前不扩为设备部署或平台上线验收。
