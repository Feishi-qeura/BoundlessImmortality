# 青云初启开服活动 · launch

交付：redesign-planned；视觉状态：未批准。

### launch · 青云初启开服活动
目的：重制青云初启。
入口/退出：activities；返回按导航栈，不默认回主菜单。
主流程：从activities进入→加载权威快照→查看状态与条件→查看目标或确认展示→待对账→结果或原因→返回原入口。
控件：标题/返回、总览/个人任务/武灵目标/规则四内栏、本人报名、七任务、1/3/6/12轮次、称号到期；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未报名/进行/达成/自动入账/过期/profile禁用。
数据：LaunchVm / RewardVm。
规则：四内栏不重复压路由；发布候选隐藏并拒绝直达。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
