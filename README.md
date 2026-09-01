# OpenZone Radio

*[Українською](README.uk.md)* · [**Steam Workshop**](https://steamcommunity.com/sharedfiles/filedetails/?id=3794105144)

DayZ ships with **eight** radio frequencies. This mod turns that into as many as you
configure — up to thousands — so a server can have real channels instead of eight
crowded ones.

It runs on **any server and any map**, and it is configured entirely from JSON.

> **This mod alone will not widen the band.** The eight frequencies are a limit in the
> game's own executable, not in its scripts, so no script mod can lift it. A small
> **server-side native library** does the lifting, and it ships in this repository —
> see [Widening the band](#widening-the-band). Without it the mod still installs and
> runs; it detects the unpatched server, says so in the log, and leaves the vanilla
> eight channels alone.

## What you get

**A measured band table.** Everyone repeats that DayZ has seven radio frequencies. That
number is nowhere in the scripts — it lives in the engine. On start-up the mod spawns
one vanilla radio outside the world, walks the frequency indices until they stop being
new, writes down what it found, and deletes the radio. The count decides how many
conversations can be on the air at once without mixing, so it is the first thing the mod
has to know about itself — and it measures rather than assumes.

**Radio profiles.** Bands, steps and limits in JSON: who may tune where, in what
increments, and between which bounds. Eleven profiles ship as defaults.

**Push-to-talk with a squelch.** A short burst of static on key down and key up, heard
from the radio itself at close range — not in your headphones.

**A frequency keypad.** Type a frequency instead of pressing "next channel" until you
arrive.

## The three parts

The repository builds **three separate mods**, and this matters when you install:

| Mod | Requires | Install it when |
|---|---|---|
| **`@OpenZone_Radio`** | Community Framework only | Always. This is the radio. |
| `@OpenZone_Radio_PDA` | + OpenZone Core, OpenZone PDA | You run the OpenZone PDA and want the radio as a board in a device bay |
| `@OpenZone_Radio_VPP` | + OpenZone VPP, VPPAdminTools | You want the admin tab for editing profiles in game |

**`@OpenZone_Radio` depends on nothing of ours.** Verified by booting a server with
Community Framework and this mod and nothing else — no Core, no PDA, no VPP:

```
[OpenZone/Radio] radio loaded: bands=8 profiles=11
```

The other two are glue. They declare their dependencies **hard**, which in DayZ means a
blocking window before the game loads rather than a silent skip — so install a glue mod
only together with what it glues to.

## Requirements

- [Community Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036)
- For a band wider than vanilla: the native library below, **on the server**.

Clients need nothing beyond the mod itself. The frequency table is resolved on the
server, so a stock client plays on a patched server normally — measured, not assumed.

## Widening the band

The engine computes a frequency as `table[index & 7]`: eight floats and a bitmask, in an
eighteen-byte function. `native/` contains a proxy `hid.dll` that replaces that one
function with `base + index * step`, which removes the table, the mask and the
power-of-two limit together.

**Server only, by design.** The proxy patches nothing unless the process was started
with `-server`. A stock client against a patched server was tested and plays normally.

Build and install it from [`native/README.md`](native/README.md):

```powershell
cd native
.\build.ps1 -Deploy -GameDir 'C:\path\to\DayZServer'
```

Then set the band in `oz_frequencies.json` beside the DLL:

```json
{ "base_mhz": 136.0, "step_mhz": 0.0125, "count": 1281 }
```

It writes `oz_frequencies.log` next to itself on **every** start-up, including the paths
that patch nothing — a mod that quietly does nothing while looking installed is the
failure that log exists to prevent.

> **One directory, every server.** The proxy sits beside the game executable. Every
> server started from that directory is patched, not only this one. Delete `hid.dll`
> there to undo it. Do not put it in a directory you also **play** from: it is unsigned,
> and BattlEye has opinions about unsigned libraries loaded into the game.

## Configuration

All of it is JSON in the server's `-profiles` directory, under `OpenZone/`:

| File | What it holds |
|---|---|
| `OZ_Radio_Profiles.json` | The radio profiles: bands, steps, limits |
| `OZ_Radio_Settings.json` | Mod settings |
| `OZ_Radio_Frequencies.json` | Derived from the profiles, read by the native library |

`OZ_Radio_Frequencies.json` is **not written by hand**. The mod derives it from the
profiles an admin edits — the lowest bottom, the highest top, and the greatest common
divisor of every step — so the ether is always exactly what the radios ask for.

The native library reads it once, at process start, because that is when the patch is
applied: a change takes effect on the **next** server start, and the library says so in
its log rather than leaving the delay to be discovered.

## Documentation

- [`native/README.md`](native/README.md) — the library: what it patches, how it finds
  the function, how to build it, what it refuses to do
- [`docs/engine-frequency-table.md`](docs/engine-frequency-table.md) — what was measured
  in the binaries
- [`docs/more-frequencies-plan.md`](docs/more-frequencies-plan.md) — why the design
  looks like this
- [`docs/workshop-description.md`](docs/workshop-description.md) — the Steam Workshop
  text, English and Ukrainian
- [`docs/publishing.md`](docs/publishing.md) — what is ready to publish and what still
  needs a decision

## Status

The band system, the profiles, push-to-talk and the keypad work. The native patch works
on a live server: sixteen distinct frequencies where the engine had eight, confirmed
independently by the mod's own probe driving the engine's API.

**Not yet proven:** that two players eight indices apart are separate *conversations*.
The frequencies are distinct and the router keys on the frequency's bytes, so the
remaining risk is small — but distinct values are not the same as having heard the
separation.

## Licence

CC BY-NC-SA 4.0, plus two additional permissions in [NOTICE](NOTICE):

**Repacking is allowed.** Unpack it, change it, put it in a modpack, redistribute it.
Most DayZ mods forbid this, so silence would read as a refusal — it is not one. Keep the
attribution, keep the licence, sign the repack with **your own** key rather than the
author's, and do not sell it.

**Selling is not.** A server taking donations, selling cosmetics or selling queue slots
is fine and is not commercial use. Selling the mod itself, or any derivative, or putting
it behind a paywall — including a "donate to download" one — is not.
