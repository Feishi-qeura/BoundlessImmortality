# v10：已证链与精确剩余

本轮只写 `model/review`，未运行UE/UBT/服务/测试，未看图签视觉。依据 `selected-inputs-v10.json` 的冻结字节，另保留attempt10由in-progress到失败的独立晚到副本 `late-default-pie10-final-v10.json`，未覆盖原checkpoint或原失败报告。

## 不再作为未证事项派发

| 项 | 本轮核验结果 | 边界 |
|---|---|---|
| Avatar真实退避恢复 | service10 **finished/pass=true，14checks**；实际HTTPS503→200，两次请求相隔 **72.265秒**；同subject/URL在退避中真实刷新；恢复后1个可见participant UImage使用decoded `/Engine/Transient.Texture2D_6` | 实际默认root，Director tick临时暂停、声明显示行、随后恢复provider tick；是隔离transport fixture，不是生产榜单/平台认证 |
| Stage Cancel | cancel01 **53checks，finished/pass=true**；真实UI Quote→Prepare→取消双确认，材料归还；持久attempt Cancelled、realm21保持、预留0 | 独立app、声明realm/qi/material前置，naturally_earned=false |
| Stage 8→9提交 | commit01中Prepare回执和实际realm8→9已通过，持久attempt Succeeded；该轮后续切重炼步骤失败仍保留overall=false | 不能因后续失败抹去已提交成功；也不能把它改写成整轮通过 |
| Freshstage运行负例 | commit01四个检查均通过：旧quote仍from8而当前realm9；旧确认无物理Slate hit；当前正常申请disabled；**没有额外Prepare入站**。SQLite相应唯一request/receipt/audit对应 | 原v7缓存阶段缺陷已有Source、native helper及这个真实变境界负例证据；不扩称所有actor切换/每字段并发负例均已运行 |
| Rework | rework01 **43checks，finished/pass=true**；真实UI报价/确认/提交，持久Q **10→18**；普通/高阶核心各消费1，预留0 | 原生服务动作未改成假成功；缺失display-only字段不额外阻塞有效native quote |
| 持久审计 | 最新普通业务报告 **11/11**、stage报告 **7/7**；本轮再次只读提取普通钱包/库存/loadout/listing及stage状态。额外独立核对 **8条stage命令** 的request、归一payload hash、success receipt、唯一command_audit | 7是audit检查数，不是stage命令数；原普通业务15request/14类命令对应保留v9证明 |
| Appearance共享图集 | connected `service-pie-03/appearance-native-data.json` 有 **9个visible=true**属性atlas UImage；与已存新readback报告一致 | v9只检查default06隐藏视图的状态已被补充；该项不再pending“展开可见” |
| 13候选brush | 前13候选跨已保存attempt的可见brush继续成立，Appearance共享复用可见也补齐 | **当前build26最终13资源对账**仍未完成；跨epoch资源存在不替代全44/视觉通过 |
| Native05 | 正式总49＝**48 Success＋1 SuccessWithWarning**，0fail/0notRun。warning是 `SafeReentry.StageCoordinatesKeepPawnRoles` 中transient world DestroyActor无context | process明确scope为 **build24整个Frontend suite**，不是49个NewUI测试，也不是build25/26的新Source测试证明；未重跑 |

Stage请求、receipt_id、version、payload、audit和状态投影见 `stage-persisted-proof-v10.json/.csv`；Avatar及Native05见 `runtime-evidence-v10.json`。普通审计采用修正后 `actual-service-persisted-audit.json`（已计recipe石耗20），不沿用旧omitted-craft-cost报告。

## 当前精确剩余 / 阻断

| 优先 | 剩余证据 |
|---|---|
| **P1 当前地图pin阻断** | attempt10最终 **222checks/16captures，finished/pass=false**，失败于 `actual provided journey x-y coordinates create native pins WorldCultivation`：**authoritative_positioned_entries=8，actual_pin_buttons=0**。build26已有map_x/map_y或x/y有限值投影代码，但本次真实结果不满足。由唯一integration核对projection→owner VM→实际pin控件链并另存新attempt；本review不推测单一原因 |
| Merchant刷新负例 | Purchase及买/上架/撤单真实正向回执已证；仍缺同SKU/listing在重排刷新后保持subject，以及ID消失、version/price变化时旧确认撤销、零写的独立运行证据。不要再将正向交易列为未执行 |
| 当前全44 / geometry / 最终13brush | build25 exit0为88/222，build26 exit0为88/224；当前Root对临时零Desired保留旧extent高度，Journey新增实际x/y fallback均有选定hash。它们不自动继承build25或旧runtime证明。07/08/09/10失败保留；当前26全44正常返回、Character反复刷新滚动、map pin和横竖strictgeometry、最终13资源对账仍须新的实际完整记录 |
| 当前26 native范围 | 已有49正式结果仅属build24；未发现本轮选定证据中的当前26同范围执行回执。不把新Source纳入旧结果；reviewer不会重跑成功测试 |
| 全部owned进程收尾 | 精确PID只读检查及已有日志见 `process-exit-evidence-v10.json`：service10、三个stage、default07–10有正常shutdown markers且PID已不在；native05已不在，日志有TestExit/Engine exit requested。**authority PID40168、avatar PID28596两个python仍在**，不能声称全owned正常退出。未操作任何进程 |

取消/提交/重炼的seeded actors属于独立namespace，不能写作自然成长或生产玩家验收。TLS peer关闭证据限owned testprocess参数，HTTPS图片fixture不构成平台身份/证书认证。ServiceDisabled展示、native suite、真实隔离authority动作、当前全44、设备和用户视觉各自记账；视觉approved仍为false。

只有当前26全量与新Source相关的未证事项留在remaining。原stage positive、已实际证明的freshstage负例、Avatar恢复及Appearance可见复用已从旧v9待办中移出。后续Source或报告变化单列，不回改旧attempt为通过。
