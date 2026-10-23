# 物品炼制 · craft

交付：redesign-planned；视觉状态：未批准。

### craft · 物品炼制
目的：重制物品炼制。
入口/退出：inventory；返回按导航栈，不默认回主菜单。
主流程：从inventory进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、配方、材料分型与品质、共享额度、产物、缺口与来源；完整成本确认后原子制作；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：缺料、额度耗尽、版本冲突、满包待入账、重复提交。
数据：RecipeVm / InventoryVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

竖屏：单列内容滚动，主操作固定。
横屏：列表与详情双栏。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：source-present-not-runtime-tested-this-run。
