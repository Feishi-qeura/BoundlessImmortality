from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())/'Tools/InkFrontend'
for name in ['import_home_v2.py','audit_home_v2_assets.py']:
 p=root/name;exec(compile(p.read_text('utf-8'),str(p),'exec'),{'__file__':str(p),'__name__':'__main__'})
