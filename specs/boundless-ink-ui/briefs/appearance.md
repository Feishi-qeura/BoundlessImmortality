# 名片外观 · appearance

交付：redesign-planned；视觉状态：未批准。

### appearance · 名片外观
目的：重制名片外观。
入口/退出：character；返回按导航栈，不默认回主菜单。
主流程：从character进入→加载权威快照→查看状态与条件→查看条件后确认允许动作→待对账→结果或原因→返回原入口。
控件：标题/返回、已获头像框与称号、等比试戴、来源、期限和应用；未获只预览；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：未解锁、过期、当前规则不适用；不得试戴即取得权益。
数据：AppearanceVm / GiftEffectVm。
规则：当前接口能力优先，不画假成功；不覆盖保留页。

竖屏：单列内容滚动，主操作固定。
横屏：列表与详情双栏。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：source-present-not-runtime-tested-this-run。
