# 主菜单 · main

交付：preserve-runtime；视觉状态：未批准。

### main · 主菜单
目的：保留启动页的水墨题字、人物与按钮构图。
入口/退出：application-start；返回按导航栈，不默认回主菜单。
主流程：保留当前运行流程，本轮只记录约束；从application-start进入→加载权威快照→查看状态与条件→进入HUD→待对账→结果或原因→返回原入口。
控件：标题/返回、开始、继续、存档、设置、退出；沿用当前可达路径；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：存档为空、继续不可用、退出确认；不为参考图增设选服登录。
数据：FrontendVm。
规则：保留现有启动页；导航清单见当前路由，缺项只登记。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
