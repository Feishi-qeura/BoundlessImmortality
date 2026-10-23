# 开服玩法贡献榜 · launch-rank

交付：redesign-planned；视觉状态：未批准。

### launch-rank · 开服玩法贡献榜
目的：重制开服贡献榜。
入口/退出：launch；返回按导航栈，不默认回主菜单。
主流程：从launch进入→加载权威快照→查看状态与条件→查看贡献明细→待对账→结果或原因→返回原入口。
控件：标题/返回、免费贡献四来源、轮数、资格、并列名次和荣誉；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未达资格/并列/周期结算/未上榜/禁用。
数据：LaunchVm。
规则：不展示消费排名或已关闭的修为奖励。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
