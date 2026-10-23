# 当前开采权 · lease

交付：redesign-planned；视觉状态：未批准。

### lease · 当前开采权
目的：重制当前开采权。
入口/退出：race-result；返回按导航栈，不默认回主菜单。
主流程：从race-result进入→加载权威快照→查看状态与条件→查看收益来源并回历练→待对账→结果或原因→返回原入口。
控件：标题/返回、服务器start/end、剩余时间、受益身份、在线积累、已入账和下一档；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：到期/断线暂停更新/无租约/跨房不适用。
数据：LeaseVm。
规则：离线不自行计数；允许并列配额，不假装唯一控制者。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
