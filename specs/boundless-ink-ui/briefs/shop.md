# 原型商店 · shop

交付：redesign-planned；视觉状态：未批准。

### shop · 原型商店
目的：重制原型商店。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→核对完整成本后制作或购买→待对账→结果或原因→返回原入口。
控件：标题/返回、SKU、成本、绑定材料、产物、限购、唯一窗口、期限、数量与总价；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：低余额/售罄/旧价过期/事务中/feature禁用。
数据：EconomyVm / InventoryVm。
规则：仅prototype_full；发布候选入口隐藏且直达请求拒绝。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
