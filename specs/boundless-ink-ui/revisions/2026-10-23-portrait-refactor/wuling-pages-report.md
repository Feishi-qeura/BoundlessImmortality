# 武灵、仙途工具与角色子页实施记录

2026-10-23；源码已修改，尚无本分工的编译或运行结论。主代理统一执行 UE 构建、PIE、Cook 和 Windows ZIP。

## 修改范围

- `Source/BoundlessImmortality/Frontend/InkWulingPanel.cpp`：竖屏“历练与成长 / 武灵与成员”页签；横屏组织与历练双栏。成员单独滚动，区域山水、阶段、奖励、规模、路线、成长、区域机制在历练滚动区，开始历练保持底部 88 高。创建、翻页、突破、结束和原路由全部保留。主代理在 `InkFeaturePanel.h` 的 UInkWulingPanel 添加 `bGuildView`。
- `Source/BoundlessImmortality/Frontend/UI/InkWulingToolsPanel.cpp/.h`：九项导航改成可展开的独立滚动目录。炼制/来源/装备/功法/建设/商店/寄售/突破采用竖屏详情+目录、横屏列表+详情；商店竖屏两列、横屏三列；列表分页放在列表内，主确认在 `BodyHeight-88`。次要配置操作在详情滚动区。刷新与尺寸变化保留现存选择，工具列表及详情保存滚动偏移。
- 活动与贡献保留原权威数据生成，改用 88 高页签和独立文本滚动区；竞采阶段、规则和资源选择独立滚动，共同护阵/破障在底部；匹配和取消原命令不变。
- 明细补充：来源开放条件、建设前置等级/目标等级/收益/成本、功法主辅修效果、当前装备槽位及候选物品效果、商品完整成本、寄售总价/卖方手续费/实收及上架立即扣 1 灵石。手续费遵循服务的向上取整 `ceil(total/20)`。
- `Source/BoundlessImmortality/Frontend/UI/InkWulingRecordsPanel.cpp/.h`：组织管理、订单、离线收益和剑灵权益分成摘要与独立记录滚动区；横屏双栏，竖屏摘要随记录滚动；撤单、移除成员、确认离线展示固定底部。改名文本、解散二次确认、已选成员/订单和各条 authority 请求保留。记录页每页统一 8 条，避免横竖切换改变翻页步长。
- `Source/BoundlessImmortality/Frontend/InkCharacterPanels.cpp`：装备、技能、法宝、仙缘子页改成明确条目及空/未开放状态；管理入口固定底部；外观名片预览+三列头像框选项+两列血量条选项，横屏预览/选项双栏，保留本地试戴操作。

## 保留证明

`validate-wuling-pages.ps1` 从本次备份读取原件；2026-10-23 执行结果写入 `wuling-pages-static-checks.json`：

- Character 文件从 namespace 至 BuildCharacterSection 前的完整原文（含全部 UInkCharacterProfilePanel 方法）一致，仅忽略换行符差异。
- 四文件原 SubmitPrototypeCommand 字面命令集合无缺失。
- UInkWulingToolsPanel::ExecuteSelected 完整函数与备份一致（请求与 payload 未改）。
- 四文件基本花括号平衡；记录了 SHA256。
- 1,500 个寄售数量/价格组合的向上取整手续费与服务公式一致。

以上是源码与公式检查，不能证明 C++ 可编译、UMG 布局无溢出、点击成功或打包可运行。

## 集成要求

共享外壳须使用实际 viewport 的 BodyHeight；重排页不能再经旧 ExpandCompactActionRows；删除 WulingShop 旧的详情/按钮 reparent 到壳层逻辑。已向主代理说明，主代理负责修改。资源路径未迁移，继续复用现有美术。

## 真实边界及后续验收

- 当前服务炼制仅接受一个 recipe_id；本页明确“本次炼制一份”，未虚构批量数量功能。
- 外观原本只有试戴预览，无永久应用请求；继续明确标示本地预览。
- 角色仙缘无权威条目时继续显示未开放。装备显示当前槽位与候选加成，未用缺失的逐槽服务数据编造最终属性差。
- 寄售/商店目录沿用服务原分页与分类，未新增服务搜索 API；订单仍展示各状态记录，没有引入会隐藏历史订单的默认筛选。
- WulingActivity 的任务、阶段和榜单完整保留，但没有改写服务器周期/领奖规则。
- 主代理需在 540×960、720×1280、1080×1920、1080×2400、960×720、1280×720 实测：详情长文本、内部列表滚动、选择后刷新、横竖切换、所有主按钮位置和导航返回。需重点检查低高度 landscape 的工具分类页及最窄双列商品名字。
- 本分工没有运行 Unreal Editor、native build、PIE 或打包，没有修改主角色布局、背包、排行榜、地图、相机、资产或服务实现。

## 集成编译修复（03:08）

首轮集成日志的 C4458 已定位到 UWidget::Slot、UInkExtensionWidget::Content 的局部重名。已在 InkCharacterPanels 子页区域、InkWulingPanel、InkWulingToolsPanel 将这些局部变量改成 SectionScrollLayout / SectionCanvas、WorldScrollLayout / WorldCanvas、ToolScrollLayout / EquipmentSlot。未改公共头和角色主面板。

重跑静态脚本通过；在以上三文件搜索局部 `auto* Slot/Content` 或 `FString Slot/Content` 为零匹配。此项仍待主代理重新编译确认。评审提及的 AppearanceOptionsScroll、WulingWorldScroll 重建恢复，由主代理在共享壳层统一保存命名 ScrollBox，不在本次三文件重复实现。
