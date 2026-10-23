# 仙途菜单 · journey

交付：redesign-planned；视觉状态：未批准。

### journey · 仙途菜单
目的：重制系统总入口且保持现有导航位置。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→进入世界、武灵或资源系统→待对账→结果或原因→返回原入口。
控件：标题/返回、世界历练/武灵/我的剑灵/功法/资源五组入口，按能力隐藏；保留现有签到活动角色捷径；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：开关关闭不显示可用入口；世界事件进行中展示当前状态。
数据：WorldVm / GuildVm。
规则：不在HUD新增第二排入口。

竖屏：高频入口上置，纵向滚动。
横屏：两列入口与主题山水，真实文字独立。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：source-present; InkFeaturePanels.cpp lists six routes。
