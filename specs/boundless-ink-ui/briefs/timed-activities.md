# 限时活动 · timed-activities

交付：redesign-planned；视觉状态：未批准。

### timed-activities · 限时活动
目的：重制限时活动列表。
入口/退出：activities；返回按导航栈，不默认回主菜单。
主流程：从activities进入→加载权威快照→查看状态与条件→查看限时活动→待对账→结果或原因→返回原入口。
控件：标题/返回、等比主题卡、start/end与Asia/Hong_Kong时区、免费参与与期限；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未开始/进行/已结束/数据未知。
数据：ActivityVm。
规则：没有服务起止时间就显示待同步，不生成倒计时。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
