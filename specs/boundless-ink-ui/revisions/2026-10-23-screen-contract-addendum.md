# 竖屏套件页面契约补充（2026-10-23）

本补充挂接现有 `contracts/screen-contract.yaml` 稳定 screen ID；它提供这次明确点名的页面数据要求、布局职责和状态验收，不另造路由。共同规则：720×1280 竖屏逻辑尺寸；根 SafeZone 后预留 24 单位边距；底部触控优先；中文动态值必须以真实文本控件排版。现有 GDD、PRD、interaction 与 screen-contract 的规则优先。

| Screen ID | 页面职责与视觉焦点 | 必需字段 | 关键状态/行为 |
|---|---|---|---|
| main | 开始/继续、存档、设置、退出；以已有题字和角色构图为锚 | `FrontendVm.save_slots`, `selected_slot`, `continue_available` | 空存档、继续禁用原因、删除确认；不得生造登录/选服 |
| home | 真实场景保护区；顶部身份/血量/修为，右上活动、签到、仙途，底部角色/背包/修炼/排行/菜单 | `AnchorVm.identity`, `health`, `cultivation`, `realm`; `ActivityVm.launch_summary` | 连接/加载状态不遮返回；开服摘要只在有效活动时显示，最多一张紧凑提示卡 |
| character | 保留丹田、功法装备槽、属性评级布局 | `AnchorVm.attributes`, `MethodVm.equipped`, `EquipmentVm.slots`, `BreakthroughVm.availability` | 当前角色真实值；突破入口显示资格/缺口，不改变现有属性主面板 |
| inventory | 搜索、分类、40格分页、数量、批量操作、详情与来源 | `InventoryVm.capacity`, `items[]`, `filters`, `page`, `selection` | 空背包、无搜索结果、满仓、锁槽、材料详情；批量销毁/使用二次确认 |
| leaderboard | 顶部分段：伤害/修炼速度/主播剑灵；榜单信息表和固定“我的位置” | `LeaderboardVm.category`, `period`, `unit`, `rows[]`, `self_rank`, `updated_at` | 空榜、未上榜、并列、分页、断线旧数据；主播剑灵只取服务端榜单对象字段 |
| wuling | 主播创建的组织：徽记/名称/成员/贡献/组织进度/建设入口 | `GuildVm.owner`, `name`, `member_count`, `contribution`, `milestones`, `permissions` | 非成员、空组织、权限不足、成员离开；成员个人剑灵成长和组织值分列 |
| world | 大陆/区域路线与解锁状态，区域详情进入资源地点 | `WorldVm.regions[]`, `recommended_realm`, `unlocks`, `discoveries` | 未解锁条件、已探索、当前目标、服务不可用；地图可滚动且有列表替代入口 |
| resources | 资源点卡片/地图切换、来源类型、刷新/有效期和前往路径 | `WorldVm.resource_nodes[]`, `LeaseVm.status`, `RaceVm.availability` | 普通免费采集始终有路径；临时点过期/竞争/不可用；服务关闭隐藏匹配按钮 |
| sign-in | 每日签运卡、大吉/吉/平/凶，奖励与一键领取；与七日累计轨并列 | `ActivityVm.daily_fortune`, `reward`, `claim_state`, `streak` | 签运来自服务，单日不可重抽；未领取/处理中/已领取/补领功能关闭 |
| monthly-sign-in | 月历、当月累计奖励、当前日期和真实可领取格 | `ActivityVm.month`, `days[]`, `claimed_count`, `claimable_days[]` | 月份切换、跨时区、漏签；补签入口只按服务开关显示 |
| activities | 日常/限定/常驻/开服四类总览，卡片按截止与待领取排序 | `ActivityVm.categories[]`, `LaunchVm.active`, `end_at`, `next_task` | 空分类、尚未开启、即将结束、已结束、领奖成功/失败；客户端不判活动资格 |
| launch | 开服活动总览：剩余时间、贡献进度、每日任务、里程碑、特惠商品 | `LaunchVm.remaining`, `score`, `rank_preview`, `tasks[]`, `milestones[]`, `offers[]` | 活动未开放/关闭；榜单与任务独立数据；SKU无权限/为空时隐藏购买CTA，支付需确认和收据 |
| launch-rank | 开服贡献榜与个人贡献明细，周期与排名规则固定展示 | `LaunchVm.rank_period`, `score_dimensions`, `rows[]`, `self_rank`, `cap` | 无贡献、未入榜、并列、结算锁定；仅读取有效贡献，不使用伤害/礼物金额代替 |
| breakthrough | 目标境界、核心/护道/精华/奇珍最多四类、评分/品质/成长预估、准备度与材料来源 | `BreakthroughVm.current_stage`, `target_stage`, `slots[]`, `q`, `quality`, `growth_preview`, `readiness`, `sources[]` | 核心缺失不可突破；早期隐藏未开放类别；评分或配置不完整则禁用提交并显示原因；天劫概率独立字段 |
| tribulation | 渡劫阶段概览、已选护持、准备和服务结果 | `TribulationVm.stages[]`, `assistances[]`, `chance`, `outcome` | 成功/失败/处理中/断线恢复；UI不得根据材料质量重算概率 |

### 统一状态

页面数据状态共用 `default / loading / ready / empty / locked / pending / disconnected / error / expired / feature-disabled`。加载状态保持返回可用；失败展示重试和错误文案；禁用动作附近出现原因；过期活动保留结果信息但移除可领/可购买按钮。

### 突破数值显示字段约束

客户端消费规则服务返回的 `q_normalized`（0–100）、`quality_band`、`growth_min`、`growth_max`。天劫另读 `tribulation_chance`。现有 v1.4 的材料权重和阶段配置负责计算；不得从槽位原始分值在 UI 端相加。用户提供的 ×2.5–×5.0 档位在数值团队确认纳入 authoritative config 前只能显示为视觉稿标注，不绑定运行值。

### 交互和触控

固定页头含返回和页面名；长清单中部独立滚动，主 CTA 固定底部并留出系统手势区。点材料、奖励和榜单规则打开局部详情，不跳成重复顶层页面。交易、领取、突破和组织建设请求必须带幂等 request_id，按钮仅锁提交，不锁返回。
