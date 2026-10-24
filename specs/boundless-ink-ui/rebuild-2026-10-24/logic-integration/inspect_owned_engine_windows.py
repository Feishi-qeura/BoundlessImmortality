"""List and optionally capture only the owned Editor process windows."""
import ctypes,json
from ctypes import wintypes as w
from pathlib import Path
from PIL import Image
u=ctypes.windll.user32;g=ctypes.windll.gdi32
u.GetWindowDC.argtypes=[w.HWND];u.GetWindowDC.restype=w.HDC
g.CreateCompatibleDC.argtypes=[w.HDC];g.CreateCompatibleDC.restype=w.HDC
g.CreateCompatibleBitmap.argtypes=[w.HDC,ctypes.c_int,ctypes.c_int];g.CreateCompatibleBitmap.restype=w.HBITMAP
g.SelectObject.argtypes=[w.HDC,w.HGDIOBJ];g.SelectObject.restype=w.HGDIOBJ
u.PrintWindow.argtypes=[w.HWND,w.HDC,w.UINT]
g.GetDIBits.argtypes=[w.HDC,w.HBITMAP,w.UINT,w.UINT,ctypes.c_void_p,ctypes.c_void_p,w.UINT]
g.DeleteObject.argtypes=[w.HGDIOBJ];g.DeleteDC.argtypes=[w.HDC];u.ReleaseDC.argtypes=[w.HWND,w.HDC]
HERE=Path(__file__).resolve().parent
rows=[]
@ctypes.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
def callback(hwnd,param):
    pid=w.DWORD();u.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
    if pid.value!=47604 or not u.IsWindowVisible(hwnd):return True
    buf=ctypes.create_unicode_buffer(1024);u.GetWindowTextW(hwnd,buf,1024)
    rect=w.RECT();u.GetWindowRect(hwnd,ctypes.byref(rect));width=rect.right-rect.left;height=rect.bottom-rect.top
    row={'hwnd':int(hwnd),'title':buf.value,'rect':[rect.left,rect.top,rect.right,rect.bottom]};rows.append(row)
    if width>0 and height>0:
        u.GetWindowDC.restype=w.HDC;g.CreateCompatibleDC.restype=w.HDC;g.CreateCompatibleBitmap.restype=w.HBITMAP;g.SelectObject.restype=w.HGDIOBJ
        dc=u.GetWindowDC(hwnd);mem=g.CreateCompatibleDC(dc);bitmap=g.CreateCompatibleBitmap(dc,width,height);old=g.SelectObject(mem,bitmap)
        ok=u.PrintWindow(hwnd,mem,2)
        class Header(ctypes.Structure):_fields_=[('size',w.DWORD),('width',w.LONG),('height',w.LONG),('planes',w.WORD),('bitcount',w.WORD),('compression',w.DWORD),('sizeimage',w.DWORD),('x',w.LONG),('y',w.LONG),('used',w.DWORD),('important',w.DWORD)]
        head=Header(ctypes.sizeof(Header),width,-height,1,32,0,0,0,0,0,0);data=ctypes.create_string_buffer(width*height*4)
        if ok and g.GetDIBits(mem,bitmap,0,height,data,ctypes.byref(head),0):
            path=HERE/f'owned-editor-window-{int(hwnd)}.png';Image.frombuffer('RGB',(width,height),data,'raw','BGRX',0,1).save(path);row['image']=str(path)
        g.SelectObject(mem,old);g.DeleteObject(bitmap);g.DeleteDC(mem);u.ReleaseDC(hwnd,dc)
    return True
u.EnumWindows(callback,0)
(HERE/'owned-editor-windows.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(rows,ensure_ascii=False))
