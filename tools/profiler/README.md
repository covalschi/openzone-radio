# Server sampling profiler

Answers one question: **where is `DayZServer_x64.exe` spending its time, per
thread?**

The server's own FPS number cannot answer it. It is an average, so a stall of a
second every few seconds barely moves it, and it watches the frame loop only —
if a different thread is the one standing still, the counter stays cheerful
while players see other players freeze in place.

This samples the instruction pointer of every thread a few hundred times a
second and buckets the addresses by the function ranges in the executable's own
`.pdata`. The answer comes out as "thread N spent X% inside this function",
which is a measurement, not a theory.

## Files

| file | runs where | needs |
|---|---|---|
| `collect-samples.ps1` | the machine running the server | nothing (Windows PowerShell) |
| `resolve-samples.py` | anywhere the server exe is available | Python 3, `pefile`, `capstone` |
| `profile-live.py` | one box that has both | Python 3, `pefile`, `capstone` |
| `dayz_image.py` | library for the two Python tools | — |

The split exists so the server's machine needs nothing installed: the collector
records addresses and nothing else, and the naming happens elsewhere.

## Procedure

### 1. Collect, while the problem is happening

On the server's machine:

```powershell
.\collect-samples.ps1 -Seconds 30 -Hz 200 -Out lag.csv
```

Thirty seconds of a bad moment is worth more than ten minutes of a good one.
The process is not modified and nothing is written into the game folder.

### 2. Collect a control run

The same command again in the state where the problem is absent — the previous
build, the DLL removed, fewer players, whatever the known-good configuration
is:

```powershell
.\collect-samples.ps1 -Seconds 30 -Hz 200 -Out calm.csv
```

**The control run is the point.** An absolute profile shows what a DayZ server
always does; the difference between two profiles shows what went wrong.

### 3. Resolve

```bash
python resolve-samples.py lag.csv --exe E:/dayzmod/dayzserver-retail/DayZServer_x64.exe
python resolve-samples.py calm.csv --exe E:/dayzmod/dayzserver-retail/DayZServer_x64.exe
```

The `--exe` must be **the same build** that produced the samples. A game update
moves every function, and resolving against the wrong binary produces confident
nonsense rather than an error.

On a **diag** stand the server is `DayZDiag_x64.exe`, a different binary with its
own addresses: collect with `-Exe DayZDiag_x64.exe` and resolve with
`--exe .../DayZDiag_x64.exe`. The hand-named functions in `KNOWN` are retail
addresses and will not apply there; the ranges from `.pdata` still do.

### All on one machine

If the box running the server also has Python, skip the CSV:

```bash
python profile-live.py --seconds 30 --hz 200 --out profile.txt
```

## Reading the output

```
thread 20524: 411 samples, 87% doing work
     10.71%      44  [module] ntdll.dll
      7.54%      31  +0x2E01E0
      7.06%      29  +0x87363B
```

Two things to look at, in this order:

1. **Which thread is busy.** Threads that only ever wait are omitted. If the
   busy one is not the frame loop, the frame counter was never going to show
   the problem.
2. **Whether a name appears.** Functions established by reading the
   disassembly print with a description instead of an address, e.g.
   `VON: update one transmitter against ALL players`. Anything else is an
   address, which is still useful: it can be looked up in the binary.

Percentages are of that thread's own samples, so a thread parked in `ntdll` at
99% is idle, not hot.

## Adding names

`KNOWN` at the top of `resolve-samples.py` maps a function's RVA to a
description. Entries are added only after the function has actually been read
in the disassembler — a guessed name is worse than an address, because an
address invites checking and a name invites belief.

The VoN chain currently named there was established while investigating the
radio mod; see `docs/engine-frequency-table.md` for the frequency lookup and
how it was located.

## Caveats

- The collector must be able to open the process: same user, or elevated.
- Sampling suspends each thread for a few microseconds. At 200 Hz that is well
  under a percent of wall clock. Do not run it at thousands of hertz against a
  live server.
- Addresses are recorded relative to the module base, so ASLR does not matter
  and two runs are comparable.
- A sample lands on the function that was executing, not on whoever called it.
  This says *where* the time went, not *why*; the call graph still has to be
  read out of the binary.
