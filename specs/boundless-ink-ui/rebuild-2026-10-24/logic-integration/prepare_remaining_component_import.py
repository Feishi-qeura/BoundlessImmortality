from pathlib import Path
import json,hashlib,re
from PIL import Image
H=Path(__file__).resolve().parent;N=H.parent;assets=[]
def blocks(path):
 text=path.read_text(encoding='utf-8-sig');out=[]
 for block in re.split(r'^  - id: ',text,flags=re.M)[1:]:
  row={'id':block.splitlines()[0].strip()}
  for m in re.finditer(r'^    ([a-z0-9_]+): (.+)$',block,re.M):
   raw=m[2].strip();row[m[1]]=json.loads(raw) if raw.startswith('[') or raw.isdigit() else raw
  out.append(row)
 return out
def add(source,package,sha,size,mode,alpha):
 im=Image.open(source);actual=hashlib.sha256(source.read_bytes()).hexdigest();assert actual==sha.lower() and im.size==tuple(size) and im.mode==mode;ext=im.getchannel('A').getextrema() if im.mode=='RGBA' else None;assert tuple(alpha)==ext
 assets.append(dict(source=str(source),source_sha256=actual,package=package,name=package.rsplit('/',1)[1],width=im.width,height=im.height,mode=im.mode,alpha_extrema=ext,approved=False,usage='semantic UI component; text/data/actions stay live widgets'))
r=blocks(N/'panel-rank/asset-manifest.yaml')[-1];add(N/'panel-rank'/r['path'],r['object_path_target'],r['sha256'],[r['width'],r['height']],r['mode'],r['alpha_extrema'])
for r in blocks(N/'panel-activities/asset-manifest.yaml'):
 if r['kind']=='semantic-component-candidate':add(N/'panel-activities'/r['file'],r['intended_object_path'],r['sha256'],r['dimensions_px'],r['mode'],r['alpha'])
C=N/'panel-player/player-c/visible-entry/assets/components';rows=json.loads((C/'asset-manifest.json').read_text(encoding='utf-8-sig'))['assets'];names=['T_MethodLoadoutGlyphAtlas_v1','T_EquipmentFabaoGlyphAtlas_v1','T_AppearancePreviewOrnamentAtlas_v1']
for r,name in zip(rows,names):add(C/r['path'],'/Game/newUI/Components/Player/'+name,r['sha256'],r['dimensions'],r['mode'],r['alpha_extrema'])
assert len(assets)==7
(H/'remaining-component-import-manifest.json').write_text(json.dumps(dict(approved=False,assets=assets),ensure_ascii=False,indent=2),encoding='utf-8')
s=(H/'import_journey_components_ue.py').read_text(encoding='utf-8-sig').replace('journey-import-result.json','remaining-component-import-result.json').replace('journey-import-manifest.json','remaining-component-import-manifest.json').replace("assert row['package'].startswith('/Game/newUI/Textures/Journey/')","assert row['package'].startswith(('/Game/newUI/Components/RankWuling/','/Game/newUI/Components/Player/','/Game/newUI/Activities/Icons/'))")
s=s.replace("save()\n    for name in", "save()\n    registry=unreal.AssetRegistryHelpers.get_asset_registry()\n    for row in inputs:\n        data=registry.get_asset_by_object_path(unreal.Name(row['package']+'.'+row['name']))\n        assert data.is_valid() and str(data.asset_class_path.asset_name)=='Texture2D'\n        assert isinstance(unreal.load_asset(row['package']),unreal.Texture2D)\n    REPORT['asset_registry_readback_passed']=True\n    for name in")
(H/'import_remaining_components_ue.py').write_text(s,encoding='utf-8')
print(json.dumps([dict(name=a['name'],dimensions=[a['width'],a['height']],alpha=a['alpha_extrema']) for a in assets],indent=2))
