"""Sampling profiler for a running DayZServer_x64.exe.

Why this exists: the server's own FPS counter says nothing about where the time
goes, and the symptom under investigation (other players freezing while your
own client is smooth) points at a thread that the frame counter does not watch.
This suspends each thread briefly, reads RIP, and buckets it by the function
ranges in the executable's .pdata -- so the answer is "the server was inside
THIS function N% of the time", per thread, with no guessing.

Usage:
    python profile-live.py --seconds 30 --hz 200
    python profile-live.py --pid 1234 --seconds 60 --hz 200 --out profile.txt

It perturbs the process a little (each sample suspends one thread for a few
microseconds). At 200 Hz over a handful of threads that is well under a percent
of wall clock, but do not run it at 5000 Hz on a live server.
"""

import argparse
import bisect
import collections
import ctypes
import ctypes.wintypes as w
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dayz_image as dz

k32 = ctypes.WinDLL("kernel32", use_last_error=True)

TH32CS_SNAPTHREAD = 0x00000004
TH32CS_SNAPMODULE = 0x00000008
THREAD_SUSPEND_RESUME = 0x0002
THREAD_GET_CONTEXT = 0x0008
THREAD_QUERY_INFORMATION = 0x0040
CONTEXT_AMD64 = 0x00100000
CONTEXT_CONTROL = CONTEXT_AMD64 | 0x1
CONTEXT_SIZE = 1232
RIP_OFFSET = 0xF8


class THREADENTRY32(ctypes.Structure):
    _fields_ = [
        ("dwSize", w.DWORD),
        ("cntUsage", w.DWORD),
        ("th32ThreadID", w.DWORD),
        ("th32OwnerProcessID", w.DWORD),
        ("tpBasePri", ctypes.c_long),
        ("tpDeltaPri", ctypes.c_long),
        ("dwFlags", w.DWORD),
    ]


class MODULEENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", w.DWORD),
        ("th32ModuleID", w.DWORD),
        ("th32ProcessID", w.DWORD),
        ("GlblcntUsage", w.DWORD),
        ("ProccntUsage", w.DWORD),
        ("modBaseAddr", ctypes.POINTER(ctypes.c_byte)),
        ("modBaseSize", w.DWORD),
        ("hModule", w.HMODULE),
        ("szModule", ctypes.c_wchar * 256),
        ("szExePath", ctypes.c_wchar * 260),
    ]


def find_process(name):
    import subprocess

    out = subprocess.run(
        [
            "powershell",
            "-NoProfile",
            "-Command",
            "(Get-CimInstance Win32_Process -Filter \"Name='%s'\").ProcessId" % name,
        ],
        capture_output=True,
        text=True,
    )
    ids = [int(x) for x in out.stdout.split() if x.strip().isdigit()]
    return ids


def modules_of(pid):
    """[(name, base, size, path)] for the process."""
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid)
    if snap == -1:
        raise OSError("module snapshot failed: %d" % ctypes.get_last_error())
    out = []
    me = MODULEENTRY32W()
    me.dwSize = ctypes.sizeof(MODULEENTRY32W)
    ok = k32.Module32FirstW(snap, ctypes.byref(me))
    while ok:
        out.append(
            (
                me.szModule,
                ctypes.cast(me.modBaseAddr, ctypes.c_void_p).value,
                me.modBaseSize,
                me.szExePath,
            )
        )
        ok = k32.Module32NextW(snap, ctypes.byref(me))
    k32.CloseHandle(snap)
    return out


def threads_of(pid):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    if snap == -1:
        raise OSError("thread snapshot failed")
    out = []
    te = THREADENTRY32()
    te.dwSize = ctypes.sizeof(THREADENTRY32)
    ok = k32.Thread32First(snap, ctypes.byref(te))
    while ok:
        if te.th32OwnerProcessID == pid:
            out.append(te.th32ThreadID)
        ok = k32.Thread32Next(snap, ctypes.byref(te))
    k32.CloseHandle(snap)
    return out


class Sampler:
    def __init__(self, pid):
        self.pid = pid
        self.handles = {}
        # CONTEXT must be 16-byte aligned.
        self.raw = ctypes.create_string_buffer(CONTEXT_SIZE + 16)
        addr = ctypes.addressof(self.raw)
        self.ctx = addr + ((16 - (addr % 16)) % 16)

    def handle(self, tid):
        h = self.handles.get(tid)
        if h is None:
            h = k32.OpenThread(
                THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                False,
                tid,
            )
            self.handles[tid] = h or 0
        return self.handles[tid]

    def rip(self, tid):
        h = self.handle(tid)
        if not h:
            return None
        if k32.SuspendThread(h) == 0xFFFFFFFF:
            return None
        try:
            ctypes.memset(self.ctx, 0, CONTEXT_SIZE)
            ctypes.c_uint32.from_address(self.ctx + 0x30).value = CONTEXT_CONTROL
            if not k32.GetThreadContext(h, ctypes.c_void_p(self.ctx)):
                return None
            return ctypes.c_uint64.from_address(self.ctx + RIP_OFFSET).value
        finally:
            k32.ResumeThread(h)

    def close(self):
        for h in self.handles.values():
            if h:
                k32.CloseHandle(h)


# Functions named by hand from the disassembly pass, so the report reads as
# behaviour rather than as addresses.
KNOWN = {
    0x502D10: "freq lookup leaf (PATCHED BY hid.dll)",
    0x502EA0: "VON: remove turned-off transmitter (loops ALL players)",
    0x502F70: "VON: EnableBroadcast(bool)",
    0x503070: "VON: update one transmitter against ALL players",
    0x873150: "VON: for each transmitter -> update against all players",
    0x872D50: "VON: transmitter pass",
    0x8723F0: "VON: system update",
    0x9D20D0: "VON: per-frame update(dt)",
    0x9A4570: "VON map: Add(freq)",
    0x9A4FB0: "VON map: Contains(freq)",
    0x9A69A0: "VON map: Remove(freq)",
    0x9AA800: "VON: reconcile listener set",
    0x6646F0: "VON: listener owner",
    0x5685C0: "SetFrequencyByIndex",
    0x568600: "SetNextChannel",
    0x568660: "SetPrevChannel",
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pid", type=int, default=0)
    ap.add_argument("--exe", default=r"E:\dayzmod\dayzserver-retail\DayZServer_x64.exe")
    ap.add_argument("--seconds", type=float, default=30.0)
    ap.add_argument("--hz", type=float, default=200.0)
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--out", default="")
    args = ap.parse_args()

    pid = args.pid
    if not pid:
        ids = find_process(os.path.basename(args.exe))
        if not ids:
            print("no %s running" % os.path.basename(args.exe))
            return 1
        pid = ids[0]

    mods = modules_of(pid)
    main_mod = None
    for name, base, size, path in mods:
        if name.lower() == os.path.basename(args.exe).lower():
            main_mod = (name, base, size, path)
    if not main_mod:
        print("could not find the executable's own module in pid %d" % pid)
        return 1

    exe_base = main_mod[1]
    print("pid %d, %s at 0x%X (%d modules)" % (pid, main_mod[0], exe_base, len(mods)))

    img = dz.load(args.exe)
    pd = dz.Pdata(img)
    starts = [e[0] for e in pd.entries]
    print("function ranges from .pdata: %d" % len(pd))

    # Other modules, so time spent in hid.dll / ntdll / winsock is attributed.
    ranges = sorted((b, b + s, n) for n, b, s, p in mods)
    rstarts = [r[0] for r in ranges]

    sampler = Sampler(pid)
    tids = threads_of(pid)
    print("threads: %d" % len(tids))

    period = 1.0 / args.hz
    per_thread = collections.Counter()
    buckets = collections.defaultdict(collections.Counter)
    total = 0
    failed = 0
    refresh = 0.0

    t_end = time.time() + args.seconds
    while time.time() < t_end:
        loop_start = time.time()
        if loop_start - refresh > 2.0:
            tids = threads_of(pid)
            refresh = loop_start
        for tid in tids:
            r = sampler.rip(tid)
            total += 1
            if r is None:
                failed += 1
                continue
            i = bisect.bisect_right(rstarts, r) - 1
            modname = "?"
            if i >= 0 and ranges[i][0] <= r < ranges[i][1]:
                modname = ranges[i][2]
            if modname.lower() == main_mod[0].lower():
                rva = r - exe_base
                j = bisect.bisect_right(starts, rva) - 1
                key = "unknown +0x%X" % rva
                if j >= 0:
                    b, e, _ = pd.entries[j]
                    if b <= rva < e:
                        key = "+0x%X" % b
                        if b in KNOWN:
                            key += "  " + KNOWN[b]
                buckets[tid][key] += 1
            else:
                buckets[tid]["[module] " + modname] += 1
            per_thread[tid] += 1
        slept = time.time() - loop_start
        if slept < period:
            time.sleep(period - slept)

    sampler.close()

    lines = []
    lines.append("samples: %d (failed %d) over %.1fs at %.0f Hz" % (total, failed, args.seconds, args.hz))
    lines.append("")
    hot = sorted(per_thread.items(), key=lambda kv: -kv[1])
    for tid, n in hot:
        busy = buckets[tid]
        idle = sum(v for k, v in busy.items() if k.startswith("[module] ntdll") or k.startswith("[module] KERNEL"))
        lines.append("thread %d: %d samples, %.0f%% inside the exe" % (tid, n, 100.0 * (n - idle) / max(1, n)))
        for key, cnt in busy.most_common(args.top):
            lines.append("    %6.2f%%  %5d  %s" % (100.0 * cnt / max(1, n), cnt, key))
        lines.append("")

    text = "\n".join(lines)
    print(text)
    if args.out:
        with open(args.out, "w", encoding="utf-8") as fh:
            fh.write(text)
        print("written to %s" % args.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
