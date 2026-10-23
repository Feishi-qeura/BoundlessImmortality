# 排行 · leaderboard

交付：redesign-planned；视觉状态：未批准。

### leaderboard · 排行
目的：重制榜单外壳与阅读层级。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→切换榜单→待对账→结果或原因→返回原入口。
控件：标题/返回、本房/全服、剑灵数/修炼速度/伤害；窄榜8行、我的位置、规则与周期；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未上榜、空榜、头像失败、周期切换、并列；不提供假好友榜。
数据：LeaderboardVm。
规则：保留持久行/头像更新逻辑与8行边界；重制视觉不能造成头像闪烁。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
