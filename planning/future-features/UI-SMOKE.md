# UI smoke tests (menu screens)

Proposed 2026-10-31. Slice 1 built 2026-10-31 (`-uismoke`, see "Slices").

## Problem

Smoke runs and `-bench` go straight into a mission (`-mission <name>`), so no
automated run ever opens a menu screen: main menu, logistics (mission
selection, briefing, mech bay, mech purchase, mech lab, pilot ready,
salvage), the Mechlopedia, options, load/save, the tutorial. Those screens
read most of the game's data that missions do not (purchase lists, variants,
component and pilot tables, every name and description string), so data bugs
and `STOP`s there go unseen. Cases so far:

- MechCommander Omnitech's purchasable vehicles and buildings were missing
  from logistics until PR 17, and nobody has seen them on screen since.
- The `STOP`/`Fatal()` plan in `planning/PROGRESS.md` needs the screens smoke
  never opens exercised now that `STOP` ends smoke runs (`MC2_STOP_EXITS`).
- Many cppcheck and uninitialised-member fixes landed in these screens
  (`LogisticsScreen` copy/assign, `OptionsXScreen`, mech lab, the tutorial
  button, `logisticsdata`) and were only ever built, never run.

## Scope

In:

- A walker in the game, run headless under Xvfb + llvmpipe by
  `scripts/smoke.sh` with the same failure rules as missions (crash, hang,
  non-zero exit, GL error, `STOP: `, sanitizer errors).
- Walks through: main menu, the four options tabs, new campaign through its
  logistics screens (slice 1); then the tutorial, salvage / mission results,
  Mechlopedia, load/save, single mission; then mod data (`~/gd-mco`).
- Screenshots per screen for a person to look at (later slice).

Out:

- Pixel comparison against reference images: llvmpipe, Mesa versions and the
  Pi's V3D differ slightly, so it would be flaky. Screenshots are for eyes.
- Multiplayer screens (networking is stubbed).
- Testing gameplay input (mission orders); `-bench` scenarios cover missions.

## How the game reaches these screens

- `code/mechcmd2.cpp`: without `-mission`, `InitializeGameEngine` calls
  `logistics->start(log_SPLASH)`, which creates `MissionBegin`
  (`code/missionbegin.cpp`). With `-mission` it skips all of this
  (`log_STARTMISSIONFROMCMDLINE`).
- `MissionBegin` owns the main menu (`MainMenu`, shown while `bSplash`) and a
  5x3 grid of `LogisticsScreen`s, `screens[x][y]`: mission selection (0,1),
  briefing (1,1), mech bay (2,1), pilot ready (3,1), mech purchase (2,0),
  mech lab (2,2), loading (4,1). A screen leaves by setting its status
  (`NEXT`, `PREVIOUS`, `UP`, `DOWN`, `MAINMENU`); `MissionBegin::update()`
  then moves `curScreenX/Y` and plays a slide animation.
- Buttons: `aButton::update()` sends its parent
  `handleMessage(aMSG_LEFTMOUSEDOWN, id)` on a click; every screen's
  `handleMessage` switches on the id (`MB_MSG_NEXT` = 50, `MB_MSG_PREV`,
  `MB_MSG_BUY`, `MB_MSG_CHANGE_LOADOUT`, main menu `MM_MSG_*` = 90..99).
- Main menu "New Campaign" opens `LogisticsSaveDialog::beginCampaign()` (list:
  campaign, tutorial); OK calls `LogisticsData::startNewCampaign()` and the
  main menu returns `RESTART`, which opens mission selection.
- Main menu "Options" runs `OptionsScreenWrapper` / `OptionsXScreen` inside
  the main menu (`bOptions`): four tabs (graphics, audio, gameplay, hot keys;
  buttons 200..203), OK (`YES`, saves prefs) and Cancel (`NO`, reloads them).
- Mission results and salvage (`log_RESULTS`) only follow a played mission.
- `-autoexit N` (`gameosmain.cpp`) counts frames from
  `gos_MarkGameplayStarted()`, which `code/mission.cpp` calls on a mission's
  first frame; since #19 an automated run ignores real input, pins the cursor
  at the centre and runs windowed.

## Approach (chosen)

`-uismoke <walk>`: a comma-separated list of screens, starting at `mainmenu`.

```
./mc2 -uismoke mainmenu,options-graphics,options-audio,mainmenu,campaign,missionselection,briefing,mechbay
```

- **The walk** (`code/uismokewalk.cpp`, pure logic, unit tested in
  `tests/unit/test_uismokewalk.cpp`): parses the names, checks that each pair
  of neighbours has a button between them (`uiRoute()`), and per frame, given
  the screen that has settled, says: wait, "shown", press this, done, or
  timeout. Each screen is held for 60 frames (so it updates and renders for a
  while, transition animation included); a screen that does not settle within
  60 s of game time after the press that should open it fails the walk.
- **The driver** (`code/uismokedriver.cpp`, game glue like
  `benchdirector.cpp`): called once per frame from `DoGameLogic()`. Works out
  the settled screen from `MissionBegin`/`MainMenu`/`OptionsXScreen` state
  (a `friend` line in each class, no new accessors) and presses buttons by
  sending the screen the same `handleMessage(aMSG_LEFTMOUSEDOWN, id)` a click
  sends, but only if the button exists and is enabled, so the walk can only
  go where a player could. Prints `[SCREEN] <name> shown`, then
  `[UISMOKE] done: ...` or `[UISMOKE] FAIL: <why>`, and quits.
- **GameOS** (`gameosmain.cpp`): parses `-uismoke` and hands it to the game
  through `gos_GetUiSmokeWalk()` (as `-benchscenario`), and makes the run an
  automated one (input ignored, windowed). Nothing game-specific lives there.
- **smoke.sh**: `--ui "<walk> <walk> ..."` (default: the two walks of slice
  1, below; `--ui ""` skips). A walk passes when the game
  exits 0, prints `[UISMOKE] done`, shows exactly the walk's screens in
  order, and logs no error lines (`BAD`, same as missions). `check.sh --level
  batch` and the nightly `smoke.yml` get it with no change.

Why messages, not a scripted cursor (the earlier proposal): since #19 an
automated run ignores the mouse and keyboard on purpose; a message press does
not depend on resolution, button layout or frame timing; and a failure says
which button was missing or disabled. What it skips is `aButton`'s own hit
test and press animation, and anything a screen does only on hover. If that
ever matters, a later slice can move the pinned cursor to the button centre
and synthesise a click in `gos_input.cpp` instead; the walk logic stays.

Alternative rejected: `xdotool` from outside. Fixed screen positions and
wall-clock timing break on a resolution change or a slow Pi frame, and it
cannot say why a click did nothing.

## What slice 1 found about the stock data

- **New campaign goes straight into mc2_01.** Its `[Group0Mission0]` has
  `PlayLogistics = 0`, so OK in the campaign dialog loads the mission (the
  loading screen, `screens[4][1]`); mission selection and the rest only come
  after a mission is won. The walk reaches gameplay this way (`mission`).
- **Single missions come from campaign saves.** `MPLoadMap::seedDialog()`
  lists `data/missions/*.fit` on disk (none: built data keeps them in
  `mission.fst`) plus every mission the newest save in `savePath` has reached
  (`seedFromCampaign()`). A clean install offers none. `smoke.sh` gives each
  walk a temporary `XDG_CONFIG_HOME` holding `tests/smoke/campaign-save.fit`
  (only `[General]`: group 2, so mc2_01 and mc2_02); the driver picks the
  last, mc2_02, which has logistics. The temporary directory also keeps
  walks away from the real `~/.mechcommander2`. It must be lower case: the
  game lowercases the save path (`FullPathFileName::init`).
- **A single mission starts with no mechs.** The mech bay selects nothing,
  so "Change Loadout" (mech lab) is disabled, and pilot ready's "Launch" is
  disabled with no force. Both need a mech bought first (slice 2).

## Slices

1. **Built (2026-10-31):** walker, driver, `smoke.sh --ui`, two default
   walks. (1) main menu, options (4 tabs, Cancel), new campaign dialog, into
   mc2_01. (2) single mission dialog (mc2_02), briefing, mech bay, mech
   purchase, back to the mech bay, pilot ready, back to the mech bay and
   briefing. Routes for mission selection and the mech lab exist and are unit
   tested, but no default walk reaches them yet.
2. Actions inside screens, so the walk can buy a mech (purchase screen
   `MB_MSG_ADD` on a selected list item), put it in the force group, open the
   mech lab (and Cancel, and later change a weapon), pick pilots, and launch
   from pilot ready. Needs list-box item access in the driver (`MechListBox`,
   `LogisticsPilotListBox`) and a walk syntax for actions that stay on one
   screen (for example `mechpurchase+buy`).
3. Tutorial: pick the tutorial entry of the campaign dialog
   (`LogisticsSaveDialog` list item 1) before OK; its logistics ABL brain
   (`MissionBegin::begin`, `startAnimation`) moves the cursor itself, so
   check it does not fight the pinned cursor.
4. Out of a mission: end it from the driver after N frames (a win through
   `mission->update()`'s result or the objectives) to reach mission results,
   salvage and pilot promotion (`log_RESULTS`, `code/missionresults.cpp`,
   `salvagemecharea.cpp`, `pilotreviewarea.cpp`), then mission selection.
5. Mechlopedia (`MM_MSG_ENCYCLOPEDIA`), load/save (`MM_MSG_SAVE`/`LOAD`, in
   the temporary config directory only), the in-mission options (Ctrl+O
   wrapper in `mechcmd2.cpp`), main menu from a logistics screen
   (`MB_MSG_MAINMENU`) and back.
6. Screenshots per screen (reuse `bench_capture` / `tga_writer`), and the
   same walks on the Omnitech data folder.

## Verification

- Unit tests: walk parsing, routes, wait / hold / act / timeout sequencing.
- Each walk passes headless under GL and GLES builds (`smoke.sh`), and under
  the sanitized build in `smoke.yml`.
- A broken walk (unknown screen, no route, disabled button) fails with the
  reason in the log and in smoke's FAIL line.

## Open questions

- Should the walks also run on mod data in CI, or only locally?
- Hold length: 60 frames is enough to render each screen and its slide; a
  longer hold would catch slow leaks, at more CI time.
