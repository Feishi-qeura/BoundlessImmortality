# 背包

## 页面定位
保留既有背包结构、容量、筛选和操作。竖屏概念稿只检视空间层级，不改变运行控件、数据或物品目录。

## 视觉约束
沿用 HUD v2 的暖宣纸、墨绿、青玉细线与少量旧金。主审 720×1280，SafeZone 24，热区至少 88×88；动态物品名、数量、绑定和期限由引擎文字控件排版。

## 布局与组件
固定页头显示返回和容量摘要；分类/筛选与搜索位于其下；中段是五列网格并留出每页 40 格的可滚动区域；下部显示选中物品、来源说明和既有操作，底部固定操作区。空、满仓和无筛选结果应有独立版式空间。新图若无法同时体现 40 格，不作为布局验收稿。

## 状态
默认、选中、空背包、无搜索结果、满仓、锁槽、批量选择、材料详情、加载失败。数量和容量不造假；绑定/到期权限不靠颜色单独表示。

## 图像生成提示词
Create one portrait 9:16, 720x1280 concept mockup for an inventory page in a Chinese xianxia mobile RPG. Preserve an established inventory structure and its five-column, forty-slot-per-page grid: show five columns and eight compact rows within a scrollable center, category tabs, a search field and filter control, one selected item with a clear selection border, and an item detail/source panel above a fixed bottom action bar. Use warm xuan-paper surfaces, deep ink-green controls, muted jade dividers and restrained antique-gold accents, matching the approved project ink UI. Keep item art small and consistent so the grid reads as forty independent slots; leave names, quantities, capacity values and descriptions blank for engine typesetting. Keep touch targets clear, orthographic straight-on view, soft top-left light, no phone frame.

## 反向限制
No local screenshot as a reference; do not change the existing five-column / forty-slot page structure or 200-slot capacity rule. No fake text, item names, counts, currency, purchase/sale feature, new filters or extra actions. No enlarged decorative art that reduces the visible grid, no missing rows, no clipped content, no gibberish, logo, watermark, neon, glass panels or tilted device view.

## 验收记录
`inventory-v1.png` 已审查：水墨风格可作探索，但只有五行物品格、无明确页标题/容量文字且底部动作语义不清，不能验收为布局基准。原图保留且保持未批准；下一候选使用 `inventory-v2.png`。
