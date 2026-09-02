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

**Push-to-talk with a squelch.** A short burst of static on key down and key up, played
on the radio itself, so everyone near you hears it too — it is the set giving you away,
not a noise in your headphones.

**One radio speaks, not all of them.** The one in your hands; with your hands empty, the
one on a slot that you held last. A radio in cargo never speaks — that one is a spare.
Carry three sets on three frequencies and the key still opens exactly one.

**A frequency keypad.** Type a frequency instead of pressing "next channel" until you
arrive.

### One key for "talk", another for "talk on the air"

Push-to-talk opens the radio's transmitter; it does not make you speak. Speaking is
still the game's own voice key, because **no mod can start voice transmission** — the
engine offers script only `EnableVoN(player, bool)`, which is a permission gate vanilla
uses to silence the dead, and `SetVoiceLevel`, which is whisper/talk/shout. Transmitting
is the native input `UAVoiceOverNet`, and nothing in script can press it.

Holding two keys at once is not the answer either. Give the game's voice input a
**second binding** on the key you use for the radio, and one press does both:

| input | key | result |
|---|---|---|
| `UAVoiceOverNet` | CapsLock *(primary)* | speak to the people around you |
| `UAVoiceOverNet` | End *(alternative)* | — |
| `UAOZRadioPtt` | End | — |

Now **CapsLock is proximity only** and **End is proximity and radio together**, one key
each. In the controls screen: bind "Radio: push to talk" to the key you want, then add
that same key as the alternative binding of "Voice over Network". The two inputs share
the key and both fire; DayZ allows this, and the pair above was driven on the stand.

The choice of End is only an example — any free key works. What matters is that the
radio key carries both inputs and the plain voice key carries only one.

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
| `OZ_Radio_Settings.json` | Mod settings, including how loud the air is |
| `OZ_Radio_Frequencies.json` | Derived from the profiles, read by the native library |

`OZ_Radio_Frequencies.json` is **not written by hand**. The mod derives it from the
profiles an admin edits — the lowest bottom, the highest top, and the greatest common
divisor of every step — so the ether is always exactly what the radios ask for.

The native library reads it once, at process start, because that is when the patch is
applied: a change takes effect on the **next** server start, and the library says so in
its log rather than leaving the delay to be discovered.

### How loud the squelch is, and how far it carries

`OZ_Radio_Settings.json`:

| Setting | What it does |
|---|---|
| `SquelchGain` | Multiplies the burst of static. `1.0` is the level set in `config.cpp` |
| `SquelchRange` | How far it is heard, in metres. `15` by default |
| `MirrorPtt` | Adds your push-to-talk key as a second binding on the game's voice key, so one press does both. `true` by default |
| `PttFromCargo` | Whether a radio in a backpack may speak. `true` by default |

`SquelchRange` moves in **steps** — 5, 10, 15, 20, 25 — and the nearest one is
taken; ask for 13 and you get 15, and the log says so rather than leaving you to
wonder. The steps exist because DayZ's two sound APIs each withhold one dial:
sound sets let a mod change volume at runtime but fix the radius in config, while
`Object.PlaySound` takes a radius as an argument and offers no volume at all.
Keeping the sets means keeping the sound; the radius then comes from picking
among prepared ones.

All four are read when the server starts and travel to each client on connect, so a
change needs a server restart. Moving `SquelchRange` between steps needs nothing from
players — the sets are already in their copy of the mod, and the server only names the
one to use.

### Which radio speaks

Still one, never all of them, and `PttFromCargo` only widens where that one may be
found. The order is place first, freshness second:

1. the radio **in your hands** — it wins outright
2. otherwise the one **on a slot**, most recently held
3. otherwise the one **in cargo**, most recently held — unless `PttFromCargo` is off

A worn radio beats one in a backpack even if the backpack one was held later: the place
is the intent, and the timestamp only separates equals.

**A radio you have never picked up does not speak at all.** Taking one in hands is what
chooses it, and that choice sticks to the radio — it survives logging out and it survives
a server restart, so the gesture is needed once per radio rather than once per session.
Taking a different one in hands moves the choice to it; a player has one working radio,
not several. Only the radio actually in your hands is exempt, because holding it is the
same statement.

Without that rule the traversal order of your inventory would choose for you, silently,
between radios on different frequencies.

The HUD icon follows exactly the same rule and lights only when the key will actually do
something — which, with this rule, is the only warning you get.

> The choice is stored through Community Framework's ModStorage, which means the mod's
> `CfgMods` class name is now part of the save format. Renaming it would leave every
> player's choice unreachable.

**There is no setting for the loudness of the voice itself, and that is a finding
rather than an omission.** The engine has a radio mixer channel, and DayZ's sound
options do not expose it — five sliders, none of them radio. The one script handle,
`SoundScene.SetRadioVolume`, was tried with a multiplier of ten and measured on a
live client: the engine accepted the value and read it back unchanged, and nothing
about the voice got louder. The channel exists, the number is stored, and it does
not reach what comes out of a radio. A knob that promises what it cannot do is
worse than no knob, so it was removed.

## When the frequencies look absurd

A radio reading **2108.000 MHz**, a knob that steps in **2.1 MHz**, a keypad that
accepts a number and does nothing: all three are one fault, and it is not in the
radio. **The native library is not in effect on that server.**

Without the patch the engine serves its own eight frequencies — 87.8, 89.5, 91.3,
91.9, 94.6, 96.6, 99.7, 102.5 — which are not evenly spaced, and the mod says so
in the server log:

```
the engine's frequency table is not an even grid (8 bands) - radio profiles stay unapplied
```

Since 2026-09-02 that is all a player sees: radios fall back to the engine's own
frequency and the keypad does not open. Before that fix the server also handed
clients the eight as if they were a grid, and a radio still carrying an index from
the real one was labelled with `87.8 + index × 2.1` — which is where 2108.000 came
from. It was only ever the label; the radio was audible on a real vanilla channel
the whole time.

**What to check, in order:**

1. `oz_frequencies.log` beside the server executable. `patched: redirected 12 bytes`
   means the library is working and the fault is elsewhere.
2. **No log file at all** means the library never ran: it is not beside the
   executable that started, or the process was one of the game's own. A server
   launched under a renamed or wrapped binary now names itself in that log instead
   of failing in silence — so if the file exists and says `NOT PATCHED: loaded
   into "…"`, add `-server` to its command line.
3. A log ending in `NOT PATCHED: no match` means a game update moved the function.

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
