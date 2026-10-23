# 竞采结算 · race-result

交付：redesign-planned；视觉状态：未批准。

### race-result · 竞采结算
目的：重制竞采结算。
入口/退出：race-live；返回按导航栈，不默认回主菜单。
主流程：从race-live进入→加载权威快照→查看状态与条件→查看开采权或回世界→待对账→结果或原因→返回原入口。
控件：标题/返回、总分与三项来源、600资格线、并列组、30/20/10分钟租期；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未达标原因、并列、对账中、租约已开始。
数据：RaceVm / RewardVm。
规则：第600秒租约自动开始；确认不延后计时。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
