# 资源奖励明细 · resource-rewards

交付：redesign-planned；视觉状态：未批准。

### resource-rewards · 资源奖励明细
目的：重制资源奖励明细。
入口/退出：resource-detail；返回按导航栈，不默认回主菜单。
主流程：从resource-detail进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、租期、实际受益、在线积累、收据与来源明细；确认只已读；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：无租约、过期、断线、未达资格。
数据：LeaseVm / RewardVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

竖屏：单列内容滚动，主操作固定。
横屏：列表与详情双栏。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：source-present-not-runtime-tested-this-run。
