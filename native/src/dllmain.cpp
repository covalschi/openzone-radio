// OpenZone: more radio frequencies, server side.
//
// A proxy hid.dll that replaces the engine's frequency lookup so that a tuned
// index maps to `base + index * step` instead of to one of eight hardcoded
// values. See ../../docs/engine-frequency-table.md for why that one function is
// the whole limit, and ../../docs/more-frequencies-plan.md for the plan this
// implements.
//
// It patches the SERVER ONLY, on purpose. Client and server are the same
// executable in the same directory on a Diag stand, so an ungated proxy would
// patch both -- and the entire question this exists to answer is whether a stock
// client works against a patched server.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "forwards.h"
#include "patch.h"

// ---------------------------------------------------------------- configuration

struct Config
{
    double base;    // MHz of index 0
    double step;    // MHz between neighbouring indices
    int    count;   // how many channels exist; indices wrap within this
};

static Config g_config = { 87.8, 0.2, 64 };
static wchar_t g_dir[MAX_PATH] = L"";

// ------------------------------------------------------------------- the log

// Written beside the DLL. A mod that quietly does nothing is the worst outcome
// here -- every path through this file ends in a line saying what happened.
static void Log(const char* fmt, ...)
{
    wchar_t path[MAX_PATH];
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s\\oz_frequencies.log", g_dir);

    HANDLE h = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;

    SYSTEMTIME t;
    GetLocalTime(&t);

    char line[1024];
    int n = _snprintf_s(line, sizeof(line), _TRUNCATE, "%02d:%02d:%02d  ",
                        t.wHour, t.wMinute, t.wSecond);

    va_list args;
    va_start(args, fmt);
    n += _vsnprintf_s(line + n, sizeof(line) - n, _TRUNCATE, fmt, args);
    va_end(args);

    if (n > 0 && n < static_cast<int>(sizeof(line)) - 2)
    {
        line[n++] = '\r';
        line[n++] = '\n';
        DWORD written = 0;
        WriteFile(h, line, n, &written, NULL);
    }
    CloseHandle(h);
}

// -------------------------------------------------------------- config reading

// A three-number config does not justify a JSON library, and a dependency in a
// DLL that loads this early is a liability. This reads exactly the shape the
// documented file has: it finds "key", skips to the colon, and parses a number.
// Anything it cannot find keeps its default, and says so in the log.
static bool ReadNumber(const char* text, const char* key, double* out)
{
    char quoted[64];
    _snprintf_s(quoted, sizeof(quoted), _TRUNCATE, "\"%s\"", key);

    const char* at = strstr(text, quoted);
    if (!at)
        return false;

    const char* colon = strchr(at + strlen(quoted), ':');
    if (!colon)
        return false;

    char* end = NULL;
    double v = strtod(colon + 1, &end);
    if (end == colon + 1)
        return false;

    *out = v;
    return true;
}

static void LoadConfig()
{
    wchar_t path[MAX_PATH];
    _snwprintf_s(path, MAX_PATH, _TRUNCATE, L"%s\\oz_frequencies.json", g_dir);

    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        Log("no oz_frequencies.json beside the DLL; using defaults "
            "base=%.3f step=%.3f count=%d", g_config.base, g_config.step, g_config.count);
        return;
    }

    char text[4096];
    DWORD read = 0;
    BOOL ok = ReadFile(h, text, sizeof(text) - 1, &read, NULL);
    CloseHandle(h);
    if (!ok)
    {
        Log("oz_frequencies.json could not be read (%lu); using defaults", GetLastError());
        return;
    }
    text[read] = '\0';

    double count = g_config.count;
    if (!ReadNumber(text, "base_mhz", &g_config.base))
        Log("oz_frequencies.json has no base_mhz; keeping %.3f", g_config.base);
    if (!ReadNumber(text, "step_mhz", &g_config.step))
        Log("oz_frequencies.json has no step_mhz; keeping %.3f", g_config.step);
    if (ReadNumber(text, "count", &count))
        g_config.count = static_cast<int>(count);
    else
        Log("oz_frequencies.json has no count; keeping %d", g_config.count);

    if (g_config.count < 1)
    {
        Log("count=%d is not usable; forcing 1", g_config.count);
        g_config.count = 1;
    }
    if (g_config.step == 0.0)
    {
        // Every channel would hash to the same key and the mod would appear to
        // do nothing at all. Refuse the value rather than ship that confusion.
        Log("step_mhz=0 would make every channel identical; forcing 0.2");
        g_config.step = 0.2;
    }
}

// ------------------------------------------------------------ the replacement

// Replaces the engine's `float (*)(void* self, int index)`. On x64 the index
// arrives in edx and the result goes back in xmm0, which is what a plain
// function of this signature already does.
//
// Indices wrap rather than clamp, because SetNextChannel is `inc index` with no
// bound of its own: clamping would make the last channel a dead end instead of
// cycling the way the vanilla mask did.
extern "C" float OzFrequencyByIndex(void* /*self*/, int index)
{
    const int n = g_config.count;
    int i = index % n;
    if (i < 0)
        i += n;
    return static_cast<float>(g_config.base + i * g_config.step);
}

// ------------------------------------------------------------------- start-up

// Two ways to be the server, and both are needed.
//
// A Diag build is one executable for client and server, told apart only by
// -server on the command line. A real DayZServer_x64.exe needs no such flag --
// it is the dedicated server -- so a gate that read the command line alone
// would skip the very install this is meant for, and skip it silently.
static bool IsDedicatedServer()
{
    const wchar_t* cmd = GetCommandLineW();
    if (cmd && wcsstr(cmd, L"-server") != NULL)
        return true;

    wchar_t exe[MAX_PATH] = L"";
    if (GetModuleFileNameW(NULL, exe, MAX_PATH))
    {
        const wchar_t* name = wcsrchr(exe, L'\\');
        name = name ? name + 1 : exe;
        if (_wcsicmp(name, L"DayZServer_x64.exe") == 0)
            return true;
    }
    return false;
}

static void Start(HMODULE self)
{
    GetModuleFileNameW(self, g_dir, MAX_PATH);
    wchar_t* slash = wcsrchr(g_dir, L'\\');
    if (slash)
        *slash = L'\0';

    if (!IsDedicatedServer())
    {
        // Deliberately silent: on a Diag stand this branch runs for the game
        // client on every launch, and a log line per launch would be noise.
        return;
    }

    Log("----");
    LoadConfig();

    HMODULE host = GetModuleHandleW(NULL);
    PatchTargets targets = { 0, 0, false };
    char why[512] = "";

    if (!FindFrequencyCode(host, &targets, why, sizeof(why)))
    {
        Log("NOT PATCHED: %s", why);
        return;
    }
    Log("found: %s", why);

    if (!targets.tableIsVanilla)
    {
        Log("note: the game's own frequency table is not the eight this was "
            "written against -- the patch still applies, but re-check the "
            "research notes against this build");
    }

    if (!RedirectFunction(targets.func, reinterpret_cast<void*>(&OzFrequencyByIndex),
                          why, sizeof(why)))
    {
        Log("NOT PATCHED: %s", why);
        return;
    }

    Log("patched: %s", why);
    Log("channels: %d, from %.3f MHz in steps of %.3f MHz (index 0 = %.3f, "
        "index %d = %.3f)",
        g_config.count, g_config.base, g_config.step,
        g_config.base, g_config.count - 1,
        g_config.base + (g_config.count - 1) * g_config.step);
}

BOOL APIENTRY DllMain(HMODULE self, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(self);
        Start(self);
    }
    return TRUE;
}
