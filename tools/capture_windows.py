# SPDX-License-Identifier: GPL-3.0-or-later
"""Capture only a known RiffCalc process/window. Requires Pillow on Windows.
No desktop-wide capture or other-process input. Coordinates are client pixels.
See docs/screenshots/README.md for the reproducible documentation procedure.
"""
import argparse,ctypes,time
from ctypes import wintypes
from pathlib import Path
from PIL import ImageGrab
p=argparse.ArgumentParser();p.add_argument('--pid',type=int,required=True);p.add_argument('--title',default='RiffCalc 2000 Pro');p.add_argument('--out',type=Path,required=True);a=p.parse_args()
u=ctypes.windll.user32
u.GetWindowTextW.argtypes=[wintypes.HWND,wintypes.LPWSTR,ctypes.c_int]
# Resolve the actual Win32 API explicitly; do not match by title alone.
get_pid=u.GetWindowThreadProcessId
get_pid.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
u.GetWindowRect.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.RECT)]
u.IsWindowVisible.argtypes=[wintypes.HWND]
u.PostMessageW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM]
u.ShowWindow.argtypes=[wintypes.HWND,ctypes.c_int];u.SetForegroundWindow.argtypes=[wintypes.HWND]
windows=[]
@ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
def enum(hwnd,_):
 pid=wintypes.DWORD();get_pid(hwnd,ctypes.byref(pid))
 if pid.value==a.pid and u.IsWindowVisible(hwnd):
  text=ctypes.create_unicode_buffer(512);u.GetWindowTextW(hwnd,text,512);r=wintypes.RECT();u.GetWindowRect(hwnd,ctypes.byref(r));windows.append((hwnd,text.value,(r.left,r.top,r.right,r.bottom)))
 return True
u.EnumWindows(enum,0)
match=[x for x in windows if x[1]==a.title]
if len(match)!=1:raise SystemExit('Expected one matching window in the explicitly selected process')
hwnd=match[0][0];u.ShowWindow(hwnd,4);time.sleep(.3)
a.out.parent.mkdir(parents=True,exist_ok=True);ImageGrab.grab(window=hwnd).save(a.out);print(a.out)
