# 角色 · character

交付：preserve-runtime；视觉状态：未批准。

### character · 角色
目的：保留当前丹田与功法装备槽及属性评级面板。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从home进入→加载权威快照→查看状态与条件→查看当前属性及进入获准子页→待对账→结果或原因→返回原入口。
控件：标题/返回、功法行、丹田、六侧槽、下方已配槽与实际属性；保持当前品质框和减弱动态选项；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：空槽、锁槽、长属性、未配置；不套用参考图旧立绘属性卡或虚构总战力。
数据：AnchorVm / MethodVm / EquipmentVm。
规则：保留当前角色评级系统，不恢复旧稿；用户保留优先。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
