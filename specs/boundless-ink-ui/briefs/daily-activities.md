# 日常活动 · daily-activities

交付：redesign-planned；视觉状态：未批准。

### daily-activities · 日常活动
目的：重制日常活动列表。
入口/退出：activities；返回按导航栈，不默认回主菜单。
主流程：从activities进入→加载权威快照→查看状态与条件→前往或查看奖励→待对账→结果或原因→返回原入口。
控件：标题/返回、实际目标、当前/需求、预算、固定奖励、前往和已领取；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未解锁/进行/达成/待对账/已领/周期切换。
数据：ActivityVm。
规则：不能以页面打开产生进度；领取沿用原request_id。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
