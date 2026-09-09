"""Minimal PE/x64 analysis toolkit for DayZ engine binaries.

No IDA on this box; pefile + capstone are installed. Everything here works on
the file image (raw), converting to RVA/VA when it needs to talk about addresses.
"""

import bisect
import struct
import sys

import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OPT_SYNTAX_INTEL


class Image:
    def __init__(self, path):
        self.path = path
        self.pe = pefile.PE(path, fast_load=True)
        with open(path, "rb") as fh:
            self.data = fh.read()
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.secs = []
        for s in self.pe.sections:
            name = s.Name.rstrip(b"\x00").decode("latin1")
            self.secs.append(
                {
                    "name": name,
                    "rva": s.VirtualAddress,
                    "vsize": s.Misc_VirtualSize,
                    "off": s.PointerToRawData,
                    "rsize": s.SizeOfRawData,
                    "exec": bool(s.Characteristics & 0x20000000),
                    "write": bool(s.Characteristics & 0x80000000),
                }
            )
        self.md = Cs(CS_ARCH_X86, CS_MODE_64)
        self.md.detail = True
        self.md.syntax = CS_OPT_SYNTAX_INTEL

    # ---- address conversion -------------------------------------------------
    def rva2off(self, rva):
        for s in self.secs:
            if s["rva"] <= rva < s["rva"] + max(s["vsize"], s["rsize"]):
                d = rva - s["rva"]
                if d < s["rsize"]:
                    return s["off"] + d
                return None
        return None

    def off2rva(self, off):
        for s in self.secs:
            if s["off"] <= off < s["off"] + s["rsize"]:
                return s["rva"] + (off - s["off"])
        return None

    def va(self, rva):
        return self.base + rva

    def rva_of_va(self, va):
        return va - self.base

    def sec_of_rva(self, rva):
        for s in self.secs:
            if s["rva"] <= rva < s["rva"] + max(s["vsize"], s["rsize"]):
                return s
        return None

    # ---- reading ------------------------------------------------------------
    def read(self, rva, n):
        off = self.rva2off(rva)
        if off is None:
            return None
        return self.data[off : off + n]

    def u32(self, rva):
        b = self.read(rva, 4)
        return None if b is None or len(b) < 4 else struct.unpack("<I", b)[0]

    def u64(self, rva):
        b = self.read(rva, 8)
        return None if b is None or len(b) < 8 else struct.unpack("<Q", b)[0]

    def f32(self, rva):
        b = self.read(rva, 4)
        return None if b is None or len(b) < 4 else struct.unpack("<f", b)[0]

    # ---- searching ----------------------------------------------------------
    def find_pattern(self, pattern, mask, exec_only=True):
        """pattern: bytes, mask: str of 'x' / '?'. Returns list of RVAs."""
        hits = []
        n = len(pattern)
        for s in self.secs:
            if exec_only and not s["exec"]:
                continue
            if s["rsize"] < n:
                continue
            blob = self.data[s["off"] : s["off"] + s["rsize"]]
            # anchor on the first fixed byte for speed
            anchor = None
            for i, m in enumerate(mask):
                if m == "x":
                    anchor = i
                    break
            start = 0
            while True:
                idx = blob.find(pattern[anchor : anchor + 1], start)
                if idx < 0:
                    break
                off = idx - anchor
                start = idx + 1
                if off < 0 or off + n > len(blob):
                    continue
                ok = True
                for i in range(n):
                    if mask[i] == "x" and blob[off + i] != pattern[i]:
                        ok = False
                        break
                if ok:
                    hits.append(s["rva"] + off)
        return hits

    def find_bytes(self, needle, exec_only=False):
        hits = []
        for s in self.secs:
            if exec_only and not s["exec"]:
                continue
            blob = self.data[s["off"] : s["off"] + s["rsize"]]
            start = 0
            while True:
                idx = blob.find(needle, start)
                if idx < 0:
                    break
                hits.append(s["rva"] + idx)
                start = idx + 1
        return hits

    # ---- xrefs --------------------------------------------------------------
    def call_xrefs(self, target_rva):
        """Direct E8 rel32 calls and E9 rel32 jmps landing on target_rva."""
        out = []
        for s in self.secs:
            if not s["exec"]:
                continue
            blob = self.data[s["off"] : s["off"] + s["rsize"]]
            for opc, kind in ((0xE8, "call"), (0xE9, "jmp")):
                start = 0
                b = bytes([opc])
                while True:
                    idx = blob.find(b, start)
                    if idx < 0 or idx + 5 > len(blob):
                        break
                    start = idx + 1
                    rel = struct.unpack("<i", blob[idx + 1 : idx + 5])[0]
                    site = s["rva"] + idx
                    if site + 5 + rel == target_rva:
                        out.append((site, kind))
        return sorted(set(out))

    def data_xrefs(self, target_va):
        """Absolute 8-byte pointers to target_va anywhere in the image (vtables)."""
        needle = struct.pack("<Q", target_va)
        return self.find_bytes(needle)

    def lea_xrefs(self, target_rva):
        """RIP-relative references (lea/mov) whose effective address is target_rva.

        Brute force: for every executable byte, try the common encodings.
        Cheap enough: we scan for the 4-byte displacement value instead.
        """
        out = []
        for s in self.secs:
            if not s["exec"]:
                continue
            blob = self.data[s["off"] : s["off"] + s["rsize"]]
            # displacement is relative to end of instruction; scan all positions
            for i in range(len(blob) - 4):
                disp = struct.unpack("<i", blob[i : i + 4])[0]
                # instruction end could be i+4 .. i+8 (immediate follows)
                end = s["rva"] + i + 4
                if end + disp == target_rva:
                    out.append(s["rva"] + i - 3)  # rough: start of lea
        return out

    # ---- disassembly --------------------------------------------------------
    def disas(self, rva, count=40, stop_at_ret=False):
        off = self.rva2off(rva)
        if off is None:
            return []
        blob = self.data[off : off + count * 15 + 32]
        out = []
        for ins in self.md.disasm(blob, self.va(rva)):
            out.append(ins)
            if len(out) >= count:
                break
            if stop_at_ret and ins.mnemonic in ("ret", "jmp"):
                break
        return out

    def show(self, rva, count=40, stop_at_ret=False, label=""):
        if label:
            print("--- %s @ +0x%X (VA 0x%X)" % (label, rva, self.va(rva)))
        for ins in self.disas(rva, count, stop_at_ret):
            print(
                "  %012X  %-24s %s %s"
                % (
                    ins.address,
                    ins.bytes.hex(),
                    ins.mnemonic,
                    ins.op_str,
                )
            )

    # ---- function start heuristic ------------------------------------------
    def func_start(self, rva, max_back=0x900):
        """Walk back to a plausible prologue / after a previous ret+int3 pad."""
        off = self.rva2off(rva)
        if off is None:
            return None
        lo = max(0, off - max_back)
        window = self.data[lo:off]
        # look for the last  cc cc  (int3 padding) or  c3 cc  before us
        best = None
        for pat in (b"\xcc\xcc", b"\xc3\xcc", b"\xc2\xcc"):
            i = window.rfind(pat)
            if i >= 0:
                cand = lo + i + 2
                while cand < off and self.data[cand] == 0xCC:
                    cand += 1
                if best is None or cand > best:
                    best = cand
        if best is None:
            return None
        return self.off2rva(best)


# ---- pdata (exception directory) gives real function bounds ---------------
class Pdata:
    def __init__(self, img):
        self.img = img
        self.entries = []
        d = img.pe.OPTIONAL_HEADER.DATA_ENTRY if False else None
        de = img.pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]  # EXCEPTION
        rva, size = de.VirtualAddress, de.Size
        off = img.rva2off(rva)
        if off is None or size == 0:
            return
        blob = img.data[off : off + size]
        for i in range(0, len(blob) - 11, 12):
            beg, end, unw = struct.unpack("<III", blob[i : i + 12])
            if beg == 0 and end == 0:
                continue
            self.entries.append((beg, end, unw))
        self.entries.sort()
        self.starts = [e[0] for e in self.entries]

    def func_of(self, rva):
        i = bisect.bisect_right(self.starts, rva) - 1
        if i < 0:
            return None
        beg, end, unw = self.entries[i]
        if beg <= rva < end:
            return (beg, end)
        return None

    def __len__(self):
        return len(self.entries)


def load(path):
    return Image(path)
