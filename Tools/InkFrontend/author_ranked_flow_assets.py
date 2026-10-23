"""One isolated editor session for tool-owned rank art and actual Niagara trails."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())/'Tools/InkFrontend'
for name in ('create_ink_gold_backings.py','create_rank_theme_assets.py','create_title_niagara_assets.py'):
    path=root/name
    source=path.read_text(encoding='utf-8').replace('unreal.SystemLibrary.quit_editor()','pass # outer runner owns exit')
    exec(compile(source,str(path),'exec'),{'__name__':'__rank_asset_author__'})
unreal.SystemLibrary.quit_editor()
