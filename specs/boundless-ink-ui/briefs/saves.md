# 存档 · saves

交付：redesign-planned；视觉状态：未批准。

### saves · 存档
目的：重制存档列表，明确本地范围。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→载入或保存→待对账→结果或原因→返回原入口。
控件：标题/返回、槽位、时间、模式、摘要、覆盖或载入；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空槽、坏档、版本不兼容、覆盖和删除二次确认。
数据：SaveVm。
规则：不可把本地存档覆盖在线成长或服务账本。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
