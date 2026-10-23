# 礼物效果说明 · gift-rules

交付：redesign-planned；视觉状态：未批准。

### gift-rules · 礼物效果说明
目的：重制效果规则说明页。
入口/退出：sword-spirit；返回按导航栈，不默认回主菜单。
主流程：从sword-spirit进入→加载权威快照→查看状态与条件→查看效果说明→待对账→结果或原因→返回原入口。
控件：标题/返回、来源/对象/原期限/到期/叠加/当前模式；旧权益与候选规则分列；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：测试/正式、过期、暂不适用、断线未确认。
数据：GiftEffectVm。
规则：送礼跳官方路径；说明点击不购买，不生成付费加概率入口。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
