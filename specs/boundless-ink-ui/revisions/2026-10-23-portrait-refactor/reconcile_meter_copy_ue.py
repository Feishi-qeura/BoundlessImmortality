"""Consolidate the migration's redundant meter texture only after pixel identity proof."""
from pathlib import Path
import hashlib,json,unreal
ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/PortraitUI20261023'
old=unreal.load_asset('/Game/UI/Ink/NiagaraRevision20261022/Textures/T_XianxiaMeters')
new=unreal.load_asset('/Game/UI/Ink/Textures/HUD/T_XianxiaMeters')
assert old and new and old!=new and old.get_class()==unreal.Texture2D.static_class() and new.get_class()==unreal.Texture2D.static_class()
digests=[]
for key,obj in [('old',old),('canonical',new)]:
 task=unreal.AssetExportTask();task.object=obj;task.filename=str(OUT/('meter-'+key+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterPNG()
 assert unreal.Exporter.run_asset_export_task(task)
 digests.append(hashlib.sha256(Path(task.filename).read_bytes()).hexdigest())
assert digests[0]==digests[1],digests
assert unreal.EditorAssetLibrary.consolidate_assets(new,[old])
(OUT/'meter-consolidation.json').write_text(json.dumps(dict(pixel_identical=True,sha256=digests[0],canonical=new.get_path_name(),redundant_migration_copy_consolidated=True)),encoding='utf-8')
exec((ROOT/'specs/boundless-ink-ui/revisions/2026-10-23-portrait-refactor/finish_migration_ue.py').read_text('utf-8'))
