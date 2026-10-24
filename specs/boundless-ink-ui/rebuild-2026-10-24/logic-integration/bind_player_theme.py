from pathlib import Path
import re, runpy
ROOT=Path(__file__).resolve().parents[4]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'))
for name in ('InkPlayerMenuPageWidgets','InkPlayerVisibleEntryWidgets','InkPlayerCharacterInventoryWidgets'):
    relative=f'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Player/{name}.cpp'
    path=ROOT/relative
    raw=path.read_bytes()
    text=raw.decode('utf-8-sig')
    assert '#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"' not in text
    text=text.replace(f'#include "{name}.h"',f'#include "{name}.h"\n#include "Frontend/UIRebuild20261024/InkRebuildTheme.h"')
    text,count=re.subn(r'FSlateFontInfo Font;\s*Font.Size = (\w+);',r'const FSlateFontInfo Font = InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, \1);',text)
    assert count==1,(name,count)
    text=text.replace('Brush.DrawAs = ESlateBrushDrawType::Box;','Brush.DrawAs = ESlateBrushDrawType::RoundedBox;')
    rec['begin'](relative,'Bind reviewed Player helpers to shared Chinese font/DPI contract and native brush rendering')
    assert path.read_bytes()==raw
    path.write_bytes(text.encode('utf-8'))
    rec['finish'](relative)
print('Three Player helpers bound to shared typography; asset loading/runtime pending')
