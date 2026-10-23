# 分类物品配置说明 v1.4

本目录是新增模块参数真源，未导入游戏。CSV为UTF-8 BOM，布尔值写True/False；加载器需明确类型，不把字符串False当真。整数是份、点或秒，百分率为0至1小数；ID保持字符串。stage为BR01至BR10，major序号0练气、1筑基、2结晶、3金丹、4元婴、5化神、6元神、7炼虚、8大乘、9飞升、10混元无极，realm_index沿既有34档。

item_master是唯一索引；各分类表按item_id补行为。breakthrough_species定义160种，material_items为640品质变体；不把两张都当可重复发放的物品。base_source_id为初级来源，item_sources记录实际品质渠道，recipes/recipe_inputs是高品质可达链。

获取：按source_id读取地点、境界条件、quota_group和产出规则，按item_sources目标发物；method=recipe时note引用recipe_id。CH_LEGACY仅引用已有行为不再追加掉落。quantity=0且method=baseline_reference表示非新增发物配置。

大境界成本/品质/建设覆盖旧表以15为准，随机失败规则以16与tribulation.json为准。core_only依然可突破；材料不改成功率；每种材料均可通过确定渠道获得。道具效果ID是待实现契约，不能伪装已有运行能力。

账本按个人、武灵、记录分开。公库材料配方由主播支付输入但输出所有者为本武灵，事务需同时校验双方，不支持提现或直接改owner。到期取来源最早时刻；重炼、转栈、退款均不续期。

item_system/tribulation的发布候选都关闭。验证命令：用随包verify_item_system.py检查引用、可达链、预算和概率；acceptance.csv的30条真实运行测试仍not_run。Excel为人工审阅与公式示例，不是已加载运行配置。
