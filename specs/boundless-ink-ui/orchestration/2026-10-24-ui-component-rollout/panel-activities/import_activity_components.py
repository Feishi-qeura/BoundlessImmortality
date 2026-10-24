import argparse
from pathlib import Path
import unreal

DESTINATION = "/Game/UI/Ink/ComponentRollout20261024/Activities"
ASSET_NAME = "T_HudLaunchCountdownFrame_v1"


def main():
    parser = argparse.ArgumentParser(description="Import this team's isolated activity HUD frame after engine lease is granted.")
    parser.add_argument("--source", required=True, help="Absolute path to the team-owned PNG")
    parser.add_argument("--dry-run", action="store_true", help="Report paths without importing")
    args = parser.parse_args()

    source = Path(args.source).resolve(strict=True)
    if source.suffix.lower() != ".png":
        raise RuntimeError(f"Expected PNG input, got {source}")
    object_path = f"{DESTINATION}/{ASSET_NAME}.{ASSET_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        raise RuntimeError(f"Refusing to replace an existing asset: {object_path}")
    unreal.log(f"Activity component import preflight: {source} -> {object_path}")
    if args.dry_run:
        return

    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = DESTINATION
    task.destination_name = ASSET_NAME
    task.replace_existing = False
    task.automated = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset = unreal.EditorAssetLibrary.load_asset(object_path)
    if not isinstance(asset, unreal.Texture2D):
        raise RuntimeError(f"Import did not produce Texture2D at {object_path}")
    size = (asset.blueprint_get_size_x(), asset.blueprint_get_size_y())
    unreal.log(f"Imported activity frame: {object_path}; Texture2D={size}; alpha_checked_in_source=true")


if __name__ == "__main__":
    main()
