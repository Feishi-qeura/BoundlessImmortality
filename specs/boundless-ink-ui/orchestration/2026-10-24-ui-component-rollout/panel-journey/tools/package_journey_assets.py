from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "packages" / "journey-components-v1-png.zip"

FILES = [
    ROOT / "quickstart.md",
    ROOT / "component-contract.yaml",
    ROOT / "sprite-contract.yaml",
    ROOT / "asset-import-map.yaml",
    ROOT / "component-to-control.csv",
    ROOT / "assets" / "journey-sprite-manifest-v1.json",
    ROOT / "prompts" / "journey-state-sprites-v1.md",
]
FILES += sorted((ROOT / "assets" / "sprites").glob("*.png"))

PACKAGE.parent.mkdir(parents=True, exist_ok=True)
with ZipFile(PACKAGE, "w", compression=ZIP_DEFLATED, compresslevel=9) as archive:
    for path in FILES:
        archive.write(path, path.relative_to(ROOT).as_posix())
print(f"created={PACKAGE.relative_to(ROOT)} entries={len(FILES)}")
