# 世界历练 · world

交付：redesign-planned；视觉状态：未批准。

### world · 世界历练
目的：重制世界历练主页。
入口/退出：journey；返回按导航栈，不默认回主菜单。
主流程：从journey进入→加载权威快照→查看状态与条件→选择路线并开始历练→待对账→结果或原因→返回原入口。
控件：标题/返回、首发三区与后续锁区、独行/小/中/大预设、路线、10分钟阶段、免费固定供给；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：准备/进行/30秒恢复/结算中/完成/中断；运行中锁预设。
数据：WorldVm / AnchorVm。
规则：非送礼补给入口可达；同一轮在武灵与世界共用round_id。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：existing_live_qi_plus_preview_regions。
