# OpenZone frequency proxy

A server-side native mod that raises DayZ's eight radio channels to a number set
in JSON. Loads as a proxy `hid.dll` beside the game executable and replaces the
one engine function that turns a tuned index into a frequency.

Background: [engine-frequency-table](../docs/engine-frequency-table.md) for what
was measured in the binaries, [more-frequencies-plan](../docs/more-frequencies-plan.md)
for why the design looks like this.

## What it does

The engine computes a frequency as `table[index & 7]` — eight floats in `.data`
and one `and eax, 7`, in an eighteen-byte leaf function. This replaces that
function with `base + index * step`, which removes the table, the mask and the
power-of-two constraint at once. Indices wrap within `count`, so `SetNextChannel`
still cycles the way the vanilla mask made it.

Delivery is decided only on the server, so **stock clients need no native code**
— see the research notes for the call-graph evidence.

## Server only, by design

The proxy patches nothing unless the process was launched with `-server`. On a
Diag stand the client and the server are the same executable in the same
directory, so an ungated proxy would patch both — and the whole question this
exists to answer is whether an unpatched client works against a patched server.

## Build

Needs the MSVC C++ toolchain (Visual Studio or the standalone Build Tools, with
the "Desktop development with C++" workload — the Windows SDK comes with it).
`build.ps1` finds it through `vswhere` and imports the x64 environment itself, so
no developer prompt is required.

```powershell
.\build.ps1                 # build into .\build
.\build.ps1 -Deploy         # build, then install beside the game executable
.\build.ps1 -Deploy -GameDir 'D:\somewhere\DayZ'
```

Deploying refuses while the game or server is running, because a loaded DLL
cannot be overwritten.

## Configuration

`oz_frequencies.json`, beside the DLL:

```json
{
  "base_mhz": 87.8,
  "step_mhz": 0.2,
  "count": 64
}
```

Only distinctness matters to the engine — it keys channels on the four bytes of
the `float`, so any step that keeps values apart in `float32` works. A missing
field keeps its default and is reported; `count < 1` and `step_mhz == 0` are
refused and replaced, because both would silently collapse every channel into
one.

## Log

`oz_frequencies.log`, beside the DLL. Every path through start-up writes a line,
including the ones that patch nothing — the failure this file exists to prevent
is a mod that quietly does nothing while appearing installed.

```
08:41:02  found: lookup at +0x5C7790, table at +0x114DF30, table is the known vanilla eight
08:41:02  patched: redirected 12 bytes
08:41:02  channels: 64, from 87.800 MHz in steps of 0.200 MHz (index 0 = 87.800, index 63 = 100.400)
```

## Layout

```
src/forwards.h    47 pragmas forwarding hid.dll's exports to the real one
src/patch.h/.cpp  finding the lookup by its code shape, and redirecting it
src/dllmain.cpp   the -server gate, the JSON, the log, the replacement function
build.ps1         toolchain discovery, build, deploy
```

`forwards.h` is generated from the real `hid.dll`'s export table rather than
written by hand. Regenerate it if a future Windows adds an export.

## Finding the function

No address is compiled in. At start-up the DLL scans the host image's executable
sections for the lookup's own machine code, with the `lea`'s displacement
wildcarded, then follows that displacement and checks what it points at. A game
update therefore moves the function without breaking anything.

Two refusals are deliberate:

- **No match** — the game changed how it compiles this. Nothing is patched.
- **More than one match** — patching the wrong one of two identical leaves would
  be silent and very hard to trace, so it refuses rather than guess.

Both are written to the log. The shape has been checked against
`DayZServer_x64.exe`, `DayZ_x64.exe` and `DayZDiag_x64.exe`: exactly one match in
each, and in each the displacement lands on the eight vanilla frequencies.

## Status

**The patch works on a live server.** Measured 2026-08-30 against
`DayZDiag_x64.exe -server` with `count: 16`:

```
found: lookup at +0x5C7790, table at +0x114DF30, table is the known vanilla eight
patched: redirected 12 bytes
channels: 16, from 87.800 MHz in steps of 0.200 MHz (index 0 = 87.800, index 15 = 90.800)
```

Confirmed independently by `OZR_Bands.Probe`, which predates this work and knows
nothing about it. It drives the engine's own `SetFrequencyByIndex` /
`GetTunedFrequency` and reads back what the engine actually returned:

```
band table measured: 16 frequencies  87.8 88 88.2 88.4 88.6 88.8 89 89.2
                                     89.4 89.6 89.8 90 90.2 90.4 90.6 90.8
```

Sixteen distinct frequencies where the engine had eight, matching the JSON
exactly — so both the patch and the config reader are doing what they claim.

The `-server` gate was verified in the same session: the game client loaded the
same `hid.dll` from the same directory and added no line to the log, leaving its
own table vanilla. That is the shipping configuration — patched server, stock
client — and the client connected and played normally in it.

### What is still unproven

That two players on indices 0 and 8 are separate **conversations**. The
frequencies are now distinct and the router keys on the frequency's four bytes
(see the research notes), so the remaining risk is small — but distinct values
are not the same as having heard the separation. The test is two players, two
handheld radios, eight presses of "tune" apart: before the patch both are `87.8`
and they hear each other, after it they must not.

## Note on a shared game directory

The proxy is deployed beside the game executable, which on this machine is the
one every DayZ project shares. **Every server started from that directory is
patched**, not only this project's. Delete `hid.dll` there to undo it.
