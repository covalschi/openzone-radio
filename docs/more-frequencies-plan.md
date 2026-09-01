> **Status as of 2026-09-01.** The plan is done: the patch works, and the stand ran
> with 5281 frequencies.
>
> **One error in the text below.** The patch **WRAPS** an out-of-range index modulo the
> count; it does not clamp it, as written here. Checked against the code 2026-09-01;
> the functional registry §3.1 carries the correct wording.
>
> What is still live: the ceiling of 65536 divisions was left hard (owner's decision
> 2026-09-01, TZ-5 §E4), while the reasoning it rested on has been removed from the
> code — the synchronised variable is registered without bounds.

# More radio frequencies: plan

Goal: raise DayZ's eight radio channels to a number the server owner chooses, with
the range and step given in JSON, on a **public server with stock clients**.

Rests entirely on [engine-frequency-table](engine-frequency-table.md), which
established the three facts this plan needs:

1. A frequency is `table[index & 7]`, computed by one 18-byte leaf function.
2. The tuned index is an unbounded `int32` — nothing clamps it, on either side.
3. Delivery is decided **only** on the server. The client maintains a
   frequency→radios map but never queries it; every float-keyed lookup on the
   client belongs to the register/unregister path.

## Shape

Two parts, and only the first contains anything unusual.

### Server: a native DLL

Loaded by proxying `hid.dll`, which the server imports and which has no copy of
its own beside the executable. The proxy forwards every export to the real
`System32\hid.dll` and does its work on load.

**It must gate on `-server`.** The stand — and any Diag setup — runs client and
server from the same directory and the same `DayZDiag_x64.exe`, so an ungated
proxy patches both. That would make every test meaningless, because the whole
question is whether a *stock* client works against a patched server. The gate is
`GetCommandLineW()`.

What it patches, found at runtime and never hardcoded:

```
1. locate the 32-byte table of eight floats in .data
2. find the single RIP-relative reference to it in .text
3. walk back to the int3 padding -> the function entry
4. overwrite 18 bytes with:  mov rax, <ours> ; jmp rax     (12 bytes, fits)
```

Replacing the function rather than growing the table is deliberate: the 32 bytes
have unrelated data immediately behind them and cannot grow in place, and a
relocated array would have to stay within ±2 GB for the `lea`'s rel32 to reach.
Replacing it also drops the `& 7` mask and the power-of-two constraint together.

Ours is:

```c
float __fastcall frequency(void* self, int index);   // index arrives in edx
    -> base + clamp(index, 0, count-1) * step
```

If the pattern is not found — a game update changed the vanilla frequencies — it
patches nothing and says so in the log. A mod that silently does nothing is worse
than one that refuses loudly.

### Client: an ordinary PBO

Pure script, nothing native, installs like any Workshop mod.

- Set indices past 7. Already possible; the field is never clamped.
- Draw the frequency label itself. `GetTunedFrequency()` on the client still
  returns `table[index & 7]` and will be wrong, so the two vanilla places that
  show it — `ActionTuneFrequencyOnGround` and `ItemActionsWidget` — need
  overriding. The wrong local float is otherwise harmless: it only mis-keys an
  entry in a map nothing reads.

## Configuration

JSON beside the DLL. Three numbers, no dependencies — a hand-rolled reader is
smaller than pulling in a library for this:

```json
{
  "base_mhz": 87.8,
  "step_mhz": 0.2,
  "count": 64
}
```

`count` bounds the index; `base + index * step` gives the value. Distinctness is
what matters to the engine, not realism — the hash map keys on the float's four
bytes, so any step that keeps values distinct in `float32` works.

## Test

Two players, two handheld radios, indices 0 and 8 — the configuration in which
the owner already knows vanilla behaviour cold, so no control run is needed.

- Before the patch: 0 and 8 are both `87.8`. They hear each other.
- After: 8 becomes a value no vanilla index produces. They must not.

A single-player version was attempted and abandoned: the transmitting radio in
your own hands is audible to you regardless of what the second radio is tuned to,
so "can I hear it" stops being an answer. Distance would work around it, but with
a second person available there is no reason to.

## Consequence for OpenZone Radio's own design

`OZR_Config` opens by stating the premise the whole channel model rests on:

> A channel is not a frequency. The engine has exactly as many frequencies as
> `OZR_Bands` measured, and there is nowhere else for them to come from.

The patch retires that sentence. Two things follow, and only one of them is free.

**`OZR_Bands.Probe` adapts by itself.** It walks indices until a frequency
repeats, and with `base + index * step` nothing repeats before `count`. Against a
patched server it measures the expanded table with no change to its code — the
only edit it needs is a larger `BAND_PROBE_MAX`, which is 32 today and is a
deliberate "look for the limit, do not assume it" number rather than a belief
about the engine.

**The probe measures where it runs.** On a patched server it finds `count`
frequencies; on a stock client it finds the vanilla eight, because the client's
own table is untouched. The two sides then disagree about how many channels
exist — and the PDA radio page is drawn on the client.

So the client PBO has two jobs, not one:

- draw the frequency label from the formula instead of `GetTunedFrequency()`;
- **receive** the band table from the server instead of probing for it locally.

The second is the larger change, and it is a change to existing code rather than
new code beside it: `OZR_Bands` becomes "the table, however it was learned", with
probing as what a client does when the server tells it nothing — which is also
exactly what happens on an unpatched server, so the fallback is not dead code.

## Risks

- **Game updates.** Addresses are found by pattern, so a patch that leaves the
  eight vanilla frequencies alone changes nothing. One that changes them breaks
  the finder — loudly, by design.
- **Steam file verification.** The proxy sits in the game directory. Steam leaves
  extra files alone but this is worth knowing before it surprises someone.
- **Partly unverified.** As of 2026-08-30 the server side is measured, not
  argued: the patch applies, the engine returns sixteen distinct frequencies for
  a `count: 16` config, and a stock client connects and plays against it without
  being patched itself (see [../native/README.md](../native/README.md)). What
  remains unproven is that two players on indices 0 and 8 are separate
  *conversations* — distinct frequency values are not the same as having heard
  the separation. Nothing should ship before the two-player test above.
