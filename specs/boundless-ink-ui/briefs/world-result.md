# 历练结算 · world-result

交付：redesign-planned；视觉状态：未批准。

### world-result · 历练结算
目的：重制历练结算。
入口/退出：world；返回按导航栈，不默认回主菜单。
主流程：从world进入→加载权威快照→查看状态与条件→确认展示或再去历练→待对账→结果或原因→返回原入口。
控件：标题/返回、基础阶段、个人修为、个人贡献、公库、额外核心分别列归属和收据；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：中断保留已完成阶段；待对账/已到账/已展示。
数据：RewardVm。
规则：确认只标已读，不能再次发奖励；满包转待入账收据。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
