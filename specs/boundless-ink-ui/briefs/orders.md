# 我的订单 · orders

交付：redesign-planned；视觉状态：未批准。

### orders · 我的订单
目的：重制我的订单。
入口/退出：market；返回按导航栈，不默认回主菜单。
主流程：从market进入→加载权威快照→查看状态与条件→撤单或查看应收→待对账→结果或原因→返回原入口。
控件：标题/返回、挂牌/成交/取消/到期/应收；时间、数量、手续费与唯一收据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空订单/撤单竞态/应收待对账/禁用。
数据：EconomyVm。
规则：按订单版本原子请求；不能因关闭重开发一次应收。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
