from pathlib import Path
from PIL import Image
import hashlib, json
root = Path(r'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/panel-activities/batch-2/assets')
rows=[]
for p in sorted(root.glob('*.png')):
    image=Image.open(p).convert('RGBA')
    alpha=image.getchannel('A')
    hist=alpha.histogram()
    component=p.stem in {'launch-seal-neutral-v1','activity-detail-divider-v1','rank-position-frame-v1'}
    row={'path':p.as_posix(),'kind':'component-sprite' if component else 'page-concept','width':image.width,'height':image.height,'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'alpha_transparent_pixels':hist[0],'alpha_partial_pixels':sum(hist[1:255]),'alpha_opaque_pixels':hist[255],'transparent_component_alpha_present':bool(component and (hist[0] or sum(hist[1:255]))),'runtime_texture':False,'approved':False,'import_verified':False}
    rows.append(row)
    print(f"{p.name}\t{image.width}x{image.height}\ttransparent={hist[0]} partial={sum(hist[1:255])} opaque={hist[255]} sha256={row['sha256']}")
Path(root.parent/'assets-manifest.json').write_text(json.dumps({'approved':False,'assets':rows},indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
