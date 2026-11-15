# wolfman-MC2X content on this port

Status: built, 2026-11-15, on `pi-port` (PRs #157, #164, #165, #166). Built:
the loader fixes, the ABL commands MC2X's missions call, the fixes the
sanitizer runs found, the in-mission interface and the front end on MC2X's
own layouts, MC2X's own HUD features (formations, the 22 support units,
C-Bills and the spy map, the buttons for selected units), operation maps of
up to 32 missions, and the import script; all 32 missions run 300 frames,
and a REAP3 campaign plays from the main menu through logistics, a mission,
salvage and pilot review. Not built: `TacWaterColor`, the per-type deploy
switches and `max<n>tons` (undocumented), the ABL commands no mission calls,
and CVE-G (its download could not be fetched). The HUD features are checked
by unit tests and smoke runs only; the hands-on checks are in
`planning/HARDWARE-TESTS.md` ("MC2X").
Companion to `MOD-COMPAT.md` (MechCommander Omnitech), whose work this leans
on.

## What MC2X is

wolfman-MC2X (wolfman-x, mc2x.net) is a Windows game and mission editor built
from Microsoft's MechCommander 2 source, extended over about 15 years with
content and features largely from magicX, the Omnitech author ("THERE IS A
LOT OF MAGIC IN MC2X", its readme). No source is published. Its terms:
non-commercial, under Microsoft's shared source licence. It is a stand-alone
install: its own `MC2XGame.exe`, FSTs and `data/`, not a mod over stock MC2.

Releases (ModDB, `moddb.com/mods/wolfman-mc2x/downloads`):

| File | What | Size | MD5 | Got it |
|---|---|---|---|---|
| `MC2X.1.exe` | MC2X R70 (Oct 2026): game, editor, 7 test missions | 1,005,169,355 | `de0f450cab1f4382f437e9824ab62a9d` | yes, MD5 matches ModDB |
| `MC2X-REAP3-G.4.exe` | REAP3-G R05 (Jul 2026): game core R68, the 7 missions, 24-mission REAP3 campaign | 1,077,278,503 | `ae9d81407f875699207d1b1e5083e6b9` | yes, MD5 matches ModDB |
| `MC2X-CVE-G.7.exe` | CVE-G (Sep 2021): Carver V on MC2X | 1,035,539,884 | `948a681ad6ba78a2b728bab91f6d8fbf` | no: ModDB's download did not start in the browser; GamePressure has an older 755 MB build (2017) |

**Dark Gray: Border Worlds** had no copy to get when this study started (its
ModDB page listed no file and its 2013 site is gone). The user later found
v1.4, `mc2_DarkGray_Borderworlds_v1.4.zip` (441,880,474 bytes, MD5
`6019fe37e48b9ff8d660ef01ab9d1458`): a whole MC2X R20 install with the
campaign (5 missions, 6 purchase files). See Progress 19. REAP3 (24
missions, built by wolfman on core R68) stays the main MC2X test campaign.

The installers are **Clickteam Install Creator**, not Inno Setup, so
`innoextract` and 7z do not read them. They were unpacked without running
them by a short Python reader written for this study (kept in the session
scratchpad, not the repo): overlay signature `77 77 67 54 29 48`, blocks of
`u16 id, u16, u32 size`, block `0x143a` the file list (bzip2), `0x7f7f` the
data, each file a type byte (1 zlib, 2 bzip2) and the stream. Every file
decoded to its listed size (2347 in R70, 751 in REAP3).

Local copies (WSL, never in the repo or CI): `~/mc2x-dl/` installers,
`~/mc2x-r70`, `~/mc2x-reap3` unpacked, `~/mc2x-work/*-fst` FST contents,
`~/gd-mc2x` the test game folder.

## What is in it

### Docs

- `MC2X Readme.pdf` (R70): install notes, credits, resolutions 1280x720 to
  2048x1536 (no 800x600 or 1024x768 game screens), "not compatible with any
  other MC2 mod", no multiplayer, no in-game video (so no Carver V in the base
  install).
- `MC2X Advanced Scripting Options.pdf`: the ABL side. `mc2xcore.abx`
  (core library, replaces `corebrain.abx`), `mc2xplay.abl` (the brain every
  player unit gets), `mc2xscript.abl` (mission script template); eternal
  flags `noAttackCode`, `wxmCode`, `fuelCode`, `repairCode`, `jammerCode`,
  `wmxid`, `mapsize` (60 to 300); a standard objective set for intro and
  outro camera moves; and about 70 ABL commands (list below).
- Not in R70 despite the readme naming them: `MC2X WarriorBrains.pdf`,
  `MC2X MechSwapper Documentation.pdf`, the terrain overlay PDFs.

### Files against stock

FST contents plus loose files, compared by path with a stock game folder
built from `mc2srcdata` (REAP3; R70 is within a few dozen files of it):

| dir | new | changed | same | stock only |
|---|---|---|---|---|
| tgl (models) | 7692 | 2848 | 1804 | 15 |
| art (UI) | 1230 | 122 | 298 | 246 |
| textures | 378 | 13 | 628 | 4584 (stock's cached `.txm`) |
| missions (+ brains) | 440 | 50 | 42 | 1252 (stock missions) |
| objects | 246 | 35 | 0 | 7 |
| sound | 198 | 40 | 30 | 242 |
| movies | 111 (TGA stills) | 0 | 0 | 0 |
| terrain, campaign, cameras, effects | 13 | 1 | 0 | 4 |

16275 files against stock's 12316.

### Formats

All stock formats, so our readers take them:

- FST: `0xCADDECAF` (zlib) for all but `tgl.fst`, which is `0xFADDECAF`
  (Microsoft's LZ, read since the Omnitech work). One entry,
  `tgl/128/medwall5.tga`, is not valid LZ in MC2X's own archive; it is logged
  as a missing texture.
- `.tgl` header `0xBAFDECAF`, `.agl` `0xBADDECAF`, TGA and `.burnin.jpg`
  colour maps (read since Omnitech), WAV 16-bit PCM.
- Unit CSVs: stock layout (14 columns), more variant rows (`atlas.csv` 600
  lines against 309), new `DescIndex` ids.
- Mission `.fit`: stock keys plus `CSVFile` per part (as Omnitech), deploy
  and support-unit switches in `[MissionSettings]` (`MechDeploy`,
  `DroneEnabledDefault`, `max1tons`... 32 keys), objective
  `FailureAction` blocks and `InfoMsg`, `[Water] TacWaterColor`, forest
  `TreeType15-17`. Unknown keys are ignored, so these lose features, not
  loads.
- Objects: **MC2X extends `object2.pak` itself** (1483 packets against
  stock's 1188; mission `ObjectNumber`s up to 1481). No `movobj.fit`. Our
  4096-entry type table (from the Omnitech work) holds them.
- Strings: `mc2xres.dll`, PE32, 4617 string ids our table lacks.
- Configs: MC2X renamed the blocks (`[wolfman-MC2X]` in `prefs.cfg`,
  `mc2x_system.cfg` with `[mc2xSystemHeap]` and `[mc2xSystemPaths]`), so a
  test folder uses our stock config files.

### Renamed files

The pattern behind most of the blockers: MC2X renamed files the engine opens
by name.

| stock name | MC2X name | status |
|---|---|---|
| `effects/mc2.fx` | `mc2x.fx` | fallback built |
| `missions/corebrain.abx` | `mc2xcore.abx` | fallback built |
| `objects/compbas.csv` | `mc2xcomp.csv` (+ `mc2xcomplist.csv`, a readable list) | fallback built |
| `mc2res.dll` (Omnitech's) | `mc2xres.dll` | fallback built |
| `art/buttonlayout<w>.fit` | `mc2x_buttonlayout<w>.fit`, 19 widths 1024 to 3840 | fallback built (`FitIniFile::open`, Progress 16) |
| `art/mcl_*.fit`, `mcui_keyref_*` | `mc2x_*` (main menu, mission selection, briefing, mech bay, pilot ready, loading screens, dialogs, Mechlopedia, options, key reference) | fallback built (`FitIniFile::open`, Progress 17) |

## Progress: the test load

Test folder `~/gd-mc2x`: the REAP3 install, paths lowercased, plus this
port's binary, shaders and stock `.cfg` files. Run:
`scripts/smoke.sh --game-dir ~/gd-mc2x --missions metropolitan --frames 300 --ui ""`.
Blockers in the order they appeared on R70's `metropolitan`, then across
all 32 missions (R70's 7, REAP3's 24 and `reap3-x`), 300 frames each,
desktop GL under llvmpipe. Items 1 to 11 are as found at the test load:
every one marked "not built" there is built since (see "Effort").

1. **`mc2.fx` not found** (STOP at start-up). **Fixed on the branch:**
   `mechcmd2.cpp` and `txmmgr.cpp` fall back to `mc2x.fx`. MC2X's effect
   library loads as it is.
2. **Special-area footprints, 520 bytes each** (STOP in `GlobalMap.init`,
   284 records in `metropolitan`). A third layout beside stock's 264 and
   Omnitech's 528: MC2X keeps the stock `short` fields and doubles
   `MAX_GAME_OBJECT_CELLS` to 128 (real footprints use at most 4 cells).
   **Fixed on the branch:** `readFootPrints` in `globalmapio.h` takes all
   three by packet size, refuses any other size and any footprint with more
   cells than the stock list (unit tested). MC2X's other pathing packets are
   the stock layout.
3. **`corebrain.abx` not found** (STOP). **Fixed on the branch:**
   `Mission::init` loads `mc2xcore.abx` when there is no `corebrain.abx`.
   (`saveload.cpp` has the same code compiled out by `USE_ABL_LOAD`; a save
   restores its libraries by name.)
4. **ABL commands** (compile STOP in `warriors/mc2xplay.abl`, the brain every
   player unit gets, at `teleporttopoint`). Not built; see "ABL" below. A
   local experiment (never committed) registered the missing ones as no-ops
   to find what lies past them.
5. **No component table** (STOP "Mech Challenger has ammo for a weapon it is
   not carrying"). MC2X renamed `compbas.csv` to `mc2xcomp.csv`; the load
   failed silently (only a debug `gosASSERT` checked it), so every unit had
   blank weapons. **Fixed on the branch:**
   `MasterComponent::tableFileName` picks `compbas.csv`, else `mc2xcomp.csv`,
   at all four sites (mission, save load, logistics, the multiplayer file
   hash), and a table that cannot be read now STOPs with its name.
6. **`mc2xres.dll`** (no error, blank names): our string fallback read only
   `mc2res.dll`. **Fixed on the branch:** it reads `mc2xres.dll` when there is
   no `mc2res.dll` (4617 strings).
7. **`buttonlayout640.fit` not found** (STOP, the in-mission HUD). MC2X
   ships only `mc2x_buttonlayout<w>.fit` (19 widths, 1024 to 3840; nothing
   for 800x600). Not built; see "Interface".
8. **Crash in `KeyboardRef::init`** with no `mcui_keyref` layout: the F1 key
   list read rects from a layout that never opened. **Fixed on the branch:**
   a missing layout, or one with fewer than 3 rects or no button, is logged
   (`PAUSE`) and the key list stays empty. Stock data never reaches it.
9. **`mcl_dialog.fit` not found**, and more renamed layouts after it. For the
   experiment the test folder got stock's 240 art files that MC2X lacks
   (local only). With that, **`metropolitan` ran 300 frames.**
10. Across all 32 missions with the experiment's stubs, three more classes:
    - **"String too long for buffer"** (11 missions): 12 of MC2X's mission
      strings (ids 34110 to 34160, 159 to 726 characters) overflow buffers of
      65, 127 and 254 bytes. `cLoadString` cuts them, but STOPs first, by
      design (`STRING-OVERFLOW-TRIAGE.md`). Needs the callers' buffers found
      and sized; not built.
    - **"Mover.setMoveType: bad moveType 2"** (9 missions): MC2X's
      `object2.pak` gives three of its new support units move type 2 (Drone
      packet 1367, JamFly 1465, Kamikaze 1479); stock has ground (0) and air
      (1). Its meaning is not documented; the experiment treated it as air.
    - **`newpower` with two arguments** (11 missions): stock registers
      `newpower(boolean)` for the calling unit; REAP3 scripts call
      `newpower(partId, false)` on another unit. ABL signatures have a fixed
      arity, so MC2X changed it; supporting both needs the ABL compiler to
      accept either form.
11. With those three also stubbed (local only): **28 of 32 pass 300
    frames**, among them 23 of REAP3's 300x300 maps. The other four:
    - `reap3m11`, `13`, `14`: `toggledeployer`, one more command.
    - `reap3m02`: glibc "free(): invalid pointer" in ABL `routineExit` for the
      mission script: the stack slot of its local array `aPoint` holds
      `0x555500000001`, the top half of a 64-bit pointer with a 32-bit `1`
      written over the bottom half. Something stores an integer one slot off.
      Not caused by the stubs (the script calls none). Moving the next local
      (`boolean PerimeterBreach`) above the script's `eternal` block did not
      change it. Open; possibly the remaster's note about old scripts whose
      stack frames land on the wrong slot. Needs an ASan run and the ABL
      debugger.

Also seen, harmless: `textures/64/detail_10000.tga` missing (MC2X's
per-mission detail textures are named `<mission>.detail.tga`; REAP3 ships
no `detail textures` folder), and the one bad `tgl.fst` entry. No
"requested packet" or missing-string noise.

12. **ASan + UBSan (2026-11-14, step 1 of the recommendation)**, all 32
    missions with the same local stubs. Fixed on the branch, each a bug a
    short or odd file could hit with stock data too:
    - **WAV parse past a sound bite** (every mission, at start-up): MC2X's
      `sound.pak` holds a 1-byte bite, and `wave_ParseWaveMemory` read a
      12-byte header from it; it had no buffer size at all.
      `wave_ParseWaveMemorySized` bounds every read by the packet size, cuts
      a data chunk that runs past the end, and is used where the size is
      known (sound bites, Betty); Betty and the support radio now also skip
      a bite that does not parse instead of dividing by its channel count.
      Unit tested.
    - **In-memory `File` reads past the end** (`cityassault`): a MC2X model
      in `tgl.fst` is 25 bytes shorter than `TG_TypeShape::LoadBinaryCopy`
      reads. `File::read` and the fixed-size readers of an in-memory file
      (every FST entry is one) now stop at its end and zero the rest. Unit
      tested. Which model it is was not tracked down.
    - **ABL string assignment read past its source** (`redrockoutpost`): a
      warrior brain fills a `char[12]` to its last byte, and assigning it
      used `strlen`. The length is now bounded by the source array's size.
      Unit tested (ABL compiled and run in the test; the over-read shows
      under ASan only).
    - **Turret and gate control buildings that are not buildings**
      (`cityassault`, `reap3m19`, UBSan): `findByCellPosition` returned the
      first object of any kind in the linked cell, and the code cast it to
      `Building` and wrote a list index into it. The lookup takes an object
      class now, the seven link sites ask for `BUILDING`, and the turret,
      gate and building `parent` lookups no longer dereference NULL when
      there is none (the existing PAUSE logs it).
    Result: **28 of 32 pass 300 frames under ASan + UBSan** (all but
    `reap3m02` and the three missions that need `toggledeployer`), with 23
    of the 300x300 maps among them. Stock `mc2_01`, `mc2_03` and Omnitech's
    `innersphere1`, `innersphere5`, `a_0_2` still pass.
13. **Part ids on 300x300 maps: MC2X keeps the stock multiplier.** The probe
    logged what `settxtbuildingname(248029, "Drone Command Center")`
    (`reap3m06`) finds: read as `4096 + row * 720 + col` it is cell
    (338, 573), and the building there is the Drone Command Center; read
    with 900 there is nothing. So MC2X kept `MAX_MAP_CELL_WIDTH` 720 for ids,
    and so does this port: stock scripts' building ids stay valid. Columns
    720 to 899 alias the next row's ids in MC2X as well. **The trigger map**
    (`TriggerAreaManager::map`, tiles of 3 cells) was 240 tiles wide with
    only release-off `gosASSERT`s on its bounds; it is now 300 tiles
    (`TRIGGER_MAP_TILES`) and `add` / `remove` clamp to it.

14. **ABL commands and `reap3m02` (2026-11-14, steps 2 and 5).** The ten
    commands the 32 missions load are built (table under "ABL"), and
    `newpower` takes MC2X's (part id, boolean) form through a new ABL
    feature, optional trailing parameters (`"?|b"`, `ABLi_moreParams()`).
    `reap3m02`'s corruption was a stock bug: `setobjectdamage` is
    registered with a result but pushed none, so each call as a statement
    took one slot too many off the ABL stack and a later push landed in the
    mission script's frame (a probe watched the slot of its local `aPoint`
    change at `setobjectdamage(4820, 100)`). REAP3 calls it 408 times; stock
    once. A scan of every registered command found the same in `stopmusic`
    (475 REAP3 calls, 5 stock), `setradio` and `sortweapons`, and three that
    poked a result without peeking their last argument (`setsalvage`,
    `setsalvagestatus`, `addprisoner`). All fixed, and the executor now
    evens the stack after every command and logs one that breaks the rule.
    Both ABL changes are unit tested with ABL compiled and run.
    **Result: all 32 missions pass 300 frames under ASan + UBSan** with no
    script stubs; the only local workarounds left are move type 2 treated
    as air, long strings logged instead of stopping, and stock's UI art in
    the test folder.

15. **Long names and drones (2026-11-14, steps 3 and 4).** The 12 long
    strings are unit types' `DescIndex`, a display name in stock data,
    which MC2X points at encyclopedia texts (up to 590 characters):
    `cLoadStringIfFits` loads a name only if it fits, and otherwise a mech
    keeps its CSV name and a vehicle its type's `[ObjectType] Name`. Move
    type 2 is documented in MC2X's own data, "2 = drone"; it is read as air
    where mech and vehicle types load (`moveTypeFromData`).
    **Result: all 32 missions pass 300 frames under ASan + UBSan from a
    build with no local changes at all.** The only workaround left is in
    the test folder: stock's interface art beside MC2X's renamed layouts
    (step 6).

16. **Campaign variables and the in-mission interface (2026-11-14, steps 2
    and 6, mission part).** The campaign variables are saved with the
    campaign (`code/campaignglobals.cpp`, an `MC2XCampaignGlobals` block
    written only when one is not 0). For the renamed layouts,
    `mc2xArtName` (`mclib/mc2xart.cpp`) gives MC2X's name for a stock
    one (step 17 moved the fallback into `FitIniFile::open`). MC2X's HUD files place the HUD in the same 1024x768
    centre column as stock's, so they are read as stock's: the first 20
    command buttons (same numbers) and stock's 8 support buttons from
    where MC2X has them among its 22 (`mc2xVehicleButton`), with stock's
    ids. MC2X has no layout under 1024 wide; `smoke.sh --screen` runs the
    test folder at 1024x768. A screenshot of `metropolitan` shows MC2X's
    HUD art in place.
    **Result: all 32 missions pass 300 frames under ASan + UBSan with no
    stock art in the folder and no local changes.** MC2X's extra buttons
    (formations, its 14 other support units) are not shown.

17. **The front end (2026-11-14, step 6).** The fallback moved into
    `FitIniFile::open`: a `.fit` in the art or camera folder that is not
    there is opened under MC2X's name (`mc2xArtName`, now in `mclib/`,
    with the names MC2X shortened, `mcl_gn_deploymentteams` to
    `mc2x_deploymentteams`, and `cameras.fit` / `colors.fit` to
    `mc2xcams.fit` / `mc2xcols.fit`), so every screen finds its layout
    with no change at its call site; `openedMC2XName()` tells the HUD.
    Two data files MC2X renamed are chosen the same way:
    `art/mc2x_inv.csv` for `buildings.csv` (logistics, the Mechlopedia,
    the network hash), `objects/mc2xpilots.csv` for `pilots.csv` (no
    pilots, so pilot ready could not launch). MC2X keeps its campaigns
    only in `mission.fst`: the new campaign dialog now also lists a
    campaign `.fit` found in a fast file, its entries as tall as their
    text ("Test Mission Collection" wrapped over the next one). A mech whose name id points at
    a long encyclopedia text takes its CSV name, under a new string id,
    so every later lookup shows the name.
    MC2X's front-end layouts mostly have stock's counts; the mech bay
    (20 buttons, stock 7) and pilot ready (80 statics, stock 74) have
    more, and are read by stock's indexes, which hold.
    **Result: with no stock art in the folder, a UI walk of REAP3's
    campaign passes under ASan + UBSan**: new campaign, save and load,
    briefing, mech bay, purchase, mech lab (swap, own, accept), pilot
    ready, launch, the mission's options, win, salvage, pilot review and
    a second launch. MC2X's extra front-end buttons do nothing.

18. **Played on the Quest (2026-11-14).** An MC2X copy of the Quest app
    (package `com.nicholasthorn.mc2.mc2x`, its own data folder, as the
    Omnitech one) was played through REAP3's first missions and
    `metropolitan`. What the play found, all fixed:
    - **Long texts as mech names, MC2X's wording missing:** `mc2xres.dll`
      is MC2X's whole string table and reuses stock ids; its text now
      wins over the game's table (Omnitech's `mc2res.dll` still only
      fills gaps).
    - **Wrong mech icons:** `mc2x_mechicons.tga` is 512 wide, 20 a row.
    - **No pilot portraits:** they are `mc2x_<pilot>.tga`.
    - **White square on pilot ready:** MC2X's layout has a mission map
      static (78), filled as Omnitech's (38) is.
    - **White box before the logo:** MC2X's intro is `mc2x_screenintro.fit`;
      it also ships a stale `mcl_splashscreenintro.fit` whose art it lacks.
    - **Half-drawn mechs in the menus** (Fafnir, five times an Atlas, also
      some of Omnitech's): the logistics TGL pools were 2000 vertices,
      now sixteen times that.
    - **Minimap off the world:** MC2X's mission maps are 256x256 (stock
      128); the minimap, briefing and pilot ready drew a quarter.
    - **No firing, no enemies seen, force bar near empty:** MC2X renames
      `gamesys.fit` to `mc2xgsys.fit` and `[General]` to `[mc2xGeneral]`;
      unread, weapon, visual and sensor ranges were 0. Three of its tables
      are longer than stock's (indirect fire 30, sensors 10, rank scale
      6), and a longer table failed and ended `Mover::loadGameSystem`.
    - Objective actions `DisplayInfoMessage` (20 in REAP3) were dropped:
      shown now as a text message.
    Engine fixes found on the way, for any data: a leak when a file opens
    under a second name after a failed open, and `RandomNumber(0)`.
    **3000-frame runs under ASan + UBSan: all 32 MC2X missions reach 3000
    frames.** Four (`reap3m02`, `m06`, `m07`, `m21`) had ended at once as
    lost: MC2X places its deployer dropship (a flying mech) with `Damage 1`,
    which starts a part disabled, and a disabled flying mech was then shot
    down, failing "Protect your deployer dropship". A mech the mission
    starts disabled now stays as placed (stock has no flying mechs).

19. **Dark Gray: Border Worlds (2026-11-14).** Compared with R70 + REAP3 as
    the game sees both (FSTs and loose files): 9,429 of its 12,825 files
    are the same, 973 only in Dark Gray (missions, campaign, art, models),
    2,423 differ. R70's `object2.pak` keeps all of R20's 1,293 packets at
    their numbers; of the 92 its missions place, 71 are the same object
    with keys added or rebalanced, and three are other objects now (548
    artillery piece is a fixed artillery, 641 a twin cannon, 808 `rphover`
    a supply hover). So it runs in the MC2X folder with R70's shared files
    and only its own 973 added, on R70's balance. There its five missions
    pass 3000 frames under ASan + UBSan (after the `RandomNumber(0)` fix: a
    script's `random(0)` was a division by zero), and a walk plays its
    campaign through two missions; REAP3 missions in the same folder still
    pass. Its purchase files are not missions.

20. **Formations (2026-11-15, step 8, first part).** MC2X's HUD has a
    Formation button (layout button 34, in the slot the minelayer, repair
    and salvage buttons share), a palette of ten (24 to 33: none, line,
    double line, column, double column, wedge, arrow, dispersion, small
    and large circle) and an indicator of the current one (35 to 43, at
    x 1024 of the 1024 layout, so on screen from 1280 wide, MC2X's
    smallest). Its string table names them; nothing documents the shapes,
    so they are this port's reading of the names (`code/formation.h`, unit
    tested): three cells between units, centred on the point clicked,
    facing from the group to it, each unit taking the nearest slot, and
    each goal moved to the nearest free cell walkable from the stock goal
    (a local flood fill, not the global path search, which the path worker
    may be using). A unit with no such cell keeps its stock goal. "None"
    is the stock spread, so with no formation chosen nothing changes,
    including the kept-as-shipped packing. Checked: a three-unit move on
    stock `mc2_01` ends in a line across the direction of travel, a wedge,
    a column, a dispersion and a circle as chosen; a screenshot of
    `reap3m01` shows the palette in MC2X's art. Layouts without these
    buttons (stock, Omnitech) are unchanged.

21. **Support units (2026-11-15, step 8, second part).** MC2X's 22 support
    buttons sit on two support tabs (the second is layout button 49): 14
    units and Cancel on the first, 7 on the second. Its own 14 units now
    work like stock's: bought with resource points at the price in their
    help text (stock's seven take MC2X's prices too, 3,000 to 25,000;
    MC2X's run from 5,000 to 1,020,000, and its missions give 100,000 to
    2,000,000), brought in by the VTOL, with the pilot of their own
    `pmw<unit>.fit`. Object numbers come from MC2X's inventory (its FitID
    column, which matches `object2.pak`): JamFly 1464, infantry carrier
    238, specialist 1342, drone 1366, supplier 1259, mercenary 1444, lander
    1470, humvee 1336, transport 1465, kamikaze 1478 (the swarm brings 5),
    deployer 1473. Two are this port's reading: the Armored Rescue Vehicle
    is the inventory's ambulance (the only rescue-like vehicle, and it
    repairs), and the FlySwatter button brings the FlySwatter vehicle
    (1480; the button, its pilot and the inventory row are all Swatter,
    though the help text says RavenDX). Each mission's
    `<x>EnabledDefault` switch turns a unit off (`SpotterEnabledDefault`
    the JamFly, `HoverEnabledDefault` the carrier, `ElementalEnabledDefault`
    the specialist, `MarinesEnabledDefault` the mercenaries,
    `StingerEnabledDefault` the FlySwatter, `KamikazeEnabledDefault` both
    kamikaze buttons), and `toggledeployer` / `setdeployerenabled` now
    switch the deployer. Pure table in `code/mc2xsupport.h`, unit tested.
    Not in multiplayer (MC2X has none). Checked on `reap3m01`: a drone,
    the swarm of five, a humvee and a carrier are delivered and charged;
    pictures of both tabs. At 1024 wide MC2X's leftmost column (x -45)
    and its tac-map tab are off the screen; MC2X needs 1280, where its
    offsets bring them in. MC2X's own layout puts the resource-point line
    over the bottom row of the first tab.

22. **C-Bills, resource points and the spy map (2026-11-15, step 8).**
    Three more HUD buttons, as their help text describes them
    (`code/mc2xhud.h`, unit tested): Button22 turns 10,000 of the mission's
    C-Bills into resource points, Button23 turns all resource points into
    C-Bills (one for one: the help gives no rate), and Button53 buys a
    60-second spy map for 1,000,000 C-Bills, which draws every unit on the
    tac map, seen or not, and switches to it. Each is greyed out when it
    cannot be used. Not in multiplayer. A picture at 1280x1024 shows the
    tac map full of enemies with the spy map on.

23. **The buttons for selected units (2026-11-15, step 8, the last of the
    HUD).** MC2X documents each with one line of help, so all of these
    are this port's reading (`code/mc2xhud.h` for which selection shows
    which button, unit tested; `code/mc2xunits.cpp` for what they do,
    runtime state by watch ID, not saved):
    - **Dropship fire** (Button44, spheroid dropships: inventory group 4):
      the next click fires at that point, through stock's suppression-fire
      order (`doGuardTower`). Replaces the hold-position button.
    - **Hide ID / un-hide** (51, 52): enemies' sensors and eyes do not
      count the unit (`SensorSystem::calcContactStatus`) until it fires or
      is un-hidden. On stock `mc2_01` with an enemy moved alongside: seen
      at contact 5 without it, not seen with it, seen again once ordered
      to attack.
    - **Pick-up / drop-off** (47, 48): a transport (its profile's
      `TransportCapacity`, which reads as tons: Transport 140, Lander 80,
      Deployer 650, the infantry carrier 4 with `TransportItemLimit` 1)
      flies to a friendly unit and takes it off the map; drop-off puts its
      cargo down round it. A transport lost puts its cargo down where it
      was, and a save puts everything down first. On `reap3m01` a
      Transport collected the 55-ton MenShen from 300 units away and put
      it down again.
    - **Recharge / reload** (45, 46, supply carriers: SupplyCopter and the
      supply vehicles): repair vehicles within 250 units regain refit
      points, or every unit within 250 units its ammunition, 10 points a
      second; Button20 lights for a repair vehicle when a recharging
      carrier is in reach. Unit tested only, not tried in a mission.
    - **Exit unit** (50): the pilot ejects without a wound and the unit
      stands empty and whole; the salvage craft can man another. On
      `reap3m01`: the mech disabled, the pilot ejected.

Not tried: the GLES build on desktop (it runs on the Quest).

24. **Operation maps and main menus (2026-11-15).** Stock's mission
    selection screen already opens the operation map a campaign names
    (`OperationFile = "mc2x_ops_reap3"`), but it listed at most 6 missions
    (`MAX_MISSIONS_IN_GROUP`). MC2X's operations hold up to 24 (REAP3, on a
    25-button `mc2x_ops_reap3.fit`; the Test Mission Collection 7), so the
    rest stayed hidden and the list overflowed, leaving no mission chosen.
    The limit is now 32 (button ids 100 to 131, clear of the screen's own
    50 to 82). REAP3's map shows all 24 with their pictures and the first
    mission's briefing. The per-campaign main menus are not a feature of
    the game: each `mc2x_mm_<campaign>.fit` is marked "File for use in ...
    campaign only!" and differs from `mc2x_mm.fit` only in its background
    picture, so a campaign's installer puts its own in place of
    `mc2x_mm.fit`, which is all the game opens. Nothing to build.

## ABL

MC2X's scripting manual lists about 70 commands. Many are the same names
Omnitech added, from the same author (magicX), and are already registered
here from the Omnitech work: `addmoney`, `addresourcepoints`,
`addmovertoplayer`, `removemoverfromplayer`, `setbrain`, `settextmsg`,
`settxtbuildingname`, `setfixedbuildingrp`, `setmechgesture`,
`destroymechbodylocation`, `damagemecharmor`, `setsalvagecraftenabled`,
`togglesalvagecraft`, `gettargetrelativeposition`,
`getrelativepositiontotarget`, `numenemieswithinradius`, `isrefit`,
`needsrefit`, `scanareacapture`, `scanarearepair`, `autorepairwithinradius`,
`setobjectivestatus`, `setobjectivetype`.

Called by the scripts the 32 missions load (their brains, mission
scripts, includes, `mc2xplay` and `mc2xcore`), all built now
(`code/ablmc2.cpp`, "wolfman-MC2X commands"):

| command | form | built as |
|---|---|---|
| `teleporttopoint` | (part id, real[3]) | the mover's own teleport, on the ground |
| `setpartid` | (unit, part id) | movers only, within the mover id range |
| `setunitforcegroup` | (part id, group) | the Ctrl + number selection groups |
| `getcampaignglobalvar` / `setcampaignglobalvar` | (index 0-4) / (index, value) | held for the session only; no campaign save field yet (the scripts only read them) |
| `togglerepairtruck`, `toggleartillerypiece` | no arguments | per-mission overrides like `togglesalvagecraft` |
| `toggledeployer`, `setdeployerenabled` | -, (boolean) | accepted, no-ops: no VTOL deployer here |
| `autoscan` | returns a boolean | false: no AutoScan option (MC2X's default) |
| `automedicalwithinradius` | (part id, radius) | heals friendly pilots in range with no enemy near (only reached with AutoScan on) |
| `newpower` | (part id, boolean) as well as stock's (boolean) | optional first parameter; with a part id it powers that unit's pilot |

Not built, called only by brains no mission loads: `setnewpilot`,
`setbismsg`, `getcurrenttonnage`.

The manual's other commands (`toggle`/`set...enabled` for 18 support types,
`setmissiontune`, `hirepilot`, `numfriendswithinradius`, `ablprint`,
`getpilotphoto`, `addmovertoinventory`, `setpilot`, `isrepairbay`,
`rechargerepairbay`, tac-order queries) are not called by the shipped
content. Omnitech's 6.28 changelog and debug symbols, which this repo
already used, document most of the same names (`MOD-COMPAT.md`).

The core library `mc2xcore.abx` (1900 lines, "R67") declares the eternal
flags of the scripting manual; it compiles here once the commands exist.

## Interface

The largest piece, and the one MC2X changed most:

- **Every layout renamed** `mc2x_*` (table above), plus MC2X-only screens
  (`mc2x_ops_*` operation maps per campaign, `mc2x_deploymentteams`,
  `mc2x_mm_<campaign>` main menus; see Progress 24).
- **The HUD is a superset.** `mc2x_buttonlayout1024.fit` has 54 buttons
  (stock 21): formations (10 kinds), C-Bills up/down, recharge, pick-up and
  drop-off, hide ids, spy map, a second support tab; 22 support units
  (stock 8) and **renumbered**: stock's `Vehicle1` is the guard tower,
  MC2X's the sensor probe; 10 pilot skills (stock 7: adds ultimate, sensors,
  jumping); penalty check marks; a message window. Our HUD reads buttons by
  index, so MC2X's file cannot just be loaded under the stock name.
- **Resolutions:** MC2X runs only at 1280x720 and up (its layouts start at
  1024 wide), with `[HiresOffsets]` blocks like Omnitech's front end.

Two routes:

1. Map MC2X's names to stock's and read its files where the indexes agree,
   as `MOD-COMPAT.md` step 12 did for Omnitech's screens (index sets chosen by
   static count), leaving MC2X's extra buttons and support units inert.
   Days, and the result looks like MC2X with dead buttons. Built (Progress
   16 and 17): one stock-name to MC2X-name table, consulted by
   `FitIniFile::open`, so call sites stay as they are.
2. Build MC2X's HUD features (formations, the 22 support units, the
   deployer/transport/kamikaze units, skills). Weeks, and needs their
   behaviour, which only the manual and play describe.

## Raised limits and sizes it implies

- **Maps up to 300x300** (all 24 REAP3 missions; R70 has 160 and 240).
  `MAX_MAP_CELL_WIDTH` is 720 cells (240 vertices), but it only sets the
  part id multiplier, which MC2X keeps (step 13), and the trigger map, now
  300 tiles. The terrain itself is sized from the map. All 24 REAP3 maps
  run 300 frames under ASan + UBSan.
  Rows past 640 give ids past `MAX_TERRAIN_PART_ID` (464895), which
  nothing checks; the multiplayer chunk masks (20 bits) hold them.
- **256 components** (ids 0 to 255; stock 255 entries, 0 to 254).
  `loadMasterList(..., 255)` drops MC2X's id 255; component ids are bytes in
  places, so raising it needs a check of their users.
- `MAX_GAME_OBJECT_CELLS` 128 on disk (read and narrowed now; real
  footprints are small).
- System heap 1 GB in `mc2x_system.cfg` (stock about 3 MB of the old GameOS
  heap; ours is malloc-backed).
- Mission sizes are within ours: at most 108 parts, 44 objectives.
- **Global routes up to 120 doors** (`MAX_GLOBAL_PATH`, stock 50). On six
  REAP3 maps, routes between random areas reach 55 to 63 doors, and 2 to 97
  per 6000 pairs were longer than 50, which the game treats as no route:
  units far from their goal answered "no path". Stock routes reach 26.
  Saves still hold 50 (`MAX_SAVED_GLOBAL_PATH`, `MechWarriorData` keeps its
  size); a unit saved on a longer route works out a new one after loading.

## Overlap with the Omnitech work

Already in and used by MC2X content: the LZ FST decoder, the 4096-entry
object type table, `openTypeProfile` (MC2X needs only the pak), `.burnin.jpg`
colour maps, the string DLL fallback (extended here), the pathing packet size
checks (extended here), `MAX_WARRIORS` 160, `MAX_STANDARD_FUNCTIONS` 512, about
25 of MC2X's ABL commands, missing-texture handling, paletted TGAs. MC2X needs
no `movobj.fit` and no wide pathing.

Not shared: MC2X's renamed files, its HUD and front end, 300x300 maps, move
type 2, its component numbering (its own complete table, so no mixing with
stock or Omnitech units).

## Effort

| step | what | size |
|---|---|---|
| done | loader fallbacks and checks on this branch | |
| done | ASan + UBSan on all 32 missions, its four fixes, part ids, trigger map | |
| done | the called ABL commands, `newpower`'s second form, the ABL stack fixes (`reap3m02`) | |
| done | campaign globals in the campaign save | |
| done | long `DescIndex` names, move type 2 (drones) | |
| done | in-mission HUD, key reference, dialogs, route 1 | |
| done | front end, route 1 (main menu, campaign and logistics screens, loading screens) | |
| done | step 7: component 255 is an empty row (nothing to do); `DisplayInfoMessage`; failure actions are stock's; `mc2xgsys.fit` | |
| - | `TacWaterColor`, the per-type deploy switches and `max<n>tons`: undocumented, left | |
| done | the shop's unit-type buttons: MC2X lists its vehicles, infantry and dropships as mechs (inventory Type MECH, its Group ID the unit type), so they were for sale already; the buttons filter by group (`code/shopfilter.h`) | |
| done | formations: the Formation button, its palette of ten and the indicator; a group move order takes the shape (`code/formation.h`, unit tested) | |
| done | the 14 other support units and the second support tab (`code/mc2xsupport.h`, unit tested) | |
| done | C-Bills to resource points and back, the spy map (`code/mc2xhud.h`, unit tested) | |
| done | the buttons for selected units: dropship fire, hide ID, pick-up and drop-off, recharge and reload, exit unit (`code/mc2xhud.h`, `code/mc2xunits.cpp`; each a reading of one line of help) | |
| done | operation maps of more than 6 missions (Progress 24); per-campaign main menus are an install-time file swap | |
| done | install: `scripts/mc2x_import.py` builds game folders from the downloads (see Import below) | |

REAP3 runs from the menus on MC2X's own art with no local workaround.

## Recommendation

**Worth doing, in a narrower form than "MC2X support", and after the open
Omnitech work.** The engine side is closer than MOD-COMPAT feared: with six
loader fixes, the commands its missions call and four ABL fixes, all 32
MC2X missions (24 on 300x300 maps) run 300 frames on this port under ASan +
UBSan. MC2X is actively maintained (R70 in
October 2026, REAP3 in July 2026), and much of the scripting work is shared
with Omnitech.

The cost is in MC2X's interface (every layout renamed, a larger HUD) and its
own features, which no data file explains. Order:

1. Merge this branch: done (PR #157).
2. Step 1 (ASan on the 300x300 maps): done, see Progress 12 and 13.
3. 3000-frame runs of the REAP3 missions: done (Progress 18).
4. Step 6: done. Step 8: built from the help text (Progress 20 to 23), as this port's reading; the three undocumented settings stay left.

REAP3 is the MC2X test campaign; Dark Gray runs beside it (Progress 19).
CVE-G (Carver V on MC2X) when its download can be fetched.

## Import

`scripts/mc2x_import.py` (2026-11-15; the user's go-ahead after the terms
question below) turns a player's own MC2X downloads into game folders for
this port, never running an installer and never redistributing anything:

```bash
python3 scripts/mc2x_import.py --downloads <folder with the downloads>     --stock-game-dir <stock game folder> --out <folder> --engine <unpacked package>
```

- Recognises `MC2X.1.exe` (R70), `MC2X-REAP3-G.4.exe` and Dark Gray's zip
  by size, checks each against ModDB's MD5, and refuses anything else.
- Reads the Clickteam installers as data (the reader above), lower-cases
  every name, moves MC2X's own `prefs.cfg` / `options.cfg` to `mc2x-cfg/`,
  copies the stock folder's config files and `assets/` (the port's fonts),
  and sets every prefs file to at least 1024x768 (stock's have no resolution
  lines; MC2X has no layout under 1024 wide). `--engine` copies in `mc2` /
  `mc2.exe`, the resource library and `shaders/`.
- One folder per installer, `mc2x-r70` and `mc2x-reap3`. Dark Gray gets
  `mc2x-darkgray`: a hard-linked copy of the REAP3 folder (else R70) with
  only the files the game does not already see there added, read from its
  fast files where needed (Progress 19's setup; 973 files).
- Tested on made-up files in CI (`scripts/tests/`, in the shader job), and
  on the real downloads in WSL: about 1.5 minutes for all three; R70
  `metropolitan`, REAP3 `reap3m01` and `reap3m12` with its campaign walk,
  and Dark Gray's prologue with its campaign walk pass on the folders it
  builds (Linux desktop GL). Not yet run on Windows or for the Quest.
- Later, from its review: the game could read MC2X's renamed config
  blocks (`[wolfman-MC2X]`, `mc2x_system.cfg`) and clamp the resolution to
  the smallest HUD layout it has, so the script would not need to swap
  config files or raise resolutions; the download, extraction, lower-casing,
  config and engine-copy pieces should become a module the Omnitech helper
  (`EASY-INSTALL.md`) shares; and one engine install serving several game
  folders would save copying the engine into each.

## Clean-up left

From the whole-branch /simplify of PR #157; the rest is done
(`claude/mc2x-cleanup`). Each is a larger change than it saves now:

- **One file-name fallback.** MC2X's renamed data files are found by
  `stockOrMC2XFileName` at 7 call sites, its layouts by the retry in
  `FitIniFile::open`. One retry in `File::open` after a failed read would
  cover both, but the ABL library opens through its own callback, and every
  opener (CSV, ABL, effects, the multiplayer data hash) would need checking.
- **One ABL stack fix.** `evenStackAfterCallback` and the per-command fixes
  in `ablmc2.cpp` both stay: the commands give the right results, the evener
  guards commands a mod registers. A test mode where the evener stops would
  catch a new offender; no test runs those commands yet.
- **The MC2X HUD as one object.** `ControlGui` knows MC2X's layout from the
  name it opened (`openedMC2XName`) and keeps its state in about 20 members;
  detecting it by content (`Button49`, `Vehicle20`) and moving the state into
  one object it owns is a refactor of the HUD, best with tests around it.
- Not worth a change: the parent-building lookups add one argument to an
  upstream line each; the map UVs are two lines at two screens; `File`'s byte
  reads already go through the bounded `memcpy` in `read`; the stock support
  prices are saved once because the legacy table is not `const`.

## Open questions

- Whether MC2X's terms allow a converter that reads its install (no
  redistribution) the way the remaster's importer does. The user chose to
  build the import step (2026-11-15); it reads only the player's own
  downloads and puts nothing of MC2X's anywhere else.
