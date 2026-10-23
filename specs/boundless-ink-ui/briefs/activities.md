# 活动组 · activities

交付：redesign-planned；视觉状态：未批准。

### activities · 活动组
目的：重制活动总入口。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→选择活动→待对账→结果或原因→返回原入口。
控件：标题/返回、日常/限时/开服；首屏一个主题头图、目标卡、实际时间和状态；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空活动、未开、进行、已结束、可领取、feature关闭。
数据：ActivityVm / LaunchVm。
规则：不用两张大海报撑空白；红点仅真实未读/可领。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
