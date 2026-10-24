"""UE Editor import script, prepared for the logic-integration lease holder.

Do not run from this panel team. The coordinator assigns actual imports to the
single engine executor and verifies Asset Registry state after import.
"""
import os
import unreal

SOURCE = os.path.abspath(os.path.join(
    os.path.dirname(__file__), "assets", "rank-wuling-paper-panel-default-v1.png"
))
DESTINATION = "/Game/UI/Ink/ComponentRollout20261024/RankWuling"
ASSET_NAME = "T_RankWulingPaperPanel_Default_v1"

if not os.path.isfile(SOURCE):
    raise RuntimeError("Candidate PNG is missing: " + SOURCE)

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = DESTINATION
task.destination_name = ASSET_NAME
task.automated = True
task.replace_existing = False
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

asset_path = DESTINATION + "/" + ASSET_NAME + "." + ASSET_NAME
texture = unreal.EditorAssetLibrary.load_asset(asset_path)
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError("Texture2D import was not found at " + asset_path)

texture.set_editor_property("srgb", True)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_UI)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NoMipmaps)
texture.set_editor_property("never_stream", True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
print("Imported Texture2D; return to coordinator for Asset Registry/hash/size review: " + asset_path)
