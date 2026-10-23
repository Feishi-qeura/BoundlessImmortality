"""Create the authorized Windows ZIP and verify every extracted release file."""
from pathlib import Path
import hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[4]
STAGE=ROOT/'Saved/Packages/PortraitUI20261023/Windows'
ZIP=ROOT/'Saved/Packages/BoundlessImmortality_Windows_PortraitUI_20261023.zip'
EXTRACT=ROOT/'Saved/Packages/PortraitUI20261023-Extracted'
OUT=ROOT/'Saved/Codex/PortraitUI20261023/distribution.json'
assert STAGE.is_dir() and not ZIP.exists() and not EXTRACT.exists()
hashes=json.loads((STAGE/'SHA256.json').read_text('utf-8'))
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()
for name,expected in hashes.items():
 p=(STAGE/name).resolve();assert p.is_relative_to(STAGE.resolve()) and digest(p)==expected,name
with zipfile.ZipFile(ZIP,'w',zipfile.ZIP_DEFLATED,compresslevel=4,allowZip64=True) as z:
 for p in sorted(STAGE.rglob('*')):
  if p.is_file():z.write(p,'Windows/'+p.relative_to(STAGE).as_posix())
with zipfile.ZipFile(ZIP) as z:
 assert z.testzip() is None
 for name in z.namelist():assert (EXTRACT/name).resolve().is_relative_to(EXTRACT.resolve())
 z.extractall(EXTRACT)
for name,expected in hashes.items():assert digest(EXTRACT/'Windows'/name)==expected,name
assert digest(EXTRACT/'Windows/SHA256.json')==digest(STAGE/'SHA256.json')
sha=digest(ZIP)
ZIP.with_suffix('.zip.sha256.txt').write_text(sha+'  '+ZIP.name+'\n',encoding='utf-8')
report=dict(passed=True,zip=str(ZIP),sha256=sha,bytes=ZIP.stat().st_size,verified_files=len(hashes)+1,extracted_package=str(EXTRACT/'Windows'),crc_passed=True)
OUT.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(report,ensure_ascii=False))
