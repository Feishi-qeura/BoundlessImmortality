"""Run from Unreal Editor Python after logic-integration grants the engine lease."""

import hashlib
import json
from pathlib import Path

import unreal


HERE = Path(__file__).resolve().parents[1]
MANIFEST = HERE / "assets" / "journey-sprite-manifest-v1.json"
DESTINATION = "/Game/UI/Ink/ComponentRollout20261024/Journey"
TEXTURE_NAMES = {
    "world-node-explored": "T_Journey_WorldNodeExplored",
    "world-node-available": "T_Journey_WorldNodeAvailable",
    "world-node-locked": "T_Journey_WorldNodeLocked",
    "resource-pin-free": "T_Journey_ResourcePinFree",
    "resource-pin-temporary": "T_Journey_ResourcePinTemporary",
    "resource-pin-contest-open": "T_Journey_ResourcePinContestOpen",
    "resource-pin-expired": "T_Journey_ResourcePinExpired",
    "fortune-great": "T_Journey_FortuneGreat",
    "fortune-good": "T_Journey_FortuneGood",
    "fortune-neutral": "T_Journey_FortuneNeutral",
    "fortune-bad": "T_Journey_FortuneBad",
}


def file_sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def enum_value(enum_type, names):
    for name in names:
        value = getattr(enum_type, name, None)
        if value is not None:
            return value
    raise RuntimeError("Unreal enum member not found: " + ", ".join(names))


with MANIFEST.open("r", encoding="utf-8") as stream:
    manifest = json.load(stream)

if len(manifest["items"]) != len(TEXTURE_NAMES):
    raise RuntimeError("Manifest item count does not match importer map")

assets = unreal.EditorAssetLibrary
for item in manifest["items"]:
    component_id = item["component_id"]
    texture_name = TEXTURE_NAMES[component_id]
    source = (HERE / item["path"]).resolve()
    if not source.is_file() or file_sha256(source) != item["sha256"]:
        raise RuntimeError("Missing or changed PNG: " + str(source))
    package_name = DESTINATION + "/" + texture_name
    if assets.does_asset_exist(package_name):
        raise RuntimeError("Refusing to overwrite existing asset: " + package_name)

tasks = []
for item in manifest["items"]:
    task = unreal.AssetImportTask()
    task.filename = str((HERE / item["path"]).resolve())
    task.destination_path = DESTINATION
    task.destination_name = TEXTURE_NAMES[item["component_id"]]
    task.automated = True
    task.save = True
    task.replace_existing = False
    task.replace_existing_settings = False
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
compression = enum_value(unreal.TextureCompressionSettings, ("TC_USER_INTERFACE2D", "TC_USERINTERFACE2D"))
lod_group = enum_value(unreal.TextureGroup, ("TEXTUREGROUP_UI",))
mip_policy = enum_value(unreal.TextureMipGenSettings, ("TMGS_NO_MIPMAPS",))

for component_id, texture_name in TEXTURE_NAMES.items():
    package_name = DESTINATION + "/" + texture_name
    texture = assets.load_asset(package_name)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Imported texture missing: " + package_name)
    texture.srgb = True
    texture.compression_settings = compression
    texture.lod_group = lod_group
    texture.mip_gen_settings = mip_policy
    texture.never_stream = True
    texture.post_edit_change()
    if not assets.save_loaded_asset(texture, only_if_is_dirty=True):
        raise RuntimeError("Could not save imported texture: " + package_name)

print("Imported 11 candidate textures to " + DESTINATION)
print("Import verification still requires AssetRegistry readback, texture setting inspection, and runtime references.")
