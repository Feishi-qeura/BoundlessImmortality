# 产品需求与交付范围
## 文档状态
- status: approved

## 成功指标
规划表全部页面覆盖，无无出口孤岛；每个必需页都有可读的主动作、数据归属、禁用理由和异常恢复。保留页的现有源文件/资产哈希不因本轮写入变化。首页只交一个竖屏候选，用户批准后才开始组件拆解。

| ID | 优先级 | 需求 | 屏幕ID | 验收 |
|---|---|---|---|---|
| P0-01 | P0 | 保留主页面、角色和背包 | main/home/character/inventory | 保留清单与哈希；不覆盖运行工程 |
| P0-02 | P0 | 修炼、世界历练到结算闭环 | home/journey/world/region/world-result | 开始、运行、恢复、结算和返回路径完整 |
| P0-03 | P0 | 武灵与剑灵归属明确 | wuling/sword-spirit/gift-rules | 三类人数分开；无观众血量；来源期限可查看 |
| P0-04 | P0 | 资源竞采闭环 | resources/resource-detail/race-lobby/race-live/race-result/lease | 真实对手、超时转历练、配额归属与自动租期 |
| P0-05 | P0 | 成长构筑、来源与渡劫 | methods/equipment/breakthrough/tribulation/item-sources/guild-build | 冻结配置、成本、Q、概率、免费助力与保底均可见 |
| P0-06 | P0 | 签到活动及排行重制 | sign-in/monthly-sign-in/daily-verse/activities/daily-activities/timed-activities/daily-detail/timed-detail/leaderboard | 日期与目标权威；宝箱详情；3/7/14/21；窄榜8行 |
| P0-07 | P0 | 安全返回与设置存档 | saves/settings | 覆盖删除确认；在线状态不由本地档覆盖 |
| P1-01 | P1 | 开服活动与贡献榜 | launch/launch-rank | 真期限、免费贡献、并列、自动到账与称号到期 |
| P1-02 | P1 | 原型经济与离线 | offline/shop/market/orders | 仅获准profile；价格版本与完整费用确认 |

## 本轮具体交付
策划映射三文档、参考分析、UI规范、逐屏地图/契约/brief、保留清单、首页提示词与一张实际视觉、真实资产清单、验证与后续任务。其他页面本轮完成规格和地图；其逐页视觉与组件按批准批次后续执行。
不新增游戏核心系统，不改客户端逻辑，不生成已实施的UMG/uasset，不把原型权限说成生产权限。不拆未批准首页，不交空壳PNG包。

## 数据与请求
所有正式VM含profile/app/room/session_generation/rule_version/server_utc/snapshot_version/is_authoritative。预览输入显式标记；未知用“待同步”，不能填假0冒充余额。服务权威控制归属、权限、支付、奖励、概率、UTC与事务。
通用Ready→Submitting→PendingReconcile→Confirmed/Rejected；一次操作一个request_id，重试保持原ID；版本变化重确认。关闭页保留请求对账，切房丢弃旧回调。

## 验收矩阵
详见evidence/acceptance.csv。检查540×960、720×1280、1280×720、960×720和超长屏：文本、热点、SafeZone、筛选/选中/请求状态保留。新视觉只证明构图候选，功能必须后续在UE和真实设备验证。

批准记录：用户于2026-10-22批准本轮规范、屏幕地图和首页v2：批准首页v2及规范和屏幕地图，开始拆解首页组件。批准仅针对当前UI设计基线，不新增玩法或发布权限。

## 2026-10-23 全量竖屏套件增量
| ID | 优先级 | 需求 | 屏幕ID | 验收 |
|---|---|---|---|---|
| P0-08 | P0 | 全量页面信息与交互状态补齐 | main/home/character/inventory/leaderboard/wuling/world/resources/sign-in/monthly-sign-in/activities/launch/launch-rank/breakthrough/tribulation | 沿用稳定 ID；页面入口、返回、ViewModel 字段、空/锁定/错误/处理中状态可追踪 |
| P0-09 | P0 | 竖屏视觉候选可复现且不覆盖保留资产 | 上述未有已批准视觉的页面 | 每页有独立提示词与 manifest 记录；720×1280 主审、横屏重排；新稿均 `approved: false`，不得拆组件 |
| P0-10 | P0 | 业务规则显示与权威服务一致 | leaderboard/sign-in/monthly-sign-in/activities/launch/launch-rank/breakthrough/tribulation | 周期、签运、截止时间、SKU、贡献、突破预估和天劫概率只消费权威字段 |

这批增量不改变已批准首页、角色主面板和背包运行结构，不新增玩法/经济公式，不声称设计候选已通过 UE、设备或平台验收。需求到页面的具体映射见 `revisions/2026-10-23-screen-contract-addendum.md`。
