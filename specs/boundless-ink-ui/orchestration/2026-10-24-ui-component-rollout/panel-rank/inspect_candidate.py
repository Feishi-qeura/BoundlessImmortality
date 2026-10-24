"""Measure the panel team's generated component and render a 9-slice guide."""
from hashlib import sha256
import json
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent
ASSET = ROOT / "assets" / "rank-wuling-paper-panel-default-v1.png"
GUIDE = ROOT / "evidence" / "rank-wuling-paper-panel-9slice-guide-v1.png"
REPORT = ROOT / "asset-measurements.json"
MARGINS = (136, 136, 136, 136)  # left, right, top, bottom in source pixels

image = Image.open(ASSET).convert("RGBA")
alpha = image.getchannel("A")
width, height = image.size
left, right, top, bottom = MARGINS
assert width > left + right and height > top + bottom

guide = image.copy()
draw = ImageDraw.Draw(guide, "RGBA")
for x in (left, width - right):
    draw.line((x, 0, x, height), fill=(230, 0, 30, 220), width=3)
for y in (top, height - bottom):
    draw.line((0, y, width, y), fill=(230, 0, 30, 220), width=3)
GUIDE.parent.mkdir(parents=True, exist_ok=True)
guide.save(GUIDE)

report = {
    "path": "assets/rank-wuling-paper-panel-default-v1.png",
    "dimensions": [width, height],
    "mode": Image.open(ASSET).mode,
    "alpha_extrema": list(alpha.getextrema()),
    "alpha_content_bbox": list(alpha.getbbox()),
    "sha256": sha256(ASSET.read_bytes()).hexdigest(),
    "slice_margins_pixels": list(MARGINS),
    "slice_status": "candidate-awaiting-UE-preview-and-user-visual-review",
    "text_free_visual_check": True,
    "visual_approval": False,
    "guide": "evidence/rank-wuling-paper-panel-9slice-guide-v1.png",
}
REPORT.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report, ensure_ascii=False, indent=2))
