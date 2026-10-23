# 突破准备 · breakthrough

交付：redesign-planned；视觉状态：未批准。

### breakthrough · 突破准备
目的：重制突破准备。
入口/退出：character；返回按导航栈，不默认回主菜单。
主流程：从character进入→加载权威快照→查看状态与条件→核对成本后确认突破→待对账→结果或原因→返回原入口。
控件：标题/返回、四类材料实例槽、数量/替换、Q评分来源、确定品质、失败扣修与保底规则；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：不足材料跳来源；准备取消；与其他事件互斥；预留过期。
数据：BreakthroughVm / InventoryVm。
规则：确定Q与天劫成功率分开展示；v1.4覆盖旧突破成本语义。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
