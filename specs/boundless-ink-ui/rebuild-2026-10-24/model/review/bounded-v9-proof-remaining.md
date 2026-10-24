# v9 独立成功链 / 剩余证据

本轮已替代v8“尚无真实service记录”的状态。截止见 `selected-inputs-v9.json`（首批本地16:11:51；补充文件单列时间）。仅静态读取现有报告、Source、trace、精确快照与SQLite只读事务；只写本review目录，没有启动UE/UBT/服务、执行测试、改生产或看图签视觉。build24记录88 Source/ledger221、16:06:26 exit0；运行证据各自保留source_epoch，不覆盖成同一epoch通过。

## 可独立承认的证据

| 范围 | 已证明 | 限制 / 证据文件 |
|---|---|---|
| Native04 | 正式4 Success、0失败、0notRun，包括 `Features.ConfirmedStageSubjectRejectsChangedQuote` | build19 native fixture；不是当前host/service真实阶段负例；未重跑 |
| 真实Busy浏览 | service01/02记录Connected+Busy，真实OpenInventory，confirmed authority inventory snapshot保持；之后收到已创建武灵状态 | 实际服务待回包，不再仅称builder fixture；不能据此概括所有控件的Busy禁用或所有业务 |
| CreateGuild丢回包重试 | 两个隔离DB各同ID/同完整transport payload接收2次，均仅1条持久receipt、1条command_audit，authority version均为2 | 真实authority提交后丢首回包的transport fixture；不是重复提交两次业务 |
| stale Rename | service01/02的版本变化后旧确认未发RenameGuild；对应trace入站数0、持久command_audit数0 | 是该操作零写，不能说期间其他QueryCatalog/fixture没有写入 |
| Viewer JoinGuild | viewer setup请求有success receipt、归一payload hash及唯一audit | fixture准备阶段原生authority调用，**不算Viewer真实Slate入会流程** |
| Learn/Configure/Equip/Unequip/Purchase | service03五条独立请求均有success trace+持久receipt+唯一audit；authoritative version依次8/9/11/12/14 | 包括实际持有物品/原生确认操作；不因该attempt后来失败而抹去已完成步骤 |
| SignIn | service04请求持久success、audit、signed=true检查通过，version15 | 原生reward mode为placeholder、qi reward=0；不声称实际发放数值奖励 |
| RemoveMember | service05持久success、audit，成员消失检查通过，version16 | 仅该隔离viewer样本与实际权限上下文 |
| Build/Craft | service07持久success、audit，BUILD_01 level1检查通过；version20/22 | 同轮之前还记录building版本漂移撤销旧确认、旧按钮无Slate hit且未派发Build；不能扩称stage或market负例已验 |
| 晚到service09补证 | BuyListing/CreateListing/CancelListing均匹配冻结trace02、持久success receipt和唯一audit，version28/30/31；转移物品/付款与新own listing检查通过 | 仅补这三条正向链；SKU/listing刷新换主体、version/price变化的运行负例仍pending；该轮后续avatar harness异常 |
| 组件13/13 | 旧12项汇总保留；service03 `equipment-native-data.json` 有3个可见装备图集UImage，补齐第13候选 | 13项是跨已保存attempt的实际可见brush回读，不是当前全44渲染/视觉通过 |
| Appearance共享属性图集 | 该Appearance快照有9个UImage绑定现有属性atlas，均visible=false | 已有实际绑定，**预览展开后的可见/paint证据仍pending**；不再说完全无绑定 |

请求ID、exact payload、receipt_id、authoritative version、scope/user、持久结果投影及audit在 `service-chain-proof-v9.json/.csv`。本轮共 **15条请求记录 /14类命令**（CreateGuild跨两个隔离DB各一条），全部完成receipt/hash/audit对应。authority内部payload hash刻意排除expected_version；transport trace hash包含它。按两种各自定义核对，未直接比较两个不同域的hash。最新已保存service07/09 outbox均commands=[]；**空outbox只作队列终态旁证，成功判断来自receipt与audit**。

## 整轮仍未通过

默认PIE06：**660 checks /42 captures，finished=false、passed=false，typed back Activities失败**。观察32个不同route，仅对该epoch实际captures保留strictgeometry结论，不扩推all44、横竖全覆盖或新build。integration已报告六Activity Back在后续build修复；本轮不将该Source/编译修复当返回成功回读。

service01–07各轮同样finished/pass=false，失败和已证成功步骤并列保存；service04与晚到09是harness异常，不伪装成全部步骤通过。service09在已成功market步骤后，avatar fixture读取不存在的 `leaderboard_entries` 属性异常；没有avatar失败→恢复→重试运行结论。失败04/05/06以及service各轮均未覆盖、改写或重跑。

## 给唯一integration的剩余清单

| 优先项 | 还缺什么 |
|---|---|
| Stage freshguard真实负例 | Source两个边界仍有最新StageSubjectIdentity/confirmed fingerprint校验，native helper已Success；还需当前host/service的阶段/actor/quote变化拒绝、零派发/零审计及重确认闭环。声明的stage actors是独立namespace、naturally_earned=false的前置fixture，不能算自然成长或生产玩家通过 |
| 组件预览 | 13候选可见brush已各有局部证据，无需再作为“7图待应用”派发；只补Appearance共享属性预览展开后可见回读，以及新epoch必要回归。局部刷图不是用户视觉批准 |
| Merchant确认保持 | 正向Purchase及market三操作已有真实回执；补同SKU/listing在重排/刷新后保持subject，以及ID消失、quote/listing version或price改变时旧确认撤销/零写。当前Source回传ID与撤销比较仍存在，未用正向成功替代这些负例 |
| 默认全44闭环 | 后续修复build的新attempt验证六Activity正常返回，真实入口/默认router分列；横竖strictgeometry逐capture记录，整轮false继续保留 |
| Avatar与验收范围 | avatar真实失败恢复和身份缓存仍pending。TLS peer关闭只见owned testprocess参数作用域；HTTPS图片fixture不等于平台身份/证书认证。ServiceDisabled展示、真实Busy局部链、native4、真实authority fixture和最终视觉/设备验收分别记账 |

本轮Source hash、报告和精确组件快照已冻结；`source-drift-after-v9.json` 列截止后差异。未选中Source沿用上一版hash，不冒充本轮重审。后续新结果不应回写旧attempt；将新的receipt、runtime负例和完整attempt分别增量交接。
