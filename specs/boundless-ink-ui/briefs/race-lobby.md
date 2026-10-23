# 竞采匹配 · race-lobby

交付：redesign-planned；视觉状态：未批准。

### race-lobby · 竞采匹配
目的：重制匹配大厅。
入口/退出：resource-detail；返回按导航栈，不默认回主菜单。
主流程：从resource-detail进入→加载权威快照→查看状态与条件→报名或取消→待对账→结果或原因→返回原入口。
控件：标题/返回、资源、人数档、2–4真实武灵、等待时间、统一规则和取消；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：60秒无对手转世界历练；取消与匹配成功竞态按服务结果。
数据：RaceVm / GuildVm。
规则：没有真实对手只留空位，不画假姓名或头像。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
