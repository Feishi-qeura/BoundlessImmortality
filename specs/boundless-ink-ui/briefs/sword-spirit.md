# 我的剑灵 · sword-spirit

交付：redesign-planned；视觉状态：未批准。

### sword-spirit · 我的剑灵
目的：重制个人剑灵权益页。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→查看个人剑灵与选择称号→待对账→结果或原因→返回原入口。
控件：标题/返回、查看身份、归属、境界修为、称号、已解锁剑形、有效增益与来源；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未加入、未解锁、到期、其他房间范围、非本人权限。
数据：SwordVm / GiftEffectVm。
规则：观众无生命防御死亡栏；解锁剑形不增加观众人数。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
