"""Capture exception addresses from an owned patch process; no user process access."""
import ctypes as C, pathlib, sys, struct, json
from ctypes import wintypes as W
K=C.WinDLL('kernel32',use_last_error=True)
class Startup(C.Structure):
    _fields_=[('cb',W.DWORD),('reserved',W.LPWSTR),('desktop',W.LPWSTR),('title',W.LPWSTR),('x',W.DWORD),('y',W.DWORD),('xs',W.DWORD),('ys',W.DWORD),('xc',W.DWORD),('yc',W.DWORD),('fill',W.DWORD),('flags',W.DWORD),('show',W.WORD),('reserved2',W.WORD),('ptr',C.c_void_p),('in_',W.HANDLE),('out',W.HANDLE),('err',W.HANDLE)]
class Process(C.Structure):_fields_=[('process',W.HANDLE),('thread',W.HANDLE),('pid',W.DWORD),('tid',W.DWORD)]
class Event(C.Structure):_fields_=[('code',W.DWORD),('pid',W.DWORD),('tid',W.DWORD),('align',W.DWORD),('data',C.c_ubyte*160)]
K.CreateProcessW.argtypes=[W.LPCWSTR,W.LPWSTR,C.c_void_p,C.c_void_p,W.BOOL,W.DWORD,C.c_void_p,W.LPCWSTR,C.POINTER(Startup),C.POINTER(Process)]
K.WaitForDebugEvent.argtypes=[C.POINTER(Event),W.DWORD]
K.ContinueDebugEvent.argtypes=[W.DWORD,W.DWORD,W.DWORD]
K.ReadProcessMemory.argtypes=[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.POINTER(C.c_size_t)]
K.Wow64GetThreadContext.argtypes=[W.HANDLE,C.c_void_p]
K.OpenThread.argtypes=[W.DWORD,W.BOOL,W.DWORD];K.OpenThread.restype=W.HANDLE
K.TerminateProcess.argtypes=[W.HANDLE,W.UINT]
K.CloseHandle.argtypes=[W.HANDLE]
sys.stdout.reconfigure(encoding='utf-8')
exe=pathlib.Path(sys.argv[1]).resolve();p=Process();s=Startup();s.cb=C.sizeof(s);s.flags=1;s.show=0
if not K.CreateProcessW(str(exe),C.create_unicode_buffer('"'+str(exe)+'" --cn-probe'),None,None,False,2|0x08000000,None,str(pathlib.Path(__file__).parent),C.byref(s),C.byref(p)):raise C.WinError(C.get_last_error())
exceptions=[];base=None
try:
    for _ in range(250):
        e=Event()
        if not K.WaitForDebugEvent(C.byref(e),10000):raise TimeoutError('debugger event wait')
        disposition=0x10002
        if e.code==3:base=struct.unpack_from('<Q',bytes(e.data),24)[0];print('base',hex(base),flush=True)
        if e.code==1:
            code=struct.unpack_from('<I',bytes(e.data),0)[0];address=struct.unpack_from('<Q',bytes(e.data),16)[0];first=struct.unpack_from('<I',bytes(e.data),152)[0]
            if code!=0x80000003:
                th=K.OpenThread(0x0008|0x0040,False,e.tid);ctx=C.create_string_buffer(716);struct.pack_into('<I',ctx,0,0x10007);K.Wow64GetThreadContext(th,ctx);K.CloseHandle(th)
                esp=struct.unpack_from('<I',ctx,196)[0];buf=C.create_string_buffer(64);got=C.c_size_t();K.ReadProcessMemory(p.process,address,buf,64,C.byref(got))
                stack=C.create_string_buffer(64);K.ReadProcessMemory(p.process,esp,stack,64,C.byref(got))
                row={'code':hex(code),'address':hex(address),'rva':hex(address-base) if base else None,'first':first,'registers':{n:hex(struct.unpack_from('<I',ctx,o)[0]) for n,o in [('eax',176),('ebx',164),('ecx',172),('edx',168),('esi',160),('edi',156),('ebp',180),('eip',184),('esp',196)]},'bytes':buf.raw.hex(),'stack':stack.raw.hex(),'params':list(struct.unpack_from('<15Q',bytes(e.data),32))}
                exceptions.append(row);print(json.dumps(row),flush=True);disposition=0x80010001
                if not first:K.TerminateProcess(p.process,1)
        if e.code==5:print('exit',struct.unpack_from('<I',bytes(e.data))[0],flush=True);break
        K.ContinueDebugEvent(e.pid,e.tid,disposition)
finally:
    K.TerminateProcess(p.process,99);K.CloseHandle(p.thread);K.CloseHandle(p.process)
(pathlib.Path(__file__).parent/'debug-probe.json').write_text(json.dumps(exceptions,indent=2),encoding='utf-8')
