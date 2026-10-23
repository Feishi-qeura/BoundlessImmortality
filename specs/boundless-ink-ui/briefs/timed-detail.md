# 限时详情 · timed-detail

交付：redesign-planned；视觉状态：未批准。

### timed-detail · 限时详情
目的：重制限时详情。
入口/退出：timed-activities；返回按导航栈，不默认回主菜单。
主流程：从timed-activities进入→加载权威快照→查看状态与条件→参与或确认结果→待对账→结果或原因→返回原入口。
控件：标题/返回、剩余时间、真实规则、固定产出、截止、有效期与入口；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未开/过期/满包/处理中/活动版本变化。
数据：ActivityVm / RewardVm。
规则：不新增随机抽奖或未规划付费档位。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
