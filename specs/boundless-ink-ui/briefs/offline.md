# 离线收益 · offline

交付：redesign-planned；视觉状态：未批准。

### offline · 离线收益
目的：重制离线收益页。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→确认已入账展示→待对账→结果或原因→返回原入口。
控件：标题/返回、上次结算、本次有效时长、8小时上限、基础10%、修为结果、原收据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：profile禁用/无收益/待对账/已入账。
数据：RewardVm。
规则：不受灵脉礼物放大；确认不重复领取；仅获准profile显示。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
