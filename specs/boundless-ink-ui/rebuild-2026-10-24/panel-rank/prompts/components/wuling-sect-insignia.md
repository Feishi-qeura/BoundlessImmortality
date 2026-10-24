# Wuling Organization Insignia

## 来源与用途

One standalone transparent semantic emblem for the rebuilt Wuling identity card. The existing `wuling-v2` candidate supplies composition/mood cues only. The PNG is one overlay component in a dynamic UMG page, never the page or a background.

## 尺寸和状态

- Intended display: 160×160 logical units, contain-scaled.
- One neutral emblem state; service status remains live TextBlock content.
- Selected file: `assets/components/wuling-sect-insignia-v3.png` (1254×1254 RGBA, alpha extrema 0–255).
- v1 and v2 remain in the manifest as superseded iterations; v3 is still `approved:false`.

## 必须继承的契约字段

- Muted jade-green, dark teal ink, pale cloth, restrained old-gold detail.
- Soft ink-wash watercolor, broad clear silhouette, readable at small mobile size.
- No text, glyphs, level marks, count, or service state baked into the image.

## 图像生成提示词

Initial generation: “One minimal isolated mobile-game UI emblem, square composition: a small pale jade-green cloth badge with a simple dark teal crossed-sword mark inside a loose circular ink brush ring. Gentle Chinese ink-wash illustration, flat watercolor and rice-paper pigment, very restrained antique-gold pinpoints, light muted palette. Clean silhouette, broad safe margins, transparent background, no scenery, no landscape, no hanging tassels, no metal filigree, no 3D gloss, no text, no glyphs, no numbers, no frame or opaque backing. A single centered icon that remains clear at 160x160 UMG size. Real transparent alpha, not checkerboard.”

Final edit: “Clean only the outer transparent alpha boundary on this existing small jade banner UI emblem. Preserve the central crossed-sword brush emblem and the light watercolor jade cloth exactly in style and composition. Remove bright green/yellow fringe pixels, stray speckles, and jagged cutout artifacts around the silhouette; make the outside transparent and edge transition smooth and clean at small icon scale. Keep genuine transparent background, no text, no new ornament, no changes to the central emblem.”

## 反向限制

- No legible or pseudo-Chinese lettering, digits, logos, watermark, portrait, or page composition.
- No opaque rectangle, white matte, large landscape, or added gameplay stat.
- No neon, glossy 3D metal, sharp clipped outline, or stray alpha artifacts.

## 验收记录

- v3 generated/edited with built-in imagegen on 2026-10-24 and saved locally.
- Measured dimensions 1254×1254, RGBA, alpha extrema 0–255; SHA-256 `d42fbe4f9a002fc8927c4cc47471a236c43a7c1eec1f94d1810c28be2a942ca8`; 1,207,656 bytes.
- PNG contains no text; transparent exterior is confirmed. Human visual approval, UE texture import/readback, and in-page scale/edge review are pending.
