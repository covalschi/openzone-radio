# packaging

What goes into a `@Mod` folder besides the packed pbo.

The `@Mod` folders are **build output and are not in git**. `mod_build` writes
`addons/` into them and signs it; everything else a published mod needs lives here and
is copied in by [`package.ps1`](../package.ps1).

```
packaging/<Mod>/mod.cpp     the name, author, version and description DayZ shows in
                            the launcher and the mod list
packaging/<Mod>/meta.cpp    the Workshop item id, for mods that have been published
```

## Why meta.cpp is here rather than only in the published folder

`meta.cpp` is neither source nor build output: it is the line that ties a local folder
to a Workshop item. Publisher normally writes it, and if it goes missing the next upload
creates a **second** item instead of updating the one that exists.

The first publish of `OpenZone_Radio` (2026-09-01, item 3794105144) left none — the
folder had `addons`, `keys` and `mod.cpp` and nothing else, and a search of the machine
found no `meta.cpp` and no file containing the id. It was reconstructed from the
Workshop URL, and it lives here so it cannot be lost again by deleting a build folder.

`timestamp` is deliberately absent: it means "when this build was uploaded", and a
number invented to fill it would be false. Publisher writes a real one.

## After Publisher runs

If Publisher rewrites `meta.cpp` in the `@Mod` folder — a new timestamp, or an id for a
mod published for the first time — **copy it back here** and commit it. Otherwise the
next `package.ps1` overwrites Publisher's version with this one.
