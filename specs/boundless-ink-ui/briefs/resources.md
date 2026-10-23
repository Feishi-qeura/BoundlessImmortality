# 资源争夺 · resources

交付：redesign-planned；视觉状态：未批准。

### resources · 资源争夺
目的：重制五类资源选择。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→查看资源并报名→待对账→结果或原因→返回原入口。
控件：标题/返回、灵泉/灵石矿/灵草园/玄铁脉/残卷遗址；对象、产出、间隔、租期、当前开采权；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未开放、已有租约、正式成员不足、网络断开。
数据：LeaseVm / RaceVm。
规则：资源是临时配额；不渲染为永久独占矿点。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_preview。
