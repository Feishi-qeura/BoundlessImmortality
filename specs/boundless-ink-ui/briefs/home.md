# 常驻HUD · home

交付：preserve-runtime；视觉状态：未批准。

### home · 常驻HUD
目的：保留游戏首页HUD与相机，作为新页面的风格衔接基准。
入口/退出：main；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从main进入→加载权威快照→查看状态与条件→修炼或收功→待对账→结果或原因→返回原入口。
控件：标题/返回、原场景与角色，侧边现有查看排行；无虚假任务；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：无目标则隐藏目标；断线不加收益；修炼/收功使用实际状态。
数据：AnchorVm / WorldVm / LeaseVm。
规则：不得改镜头、山石、人物尺寸和导航顺序；概念图不是运行截图。

竖屏：保留现有720×1280 HUD槽位，概念只作风格对照。
横屏：保留左上/右上锚点与底中五导航，不整体拉伸。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
