# 山海地图共用山水底图（world-map / resource-map）

## 页面定位
为世界地图与资源地图提供共用的纯场景底图；点击目标、开放状态、名称、资源量、路线、图例、卡片和操作都由运行时 UMG 绘制。本图不是整页界面，不含交互或动态信息。

## 视觉约束
依据本轮候选 `world-map-v3-candidate.png` 与 `resources-map-v2-candidate.png` 的山水留白和青绿墨色方向；只提取地图视口里的连续山河意境，不复制整页、节点、图标、边框、按钮或文字。色彩参考 `contracts/style-contract.yaml`：宣纸米白、石青灰、低饱和青绿、克制旧金。中心区域降低对比，为运行时地图节点和选中路径留空间。无 `/Game/UI` 引用。

## 布局与组件
竖向 2:3 山河地图背景，远山在上部，河谷自中上向下延伸，少量岛屿、古桥、松枝只在边缘。中部和左/右侧留出可叠加节点的位置；底部留出可滚动/详情面板的区域但不要绘制面板。

## 状态
背景对所有地图状态恒定。开放/锁定/未知/选中/竞采资源 pin 由 UMG 独立绘制，绝不烘焙任何状态。未知服务数据不得从画面推断。

## 图像生成提示词
Create one production-quality background illustration layer for a Chinese cultivation adventure game's interactive map viewport, portrait 2:3 composition. A tranquil, expansive ink-and-watercolor mountain valley with a winding turquoise river, layered distant peaks, tiny architecture only as scenery, sparse pine branches framing the far edges, subtle mineral washes, warm rice-paper atmosphere, muted slate blue and jade green, restrained antique-gold light accents. Keep the central and side regions calm, lower contrast and open so live UMG pins, paths, labels, and selection rings remain highly legible on top. The lower quarter should remain visually quiet for a separate runtime detail panel overlay. Sophisticated hand-painted shanshui, readable at mobile size, natural continuous scenery, no framed page layout.

## 反向限制
No text, pseudo-writing, labels, UI, controls, maps pins, paths, routes, icons, status symbols, border, card, legend, watermark, logo, characters, glowing portal, conflict markers, reward symbols, baked numbers, or interface overlay. No fake locked or explored regions. No duplicated full screen. Keep central negative space and edge-safe composition.

## 验收记录
Prompt only at this stage. Generate one shared scenic layer for both maps; inspect pixels, dimensions, format, and hash before registering it. `approved:false`; generation is not visual approval or engine import.
