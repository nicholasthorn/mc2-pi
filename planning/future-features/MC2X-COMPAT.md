# wolfman-MC2X content on this port

Status: in progress, 2026-11-14 (branch `claude/mc2x-compat-b44556`). Built:
the loader fixes, the ABL commands MC2X's missions call, the fixes the
sanitizer runs found, the in-mission interface on MC2X's own layouts; all
32 missions run 300 frames. Not built: the front end and the rest of
"Effort".
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

**Dark Gray: Borderworlds cannot be tried: there is no copy to get.** Its
ModDB page lists no file; its 2013 release posts point to
`borderworlds.maginot.eu`, which is gone, and the web archive holds only the
site's images. MC2X R70 carries its four terrain detail textures
(`darkgray_m0*.detail.tga`) and nothing else of it. REAP3 (24 missions, built
by wolfman on core R68) replaces it as the MC2X test campaign.

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
| `art/buttonlayout<w>.fit` | `mc2x_buttonlayout<w>.fit`, 19 widths 1024 to 3840 | not built |
| `art/mcl_*.fit`, `mcui_keyref_*` | `mc2x_*` (main menu, mission selection, briefing, mech bay, pilot ready, loading screens, dialogs, Mechlopedia, options, key reference) | not built |

## Progress: the test load

Test folder `~/gd-mc2x`: the REAP3 install, paths lowercased, plus this
port's binary, shaders and stock `.cfg` files. Run:
`scripts/smoke.sh --game-dir ~/gd-mc2x --missions metropolitan --frames 300 --ui ""`.
Blockers in the order they appeared on R70's `metropolitan`, then across
all 32 missions (R70's 7, REAP3's 24 and `reap3-x`), 300 frames each,
desktop GL under llvmpipe:

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
    `mc2xArtName` / `openArtLayout` (`code/mc2xart.cpp`) open the stock
    file, else MC2X's; used for the HUD, the key reference and the
    start-up dialogs. MC2X's HUD files place the HUD in the same 1024x768
    centre column as stock's, so they are read as stock's: the first 20
    command buttons (same numbers) and stock's 8 support buttons from
    where MC2X has them among its 22 (`mc2xVehicleButton`), with stock's
    ids. MC2X has no layout under 1024 wide; `smoke.sh --screen` runs the
    test folder at 1024x768. A screenshot of `metropolitan` shows MC2X's
    HUD art in place.
    **Result: all 32 missions pass 300 frames under ASan + UBSan with no
    stock art in the folder and no local changes.** MC2X's extra buttons
    (formations, its 14 other support units) are not shown.

Not tried: the GLES build, the front end (main menu, campaign screens,
loading screens: still stock names), and play past 300 frames.

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
  `mc2x_mm_<campaign>` main menus).
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
   Days, and the result looks like MC2X with dead buttons. Before adding
   dozens more renames one by one (four fallbacks are on the branch), put
   them in one stock-name to MC2X-name table consulted where files are
   opened, so call sites stay as they are.
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
| 6 | front end, route 1 (main menu, campaign and logistics screens, loading screens) | 3-5 days |
| 7 | component 255, mission keys (support switches, failure actions, tac map water colour) | 1-2 days |
| 8 | HUD features, route 2 | weeks |
| - | install: an extractor for Clickteam installers, or an import step | 1 day |

REAP3's missions run with stock's HUD over MC2X data, with no local
workaround but the UI art; with step 6 it can be started from the menus.

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

1. Merge this branch (the fallbacks and checks are harmless to stock data).
2. Step 1 (ASan on the 300x300 maps): done, see Progress 12 and 13.
3. 3000-frame runs of the REAP3 missions, to find what 300 frames do not
   reach.
4. Step 6 only if people ask for MC2X; step 8 not unless MC2X's author
   documents the features.

Dark Gray: Borderworlds is gone; use REAP3 as the MC2X test campaign, and
CVE-G (Carver V on MC2X) when its download can be fetched.

## Open questions

- Whether MC2X's terms allow a converter that reads its install (no
  redistribution) the way the remaster's importer does.
