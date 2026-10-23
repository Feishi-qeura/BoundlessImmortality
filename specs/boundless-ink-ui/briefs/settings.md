# 设置 · settings

交付：redesign-planned；视觉状态：未批准。

### settings · 设置
目的：重制设置页，降低调整成本。
入口/退出：home；返回按导航栈，不默认回主菜单。
主流程：从home进入→加载权威快照→查看状态与条件→应用设置→待对账→结果或原因→返回原入口。
控件：标题/返回、声音/图像/控制/文字可读性/减弱动态；仅列现有能力；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：应用失败恢复原值；未保存退出提示；未知能力不绘可用开关。
数据：SettingsVm。
规则：键鼠与触控焦点可达；返回不中断待对账事务。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_wbp_and_source_not_runtime_tested。
