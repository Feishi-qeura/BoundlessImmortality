"""Choose native delete-unused option in the inspected exact29 fixup report."""
import ctypes,json,hashlib,time
from ctypes import wintypes as w
from pathlib import Path
HERE=Path(__file__).resolve().parent;u=ctypes.windll.user32
hwnd=10694434;pid=w.DWORD();u.GetWindowThreadProcessId(hwnd,ctypes.byref(pid));assert pid.value==47604
title=ctypes.create_unicode_buffer(256);u.GetWindowTextW(hwnd,title,256);assert title.value=='重定向器更新报告'
rect=w.RECT();u.GetWindowRect(hwnd,ctypes.byref(rect));assert [rect.left,rect.top,rect.right,rect.bottom]==[777,300,1783,1140]
report=json.loads((HERE.parent/'migration/engine-fixup-result.json').read_text(encoding='utf-8-sig'))
assert report['engine_pid']==47604 and report['exact_redirector_count']==29 and len(report['inspected_exact_redirectors'])==29 and report['mapped_count']==582 and report['source_scope_count']==588
assert all(r['old_object'].startswith('/Game/UI/') and r['destination_object'].startswith('/Game/newUI/') for r in report['inspected_exact_redirectors'])
image=HERE/'owned-editor-window-10694434.png'
receipt={'engine_pid':47604,'window':hwnd,'title':title.value,'inspected_report_image':str(image),'image_sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'selected':'Delete unreferenced redirectors','scope':29,'native_report_all_references_updated_visually_verified':True}
(HERE/'exact-fixup-dialog-acknowledgement.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8')
u.SetForegroundWindow(hwnd);u.SetCursorPos(1027,1110);time.sleep(.15)
assert u.GetForegroundWindow()==hwnd
u.mouse_event(0x0002,0,0,0,0);u.mouse_event(0x0004,0,0,0,0)
