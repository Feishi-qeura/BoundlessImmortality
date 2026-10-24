# 设置页 Settings

## 页面定位
首页入口的本地设置页。只展示当前宿主已实现的画面质量、帧率上限与减弱动态。

## 视觉约束
720×1280竖屏、1280×720横屏。延续已批准水墨纸底与墨绿/浅玉/旧金层级；不把设置页画成泛用系统控制中心。

## 布局与组件
固定页头和返回。中部为三个清晰分组：画面品质三选一、帧率上限二选一、减弱动态单个开关。底部固定应用按钮与状态说明。每组用同一张低对比纸卡，不对每个选项重复金框。横屏两列排布、固定底部动作。

## 状态
读取中/可用/应用中/应用失败/设置不可用由宿主实际状态传入。质量和帧率选中项从 `UGameUserSettings` 当前值读取；减弱动态从 `UInkFrontendWidget::bReducedMotion` 读取。当前按钮变更立即应用并保存，故不显示虚构的未保存更改；Apply继续调用既有宿主动作。

## 图像生成提示词
Create one portrait 9:16 mobile game settings page layout concept for an original Chinese cultivation RPG, reference size 720x1280, no device frame. Restrained water-ink paper design: warm ivory surface, deep ink-green text-space, pale jade selection accents, sparse antique-gold thin borders, soft top-left light, calm low-contrast cards. Fixed header with blank title zone and a visible back affordance. Main body has exactly three grouped setting cards: first card contains three equal unselected blank choice pills; second card contains two equal unselected blank choice pills; third card contains one blank toggle track and a short blank description line. Fixed footer contains one wide primary action silhouette and a quiet blank status line. Leave every label and value area empty for runtime UE TextBlocks; do not visually mark any option as selected because live settings are unknown. Keep 24-unit safe margins, clear thumb reach, minimum 88-unit hit targets, and independently scrollable body. No character, scenery, logos, letters, numbers, slider values, sound controls, key bindings, text-scale controls, resolution mode, unsupported options, or fake success/error states. Layout concept only; no text or data is baked.

## 反向限制
No local screenshot or complete external page copied, no text/glyphs/numbers, no fake selected value, no additional settings capability, no sound/keybinding/text-scale/resolution controls, no unsupported switches, no confirmations unrelated to the host, no full-page background texture that would replace runtime UI.

## 验收记录
Candidate prompt only. Output remains approved:false; visual pending user. Concept PNG, if produced, is not a runtime layer.
