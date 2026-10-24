# Additive frame handoff

Recovered and installed the previously generated breakthrough material-slot frame from the local imagegen cache. No image was generated again.

## Delivered

- `assets/breakthrough-material-slot-frame-default-v1.png`
- `asset-generation-provenance.md`
- `additive-frame-manifest.json`

## Pixel evidence

- PNG, RGBA, 1254×1254, 774,053 bytes
- SHA-256: `8b93fa72780f31737ca5fc7926b5c79f7f796bcaf14c773cb2ba47b62193d9e3`
- Center pixel alpha 0; all four corner alpha values 0
- At alpha threshold 16, frame content bounds are x=98..1156, y=102..1144 (exclusive right/bottom in Pillow bbox)
- The center 300×300 opening has 227 pixels at alpha 1 and 89,773 pixels at alpha 0; outer 32-pixel edge contains 204 pixels at alpha 1. These faint specks are recorded for visual review.
- Single square frame, default state. `slice: full`; 9-slice is false and margins are zero.

Candidate status remains `approved:false`. The frame is not in the earlier ZIP or component manifest. No source files, Content assets, shared files, routes, Editor, or UBT were touched.

## Next step

The integration owner may import this additive PNG into the assigned Player candidate destination under the engine lease and feed it to `UInkPlayerMaterialSlotWidget::ConfigureSlot`. Keep item icon, dynamic text, counts, and selectability sourced from the authority adapter.
