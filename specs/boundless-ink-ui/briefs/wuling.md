# 武灵总览成员建设记录 · wuling

交付：redesign-planned；视觉状态：未批准。

### wuling · 武灵总览成员建设记录
目的：重制武灵组织总览与名册。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→创建或管理武灵→待对账→结果或原因→返回原入口。
控件：标题/返回、总览/成员/建设/记录四子栏；正式、参与、临时人数分开；公库与当前租约；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未创建、名称2–12等效汉字、审核驳回、改名冷却；成员20一页。
数据：GuildVm / WorldVm。
规则：武灵是主播创建的组织；移除与解散说明个人权益保留。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_native_prototype_calls。
