# 物品 · inventory

交付：preserve-runtime；视觉状态：未批准。

### inventory · 物品
目的：保留背包全套UI、模型与操作。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从home进入→加载权威快照→查看状态与条件→选物品查看允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、分类/搜索/列表与网格/新标/数量/滚动/多选/分页/详情/出售分解整理；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：200物理槽、默认20解锁、40每页；hover蓝、selected金；绑定到期托管动作权限。
数据：InventoryVm。
规则：不改格数、材质、框架；新增来源页从允许的详情入口进入。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
