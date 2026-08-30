// Finding and redirecting the engine's frequency lookup.
//
// Nothing here is an address. The one function that turns a tuned index into a
// frequency is located by its own machine code every time the server starts, so
// a game update moves it without breaking anything -- and if the shape it looks
// for is gone, it says so instead of patching whatever happens to be there.

#pragma once

#include <windows.h>
#include <stddef.h>

struct PatchTargets
{
    unsigned char* func;        // entry of the 18-byte leaf that reads the table
    unsigned char* table;       // the eight floats it reads, in .data
    bool           tableIsVanilla;   // do those floats still match 87.8 .. 102.5
};

// Locate the lookup in `module`. On failure, `why` says what was and was not
// found -- the difference between "no such code" and "found it twice" matters
// when deciding whether a game update broke this.
bool FindFrequencyCode(HMODULE module, PatchTargets* out, char* why, size_t whyLen);

// Overwrite the located function with an absolute jump to `replacement`.
// Absolute rather than relative: our DLL can land further than 2 GB from the
// executable, which a rel32 cannot reach.
bool RedirectFunction(unsigned char* func, void* replacement, char* why, size_t whyLen);
