from pathlib import Path
import hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Saved/Codex/VisualRefactor20261022'
def main():
    OUT.mkdir(parents=True,exist_ok=True);manifest=OUT/'baseline.json'
    if manifest.exists():raise RuntimeError('Baseline already exists; do not overwrite')
    paths={r['path'] for r in json.loads((ROOT/'Saved/Codex/GameplayPlan20261021/preservation-baseline.json').read_text(encoding='utf-8'))}
    for folder in ('Source','Config','Content/UI/Ink'):
        paths.update(p.relative_to(ROOT).as_posix() for p in (ROOT/folder).rglob('*') if p.is_file())
    records=[]
    for name in sorted(paths):
        path=ROOT/name
        if not path.is_file():continue
        dest=OUT/'Backup'/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest)
        records.append(dict(path=name,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),size=path.stat().st_size))
    manifest.write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8');print(f'Protected {len(records)} files, {sum(r["size"] for r in records)} bytes')
if __name__=='__main__':main()
