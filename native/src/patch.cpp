#include "patch.h"

#include <stdio.h>
#include <string.h>

// The lookup, verbatim, as every DayZ build so far compiles it:
//
//   8B C2                 mov   eax, edx            ; edx = tuned index
//   48 8D 0D rel32        lea   rcx, [rip + table]
//   83 E0 07              and   eax, 7              ; the entire eight-channel limit
//   F3 0F 10 04 81        movss xmm0, [rcx + rax*4]
//   C3                    ret
//
// Eighteen bytes, one caller path, identical in DayZServer_x64, DayZ_x64 and
// DayZDiag_x64. We match on everything except the rel32, then confirm the match
// by following that rel32 and looking at what it points at.
static const unsigned char kShape[] = {
    0x8B, 0xC2,
    0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,
    0x83, 0xE0, 0x07,
    0xF3, 0x0F, 0x10, 0x04, 0x81,
    0xC3,
};
static const char kMask[] = "xx"      "xxx????"  "xxx"  "xxxxx"  "x";
static const size_t kShapeLen = sizeof(kShape);

// What the eight vanilla frequencies are, so a build that changed them can be
// reported rather than silently accepted.
static const float kVanilla[8] = {
    87.8f, 89.5f, 91.3f, 91.9f, 94.6f, 96.6f, 99.7f, 102.5f
};

const size_t kPatchLen = 12;   // mov rax, imm64 (10) + jmp rax (2)


static bool MatchesShape(const unsigned char* p)
{
    for (size_t i = 0; i < kShapeLen; ++i)
    {
        if (kMask[i] == 'x' && p[i] != kShape[i])
            return false;
    }
    return true;
}


bool FindFrequencyCode(HMODULE module, PatchTargets* out, char* why, size_t whyLen)
{
    unsigned char* base = reinterpret_cast<unsigned char*>(module);
    IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
        _snprintf_s(why, whyLen, _TRUNCATE, "module has no DOS header");
        return false;
    }

    IMAGE_NT_HEADERS64* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
    {
        _snprintf_s(why, whyLen, _TRUNCATE, "module has no PE header");
        return false;
    }

    IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    const int nsec = nt->FileHeader.NumberOfSections;

    unsigned char* hits[4] = { 0, 0, 0, 0 };
    int hitCount = 0;
    int scanned = 0;

    for (int i = 0; i < nsec && hitCount < 4; ++i)
    {
        if (!(sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;

        ++scanned;
        unsigned char* start = base + sec[i].VirtualAddress;
        // VirtualSize can exceed the mapped raw data on the last section; the
        // engine's .text is fully backed, but stay inside it regardless.
        size_t size = sec[i].Misc.VirtualSize;
        if (size < kShapeLen)
            continue;

        for (size_t off = 0; off + kShapeLen <= size; ++off)
        {
            if (!MatchesShape(start + off))
                continue;
            if (hitCount < 4)
                hits[hitCount] = start + off;
            ++hitCount;
            if (hitCount >= 4)
                break;
        }
    }

    if (scanned == 0)
    {
        _snprintf_s(why, whyLen, _TRUNCATE, "no executable section to scan");
        return false;
    }
    if (hitCount == 0)
    {
        _snprintf_s(why, whyLen, _TRUNCATE,
                    "the frequency lookup's code shape is not present in %d executable "
                    "section(s) -- the game changed how it compiles, nothing was patched",
                    scanned);
        return false;
    }
    if (hitCount > 1)
    {
        // Ambiguity is a refusal, not a coin flip: patching the wrong one of two
        // identical-looking leaves would be silent and very hard to trace.
        _snprintf_s(why, whyLen, _TRUNCATE,
                    "the code shape matched %d times; refusing to guess which is the "
                    "frequency lookup", hitCount);
        return false;
    }

    unsigned char* func = hits[0];

    // Follow the lea's rel32 to the table it reads. The displacement is relative
    // to the END of that instruction, which sits at func + 9.
    int rel = *reinterpret_cast<int*>(func + 5);
    unsigned char* table = func + 9 + rel;

    // Confirm the target is inside the image, so a wild displacement cannot make
    // us report success about a random address.
    unsigned char* imageEnd = base + nt->OptionalHeader.SizeOfImage;
    if (table < base || table + sizeof(kVanilla) > imageEnd)
    {
        _snprintf_s(why, whyLen, _TRUNCATE,
                    "the lookup's table pointer lands outside the module image");
        return false;
    }

    bool vanilla = (memcmp(table, kVanilla, sizeof(kVanilla)) == 0);

    out->func = func;
    out->table = table;
    out->tableIsVanilla = vanilla;
    _snprintf_s(why, whyLen, _TRUNCATE,
                "lookup at +0x%llX, table at +0x%llX, table is %s",
                static_cast<unsigned long long>(func - base),
                static_cast<unsigned long long>(table - base),
                vanilla ? "the known vanilla eight" : "NOT the known vanilla eight");
    return true;
}


bool RedirectFunction(unsigned char* func, void* replacement, char* why, size_t whyLen)
{
    // mov rax, imm64 ; jmp rax  -- 12 bytes into the 18 the original occupies.
    unsigned char stub[kPatchLen];
    stub[0] = 0x48;
    stub[1] = 0xB8;
    memcpy(stub + 2, &replacement, sizeof(void*));
    stub[10] = 0xFF;
    stub[11] = 0xE0;

    DWORD old = 0;
    if (!VirtualProtect(func, kPatchLen, PAGE_EXECUTE_READWRITE, &old))
    {
        _snprintf_s(why, whyLen, _TRUNCATE,
                    "VirtualProtect failed with %lu", GetLastError());
        return false;
    }

    memcpy(func, stub, kPatchLen);

    DWORD ignored = 0;
    VirtualProtect(func, kPatchLen, old, &ignored);
    FlushInstructionCache(GetCurrentProcess(), func, kPatchLen);

    _snprintf_s(why, whyLen, _TRUNCATE, "redirected %zu bytes", kPatchLen);
    return true;
}
