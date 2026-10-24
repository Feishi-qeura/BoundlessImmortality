"""Run only inside Unreal Editor Python after logic-integration grants the engine lease.
Imports this team's PNGs to the isolated Player candidate directory and writes read-back metadata.
"""
import json
from pathlib import Path
import unreal

TEAM_DIR = Path(__file__).resolve().parent
SOURCE_DIR = TEAM_DIR / 'assets'
DESTINATION = '/Game/UI/Ink/ComponentRollout20261024/Player'
MANIFEST = json.loads((TEAM_DIR / 'asset-manifest.json').read_text(encoding='utf-8'))

existing = []
for row in MANIFEST['assets']:
    asset_name = Path(row['file']).stem
    object_path = f'{DESTINATION}/{asset_name}.{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        existing.append(object_path)
if existing:
    raise RuntimeError('Refusing to replace candidate assets: ' + ', '.join(existing))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for row in MANIFEST['assets']:
    filename = SOURCE_DIR / Path(row['file']).name
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = DESTINATION
    task.automated = True
    task.replace_existing = False
    task.save = True
    task.imported_object_paths = []
    tasks.append(task)
asset_tools.import_asset_tasks(tasks)

readback = []
for row in MANIFEST['assets']:
    asset_name = Path(row['file']).stem
    object_path = f'{DESTINATION}/{asset_name}.{asset_name}'
    texture = unreal.EditorAssetLibrary.load_asset(object_path)
    if not texture:
        raise RuntimeError('Import did not produce expected texture: ' + object_path)
    width = int(texture.blueprint_get_size_x())
    height = int(texture.blueprint_get_size_y())
    readback.append({'object_path': object_path, 'width': width, 'height': height,
                     'source_sha256': row['sha256'], 'source_dimensions_px': row['dimensions_px']})

(TEAM_DIR / 'unreal-import-readback.json').write_text(
    json.dumps({'destination': DESTINATION, 'count': len(readback), 'assets': readback}, indent=2),
    encoding='utf-8')
print('Imported and read back {} Player component textures into {}'.format(len(readback), DESTINATION))
