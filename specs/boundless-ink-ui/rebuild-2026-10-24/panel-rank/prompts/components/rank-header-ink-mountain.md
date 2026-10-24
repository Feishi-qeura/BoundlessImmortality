# Leaderboard Ink Mountain Header Decoration

## 来源与用途

Create a single transparent title-area decoration for the rebuilt shared leaderboard page. It supplies only the quiet ink-mountain/sky ornament around dynamic UMG title text, never the title or the full page.

## 尺寸和状态

- One neutral state, intended to contain-scale within a 720 logical-unit wide header.
- Transparent PNG with full alpha; wide horizontal composition preferred, complete edges.
- Dynamic title, service state, tabs, and all text remain live UMG.

## 必须继承的契约字段

- Pale rice-paper surroundings, low-contrast ink mountain silhouettes, soft upper-left light, restrained old-gold accent under five percent of the visible area.
- Quiet, solemn xianxia atmosphere; clear empty center for a large dynamic Chinese title.
- Decoration only; no controls or information are embedded.

## 图像生成提示词

Single isolated transparent game UI ornament, a wide horizontal Chinese xianxia ink-wash mountain header for a leaderboard. Soft, pale grey-green distant mountains and a few restrained pine silhouettes frame the far left and right edges; the central two-thirds is calm, open, and transparent enough to place a live title over it. A faint pale-gold sun ring and a few tiny old-gold ink details, restrained and low contrast. Rice-paper ink painting edges dissolve naturally into transparency. Complete clean silhouette, no rectangular plate, no page layout, no text, no calligraphy, no glyphs, no numbers, no watermark. Elegant, readable, low detail at mobile size.

## 反向限制

- No full-screen landscape, opaque background, paper rectangle, UI card, frame, tab, button, or footer.
- No text or generated pseudo-Chinese marks; no central high-contrast subject competing with the title.
- No dense gold filigree, hard rectangular crop, clipped branches, black shadow fringe, or false alpha checkerboard.

## 验收记录

- Two iterations were generated and visually inspected, then discarded because their alpha edges contained colored speckle/fringe and the artwork was too busy for a live-text header.
- No rank-header PNG is delivered or referenced by the component. The runtime title/header remains live UMG text over the native page frame.
- If a later header asset is needed, start from a new prompt/version; do not use the discarded iterations.
