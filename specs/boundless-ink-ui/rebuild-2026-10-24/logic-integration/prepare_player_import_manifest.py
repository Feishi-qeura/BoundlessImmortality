"""Offline exact PNG input review. No Unreal execution or Content writes."""
from pathlib import Path
import hashlib,json
from PIL import Image
HERE=Path(__file__).resolve().parent
BASE=HERE.parent/'panel-player/visible-entry/assets/components'
source=json.loads((BASE/'asset-manifest.json').read_text(encoding='utf-8-sig'))
names={
    'main-action-ornament-component-v1.png':'T_MainActionOrnament_v1',
    'hud-route-glyph-atlas-component-v1.png':'T_HudRouteGlyphAtlas_v1',
    'character-attribute-glyph-atlas-component-v1.png':'T_CharacterAttributeGlyphAtlas_v1',
    'inventory-category-glyph-atlas-component-v1.png':'T_InventoryCategoryGlyphAtlas_v1',
}
rows=[]
for row in source['assets']:
    path=BASE/row['path'];raw=path.read_bytes();digest=hashlib.sha256(raw).hexdigest()
    assert digest==row['sha256'].lower(),path
    with Image.open(path) as im:
        assert im.mode=='RGBA' and list(im.size)==row['dimensions']
        alpha=im.getchannel('A');assert alpha.getextrema()[0]==0 and alpha.getextrema()[1]>0
        histogram=alpha.histogram();fraction=histogram[0]/(im.width*im.height)
        assert fraction>.25,'Missing useful transparent component background'
        name=names[row['path']]
        rows.append(dict(source=str(path),source_sha256=digest,width=im.width,height=im.height,
            mode=im.mode,alpha_extrema=alpha.getextrema(),transparent_fraction=fraction,
            semantics=row['semantics'],cells=row.get('cells'),approved=False,
            package='/Game/newUI/Components/Player/'+name,name=name))
assert len(rows)==4
(HERE/'player-import-manifest.json').write_text(json.dumps(dict(
    scope='Four reviewed transparent components, no whole-page raster',assets=rows,
    imported=False,runtime_references_verified=False),ensure_ascii=False,indent=2),encoding='utf-8')
print('4 exact PNG inputs reviewed; import/runtime pending')
