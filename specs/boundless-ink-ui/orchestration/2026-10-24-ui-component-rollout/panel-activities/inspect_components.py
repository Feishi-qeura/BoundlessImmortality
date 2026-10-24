from pathlib import Path
from PIL import Image, ImageDraw
import hashlib, json

ROOT = Path(__file__).resolve().parent
IMAGE = ROOT / "assets" / "hud-launch-countdown-frame-neutral-v1.png"
QA = ROOT / "qa"
QA.mkdir(exist_ok=True)
MARGINS = (140, 140, 90, 90)  # left, right, top, bottom in measured source pixels

def nine_slice(source: Image.Image, target_size: tuple[int, int], margins: tuple[int, int, int, int]) -> Image.Image:
    left, right, top, bottom = margins
    width, height = source.size
    out_w, out_h = target_size
    if left + right >= width or top + bottom >= height or left + right >= out_w or top + bottom >= out_h:
        raise ValueError("nine-slice margins leave no stretchable center")
    xs = [0, left, width-right, width]
    ys = [0, top, height-bottom, height]
    dx = [0, left, out_w-right, out_w]
    dy = [0, top, out_h-bottom, out_h]
    out = Image.new("RGBA", target_size, (0, 0, 0, 0))
    for row in range(3):
        for col in range(3):
            crop = source.crop((xs[col], ys[row], xs[col+1], ys[row+1]))
            size = (dx[col+1]-dx[col], dy[row+1]-dy[row])
            if crop.size != size:
                crop = crop.resize(size, Image.Resampling.LANCZOS)
            out.alpha_composite(crop, (dx[col], dy[row]))
    return out

im = Image.open(IMAGE).convert("RGBA")
a = im.getchannel("A")
alpha_hist = a.histogram()
metrics = {
    "path": IMAGE.relative_to(ROOT).as_posix(),
    "size": list(im.size),
    "mode": "RGBA",
    "alpha_extrema": list(a.getextrema()),
    "alpha_bbox": list(a.getbbox()) if a.getbbox() else None,
    "alpha_pixel_counts": {"transparent": alpha_hist[0], "opaque": alpha_hist[255], "partial": sum(alpha_hist[1:255])},
    "sha256": hashlib.sha256(IMAGE.read_bytes()).hexdigest().upper(),
    "source_pixel_slice_margins_ltrb": list(MARGINS),
    "preview_targets_logical": [[200, 80], [280, 80]],
}
background = Image.new("RGBA", (620, 300), (224, 227, 223, 255))
draw = ImageDraw.Draw(background)
for y in range(0, 300, 16):
    for x in range(0, 620, 16):
        if ((x//16)+(y//16)) % 2 == 0:
            draw.rectangle((x, y, x+15, y+15), fill=(243, 244, 240, 255))
for index, logical in enumerate(((200, 80), (280, 80))):
    scaled = (round(MARGINS[0]*logical[0]/im.width), round(MARGINS[1]*logical[0]/im.width), round(MARGINS[2]*logical[1]/im.height), round(MARGINS[3]*logical[1]/im.height))
    frame = nine_slice(im, logical, scaled).resize((logical[0]*2, logical[1]*2), Image.Resampling.LANCZOS)
    background.alpha_composite(frame, (20, 20 + index*132))
background.convert("RGB").save(QA / "hud-launch-countdown-nine-slice-preview.jpg", quality=94)
(QA / "component-measurements.json").write_text(json.dumps(metrics, ensure_ascii=False, indent=2), encoding="utf-8")
print(json.dumps(metrics, ensure_ascii=False))

