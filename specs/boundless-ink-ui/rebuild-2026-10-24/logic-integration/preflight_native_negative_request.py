"""Reflection API preflight only; constructs data, executes no gameplay action."""
from pathlib import Path
import json,unreal
H=Path(__file__).resolve().parent;R=dict(executed_business_action=False,object_call_method_available=hasattr(unreal.Object,'call_method'),attempts=[])
try:
 r=unreal.RebuildActivityActionRequest(action='purchase',entry_id='SKU_BREAK',version='1',price_text='10 灵石',b_confirmed=True)
 R['attempts'].append(dict(method='constructor',success=True,value=str(r)))
except Exception as e:R['attempts'].append(dict(method='constructor',success=False,error=str(e)))
try:
 r=unreal.RebuildActivityActionRequest()
 for k,v in dict(Action='purchase',EntryId='SKU_BREAK',Version='1',PriceText='10 灵石',bConfirmed=True).items():r.set_editor_property(k,v)
 R['attempts'].append(dict(method='editor_native_names',success=True,value=str(r)))
except Exception as e:R['attempts'].append(dict(method='editor_native_names',success=False,error=str(e)))
R['passed']=R['object_call_method_available'] and any(x['success'] for x in R['attempts'])
(H/'native-negative-request-preflight.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),encoding='utf-8')
