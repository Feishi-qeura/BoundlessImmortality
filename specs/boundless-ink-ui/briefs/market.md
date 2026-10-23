# 原型寄售 · market

交付：redesign-planned；视觉状态：未批准。

### market · 原型寄售
目的：重制原型寄售。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→整单买入或挂牌→待对账→结果或原因→返回原入口。
控件：标题/返回、白名单原料、单价区间、数量、税费、总价、到期、托管；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：旧价失效/订单已售/背包满/超白名单/禁用。
数据：EconomyVm / InventoryVm。
规则：完整费用二次确认；无服务不假扣币或假成交。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
