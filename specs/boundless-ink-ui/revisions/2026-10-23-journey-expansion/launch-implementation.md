# 青云初启独立页面接入

- 新类 `UInkLaunchEventPanel`，文件 `Source/BoundlessImmortality/Frontend/UI/InkLaunchEventPanel.h/.cpp`。路由/HUD由主实现接入。
- 四个本地页签：活动总览、个人任务、武灵目标、荣誉规则。使用已有水墨场景与公共字体/纸面/按钮，正文至少24逻辑像素，88像素点击高度；竖屏双列页签、单列目标，宽屏四列页签与双列目标。
- 中段复用 `UInkFeaturePanel` 的响应式滚动容器；主操作放入外壳固定底栏。页签切换记住各自滚动位置；快照更新保留当前页签和滚动位置。
- 全部任务、任务进度、已确认、武灵轮次、里程碑、奖励权限、称号到期与荣誉榜来自真实 `launch`/`honors` 快照。没有活动快照时不生成占位任务或奖励。香港截止日期及剩余时间只使用 `server_utc`，断线标记上次确认记录。
- `LaunchPrimary` 动作为 `UWulingPrototypeSubsystem::StartLaunchEvent()` 或已开启活动的 `WulingWorld` 导航；仅已启用、已连接、不忙、当前页面、完整权威快照明确 `launch=null`、有效服务窗口、开放 profile、匹配武灵主播时才允许开启。没有自动开启与本地发奖。
- `LaunchTab_overview/tasks/guild/honors` 只切换页面内部内容；`LaunchSect` 前往 `Wuling`；返回/关闭沿用父页面导航。
- 渲染键仅投影活动可见状态、身份/武灵与权限，排除全局version、秒级服务时钟和轮次模拟；倒计时文字就地更新，到期/断线/忙碌等状态才重建。
- 同一cpp内新增 `Boundless.InkUI.Launch.AuthorityGuard` 与 `Boundless.InkUI.Launch.StableViewKey` 开发自动化测试，覆盖不完整快照、断线、忙碌、身份/武灵/profile/服务窗口约束、重复开启和稳定渲染键。
- 本子任务未运行Editor构建、UE自动化、PIE和真实Slate点击；这些运行验证留给主任务统一执行，不能据静态检查推断渲染验收。
