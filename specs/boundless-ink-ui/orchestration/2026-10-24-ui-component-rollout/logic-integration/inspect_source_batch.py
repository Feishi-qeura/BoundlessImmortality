"""Static inspection only. Never presents lexical checks as C++/UHT compilation."""
import ast
import hashlib
import json
from pathlib import Path
import re
import runpy
import sys
from unittest.mock import patch

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[4]


def main():
    batch=json.loads((HERE/'engine-batch.json').read_text(encoding='utf-8'))
    files={v['path'] for v in batch['new_source']}|{v['path'] for v in batch['public_source'] if v['changed']}
    errors=[]
    for name in sorted(files):
        path=ROOT/name
        text=path.read_text(encoding='utf-8-sig')
        clean=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', '',text)
        stack=[]
        for char in clean:
            if char in '([{': stack.append(char)
            elif char in ')]}':
                if not stack or stack.pop()!={')':'(',']':'[','}':'{'}[char]:
                    errors.append(name+': unmatched delimiter');break
        if stack: errors.append(name+': unclosed delimiter')
        if path.suffix=='.h' and 'UCLASS(' in text:
            includes=re.findall(r'^#include\s+"([^"\n]+)"',text,re.M)
            if not includes or not includes[-1].endswith('.generated.h'): errors.append(name+': generated include is not last')
    scripts=[]
    for path in HERE.glob('*.py'):
        ast.parse(path.read_text(encoding='utf-8'),filename=str(path));scripts.append(path.name)
    lease_guard=False
    try:
        # Never execute importer main during a real granted lease. Test only the
        # isolated grant predicate with a simulated denied lease.
        namespace=runpy.run_path(str(HERE/'run_unreal_batch.py'),run_name='isolated_lease_guard')
        with patch.object(Path,'read_text',return_value='{"state":"not_granted","owner_team":"logic-integration"}'):
            namespace['require_engine_grant']()
        errors.append('Unreal batch unexpectedly passed absent lease')
    except RuntimeError as exc:
        lease_guard='Engine lease not granted' in str(exc)
        if not lease_guard: raise
    assert 'unreal' not in sys.modules,'Unexpected Unreal import during no-lease guard check'
    # Actual runtime consumer literals must be present in the integrated source.
    all_text='\n'.join((ROOT/n).read_text(encoding='utf-8-sig') for n in files)
    for row in batch['assets']:
        if row['object_path'] not in all_text:
            # Journey widgets use a family path + texture-name mapping.
            if row['object_path'].split('.')[-1] not in all_text: errors.append('No source consumer for '+row['object_path'])
    report=dict(scope='static lexical/source-reference/Python parsing checks; not C++/UHT/UE execution',
                inspected_source_files=len(files),parsed_scripts=scripts,lease_guard_refused_before_unreal_import=lease_guard,
                errors=errors,engine_executed=False,compiled=False)
    (HERE/'source-inspection.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))
    return 1 if errors else 0


if __name__=='__main__': raise SystemExit(main())
