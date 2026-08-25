# OpenZone Radio

Radio for the [OpenZone PDA](https://github.com/covalschi/openzone-pda). Not a second
box in your pocket: a **board that goes into a PDA bay**, plus the antennas that decide
how far it reaches.

Built on [OpenZone Core](https://github.com/covalschi/openzone-core) and OpenZone PDA.
Designed to run on **any server and any map**, configured entirely from JSON.

## What it adds

- **Radio board.** A module for any PDA bay. It enables the radio page on the device —
  no board, no page, because a page nobody can use is worse than no page at all.
- **Long antenna.** The same module kind as the PDA's own stub antenna, only with a
  larger range. That *is* the extension mechanism: whichever antenna declares the
  bigger `RangeM` wins, and a server owner can retune either from JSON.
- **Measured band table.** Everyone repeats that DayZ has seven radio frequencies, but
  that number is nowhere in the scripts — it lives in the engine. On start-up the mod
  spawns one vanilla radio out of the world, walks the frequency indices until they
  stop being new, writes down what it found, and deletes the radio. The count decides
  how many conversations can be on the air at once without mixing, so it is the first
  thing the mod has to know about itself.

## Status

Early. The band table and the hardware contract are in; the channel UI and push-to-talk
are next.

## Requirements

- [Community Framework](https://steamcommunity.com/sharedfiles/filedetails/?id=1559212036)
- OpenZone Core
- OpenZone PDA

## Licence

CC BY-NC-SA 4.0, with an additional permission for servers that take donations — see
[NOTICE](NOTICE).
