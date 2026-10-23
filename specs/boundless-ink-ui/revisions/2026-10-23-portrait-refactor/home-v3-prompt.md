# 玩法 HUD · 竖屏修仙新布局 v3

## 页面定位
旧策划玩法与导航继续有效；本稿根据2026-10-23的新要求重设计HUD，非运行截图，未经批准。参考既有两张修仙参考图的宣纸、墨边、青玉与细旧金。角色属性、背包、排行榜本身不改版。

## 视觉约束
9:16，逻辑720×1280；色彩继承contracts/style-contract.yaml：墨#202823、宣纸#F3EBDD、金#B88B45、青玉#3A8D83。避免整屏白色蒙版。标题最终使用马善政体，正文数字霞鹜文楷；生成字体只是概念近似。

## 布局与组件
安全区24；上方身份与三项入口分两行；中央完整露出原场景角色，不新增场景需求；左侧一个可收起的真实目标占位；右侧窄排行入口；底部五项导航角色、背包、修炼、排行、菜单，修炼居中突出。长屏扩展场景空白，横屏将身份放左上、快捷放右上、目标靠左、底部居中，不放大整张位图。

## 状态
本图明确视觉预览，数值是固定示例。实际数据未知显示待同步；无目标时隐藏目标卡；修炼/收功互斥。图中不出现充值、虚构活动或假红点。

## 图像生成提示词
Create one polished full-screen portrait 9:16 Chinese xianxia cultivation game HUD concept, no phone bezel, no presentation collage. Premium restrained ink-wash Chinese game interface: warm parchment #F3EBDD, deep ink #202823, aged gold #B88B45, muted jade #3A8D83. Thin handcrafted gold edging and brush-ink silhouettes, clear Chinese typography, high legibility, spacious mobile touch targets. The actual game background reference is a small black-robed cultivator standing on a rocky floating mountain with foliage against blue turquoise clouds. Preserve that composition as a representative concept: small character near the center, lots of visible sky and rock, no giant replacement hero portrait. The UI is crisp, richly crafted, NOT a generic dashboard, NOT glossy western fantasy.
Top safe margin: compact round avatar on left, a small dark ink identity strip reading 主播 and 炼气四阶; two thin readable meters labelled 血量 175/175 and 修为 346/449, small shield state 未激活. Under that top strip, three evenly spaced elegantly small parchment seal shortcuts 活动, 签到, 仙途 aligned to the right, NOT an enormous row. Upper-left optional compact objective scroll labelled 当前目标 and 静心修炼, with tiny collapse chevron. Maintain central 22%-78% width and 30%-78% height clear of HUD elements for the character. A small narrow ink scroll tab 查看排行 on the right edge above center, not an expanded leaderboard. Bottom anchored 5-item navigation on a shallow smoky ink cloud edge: 角色, 背包, 修炼, 排行, 菜单; central 修炼 is a restrained luminous jade-gold circular meditating symbol slightly raised; four others smaller detailed icons. No thick rectangular black bar. A tiny visible label 视觉预览 at bottom above navigation. A SINGLE translucent dark ink support behind each text area; no full-screen white wash, no stacked opaque parchment rectangles. Ornament is strongest at anchors only, fine aged-gold lines, delicate cloud curls. No currencies, no VIP, no red notification badges, no extra actions. Render a highly finished cohesive art-directable game UI mockup, all five nav labels clear, not an engine screenshot claim.

## 反向限制
无商标、无充值/VIP、无浮夸金框、无白色雾层遮住全景、无乱码、无新增玩法、无拉伸人物、不改变受保护面板。

## 验收记录
status: pending-generation；approved: false。须记录实际图像尺寸；此图只用于确认新布局，不是UMG资产或运行验收。
