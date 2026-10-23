# 日常详情 · daily-detail

交付：redesign-planned；视觉状态：未批准。

### daily-detail · 日常详情
目的：重制日常详情。
入口/退出：daily-activities；返回按导航栈，不默认回主菜单。
主流程：从daily-activities进入→加载权威快照→查看状态与条件→参与允许活动→待对账→结果或原因→返回原入口。
控件：标题/返回、主题头图、条件、任务、奖励归属、预算、主操作；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：预算耗尽/条件不足/已完成/断线。
数据：ActivityVm / RewardVm。
规则：标明是否替换基础预算；不足给来源入口。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
