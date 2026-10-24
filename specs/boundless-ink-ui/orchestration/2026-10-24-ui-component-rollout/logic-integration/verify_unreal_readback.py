"""Normal Python/Pillow verification of PNGs exported from the actual UE textures."""
import json
from pathlib import Path
from PIL import Image, ImageChops

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]


def main():
    batch=json.loads((HERE/'engine-batch.json').read_text(encoding='utf-8'))
    result=json.loads((HERE/'unreal-batch-result.json').read_text(encoding='utf-8'))
    assert not result.get('error'),result.get('error')
    assert result['wbp_migrated']
    assert len(result['assets'])==len(batch['assets'])
    measured=[]
    for row in batch['assets']:
        actual=next(v for v in result['assets'] if v['object_path']==row['object_path'])
        with Image.open(ROOT/row['source']) as source,Image.open(actual['exported_png']) as exported:
            assert source.size==exported.size==(row['width'],row['height'])
            assert 'A' in exported.getbands(),'Exported imported texture lost source alpha'
            alpha=exported.getchannel('A')
            assert ImageChops.difference(source.getchannel('A'),alpha).getbbox() is None,'Imported texture alpha differs'
            measured.append(dict(object_path=row['object_path'],size=exported.size,alpha_extrema=alpha.getextrema(),alpha_bbox=alpha.getbbox(),alpha_matches_source=True))
    result['import_verified']=True
    result['texture_source_alpha_readback']=measured
    # Still no claim about runtime reference, rendering, or player entry.
    (HERE/'unreal-batch-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Verified',len(measured),'actual UE Texture2D source exports; runtime/PIE still pending')


if __name__=='__main__': main()
