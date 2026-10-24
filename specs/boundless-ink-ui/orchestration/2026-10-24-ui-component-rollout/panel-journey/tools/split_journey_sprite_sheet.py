from __future__ import annotations

import hashlib
import json
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "inputs" / "journey-state-sprites-v1-source.png"
OUT = ROOT / "assets" / "sprites"

# Stable semantic order for the 11 isolated alpha-connected illustrations.
ITEMS = [
    ("world-node-explored", "world-map", "explored", 0),
    ("world-node-available", "world-map", "available", 0),
    ("world-node-locked", "world-map", "locked", 0),
    ("resource-pin-free", "resources-map", "free", 1),
    ("resource-pin-temporary", "resources-map", "temporary", 1),
    ("resource-pin-contest", "resources-map", "contest-open", 1),
    ("resource-pin-expired", "resources-map", "expired", 1),
    ("fortune-great", "daily-sign-in", "great", 2),
    ("fortune-good", "daily-sign-in", "good", 2),
    ("fortune-neutral", "daily-sign-in", "neutral", 2),
    ("fortune-bad", "daily-sign-in", "bad", 2),
]
ALPHA_THRESHOLD = 8
PADDING = 8


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    source = Image.open(SOURCE).convert("RGBA")
    if source.size != (1536, 1024):
        raise SystemExit(f"Unexpected source size: {source.size}")
    OUT.mkdir(parents=True, exist_ok=True)
    alpha = source.getchannel("A").tobytes()
    # Run-length connected-component labeling (8-neighbor) avoids pulling the
    # neighboring row's dangling ornaments into a rectangular manual crop.
    parent: list[int] = []
    spans_by_row: list[list[tuple[int, int, int]]] = []

    def find(value: int) -> int:
        while parent[value] != value:
            parent[value] = parent[parent[value]]
            value = parent[value]
        return value

    previous: list[tuple[int, int, int]] = []
    for y in range(source.height):
        row: list[tuple[int, int, int]] = []
        x = 0
        offset = y * source.width
        while x < source.width:
            while x < source.width and alpha[offset + x] < ALPHA_THRESHOLD:
                x += 1
            if x >= source.width:
                break
            left = x
            while x < source.width and alpha[offset + x] >= ALPHA_THRESHOLD:
                x += 1
            right = x - 1
            label = len(parent)
            parent.append(label)
            for prev_left, prev_right, prev_label in previous:
                if prev_left <= right + 1 and prev_right >= left - 1:
                    a, b = find(label), find(prev_label)
                    if a != b:
                        parent[a] = b
            row.append((left, right, label))
        spans_by_row.append(row)
        previous = row

    root_bounds: dict[int, list[int]] = {}
    for y, row in enumerate(spans_by_row):
        for left, right, label in row:
            root = find(label)
            bounds = root_bounds.setdefault(root, [left, y, right, y, 0])
            bounds[0] = min(bounds[0], left)
            bounds[1] = min(bounds[1], y)
            bounds[2] = max(bounds[2], right)
            bounds[3] = max(bounds[3], y)
            bounds[4] += right - left + 1

    major = [(root, bounds) for root, bounds in root_bounds.items() if bounds[4] > 200]
    rows: list[list[tuple[int, list[int]]]] = [[], [], []]
    for root, bounds in major:
        row_index = 0 if bounds[1] < 340 else 1 if bounds[1] < 680 else 2
        rows[row_index].append((root, bounds))
    for row in rows:
        row.sort(key=lambda pair: pair[1][0])
    expected_per_row = [3, 4, 4]
    if [len(row) for row in rows] != expected_per_row:
        raise SystemExit(f"Expected 3/4/4 glyph components; got {[len(row) for row in rows]}")
    row_roots = [[root for root, _ in row] for row in rows]
    component_roots = [root for row in row_roots for root in row]

    records = []
    for (component_id, family, state, row_index), root in zip(ITEMS, component_roots):
        _, family_bounds = next(pair for pair in rows[row_index] if pair[0] == root)
        source_bounds = (family_bounds[0], family_bounds[1], family_bounds[2] + 1, family_bounds[3] + 1)
        left = max(0, source_bounds[0] - PADDING)
        top = max(0, source_bounds[1] - PADDING)
        right = min(source.width, source_bounds[2] + PADDING)
        bottom = min(source.height, source_bounds[3] + PADDING)
        item = source.crop((left, top, right, bottom))
        item_pixels = item.load()
        # Keep only this labeled component. Drop faint background haze/other
        # labels while preserving each source pixel's original semi-transparent alpha.
        for y in range(top, bottom):
            for span_left, span_right, span_label in spans_by_row[y]:
                if find(span_label) != root:
                    for x in range(max(left, span_left), min(right - 1, span_right) + 1):
                        px, py = x - left, y - top
                        red, green, blue, _ = item_pixels[px, py]
                        item_pixels[px, py] = (red, green, blue, 0)
        # Clear pixels below the measured alpha threshold too, so overlapping
        # antialias haze cannot leak from one isolated component to another.
        item.putalpha(item.getchannel("A").point(lambda value: value if value >= ALPHA_THRESHOLD else 0))
        if item.getchannel("A").getextrema()[1] == 0:
            raise SystemExit(f"Visible illustration alpha missing for {component_id}")
        path = OUT / f"{component_id}-v1.png"
        item.save(path, format="PNG", optimize=True)
        item_alpha = item.getchannel("A").tobytes()
        alpha_corners = [
            item.getpixel((0, 0))[3],
            item.getpixel((item.width - 1, 0))[3],
            item.getpixel((0, item.height - 1))[3],
            item.getpixel((item.width - 1, item.height - 1))[3],
        ]
        records.append({
            "component_id": component_id,
            "family": family,
            "state": state,
            "source_bounds_xyxy": list(source_bounds),
            "export_bounds_xyxy": [left, top, right, bottom],
            "dimensions": list(item.size),
            "mode": item.mode,
            "alpha_threshold": ALPHA_THRESHOLD,
            "alpha_range": [min(item_alpha), max(item_alpha)],
            "transparent_corner_alpha": alpha_corners,
            "fully_transparent_pixels": item_alpha.count(0),
            "partially_transparent_pixels": sum(1 for value in item_alpha if 0 < value < 255),
            "sha256": sha256(path),
            "path": str(path.relative_to(ROOT)).replace("\\", "/"),
            "approved": False,
        })
    result = {
        "source": str(SOURCE.relative_to(ROOT)).replace("\\", "/"),
        "source_sha256": sha256(SOURCE),
        "alpha_threshold_for_connected_components_and_output": ALPHA_THRESHOLD,
        "padding_px": PADDING,
        "items": records,
    }
    manifest = ROOT / "assets" / "journey-sprite-manifest-v1.json"
    manifest.parent.mkdir(parents=True, exist_ok=True)
    manifest.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"exported={len(records)} source={result['source_sha256']}")
    for item in records:
        print(f"{item['component_id']} {item['dimensions'][0]}x{item['dimensions'][1]} alpha={item['alpha_range']} transparent={item['fully_transparent_pixels']} partial={item['partially_transparent_pixels']} sha256={item['sha256']}")


if __name__ == "__main__":
    main()
