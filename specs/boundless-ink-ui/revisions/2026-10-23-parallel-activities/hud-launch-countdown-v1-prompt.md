# HUD 开服倒计时入口候选提示词

状态：可复现候选组件提示词；未批准。

## 页面定位

HUD 右上既有活动组内的紧凑可点击入口，提醒有效开服活动；透明背景图，约2.5:1横向组件比例，参考逻辑占位200×80，不改HUD锚点。

## 视觉约束

延续现有水墨HUD：半透明宣纸小签、墨色软边、细旧金/玉色强调；不做大横幅，不加角色或山景背景块。

## 布局与组件

小型云山/开服印记图标，标签“开服”，下方真实剩余时间/活动状态字段，两行以内；可领红点仅独立状态层，不烘焙进底图。全组件触控目标需外部容器至少88×88，图像只作为表现素材。

## 状态

进行中、即将结束、时间待同步、结果可查看、已隐藏/结束。以单独状态图层处理，不让底图含虚构数字。

## 图像生成提示词

Transparent background, isolated compact horizontal HUD badge component for a premium Chinese xianxia mobile game, approximate 2.5:1 ratio. A tiny tasteful ink-cloud mountain seal suggesting a launch event, a clean blank label area for 开服 and a separate blank countdown/status text field. Warm translucent xuan-paper plaque, deep ink-green edge, subtle jade detail, one restrained antique-gold hairline, soft upper-left light, clean alpha outside the badge, no full background, no character, no banner, no baked notification dot. Leave text areas blank for runtime typesetting, no numbers or dates. Crisp simple silhouette readable at small mobile size, compatible with an existing right-top activity icon group, not obstructing gameplay.

## 反向限制

No fabricated timer, event title glyphs, dates, red notification dot, prices, logo, large banner, opaque rectangle beyond plaque silhouette, character portrait, gold glow, text clutter, shadow haze outside alpha.

## 验收记录

候选图已落盘但实测为 RGB 不透明图，不能作为透明组件直接使用；仍未批准。锚点、布局和交互需沿用实际 HUD integration。

### 最终生图提示词（v1）

Transparent background isolated compact horizontal HUD badge for Chinese xianxia mobile game, roughly 2.5:1. Small ink-cloud mountain seal, blank label area, separate blank server countdown/status field. Translucent warm xuan paper plaque, ink green outline, jade detail, restrained old-gold hairline, polished small icon, transparent outside, no full backdrop. No digits, dates, notification dot, logo, title text, price, character or banner.
