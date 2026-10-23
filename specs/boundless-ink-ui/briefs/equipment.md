# 装备与法宝 · equipment

交付：redesign-planned；视觉状态：未批准。

### equipment · 装备与法宝
目的：重制装备法宝子页。
入口/退出：character；返回按导航栈，不默认回主菜单。
主流程：从character进入→加载权威快照→查看状态与条件→在准备期确认配置→待对账→结果或原因→返回原入口。
控件：标题/返回、主播四个非武器装备槽和一个法宝槽、属性上限、归属、替换前后；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：安全准备期/事件冻结/锁定/冷却/绑定/过期。
数据：EquipmentVm / InventoryVm。
规则：不重画被保留角色页；切装不重置法宝冷却。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
