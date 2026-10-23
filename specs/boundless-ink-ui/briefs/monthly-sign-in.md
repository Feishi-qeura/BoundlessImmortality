# 月签到 · monthly-sign-in

交付：redesign-planned；视觉状态：未批准。

### monthly-sign-in · 月签到
目的：重制月签日历。
入口/退出：sign-in；返回按导航栈，不默认回主菜单。
主流程：从sign-in进入→加载权威快照→查看状态与条件→选日或查看累计奖励→待对账→结果或原因→返回原入口。
控件：标题/返回、服务器月份、七列日历、今天/已签/未到、3/7/14/21累计宝箱和数量；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：跨月、月末、节点未达/可领/已领、断线；节点后轨道与箱中心对齐。
数据：ActivityVm。
规则：不能沿用参考图/旧稿7/14/21/28；复用现有已领取账本。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview_old_7_14_21_28。
