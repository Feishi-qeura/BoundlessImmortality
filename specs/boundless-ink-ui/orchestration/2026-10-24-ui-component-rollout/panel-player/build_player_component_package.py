from __future__ import annotations
import argparse, hashlib, json, sys, zipfile
from pathlib import Path
import yaml
from PIL import Image

FILES = [
    'home-avatar-frame-default-v1.png', 'home-avatar-portrait-default-v2.png',
    'home-identity-ink-default-v1.png', 'home-realm-plaque-default-v1.png',
    'home-health-fill-default-v1.png', 'home-bar-track-default-v1.png',
    'home-icon-sign-in-default-v1.png', 'home-icon-rank-default-v1.png',
    'home-icon-menu-default-v1.png', 'home-icon-journey-default-v1.png',
    'home-icon-cultivate-default-v1.png', 'home-icon-bag-default-v1.png',
    'home-icon-activity-default-v1.png', 'home-rank-paper-default-v1.png',
    'home-top-tile-default-v1.png', 'home-bottom-cloud-default-v1.png',
]

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--project-root', required=True, type=Path)
    args=ap.parse_args()
    root=args.project_root.resolve()
    team=(root/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/panel-player').resolve()
    assets=team/'assets'
    source_contract=yaml.safe_load((root/'specs/boundless-ink-ui/contracts/component-contract.yaml').read_text(encoding='utf-8'))
    source_by_file={Path(c.get('output','')).name:c for c in source_contract.get('components',[]) if c.get('output')}
    manifest=[]
    checks=[]
    for name in FILES:
        path=assets/name
        if not path.is_file(): raise SystemExit(f'missing asset: {path}')
        img=Image.open(path)
        img.load()
        w,h=img.size
        rgba=img.convert('RGBA')
        alpha=rgba.getchannel('A')
        bbox=alpha.getbbox()
        source=source_by_file.get(name)
        if not source: raise SystemExit(f'no upstream contract item for {name}')
        sl=source.get('slice', {'type':'full','margins':[0,0,0,0]})
        entry={
            'id':source.get('id'), 'file':f'assets/{name}', 'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
            'dimensions_px':[w,h], 'mode':img.mode, 'format':img.format,
            'has_alpha_channel':'A' in img.getbands(), 'alpha_bbox_px':list(bbox) if bbox else None,
            'alpha_extrema':list(alpha.getextrema()), 'slice':sl,
            'slice_status':'full-fixed-ratio-no-nine-slice' if sl.get('type')!='9-slice' else 'nine-slice',
            'states':source.get('states',['default']), 'source_status':source.get('status'),
            'source_review':source.get('review'), 'approved':source.get('status')=='approved' and source.get('review')=='user-approved',
            'source_contract_path':'specs/boundless-ink-ui/contracts/component-contract.yaml',
            'source_output':source.get('output'),
        }
        manifest.append(entry)
        checks.append({'file':f'assets/{name}','exists':True,'dimensions_px':[w,h],'mode':img.mode,'format':img.format,'alpha_bbox_px':entry['alpha_bbox_px'],'sha256':entry['sha256'],'slice':sl,'pass':entry['approved']})
    payload={'schema_version':1,'team_id':'panel-player','revision':'2026-10-24-player-component-rollout','approved_page_candidates':False,'asset_count':len(manifest),'assets':manifest}
    (team/'asset-manifest.json').write_text(json.dumps(payload,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    report={'checked_count':len(checks),'pass_count':sum(x['pass'] for x in checks),'fail_count':sum(not x['pass'] for x in checks),'nine_slice_count':sum(x['slice'].get('type')=='9-slice' for x in checks),'full_slice_count':sum(x['slice'].get('type')=='full' for x in checks),'checks':checks}
    (team/'pixel-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    package=team/'player-components-candidate.zip'
    with zipfile.ZipFile(package,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        z.write(team/'component-contract.yaml','component-contract.yaml')
        z.write(team/'page-component-map.md','page-component-map.md')
        z.write(team/'asset-manifest.json','asset-manifest.json')
        z.write(team/'pixel-validation.json','pixel-validation.json')
        for name in FILES: z.write(assets/name,f'assets/{name}')
    bad=[x for x in checks if not x['pass']]
    print(json.dumps({'manifest':str(team/'asset-manifest.json'),'validation':str(team/'pixel-validation.json'),'zip':str(package),'assets':len(manifest),'pass':len(manifest)-len(bad),'fail':len(bad),'nine_slice':report['nine_slice_count'],'full_slice':report['full_slice_count']},ensure_ascii=False))
    return 1 if bad else 0
if __name__=='__main__': raise SystemExit(main())
