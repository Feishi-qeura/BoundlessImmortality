# 武灵建设 · guild-build

交付：redesign-planned；视觉状态：未批准。

### guild-build · 武灵建设
目的：重制武灵建设。
入口/退出：wuling；返回按导航栈，不默认回主菜单。
主流程：从wuling进入→加载权威快照→查看状态与条件→预览并原子提交建设→待对账→结果或原因→返回原入口。
控件：标题/返回、公库余额、建设项目、材料、级别、成本、外显预览、版本与记录；底部固定主动作；需要时弹出成本确认与详情。
状态机：loading→ready→pending→ready；失败进入error并保留输入；断线disconnected保留快照；权限/开关关闭locked/feature-disabled。页面特定状态与边界：权限不足/材料不足/提交中/版本冲突/已完成。
数据：GuildBuildVm。
规则：个人与公库分开；v1.4建设表覆盖旧100资材示意，不生造永久倍率。

竖屏：720×1280单列列表→详情，标题与CTA固定，中段独立滚动。
横屏：1280×720列表+详情；常规页右侧抽屉，密集页居中宽页。

使用contracts/style-contract.yaml；UI重制不等于业务重写。当前历史状态：new_planned。
