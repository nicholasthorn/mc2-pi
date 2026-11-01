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

## Omnitech 6.28 install (2026-10-30)

`MCOsetup628.exe` (ModDB, 434.5 MB, MD5 `0c9fc1e191013ac01c8d36e40afe7068`,
Mar 2017), Inno Setup 5.3, unpacked with `innoextract` (not run) to 1.2 GB,
11046 files. Kept outside the repo (`~/mco628` in WSL); not redistributable
(MCO EULA plus Microsoft/WizKids assets).

What is in it, and what it is good for:

- **`version.txt`: full changelog**, 604 entries from MCO 0.030 (Jan 2011)
  to 0.628 (Mar 2017), written by magicX. Every new ABL command is documented
  with syntax, parameters and an example (`setTextMsg`, `objectchangesides`,
  `addmovertoplayer`...). This is the spec for re-implementing them.
- **`MCORel.pdb` / `EditRel.pdb`**: debug symbols for the 6.28 game and
  editor. `llvm-pdbutil-14 dump -types` gives exact struct layouts and every
  function name for the version whose source is not public. Examples:
  - ABL handlers: 35 `exec*` game commands beyond both stock and the 2013
    snapshot, so 6.28 has about 50 MCO-only commands in all. New since 2013:
    AddNewPilotToMover/ToPlayer, HirePilot, AutoRepairWithinRadius,
    RechargeRepairBay, IsRepairBay, IsRefit, NeedsRefit,
    ScanBuildingsForRepair/ForCapture, SetBuildingResourcePoints,
    Get/SetCampaignGlobal, ClearMoveOrder, HasPlayerOrder,
    IsCurTacOrderMoveOrder, GetCurTacOrderTime, GetTimeOfLastStep,
    GetAttackRange, NewDistanceToPosition, NumEnemies/FriendsWithinRadius,
    DamageMechArmor, DestroyMechBodyLocation, IsSelectedByPlayer,
    IsEnterPressed, SetLargeMsg, SetHelpMoveToPoint, SetMechGesture,
    SetMissionTuneId, SetRepairTruckEnabled, SetSalvageCraftEnabled,
    SetUnitForceGroup, SetBrainNew, ABLPrint. (`magicattack` from the
    snapshot is gone.)
  - On-disk pathing layouts (byte-packed): `_GlobalMapArea` 31 bytes
    (`sectorR`, `sectorC`, `numDoors` are `long`), `_DoorInfo` 5 bytes
    (`long doorIndex`), `_GlobalMapDoor` 73 bytes (`row`, `col`, `area[2]`,
    `areaCost[2]`, `numLinks[2]` are `long`). The stock layouts are smaller,
    so the packet size identifies the format.
- **Game data, mostly loose files** (only `art.fst` and `tgl.fst` are
  archives). Compared with stock source data (`mc2srcdata`) by path:

  | dir | new | changed | same |
  |---|---|---|---|
  | tgl (models, `.tgl`/`.agl`) | 6352 | 620 | 56 |
  | textures | 1684 | 8 | 49 |
  | missions (+ brains) | 871 | 81 | 21 |
  | objects (unit tables, `.fit`) | 357 | 72 | 2 |
  | art | 276 | 52 | 12 |
  | movies (`.bik`) | 163 | 30 | 2 |
  | sound | 68 | 6 | 32 |

  Formats are the stock ones: unit CSVs have the same layout as stock
  (`atlas.csv`: same rows and columns, rebalanced numbers, more variants, new
  `DescIndex` string ids); `.tgl` has the stock `0xBAFDECAF` header; `.agl`
  is the stock binary animation format. So the base data should load through
  our existing readers once ids, limits and pathing are handled.
- **`mc2res.dll`** (string table, ~3900 strings in the DLL) and
  `Script1.res`: the unit/weapon names the new `DescIndex` ids point at. We
  compile strings into `strings.res`; these would need extracting.
- **`data/missions/std.h`, `warriors/orders.h`, `*.abx`**: ABL headers and
  compiled brain libraries; `ablt.exe` (ABL compiler).
- **`How to create purchase and salvage files for MCO.txt`**: format of the
  per-mission purchase files (`[Components]` with optional `CompCountN`,
  `[Mechs]` with optional `MechVariantN`, `[Pilots]`); old files still read.
- Tools: `EditRel.exe` (editor), `viewRel.exe`, `AseConv.exe`, a purchase
  file editor.

With this data a test is possible: point a copy of our game at the 6.28 data
plus Clan Wars, then fix the loader issues in order (pathing size check and
conversion, `.burnin.jpg`, limits, string ids, ABL commands).

## Progress

Test setup (local only): `~/gd-mco` = stock game dir, then the 6.28
`art.fst`/`tgl.fst` and `data/`, then Clan Wars `data/`, all paths
lowercased. Run: `scripts/smoke.sh --game-dir ~/gd-mco --missions innersphere1`.

1. **Pathing: done** (`43ba20a`). `mclib/globalmapio.h` reads the stock or
   wide layout by packet size and rejects anything else before reading.
   Found on the way: Omnitech's editor saves the trailing
   `NUM_EXTRA_DOOR_LINKS` scratch slots of each door-link list with leftover
   memory (door index 725537101 in `innersphere1`); they are zeroed. All
   three maps of `innersphere1` load.
2. **ABL base-brain commands: added, not yet run** (`code/ablmc2.cpp`,
   "MechCommander Omnitech commands"). Omnitech's own base brains, which
   every MCO mission loads, need 10 commands; the first error list showed
   only 5 because ABL stops after too many errors. A scan of all 6.28
   scripts against our registered commands found the full set:
   `corebrain.abx`: `gettargetrelativeposition`,
   `getrelativepositiontotarget`, `newdistancetoposition`;
   `warriors/pbrain.abl`: `needsrefit`, `isrefit`, `hasorderfromplayer`,
   `numenemieswithinradius`, `scanareacapture`, `scanarearepair`,
   `autorepairwithinradius`. Written from the changelog and the brains'
   usage on stock APIs (no MCO code). Assumptions: `newdistancetoposition`
   is undocumented and treated as single-object `distancetoposition` in
   meters; scan/auto-repair orders use `ORDER_ORIGIN_SELF` so
   `hasorderfromplayer` stays false for them. Still open: a failed ABL
   compile is followed by a segfault instead of a clean stop.
   Commands used only by MCO's own campaigns (not Clan Wars), still
   missing: `addmoney`, `addmovertoplayer`, `addresourcepoints`,
   `damagemecharmor`, `destroymechbodylocation`, `removemoverfromplayer`,
   `setbrain`, `setmechgesture`, `setobjectivetype`,
   `setsalvagecraftenabled`, `settextmsg`, `settxtbuildingname`,
   `togglesalvagecraft`.
   Run 2026-10-31 (`4867483`, after the ABL table fix): the 10 compile.
   Adding them overflowed the 256-entry ABL standard function table and
   every mission, stock included, stopped at load; `MAX_STANDARD_FUNCTIONS`
   is now 512 (Omnitech's value). Next ABL error: `isselected`
   (`pbrain.abl` line 610), registered by 6.28 as `execIsSelectedByPlayer`,
   so the name-based scan missed it.
3. **Object types: next blocker, real work.** Every `ObjectNumber` in
   Omnitech and Clan Wars missions is >= 1188 (stock never exceeds 878),
   past the end of `object2.pak` (1188 packets in both). 6.28 defines them
   in `data/objects/movobj.fit`, one block per number from 1200:
   `[1318] l ObjectTypeNum = 2; st CSVname = "uziel"` (2 mech, 3 vehicle,
   1 building, 21 turret, 22 gate). 419 entries: 166 buildings, 144 mechs,
   82 vehicles, 19 turrets, 8 gates. CSVname is the mech CSV or the
   vehicle/building `.fit`; mechs have no `.fit`, only the CSV, so 6.28
   builds the mech type from the unit table (the 2013 snapshot has a
   commented-out "Magic 89" attempt in `Mission::init`). We stop with
   `ObjectTypeManager.load: bad objTypeNum` (1318): our table is 1024
   entries (`ObjectTypeManager::init` default), Omnitech's 4096. Needs: a
   larger table, `movobj.fit` lookup in `ObjectTypeManager::load` for
   numbers past the pak, and type construction from CSV/`.fit`.
   **Done on branch `omnitech-objtypes`:** table 4096 (the five MC1
   terrain handles stay at 1019-1023); `ObjectTypeManager` loads
   `objects/movobj.fit` if present and, for numbers past `object2.pak`,
   opens `objects/<CSVname>.fit` or, for a mech, builds its type FIT from
   the CSV (names = CSVname, explosion 41, destroyed 101, extent 12,
   radius 80, destruct/explosion damage by weight class as in every stock
   mech: <40t 35/15, <60t 45/20, <80t 55/25, else 65/30).
   `FitIniFile::open(parent)` reads from a memory-file parent
   (`File::getMemoryView`). `isselected` added. Clan Wars mission 1 now
   loads its units and starts (`[MISSION] innersphere1 started`).
4. **FST archives built on Windows.** Omnitech's `tgl.fst` and `art.fst`
   name entries `data\tgl\x`; `FastFile::open` now normalises to `/` and
   rehashes (branch). Separately, Omnitech's `tgl.fst` uses Microsoft's
   original LZ compression (version 0xFADDECAF, not zlib). **Decoder in
   (branch `fix/backport-lz-decoder`, back-port of the remaster's
   `7d6f9bc6` and `3b5f87ca`):** `LZDecomp` reads the original LZ as well
   as zlib, picked from the first two bytes, and raw entries
   (`size == realSize`) read. Omnitech's own `tgl.fst` now reads: all 4667
   entries decode to their exact size, 1947 byte for byte the stock ones,
   the rest differing only in float last bits and padding bytes, and
   mc2_01/mc2_03 run with it. The test setup still uses the stock
   `tgl.fst`, with Omnitech's new models as loose files; switching it is a
   separate step. Its `art.fst` is zlib and reads.
5. **`.burnin.jpg` colour maps. Done on branch `omnitech-objtypes`.** Clan
   Wars ships only `textures/<mission>.burnin.jpg` (100 maps, all square,
   baseline, 768 to 4096 px). The original engine already had the JPG branch
   in `TerrainColorMap::init`, disabled (`&& false`) because `DecodeJPG` was
   a stub on this port. `DecodeJPG` is now stb_image (v2.30, vendored
   unmodified in `thirdparty/stb`, JPEG only, 16384 px cap) through the
   GL-free `utils/jpeg_decode` (unit tested); the JPG is tried first as in
   the original code and a missing or undecodable one falls through to the
   TGAs, so stock data never reaches it.
   Found on the way:
   - Omnitech's `prefs.cfg` sets `TerrainTextureRes = 256` and its
     `textures.fit` names terrain tiles that exist only under
     `textures/256*`. At the stock 64, 128 tiles are missing. Release builds
     used to turn a missing texture into a zero-size node that segfaulted
     when cached in (`MC_TextureNode::get_gosTextureHandle`); now
     `loadTexture` logs `txmmgr: missing texture <name>` and the node draws
     nothing. A mod install must carry its `TerrainTextureRes`.
   - With both, Clan Wars mission 1 plays (300 frames at 256: no missing
     textures, about 52 fps under software GL).
   - At mission exit glibc reported `double free or corruption` deleting
     `theSky` (`TG_Shape::destroy`). The sky was only the victim: ASan
     showed `MOVE_readData` reading the special-area footprint packet raw
     into a 264-byte stock buffer, and Omnitech's `_GameObjectFootPrint` is
     528 bytes (four int32 fields, int32 cell pairs; same widening as the
     pathing structs). **Fixed on the branch:** the footprints go through
     `readPathRecords` (`narrowGameObjectFootPrint`), so a size matching
     neither layout is refused, and the count packet is size-checked.
     Clan Wars mission 1 now runs 300 frames and exits cleanly, ASan
     clean. The smoke script still flags it for the logged `STOP:` lines
     of the packet noise below.
   - "Texture not correct size!" (28 per load): the cached terrain
     transitions (`<type>.<overlay>.txm` in `data/textures`, one
     TERRAIN_TXM_SIZE square each) are named without their size and shared
     by every mission, so stock's 64 caches sit beside Omnitech's 256 ones.
     A quick load (all caches on the mission's `.lst` present) never loads
     the source tiles, so it cannot rebuild one. The old code STOPped and
     drew a part-filled buffer; a cache larger than the current size would
     have overflowed it (`LZDecomp` had no output bound). **Fixed on the
     branch:** `LZDecomp` takes an optional output limit; `TerrainTextures::
     init` turns quick load off if a listed cache has the wrong size (the
     full load rebuilds and rewrites them); an unlisted one of another size
     is resampled (nearest) to the current size; a corrupt one is drawn
     blank (a quick load cannot rebuild it). Clan Wars mission 1 now logs
     no STOP besides the packet noise.
6. **Mission editor import (idea, 2026-10-31).** When a mission editor
   lands (look at the Remaster and Omnitech editors for inspiration), an
   import step can convert mod data that the engine cannot read in a
   backwards-compatible way (to stock formats, e.g. TGA), so the engine
   only has to read stock formats. JPEG colour maps did not need this:
   reading them was the original engine's own code path and is invisible to
   stock data.
7. **"requested packet number N is bigger than number of packets" (92 per
   Clan Wars load). Fixed on the branch.** Not harmless:
   - `Mission` read ground vehicle / elemental profiles by seeking the type
     number in `object2.pak` (11), so Omnitech vehicles spawned without
     their profile; `LogisticsData::addVehicle` / `addBuilding` did the same
     (80), so Omnitech's purchasable vehicles and buildings were missing.
     Both now use `openTypeProfile` (`code/movobj.cpp`, also built by the
     viewer): the pak packet, or the loose `.fit` that `movobj.fit` names.
     The `movobj.fit` lookup moved there too and opens the file on first
     use (logistics runs before `ObjectTypeManager::init`).
   - Omnitech's own `sound.snd` lists 126 bites and its `sound.pak` holds
     125; `preloadSoundBite` skips the missing one quietly (1).
   Clan Wars mission 1 now passes `scripts/smoke.sh` (no error lines).
8. **Omnitech's strings. Done on the branch.** Its names and descriptions
   for new content live in its Windows `mc2res.dll` (PE32, 4281 strings,
   ids 10000 to 60049); Clan Wars mission 1 asked for 20 missing ids, its
   new pilots' callsigns (30924 BARON to 30942 THUNDER) and 30000.
   `gos_GetResourceString` now falls back, for ids the game's own table
   lacks, to the string table of a `mc2res.dll` in the game folder, read at
   run time by `utils/pe_strings` (bounds-checked PE resource reader,
   UTF-16 to Windows-1252, unit tested). Nothing of Omnitech's is copied
   into the repo, and stock text is never replaced. Only 30000 is still
   missing; Omnitech's own DLL lacks it too.
   A mod install therefore needs `mc2res.dll` in the game folder, as well
   as its `TerrainTextureRes`.
9. **In play (2026-10-31).** Clan Wars mission 1 under `-bench` with a
   scripted fight (3 player, 12 enemy units; headless Xvfb at 800x600,
   llvmpipe), screenshots at three heights. The JPEG colour map and the
   minimap draw correctly. Found and fixed on the branch:
   - **Untextured (white) units.** Omnitech's `prefs.cfg` sets
     `ObjectTextureRes = 512` and ships object textures only in
     `data/tgl/512`; at the stock 128 they were silently not found.
     `objectTexturePath()` (`txmmgr.cpp`, used by every actor's texture
     load) takes the configured size when the texture is there, as for all
     stock data, else the largest size that has it.
   - **Heap overrun on 512 textures.** `MAX_LZ_BUFFER_SIZE` held one 256
     texture and the size check was a `gosASSERT`: Omnitech's 512 textures
     overran it and crashed later in `ObjectTypeManager::get`. The buffer
     now takes 512x512, a texture that cannot fit is logged and drawn as
     missing, and the cache-in decompressions are bounded.
   - **Pink fog.** The fog colour (D3D 0xAARRGGBB) reached the shaders with
     red and blue swapped (`uint32_to_vec4`; vertex colours are swizzled,
     fog was not). Stock fogs are near-grey so it hardly showed; Clan Wars'
     light blue (0xA0A5FF) came out pink at the map edge. Fixed where
     `fog_color_` is set; stock mc2_19 checked by screenshot.
   A mod install should still carry its `ObjectTextureRes` (512) and
   `TerrainTextureRes` (256) for full detail.
   Not checked: the logistics screens with Omnitech's units (needs menu
   input, not scriptable headless yet; see `UI-SMOKE.md`).
10. **Every Clan Wars mission (2026-10-31, branch `omnitech-followups`).**
   All 28 (`innersphere1`-`28`) pass a 300-frame headless smoke at
   Omnitech's texture settings (`TerrainTextureRes 256`,
   `ObjectTextureRes 512`). Ten failed at first, all in data Omnitech's own
   engine never checks:
   - special-area footprints with an unset `preNumCells` (missions 5, 11,
     16, 19, 25): never read by the game, now stored as 0 when it does not
     fit;
   - leftover memory in the link lists of the pathfinder's start and goal
     doors (`numDoors + DOOR_OFFSET_*`, missions 7 and 13): scratch filled
     in per search, now zeroed like each door's trailing scratch slots;
   - more than 119 pilots (missions 21, 27, 28; up to 137 parts):
     `MAX_WARRIORS` raised to Omnitech's 160 (it only sizes the static
     warrior list; no save format). Each team's roster stays at
     `MAX_MOVERS_PER_TEAM` (120, in the save format) and is now bounded:
     movers past it are left off and logged (none in Clan Wars: at most
     102 per team).
   Found by review on the way: the start/goal door link lists are sized
   from the counts stored in the map, and setStartDoor / setGoalDoor write
   one link per door of an area into them. `GlobalMap::init` now grows
   them to the most doors any area has (what `write()` stores), so a stale
   count in an Omnitech map cannot make a path search overrun.
   Memory: `[TXMCACHE]` logs the compressed textures held (a plain malloc
   here, not capped at `TEXTURE_CACHE_SIZE` because of `USE_GOS_HEAP`) and
   `[MEM]` the peak resident set at autoexit. Clan Wars peaks at 72-92 MB
   of compressed textures (stock missions 12 MB) and 360-860 MB resident
   under llvmpipe, which varies run to run (stock 290-375 MB). Fine on a
   4 GB Pi 400 in RAM terms; GPU texture memory and `MAX_MC2_GOS_TEXTURES`
   (750; Omnitech 1500) on V3D are not measured.
   The 68 missions of Omnitech's other campaigns (`mw2_*`, `e_*`, `a_*`,
   `x_*`) have not been run.
11. **Omnitech's other campaigns (2026-10-31, branch `omnitech-campaigns`).**
   The 68 missions outside Clan Wars (`mw2_*`, `e_*`, `a_*`, `x_*`, `p`,
   `e3demo`): 45 passed a 300-frame smoke at first. The rest:
   - **13 Omnitech-only ABL commands** (21 missions; 12 use
     `addmovertoplayer`), now in `code/ablmc2.cpp`, written from the 6.28
     changelog and the missions' calls on stock engine calls:
     `addmovertoplayer` / `removemoverfromplayer` (the stock `tradeMover`
     steps: team with sensor and pilot, roster, group, mover lists; plus the
     force bar, control, old orders cleared; not salvage, single player
     only; the joining unit gets `pbrain`), `setbrain`,
     `settextmsg` (message lines, `*` = new line; mode 1's "wait for
     Enter" box and the duration are not done), `addresourcepoints`,
     `addmoney`, `setsalvagecraftenabled` / `togglesalvagecraft` (a
     per-mission override of the support option), `settxtbuildingname`
     and `setfixedbuildingrp` (per-building name / capture RP; buildings
     only, not turrets or gates), `damagemecharmor`,
     `destroymechbodylocation`, `setmechgesture` (the id read as a mech
     state, 7 / 8 fallen forward / backward, which fits Omnitech's "7
     (fallen backward)" better than the gesture table); plus the stock
     `setobjectivetype` / `checkobjectivetype`, registered (they act on the
     old objectives array, a no-op like `setobjectivestatus`). Brain
     changes are queued and applied at the start of the next
     `Mission::update`: compiling a brain resets the ABL compiler's
     globals, and the calling unit may be running the brain being
     replaced. Checked in play by injecting calls into mission `mw2_n_0`
     (test copy): the APC joined the force bar, both brains switched, the
     message showed on two lines.
   - **Paletted TGAs**: sky 12 (`tgl/512/sky12_*`, missions `e_4_7`,
     `mw2_n_6`) is 8-bit colour-mapped, which `Image::loadTGA` refused;
     now expanded through the palette.
   - **Weapon damage over 255** (`mw2_n_0`): the `WeaponShotInfo` check is
     the multiplayer packing limit; single player now only requires it
     non-negative.
   - **More than 60 off-map areas** (`mw2_8_1`): `offMapAreas` overran the
     `GlobalMap` object (found by ASan); `MAX_OFFMAP_AREAS` 120 as in
     Omnitech, and the fill is bounded.
   The test folder now uses Omnitech's own `tgl.fst` (readable since #31).

12. **Front-end screens (2026-10-31, branch `omnitech-ui`).** Mission
   smokes start in the mission, so no Omnitech screen had been opened; the
   UI smoke walks (#30 / #32) crashed at the main menu on Omnitech data.
   Omnitech ships its own `data/art/mcl_*.fit` layouts, and its code moved
   the fixed indexes to match (its "magic 19092011" changes):
   - **1024x1024 art.** `mcl_mm_o.tga` (main menu) is 1024x1024; the LZ
     buffers held 512x512, so it was refused and `aObject::init` then
     locked texture handle 0. The buffers now take 1024x1024, and
     `gos_LockTexture` reports an empty 0x0 texture for handle 0.
   - **Briefing (`mcl_mn.fit`):** 4 statics, not 36. Map, video window,
     icon strip and cover are 0-3 (Omnitech's code) or 32-35 (stock),
     chosen from the static count.
   - **Loading screen (`mcl_loadingscreen.fit`):** no animated panels; the
     `AnimObject18` text is optional.
   - **Pilot ready (`mcl_pr_layout.fit`):** 40 statics; the pilot picture is
     28, not 67 (writing 67 overran the array and crashed at exit), and
     the stock-only cover hack on 27 / 28 is skipped.
   - **Mech lab (`mcl_mc.fit`, branch `omnitech-mechlab`):** 15 statics,
     not 56: the same pieces renumbered (weapon info icons 29-34 are 1-6,
     jump jet / armor icons 47-49 are 7-9, jump jet outline 54 is 11, heat
     sink icon 55 is 12), chosen from the static count as Omnitech's code
     ("magic 18022012") has them. The four-piece mech picture 50-53 is gone:
     Omnitech's static 10 there is a black area (its code still sets the
     picture on it, which would stretch a 64x64 corner of it), so we leave
     it alone. Its static 13 is an ammo icon for Omnitech's own ammo
     readout, which we do not have, so it is hidden. "Change loadout"
     in the mech bay works again. Omnitech's other mech lab changes (repair
     buttons, ammo components, the variant name text) are not ported.
   - Other layouts (main menu, mission selection, mech bay, purchase, mech
     info) stay within their counts; whether every in-range index is the
     same piece was not checked.
   Walks now pass on Omnitech data, also under ASan: main menu, the four
   options tabs, new campaign, mission selection, briefing, mech bay, pilot
   ready, mech purchase, mech lab. "Buy" did add a mech: the walk's single
   mission (`a_0_2`, like stock's `mc2_02`) starts with an empty bay and a
   65 t drop weight, and the walk bought the first variant, an Atlas, which
   then could not join the force. The UI smoke driver's "buy" (#32) needs
   to pick the first variant that fits instead (#39).
   - **Screen size.** The front end ran in an 800x600 mode, and Omnitech's
     layouts reach 1194x896, so the right and bottom of its screens were
     cut off. Omnitech's own code runs the front end at 1280x1024 (its
     `[HiresOffsets]` 320,28 centre that on a 1920x1080 desktop; 6.28
     uses the desktop size). `LogisticsScreen::frontEndWidth()/Height()`
     now give 1280x1024 when `mcl_mm.fit` has a `[HiresOffsets]` block
     (no stock layout has one), else 800x600; the mode switches into the
     front end, the load screen and the slide animations (stock's 800/600
     pixel slides, scaled) use them. Omnitech does not ship every layout:
     it keeps stock's dialogs (single mission, new campaign / load / save,
     OK and one-button prompts, legal, mech lab variant save, map info),
     laid out for 800x600. `LogisticsDialog::centreOnFrontEnd()` moves
     each one, with the list box it keeps outside its arrays, to the
     middle of the screen each time it opens (the OK prompts also open in
     a mission, at the mission's resolution); a dialog layout with its own
     `[HiresOffsets]` block stays put, and stock data moves nothing. Omnitech ships its own results
     layout (`mcui_mr_layout.fit`). Still top left: the Mechlopedia
     (`mcl_en*`) and the multiplayer screens. Still open: front-end movies (credits, `playFullScreenVideo`) keep
     their 800x600 sizes and stretch; and the load screen picks its enter
     or exit half by comparing the screen width with the front end's, so
     a mission mode as wide as the front end keeps the enter half (as
     stock does with an 800x600 mission mode).

13. **Omnitech campaign packs (2026-10-31).** Nine 2012-2016 packs on
   ModDB (`moddb.com/mods/mechcommander-omnitech/downloads`), Inno Setup
   installers holding only data, unpacked with `innoextract`. Desert Fox
   (5 missions, `desfox1..5`): installed over the Omnitech folder with
   lower-case names, no clashes; all 5 pass a 300-frame smoke. Not yet
   downloaded: Carver V (both), Wolf Dragoons, Clan Eagle, Day of Heroes,
   Laskos Legion, Volstand, Mercenaries Star.

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

Test loads: Clan Wars (Omnitech) is the first. For MC2X, use Dark Gray:
Borderworlds (MC2X r20): it is the campaign that fits MC2X best.

## Open questions

- Which campaign first, and whether its data can be redistributed for CI (it
  probably cannot; test locally only).
- How much of 424-628 a given campaign relies on; only a test load tells us.
- Whether to ask magicX for the later source.
