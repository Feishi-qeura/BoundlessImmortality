"""Inspect alpha bands to define safe sprite UVs; never alter generated PNG pixels."""
import json
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parents[2]/'SourceAssets/UI/NiagaraRevision20261022'
meta=json.loads((root/'rank-theme-regions.json').read_text(encoding='utf-8'))
for theme in ('Sword','Speed','Damage'):
    key=theme+'Rows';texture='T_RankDamageRowsSpaced' if theme=='Damage' else 'T_Rank'+key
    meta[key]['texture']=texture;im=Image.open(root/(texture+'.png'));w,h=im.size;a=im.getchannel('A');pixels=a.load()
    active=[sum(pixels[x,y]>180 for x in range(w//3,2*w//3))>w*.22 for y in range(h)]
    bands=[];start=None
    for y,on in enumerate(active+[False]):
        if on and start is None:start=y
        elif not on and start is not None:
            if y-start>h*.025:bands.append((start,y))
            start=None
    assert len(bands)==10,(key,bands)
    splits=[0]
    for left,right in zip(bands,bands[1:]):
        threshold=200 if theme=='Damage' else 32
        values=[(sum(pixels[x,y]>threshold for x in range(w)),y) for y in range(left[1],right[0])]
        minimum=min(v for v,y in values);assert minimum==0,(key,left,right,minimum)
        clear=[y for v,y in values if v==0];splits.append(clear[len(clear)//2])
    splits.append(h);rects=[]
    for lo,hi in zip(splits,splits[1:]):
        box=a.crop((0,lo,w,hi)).point(lambda x:255 if x>32 else 0).getbbox();assert box
        rects.append([max(0,box[0]-2),max(lo,lo+box[1]-2),min(w,box[2]+2),min(hi,lo+box[3]+2)])
    meta[key]['regions']=rects;meta[key]['transparent_separators']=splits
    meta[key]['separator_alpha_threshold']=threshold
    print(key,'10 tiers, no opaque decoration crosses separators',splits)
(root/'rank-theme-regions.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
