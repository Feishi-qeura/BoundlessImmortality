# 功法构筑 · methods

交付：redesign-planned；视觉状态：未批准。

### methods · 功法构筑
目的：重制功法构筑页。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→确认准备期配置→待对账→结果或原因→返回原入口。
控件：标题/返回、七路线、1主修、当前辅修槽、锁定条件、主辅差异和草稿预览；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：事件中禁换并显示原因；不足材料跳世界来源；确认失败留旧配置。
数据：MethodVm。
规则：第3/6级强化从规则表读取；不在UI推导另一倍率。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
