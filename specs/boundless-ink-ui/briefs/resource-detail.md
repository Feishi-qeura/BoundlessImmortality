# 资源详情 · resource-detail

交付：redesign-planned；视觉状态：未批准。

### resource-detail · 资源详情
目的：重制资源规则和报名说明。
入口/退出：resources；返回按导航栈，不默认回主菜单。
主流程：从resources进入→加载权威快照→查看状态与条件→进入竞采匹配→待对账→结果或原因→返回原入口。
控件：标题/返回、收益对象、单位产出、统一竞采规则、人数档、资格；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：正式成员不足、活跃租约、profile关闭。
数据：LeaseVm / RaceVm。
规则：说明世界历练替代路线；不把礼物额作为分数。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
