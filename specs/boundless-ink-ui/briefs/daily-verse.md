# 每日剑诀 · daily-verse

交付：redesign-planned；视觉状态：未批准。

### daily-verse · 每日剑诀
目的：重制每日剑诀。
入口/退出：sign-in；返回按导航栈，不默认回主菜单。
主流程：从sign-in进入→加载权威快照→查看状态与条件→查看剑诀并去行动→待对账→结果或原因→返回原入口。
控件：标题/返回、卷轴诗签、固定轮换剑诀与行动提示、对应免费玩法入口；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：已读/未读、固定轮换未知、无网络。
数据：ActivityVm。
规则：不做随机吉凶、改签付费；不展示抽奖入口。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_demo_text。
