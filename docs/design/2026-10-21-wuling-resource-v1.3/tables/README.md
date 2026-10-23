# 配置表与验证说明

这些表是策划数据，不被UE自动加载，不直接修改项目Rules或礼物配置。数值更新必须增加规则版本，先验证再由实施者建立导入层。

v1.2新增UI表：ui-screens.csv、ui-files.csv、ui-art-requests.csv、ui-acceptance.csv、ui-existing-assets.csv。前四项是未来实施规格，最后一项是本次文件盘点。页面current_status不等同运行验收；文件action=add_planned表示当前没有该文件。源PNG生成需求不代表本次已经生成美术。

| 文件 | 主键 | 字段说明 |
|---|---|---|
| balance.json | design_version/profile | 时间秒、倍率小数、次数整数；production_allowed=false不能发布 |
| realm_targets.csv | realm_index | 从0连续；major/minor从0；累计分钟不是当前阶消耗 |
| realm_costs.csv | realm_index | 验证脚本从目标表生成累计修为、观众单阶、主播单阶、材料与灵石 |
| items.csv | item_id | owner、kind、绑定、可交易、付费来源、栈上限、过期策略 |
| world_rewards.csv | stage＋item_id | 三阶段固定奖励；次数为正整数 |
| nodes.csv | node_id | 修为加法、额外物品、在线整档间隔、租期上限、受益者 |
| shop.csv | sku | 原型购买/制作成本与产物，额外材料最多两项，日限购、窗口唯一 |
| market.csv | item_id | 原型交易白名单、价格边界、NPC回收价 |
| methods.csv | method_id | 七部功法、首版实现标记、主辅描述；效果实现必须使用08详细规则 |
| encounters.csv | preset_id | 固定难度普通目标、障碍、最终目标参数；命中率试算不代表实测 |
| activities.csv | activity_id | 时间为空的周期活动依据服务器周期；限定活动无起止必须disabled |

owner_type只允许anchor/viewer/guild/individual；individual只用于共同的修为类型，实际记录仍指定主体。max_stack=0表示账本或权益引用，不能创建普通物品栈。物品paid_origin为模板默认，实例还必须继承来源污染标记，不可通过免费配方洗掉付费来源。

expiry_policy：window=测试窗口结束；earliest_input=不晚于最早输入到期；profile=依环境与核准范围；legacy_or_approved=旧权益保留及正式提案规则，不由本包清理器覆盖。

运行：

```powershell
& 'C:\Users\zxxuh\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' 'D:\demo\BoundlessImmortality\docs\design\2026-10-21-wuling-resource-v1.3\verify_balance.py'
```

输出realm_costs.csv和上层validation-report.json。脚本只写本策划目录，不联网、不访问存档、不修改UE资源。检查是配置与算术验证，不是对未来服务器事务、竞态、反作弊或客户端渲染的功能测试。

UI表可通过上层generate_ui_tables.py重建并检查；该脚本只读取项目文件、桌面参考图并写策划目录，不会生成运行WBP。运行前确认仍使用本版源清单；若现有项目已变化，历史snapshot不得拿来冒充最新验证。

## v1.3青云初启表

- launch_event.json：活动时间、上限、称号、增益、原型/发布权限。发布默认全关，rank_numeric_rewards所有环境关闭。
- launch_tasks.csv：七项个人免费任务，固定修为总额780，只发观众。
- launch_guild_milestones.csv：1/3/6/12轮共同目标，主播灵石200/破境石60，最后一档临时仪式。
- launch_rank_candidates.csv：关闭的名次修为比较表，所有enabled=false，不得直接挂入领奖接口。
- launch-acceptance.csv：30条开服专项待运行验收，状态全部not_run。
- ui-launch-delta.csv：新增六个UI文件任务，已合入UI总表。

在本目录上级分别运行verify_launch_event.py、generate_ui_tables.py与verify_balance.py。它们只检查策划数据，不执行UE、服务或正式平台测试。14为新增活动规则说明，launch_event.json和对应CSV为参数来源。
