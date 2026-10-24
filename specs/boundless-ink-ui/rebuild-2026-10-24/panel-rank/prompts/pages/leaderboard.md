# 排行榜 · three provider-backed variants

## 页面定位

One responsive `LeaderboardVm` page with Damage, CultivationSpeed, and Anchor variants. It lets the player compare provider-ranked participants and keep the local/global context visible.

## 视觉约束

Use source compositions `rank-damage-v2`, `rank-cultivation-v2`, and `rank-spirit-v2` as layout inputs. Apply shared `newui-r1` style tokens: warm rice paper, quiet ink-green, jade focus, restrained old gold, live MaShanZheng/LXGWWenKai text, 24-unit safe inset, and 88-unit touch targets. Do not reuse any candidate as runtime artwork; do not create full-page background art.

## 布局与组件

- Top: Root-owned back control, dynamic category title, provider state.
- Next: three category tabs and Local/Global scope strip.
- Body: rank, participant identity/avatar/title/realm, and category metric columns; preserve provider ordering and ties; body scrolls independently.
- Footer: provider self-position and metric if supplied; otherwise an explicit not-provided state.
- Landscape: Root reflows to rail/list-detail; never scale portrait page as a whole.
- State surface: unknown, loading, empty, data, pending, offline, error, disabled. Offline can retain provider-supplied stale rows read-only.

## 状态

Damage uses the actual Damage metric; CultivationSpeed uses the actual rate; Anchor uses the actual sword count/metric. No local sorting, inferred self rank, reward, period picker, or claim action.

## 图像生成提示词

No new full-page image is requested: all three candidate compositions already exist and are unapproved. If a later visual component is required, generate only an isolated text-free semantic asset; runtime labels, numbers, tabs, rows, and state messages remain UMG.

## 反向限制

No full-page PNG as runtime UI, no fabricated ranks/metrics/rewards, no missing value coerced to zero, no avatar/participant conflation, and no portrait-to-landscape ScaleBox.

## 验收记录

Staged native component: `UInkRankBoardPage`. Code/route/font binding, actual portrait/landscape composition, and player-entry evidence pending integration.
