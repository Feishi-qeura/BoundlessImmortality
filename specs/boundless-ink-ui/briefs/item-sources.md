# 物品来源 · item-sources

交付：redesign-planned；视觉状态：未批准。

### item-sources · 物品来源
目的：重制物品来源追踪。
入口/退出：inventory；返回按导航栈，不默认回主菜单。
主流程：从inventory进入→加载权威快照→查看状态与条件→选择免费来源并前往→待对账→结果或原因→返回原入口。
控件：标题/返回、材料分型品质、具体区域/副本、专注进度、配方与共享额度；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未知物品/区域锁定/额度耗尽/可制作/版本变更。
数据：ItemSourceVm。
规则：来源沿v1.4表；不得导入旧表叠加奖励；回背包保留选中项。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
