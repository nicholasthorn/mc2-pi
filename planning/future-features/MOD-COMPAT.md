# Fan mod compatibility (MechCommander Omnitech, MC2X)

Status: study, nothing built. Written 2026-10-30 from a read of the public
Omnitech source snapshot against alariq's first commit (`305ad87`, close to
Microsoft's shared-source release) and `pi-port`.

## Problem

Most MC2 fan campaigns since 2012 target one of two Windows-only forks of the
Microsoft source, not stock MC2:

- **MechCommander Omnitech (MCO)**, magicX. Last release 6.28 (Mar 2017); a
  DX11 rewrite was announced on ModDB on 2026-09-21. Campaigns: Clan Wars
  (28 missions), Carver 5 Redux, Exodus, Wolf Dragoons, Liberation of
  Volstand, Mercenaries Star, Laskos Legion, Desert Fox, Day of Heroes, Clan
  Eagle.
- **wolfman-MC2X**, revision 70 (readme Oct 2026). Campaign: Dark Gray:
  Borderworlds (MC2X r20). No source published that we could find.

None of them load on this port as shipped. This file records what an Omnitech
campaign needs, so the work can be sized.

## Source

- Omnitech: `github.com/Echelon9/mechcommander2-open`, 33 commits, last
  2013-03-03. The engine is a single dump (`41f052d`, 2012-11-24) plus
  PVS-Studio fixes, so it matches roughly MCO 269-304, not the 424-628
  releases (new mechs, copters, buildings, non-linear campaigns, more ABL).
  Visual Studio 2010 + DirectX 8.
- Changes are tagged in comments as `//Magic NN` (about 50 numbered change
  sets, 15 to 100) or `//Magic <date>`; easy to trace one feature at a time.
- License: Microsoft's code stays under its shared-source licence (MS-PL).
  MCO's own additions are under `Source/MCO EULA.txt`: non-commercial, keep
  notices, no warranty. Re-implement from the behaviour; do not copy MCO code
  into this tree without deciding on that first.

## What differs (snapshot vs Microsoft base)

About 700 source files differ. The parts that decide whether a campaign loads:

1. **18 ABL (mission script) functions** that stock does not register. A
   mission brain calling one fails to compile, so the mission does not start.
   Name, return-on-call flag, argument and return signatures as in
   `ABLi_addFunction`:

   | function | args | ret | | function | args | ret |
   |---|---|---|---|---|---|---|
   | addmoney | i | - | | setbrain | iC | - |
   | addmovertoplayer | iii | - | | settextmsg | iCi | - |
   | addresourcepoints | i | - | | settxtbuildingname | iC | - |
   | removemoverfromplayer | i | - | | teleporttopoint | iR | - |
   | getrelativepositiontotarget | iriR | - | | toggleairstrike | - | - |
   | gettargetrelativeposition | irr | i | | toggleartillerypiece | - | - |
   | magicattack (orders) | ii | i | | togglerepairtruck | - | - |
   | setobjectivetype | ii | i | | togglesalvagecraft | - | - |
   | checkobjectivetype | i | i | | togglesensorstrike | - | - |

   `setobjectivetype` / `checkobjectivetype` exist in our `code/ablmc2.cpp`
   but are commented out (as Microsoft shipped them). Later MCO releases add
   more (6.28 notes mention building commands); the snapshot cannot list them.

2. **Mission file keys.** Each mission part has a required `CSVFile` (the unit
   variant table to use) next to `VariantNumber`. Stock ignores unknown keys,
   so the key itself is harmless, but the units it names come from MCO's
   variant CSVs. Profile names are also special-cased (`PM101100`,
   `PM207300`, `pv20500` set an object type).

3. **Logistics and save keys.** New: component purchase and salvage lists
   (`PurComponentCount`, `purComponents`, `SalvageComponents`...),
   per-location current armour and internals on mechs (`CurArmorHead`...),
   `maxDropWeight`, `missionRP`, pilot `Jumping` skill, variant
   `VariantSensor`. MCO saves and campaign progress files will not load here,
   and our logistics screens have no purchase or salvage UI.

4. **Raised limits.** MCO doubles or more most fixed tables; ours are still the
   Microsoft values. A large MCO map or a busy mission can overrun ours:

   | limit | ours | MCO |
   |---|---|---|
   | MAX_TERRAIN_OBJECTS | 3000 | 10000 |
   | MAX_TERRAIN_HEAP_SIZE | 1024000 | 65536000 |
   | MAX_WALL_OBJECTS / MAX_GATES_OBJECT | 2000 / 1000 | 4000 / 2000 |
   | MAX_GLOBALMAP_AREAS / DOORS | 10000 | 20000 |
   | MAX_CELL_COORDS | 5000 | 20000 |
   | MAX_MOVERS_PER_TEAM / MAX_WARRIORS | 120 / 120 | 150 / 160 |
   | MAX_SENSORS | 150 | 200 |
   | MAX_QUEUED_TACORDERS (per warrior) | 2000 (16) | 5200 (32) |
   | MAX_GLOBAL_PATH / MAX_STEPS_PER_MOVEPATH | 50 / 200 | 200 / 400 |
   | MAX_WAYPTS | 15 | 31 |
   | MAX_MC2_GOS_TEXTURES | 750 | 1500 |
   | MAX_STANDARD_FUNCTIONS (ABL) | 256 | 512 |
   | MAX_LIST_ITEMS | 128 | 512 |
   | MAX_KILL_AT_START / MAX_DISABLE_AT_START | 100 | 200 |
   | MAX_HEAPS / MAX_CACHE_SIZE | 256 / 40 | 512 / 200 |
   | MAX_GEN_ANIMATIONS | 5 | 10 |

   Some of these are on-disk or save-file sizes; raising them needs a check
   of every array they size, not just the define.

5. **Strings and data.** `Script1.rc` gains about 5470 lines and `resource.h`
   about 4600 (new unit, weapon and UI strings, IDs). Our strings come from
   `strings.res.cpp`; MCO content that refers to new string IDs shows blank
   names. MCO also ships its own `mc2res.dll` with changed weapon stats and
   new art (mechs, buildings, icons) in its data packs.

6. **Other engine work** (not needed to load a campaign): logistics screens
   rewritten (purchase, mech lab, mech bay), GUI list/button changes, terrain
   texture manager, pathing (`move.cpp`), mech appearance, AI and repair-truck
   behaviour, camera. Several backup files are checked in (`mech_old.cpp`,
   `contact_m1.cpp`, `terrtxm_*nogood.cpp`) and are not built.

## Test load: Clan Wars (2026-10-30)

`ClanWarsForMCOsetup.exe` (ModDB, 11.5 MB, MD5
`1282f28b441a67f7b75f116afd747cf4`, nicosuave, May 2016) is an Inno Setup 5.3
installer; unpacked with `innoextract`, not run. It holds only campaign data:
`data/campaign/clan wars.fit`, 28 missions (`innersphere1..28` `.fit` + `.pak`),
188 warrior brains (`data/missions/warriors/*.abl`) and 28
`data/textures/<mission>.burnin.jpg` colour maps. It installs over an
Omnitech 6.28 game and depends on it.

Findings:

- **ABL: no blocker.** The brains call none of the 18 MCO-only functions;
  everything they call is stock or defined in the stock brain library.
- **Unit data: blocker.** 2107 mission parts name 172 distinct `CSVFile` unit
  tables; only 30 exist in stock `misc.fst` (`data/objects/*.csv`). The other
  ~140 (Elemental, SalamanderArmor, Huntsman, Phantom, Goblin, copters...)
  ship with the Omnitech 6.28 base install (434 MB), as do the matching
  objects: `ObjectNumber` goes up to 2075.
- **Pathing format: blocker, fixable.** Loading `innersphere1` on stock data
  plus the campaign aborts in `GlobalMap::init` ("bad doorInfo count", then
  heap corruption). MCO change `//magic 07032012` widened `GlobalMapArea`,
  `GlobalMapDoor` and `DoorInfo` fields from `short` to `long`, and campaigns
  saved by its editor use the wide layout. MCO reads both by comparing the
  packet size with `sizeof(...Old)`; we read only the old layout and
  `readPacket` writes past the buffer. Size-checking the packet (and failing
  cleanly) is worth doing anyway: a bad map should not corrupt the heap.
- **Colour map: small.** MCO accepts `<mission>.burnin.jpg` as well as
  `.tga` / `.burnin.tga`; we look for `.tga` only. Needs a JPEG decoder
  (stb_image or SDL2_image).

So Clan Wars cannot run on stock data whatever the engine does: it needs the
Omnitech base data. Running it means the user installing Omnitech 6.28 (or
extracting its data) and pointing this port at that data, then working
through the engine list above.

## Scope if we do this

In: load and play Omnitech-era single-player campaigns on stock-format data,
on desktop GL and the Pi.

Out: MCO's editor, its DX11 work, multiplayer, MC2X content (no source).

## Approach

1. Pick one campaign (Clan Wars is the first fan campaign made for MCO) and
   run it with the game logging every ABL compile error and missing file.
   That gives the real list for one campaign instead of the snapshot's.
2. Raise the limits it actually hits, checking array users and save sizes.
3. Implement the ABL functions it calls, from their behaviour in MCO.
4. Logistics data: read `CSVFile`/variant tables and the new keys; purchase
   and salvage UI last, only if the campaign depends on them.

Rejected: porting MCO wholesale. It is Windows/DX8, its later releases are not
public, and its EULA terms differ from ours.

## Verification

The chosen campaign's first missions load and finish under `-autoexit` on
desktop GL and the Pi; stock missions still pass the 5-mission smoke.

## Open questions

- Which campaign first, and whether its data can be redistributed for CI (it
  probably cannot; test locally only).
- How much of 424-628 a given campaign relies on; only a test load tells us.
- Whether to ask magicX for the later source.
