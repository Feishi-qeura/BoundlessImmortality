# 区域详情 · region

交付：redesign-planned；视觉状态：未批准。

### region · 区域详情
目的：重制区域详情。
入口/退出：world；返回按导航栈，不默认回主菜单。
主流程：从world进入→加载权威快照→查看状态与条件→按条件进入历练→待对账→结果或原因→返回原入口。
控件：标题/返回、一幅等比区域山水、难度、阶段、固定奖励、开放条件与参与说明；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：境界锁定、事件互斥、未开放、奖励未同步。
数据：WorldVm。
规则：锁定区域保留条件说明，不画虚假的开始按钮。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
