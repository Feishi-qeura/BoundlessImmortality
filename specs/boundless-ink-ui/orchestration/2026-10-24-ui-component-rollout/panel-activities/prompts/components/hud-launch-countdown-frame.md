# HUD 开服倒计时牌底 · hud-launch-countdown-frame

## 来源与用途
- 受 `revisions/2026-10-23-activity-state-correction-3/assets/pages/launch-milestone-pending-v3.png` 的宣纸、墨山与旧金层级启发；源图为未批准候选，仅在本批用户授权范围内作为加工输入。
- 用于既有 HUD 右上活动入口组中的紧凑开服状态牌；显示文字、倒计时与可达提示由 UE TextBlock 实时绘制。

## 尺寸和状态
- 单个横向组件，目标 960×320；仅提供 neutral 底板一种状态，视觉可缩放到 HUD 逻辑尺寸 200×80。
- 透明背景；不含文字、数字、图标、按钮、红点、发光粒子或交互提示。
- 保留中央宽阔、低对比的排字区，左右边缘的山形装饰不侵入中间文字安全区。
- `9-slice`: 边缘框结构；待导入后进行实际拉伸预览与 margins 测量。

## 必须继承的契约字段
- 宣纸浅底、墨绿/青玉山形、柔和左上光、细旧金边；正文留白与HUD场景保护区优先。
- 单个紧凑牌，不新增第二排或遮挡角色主体；动态文本与状态均由运行时UI呈现。

## 图像生成提示词
Create one isolated horizontal game UI container component, a blank launch-event countdown plaque for a Chinese cultivation water-ink HUD. Use pale warm xuan paper, subtle ink-wash jade mountains only at the far left and far right edges, a thin restrained aged-gold outline, soft top-left illumination, complete clean silhouette, transparent background. Target 3:1 horizontal proportions. Keep the center 65 percent as plain low-contrast paper for live Unreal TextBlocks. High-resolution, suitable for compositing and responsive nine-slice stretching. One component only.

## 反向限制
- No Chinese or Latin text, no characters, no numerals, no countdown dashes, no icon, no badge, no red dot, no reward, no price, no button affordance, no watermark, no shadow outside silhouette, no opaque canvas/background, no full screen, no collage, no multiple states, no extra components.

## 验收记录
- 初始状态 `pending-generation`; prompt saved before generation.
- Must inspect actual dimensions, RGBA alpha extrema, edge bounds, center alpha/contrast and SHA-256 before any asset readiness claim.
- Candidate remains `approved:false`; 9-slice remains pending visual stretch review.
