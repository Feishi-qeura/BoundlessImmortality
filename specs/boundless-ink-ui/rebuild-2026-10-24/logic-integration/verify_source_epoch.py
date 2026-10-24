"""Source/hash/backup review only; never claims compiler/UMG/runtime acceptance."""
from pathlib import Path
import ast,hashlib,json,re
from datetime import datetime
from zoneinfo import ZoneInfo
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
LEDGER_ROOT=ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
ledger=json.loads((LEDGER_ROOT/'source-change-ledger.json').read_text())
terminal={};errors=[];files=[]
for row in ledger['changes']:
    assert row['state']=='recorded',row
    folder=LEDGER_ROOT/row['backup']
    if row['before_exists']:assert sha(folder/'before')==row['before_sha256']
    assert sha(folder/'after')==row['after_sha256']
    assert sha(folder/'grant.json')==row['grant_sha256']
    if row['path'] in terminal:assert terminal[row['path']]==row['before_sha256'],'Exact source hash chain broke'
    terminal[row['path']]=row['after_sha256']
for name,expected in terminal.items():
    path=ROOT/name;assert sha(path)==expected,'Unexpected concurrent Source write: '+name
    text=path.read_text(encoding='utf-8-sig');clean=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '',text)
    stack=[]
    for char in clean:
        if char in '([{':stack.append(char)
        elif char in ')]}':
            if not stack or stack.pop()!={')':'(',']':'[','}':'{'}[char]:errors.append(name+': unmatched delimiter');break
    if stack:errors.append(name+': unclosed delimiter')
    if path.suffix=='.h' and 'UCLASS(' in text:
        includes=re.findall(r'^#include\s+"([^"\n]+)"',text,re.M)
        if not includes[-1].endswith('.generated.h'):errors.append(name+': generated include is not last')
    files.append(dict(path=name,sha256=expected))
for path in HERE.glob('*.py'):ast.parse(path.read_text(encoding='utf-8'),filename=str(path))
report=dict(checked_at=datetime.now(ZoneInfo('Asia/Hong_Kong')).isoformat(),scope='static source/backup/hash/Python parsing only',files=files,errors=errors,compiled=False,runtime_verified=False)
(HERE/'source-epoch.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(source_files=len(files),ledger_records=len(ledger['changes']),errors=errors),ensure_ascii=False))
raise SystemExit(bool(errors))
