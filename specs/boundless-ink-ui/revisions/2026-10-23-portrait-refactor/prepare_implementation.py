from pathlib import Path
import json,hashlib,shutil
ROOT=Path(__file__).resolve().parents[4]
OUT=ROOT/'Saved/Codex/PortraitUI20261023'
BACKUP=OUT/'Backup'
BACKUP.mkdir(parents=True,exist_ok=True)
manifest=OUT/'backup-manifest.json'
if manifest.exists():
    print('Existing backup retained',manifest)
    raise SystemExit(0)
audit=json.loads((ROOT/'specs/boundless-ink-ui/revisions/2026-10-23-portrait-refactor/registry-audit.json').read_text('utf-8'))
files=set()
for folder in ['Content/UI','Source','Config','Tools']:
    for p in (ROOT/folder).rglob('*'):
        if p.is_file() and not any(x in p.parts for x in ['__pycache__','Intermediate','Binaries']):files.add(p)
for a in audit['assets']:
    for ref in a['referencers']:
        if ref.startswith('/Game/'):
            for suffix in ['.uasset','.umap']:
                p=ROOT/'Content'/(ref[6:]+suffix)
                if p.is_file():files.add(p)
files.add(ROOT/'BoundlessImmortality.uproject')
records=[]
for p in sorted(files):
    rel=p.relative_to(ROOT);dst=BACKUP/rel;dst.parent.mkdir(parents=True,exist_ok=True)
    before=hashlib.sha256(p.read_bytes()).hexdigest();shutil.copy2(p,dst)
    assert hashlib.sha256(dst.read_bytes()).hexdigest()==before
    records.append(dict(path=rel.as_posix(),sha256=before,size=p.stat().st_size))
manifest.write_text(json.dumps(dict(files=records,verified=True),ensure_ascii=False,indent=2),encoding='utf-8')
print('Verified backup files',len(records),'bytes',sum(r['size'] for r in records))
