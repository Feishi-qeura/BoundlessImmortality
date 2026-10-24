from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path
from zipfile import ZipFile

import yaml
from PIL import Image


ROOT = Path(__file__).resolve().parents[1]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    errors: list[str] = []
    manifest_path = ROOT / "assets" / "journey-sprite-manifest-v1.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    ids = [item["component_id"] for item in manifest["items"]]
    if len(ids) != 11 or len(set(ids)) != 11:
        errors.append("Expected 11 unique glyph assets")

    for item in manifest["items"]:
        path = ROOT / item["path"]
        if not path.is_file():
            errors.append(f"Missing image: {item['path']}")
            continue
        image = Image.open(path)
        if image.format != "PNG" or image.mode != "RGBA":
            errors.append(f"Wrong format/mode: {item['component_id']} {image.format} {image.mode}")
        if list(image.size) != item["dimensions"]:
            errors.append(f"Dimension mismatch: {item['component_id']}")
        low, high = image.getchannel("A").getextrema()
        if low != 0 or high == 0:
            errors.append(f"Alpha channel missing/empty: {item['component_id']} {low}..{high}")
        corners = [
            image.getpixel((0, 0))[3],
            image.getpixel((image.width - 1, 0))[3],
            image.getpixel((0, image.height - 1))[3],
            image.getpixel((image.width - 1, image.height - 1))[3],
        ]
        if corners != [0, 0, 0, 0] or corners != item["transparent_corner_alpha"]:
            errors.append(f"Transparent output corners missing: {item['component_id']} {corners}")
        if item["alpha_range"] != [low, high]:
            errors.append(f"Recorded alpha range mismatch: {item['component_id']}")
        if item["fully_transparent_pixels"] != image.getchannel("A").tobytes().count(0):
            errors.append(f"Recorded transparent-pixel count mismatch: {item['component_id']}")
        if image.getchannel("A").getbbox() is None:
            errors.append(f"Image has no visible alpha pixels: {item['component_id']}")
        if sha256(path) != item["sha256"]:
            errors.append(f"SHA-256 mismatch: {item['component_id']}")

    contracts = [
        ROOT / "component-contract.yaml",
        ROOT / "sprite-contract.yaml",
        ROOT / "asset-import-map.yaml",
    ]
    parsed = {}
    for path in contracts:
        parsed[path.name] = yaml.safe_load(path.read_text(encoding="utf-8"))
    if parsed["component-contract.yaml"]["approved"] is not False:
        errors.append("Candidate component approval must remain false")
    if parsed["sprite-contract.yaml"]["review"]["text_free"] is not True:
        errors.append("Generated sprite sheet must be text free")
    if len(parsed["asset-import-map.yaml"]["items"]) != 11:
        errors.append("Importer mapping does not contain all 11 textures")

    source_manifest_path = ROOT / "assets" / "source-pages" / "candidate-source-hashes.json"
    source_manifest = json.loads(source_manifest_path.read_text(encoding="utf-8"))
    for item in source_manifest["items"]:
        path = ROOT / item["path"]
        if not path.is_file() or sha256(path) != item["sha256"]:
            errors.append(f"Candidate source changed: {item['path']}")
        elif list(Image.open(path).size) != item["dimensions"]:
            errors.append(f"Candidate source dimensions changed: {item['path']}")

    project_root = ROOT.parents[4]
    cpp = (project_root / "Source" / "BoundlessImmortality" / "Frontend" / "UiRollout20261024" / "Journey" / "JourneyRolloutWidgets.cpp").read_text(encoding="utf-8")
    for texture_name in (item["texture_name"] for item in parsed["asset-import-map.yaml"]["items"]):
        if texture_name not in cpp:
            errors.append(f"C++ widget asset path missing: {texture_name}")
    header_path = project_root / "Source" / "BoundlessImmortality" / "Frontend" / "UiRollout20261024" / "Journey" / "JourneyRolloutWidgets.h"
    header = header_path.read_text(encoding="utf-8")
    for expected in ("UJourneyStateBadgeWidget", "UJourneyCalendarDayWidget", "UJourneyClaimStateWidget", "OnClaimRequested", "OnDayPressed"):
        if expected not in header:
            errors.append(f"Widget API missing: {expected}")
    if not re.search(r"ClaimState == EDailySignInClaimState::Claimable && !bClaimEventSent", cpp):
        errors.append("One-shot claim guard is missing")
    if "Family != EJourneyGlyphFamily::FortuneSeal" not in cpp:
        errors.append("Read-only fortune glyph guard is missing")
    if "EDailySignInClaimState::Pending" not in cpp or "EDailySignInClaimState::Claimed" not in cpp:
        errors.append("Mutually exclusive claim state handling is incomplete")

    zip_path = ROOT / "packages" / "journey-components-v1-png.zip"
    with ZipFile(zip_path) as package:
        names = package.namelist()
        pngs = [name for name in names if name.lower().endswith(".png")]
        if len(pngs) != 11:
            errors.append(f"ZIP PNG count is {len(pngs)}, expected 11")
        if len(names) != len(set(names)):
            errors.append("ZIP has duplicate paths")
        bad = package.testzip()
        if bad:
            errors.append(f"ZIP CRC failure: {bad}")

    if errors:
        print("FAILED")
        for error in errors:
            print("- " + error)
        raise SystemExit(1)
    print("PASS: 11 RGBA glyphs, measured dimensions, alpha bounds and SHA-256; 4 candidate pages unchanged; YAML/contracts; API source guards; ZIP contents/CRC.")
    print("LIMIT: static/source and package checks only; no UBT, asset import, route, PIE, device or visual approval was performed.")


if __name__ == "__main__":
    main()
