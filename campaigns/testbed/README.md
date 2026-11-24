# Testbed mission (tb_01)

A hand-built mission on a stock map, written to find out what authoring new
content takes before skills are written for it. Step 1 of the order of work in
`planning/future-features/MISSION-AUTHORING.md`; what it found is at the end
of this file.

## The mission

"Testbed: Airfield Raid", on mc2_01's map. A player lance (Werewolf, two
Bushwackers) raids an airfield:

1. Reach the airfield (move any unit into the area). Sets flag 0 and shows a
   text message.
2. Destroy the hangar (shown when flag 0 is set). Sets flag 1 and warns of a
   relief lance.
3. Destroy the relief lance (shown when flag 1 is set): a Hunchback and two
   Fleas that hold position until objective 2 succeeds, then march on the
   airfield (`warriors/tb_01_wave.abl`).

Secondary: destroy the two hover tanks guarding the field
(`warriors/tb_01_guard.abl`). Every player-facing string is literal text; no
`mc2res.dll` string ids.

## Files

| File | What |
|---|---|
| `tb_01.mission.fit` | The authored blocks of the mission file: units, teams, nav marker, settings and briefing, objectives, script name. `//` lines are notes, stripped on install |
| `tb_01.abl` | Mission script: keeps the mission running; objectives and the engine decide win and loss |
| `warriors/tb_01_guard.abl` | Guard brain: hold position, engage within 400 m |
| `warriors/tb_01_wave.abl` | Relief lance brain: wait for objective 2 (index 1), then patrol to the airfield and hold it |
| `warriors/tb_autoplay.abl` | Test only (`--autoplay`): the player's units play the objectives themselves |
| `purchase_tb01.fit` | What logistics sells before the mission |
| `campaign.fit` | A one-group campaign holding tb_01 |
| `install.sh` | Assembles and copies it all into a game directory |

## Install and run

`install.sh` builds `tb_01.fit` from mc2_01's map blocks (`[Cameras]`,
`[Water]`, `[Terrain]`, `[ForestN]`, `[Weather]`, `[TheSky]`) plus the authored
blocks, copies `mc2_01.pak` as `tb_01.pak` (the map is found by mission name),
and copies the scripts and purchase file as loose files, which the game reads
before `mission.fst`. Use a copy of the game directory: `--campaign` replaces
its `data/campaign/campaign.fit` (the stock one is kept as `.stock`).

```bash
cp -a ../mc2game ../mc2game-testbed
campaigns/testbed/install.sh --src-data ../mc2srcdata --game-dir ../mc2game-testbed --campaign
# loads and runs (the player's units stand still)
scripts/smoke.sh --game-dir ../mc2game-testbed --build-dir build --missions tb_01 --ui ""
# the campaign, through logistics to launch
scripts/smoke.sh --game-dir ../mc2game-testbed --missions "" \
    --ui mainmenu,campaign,missionselection,briefing,mechbay,mechpurchase+buy,mechbay+add,pilotready+pilot,mission
# to play it yourself
cd ../mc2game-testbed && ./mc2 -mission tb_01
```

Autoplay: install with `--autoplay` and run long enough to finish
(`--frames 27000 --timeout 600`). The run passes on `[MISSION] ended: won`, and
the `[OBJECTIVE]` lines show the chain with mission times. On 2026-11-20
(Linux desktop GL, llvmpipe): airfield 19.5 s, hangar 28.9 s, guards 58.2 s,
relief lance 101.0 s, won.

## What it found

1. **Unit brains go in `data/missions/warriors/`**: `system.cfg` sets
   `warriorPath` there (the default in `paths.cpp` is `profiles/`).
2. **No comments in a FIT file.** The reader searches the raw text for
   `[block]` and key names, so a note that names a block can be found
   instead of the block. `install.sh` strips `//` lines.
3. **A map is reused by copying its `.pak`** under the new mission's name (it
   is found by name; 171 KB here). The map's own blocks are `[Cameras]`,
   `[Water]`, `[Terrain]`, `[ForestN]`, `[Weather]`, `[TheSky]` and
   `[ColorMap]` (see 13); everything else is the mission's.
4. **Objectives find units and buildings by exact start position.** `Group`,
   `Mate` and `CellX`/`CellY` are not used. A building condition whose
   position matches nothing fails to read, and its `Status()` would then
   dereference a null building: the validator must check every position.
5. **Literal text works everywhere it was tried.** Mission selection shows
   `MissionName` and `Blurb2`; the briefing shows `Blurb` (a multi-line
   string reflows and keeps its blank-line paragraph break) and each
   objective's `Description`, not its `Title`. Objectives gated on a flag
   stay out of the briefing until the flag is set, so later objectives are
   reveals.
6. **A mech is for sale only if every component of its loadout is in the
   purchase file's component list** (stock data; `variantOnSale` in
   `logisticsdata.cpp`). With a short list the purchase screen was empty.
   The list has to be derived from the mechs on sale.
7. **The mission script can be ten lines.** The engine ends the mission on
   the objectives (won) or when every player unit is down (lost). Unit brains
   read triggers themselves (`checkObjectiveStatus` in `update`, then
   `trans`), as stock `mc2_17_a1_liaoson` does; stock brains hard-code map
   coordinates.
8. **Scripts cannot log** (`print` and `mcprint` go to a debug window this
   build does not have). The engine now prints `[OBJECTIVE] <time>s
   successful|failed|message: <text>` (`code/objective.cpp`).
9. **A won `-mission` run crashed** (exit 139): logistics called
   `missionBegin->beginSplash()` on a null pointer, and a splash would then
   have opened mission selection with no campaign loaded. It now ends the
   game, as a lost `-mission` run already did, and prints `[MISSION] ended:
   won|lost|draw`; `smoke.sh` counts a win before the frame budget as a pass.
   Stock smoke runs never got there: their missions do not end in 600 or
   3000 frames.
10. **A test brain can play a mission headless** (`tb_autoplay.abl`), which is
    how the validator can check that an objective chain completes.
11. **The UI driver will not win a campaign's last mission** (the final video),
    so a one-mission campaign is walked to launch only.
12. **The operation screen comes with its marker position** (`OperationFile`,
    reused from stock here); a new campaign needs its own, or the stock
    marker sits wherever the stock mission was.
13. **The ground and water textures are found by the map's name too**
    (`textures/<name>.tga`, `<name>.detail`, `<name>.water`, in
    `Terrain::init` and `TerrainColorMap::init`), so tb_01's terrain drew
    bright pink, on the PC and on a Quest 3, until `install.sh` added
    `[ColorMap] ColorMapName = "mc2_01"`. Nothing reported it: no log line, and
    every smoke, autoplay and UI run passed. Found by playing it on the Quest;
    the PC screenshots had only been of front-end screens. The validator must
    check that the color map's textures exist, and a review of a new mission
    needs an in-game screenshot (`-bench` with `MC2_SHOT_FRAME`).
