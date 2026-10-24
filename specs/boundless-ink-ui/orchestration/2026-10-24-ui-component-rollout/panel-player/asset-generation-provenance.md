# Additive frame asset recovery

Recovered the previously generated output from the Codex image-generation cache instead of generating another image:

- Original output: `C:/Users/zxxuh/.codex/generated_images/01a16b9b-3510-7f53-acb8-af72d60a1c68/exec-ea0720f5-9c72-4049-bf69-27997f81e7e9.png`
- Generated output timestamp: 2026-10-24 09:54:18 Asia/Hong_Kong (file timestamp).
- Copied unchanged to `assets/breakthrough-material-slot-frame-default-v1.png`.
- This is the second existing local output associated with the saved prompt, selected for its centered square paper/gold frame and clear center. No newly generated external/local reference was introduced.
- Referenced source candidate: `specs/boundless-ink-ui/revisions/2026-10-23-player-core-mapping-correction/breakthrough-preparation-v3.png` (layout/style cue only; remains approved:false).

## Prompt provenance

Saved prompt: `prompts/components/breakthrough-material-slot-frame-default-v1.md`.

Effective image-generation request: “Use the supplied breakthrough preparation page only as a muted style reference. Create one isolated square game UI material-slot frame for a Chinese cultivation water-ink mobile game. Warm aged ivory paper and restrained antique-gold thin linework with tiny geometric corner ornaments; straight-on orthographic view, symmetrical, centered, complete unclipped silhouette, refined and readable at small size. The frame must have a wide clear central aperture that is fully transparent so a runtime item icon can be composited beneath it. Transparent background outside the frame and inside the aperture. High-resolution clean alpha edges, suitable for UMG composition. No text, glyphs, numbers, icons, plus sign, item art, page background, button, shadow beyond the frame, extra components, or opaque margin. Do not copy any full-page text or values. Single component only.”

## Pixel inspection

Measured values and SHA-256 are in `additive-frame-manifest.json`. The PNG is 1254×1254 RGBA, 774,053 bytes. The alpha=0 center pixel and transparent corners confirm an actual transparent opening. Pixels with alpha >=16 occupy [98,102,1157,1145); faint A=1 specks occur in the outer 32 pixels and in the center aperture, so this remains a candidate rather than approved production art. There is no baked label, number, item icon, or plus mark in the frame.

The ornate frame is aspect-preserving (`full`) and has no 9-slice margins. Do not stretch it to fit a non-square slot. Suggested presentation: place the separately rendered icon and dynamic name/count above the transparent center in the existing slot layout. This file was not added to the previously delivered ZIP or manifest.

## Status

- `approved:false`; visual review remains with the user.
- No Unreal import, UBT, Content write, route binding, or runtime test was performed.
