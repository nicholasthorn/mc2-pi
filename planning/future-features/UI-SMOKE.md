# UI smoke tests (menu screens)

Proposed 2026-10-31. Not started.

## Problem

Smoke runs and `-bench` go straight into a mission (`-mission <name>`), so no
automated run ever opens a menu screen: main menu, logistics (mech purchase,
mech lab, pilot ready), the Mechlopedia, options, load/save. Those screens read
most of the game's data that missions do not (purchase lists, variants,
component and pilot tables, every name and description string), so data bugs
and `STOP`s there go unseen. Two cases so far:

- MechCommander Omnitech's purchasable vehicles and buildings were missing
  from logistics until PR 17, and nobody has seen them on screen since.
- The `STOP`/`Fatal()` plan in `planning/PROGRESS.md` (to-do, step 2) needs the
  screens smoke never opens exercised before `STOP` can exit by default.

## Scope

In:

- A UI script player in the game, run headless under Xvfb + llvmpipe like
  `smoke.sh`, on stock data and on a mod folder (`~/gd-mco`).
- A first set of scripts: main menu to campaign start (first logistics
  screen), mech purchase, mech lab, pilot ready, Mechlopedia, options,
  load/save.
- A `ui` tier in `scripts/smoke.sh`; pass/fail from logged checks plus the
  usual "no `STOP:` / error lines".
- Screenshots per step for a person to look at.

Out:

- Pixel comparison against reference images: llvmpipe, Mesa versions and the
  Pi's V3D differ slightly, so it would be flaky. Screenshots are for eyes.
- Multiplayer screens (networking is stubbed; see Known issues).
- Testing gameplay input (mission orders); `-bench` scenarios cover missions.

## Affected areas

- **Platform (GameOS):** input injection. The game reads the mouse and keys
  through `UserInput` (`mclib/userinput.cpp`), which calls GameOS's
  `gos_GetMouseInfo` (`GameOS/gameos/gameos_input.cpp`) and key functions.
  `-bench` already pins the cursor and ignores polled keys in the main loop's
  event handling (`gameosmain.cpp`, via `input::pinMouse` in `gos_input.cpp`,
  PR 12); that is the natural place to feed scripted positions and button
  states. Screens that call `gos_GetKey()` directly (e.g. `optionsarea.cpp`
  key binding) need checking.
- **Game:** the script player, beside `code/benchdirector.cpp`, driven once per
  frame from the main loop; the fixed frame step and screenshot code of
  `-bench` (`gameosmain.cpp`, `GameOS/gameos/utils/tga_writer.cpp`) are reused.
- **Data:** scripts in the repo (`ui/*.fit`), read with `FitIniFile` like bench
  scenarios.
- **Build/CI:** `smoke.sh --tier ui`; CI's smoke job (`smoke.yml`, which
  caches the built game data under `gamedata`) could run it too.

## Approach

A FIT script, played one step per frame (frame numbers relative to the
previous step, so inserting a step does not renumber the rest):

```
FITini
[UiScript]
st Start = "mainmenu"      ; or a screen reached directly, see below
l NumSteps = 4
[Step0]
st Do = "press"            ; press | wait | shot | expect | key
st Screen = "mainmenu"
l Button = 1001            ; button id as in the screen's .fit layout
[Step1]
st Do = "wait"
l Frames = 30
[Step2]
st Do = "shot"
st Name = "purchase"
[Step3]
st Do = "expect"
st What = "purchaseListCount > 0"
FITend
```

- **press:** find the button by id on the current screen
  (`LogisticsScreen::getButton(id)`; MainMenu, the Mechlopedia, options and
  the load screen all derive from `LogisticsScreen`), move the scripted cursor to its centre
  and click (down one frame, up the next). Positions come from the button,
  not the script, so scripts work at any resolution.
- **wait / shot:** as the bench director's segments and screenshots.
- **expect:** a small set of named checks the game can answer cheaply (list
  counts, current screen, "no missing string since the last step"), logged as
  `[UI] step N: expect ... ok/FAILED`. A failed expect ends the run with a
  non-zero exit, and `smoke.sh` fails it.
- Every step logs `[UI] step N: <what>`, so a failure says where it stopped.

Getting to a screen: through the menus by default (that is what is tested).
If the opening menus are slow or flaky headless, a `-uistart <screen>`
shortcut can open a logistics screen with a campaign's first mission loaded.

Alternative rejected: driving the window from outside with `xdotool`. No game
change, but it clicks fixed screen positions on wall-clock timing, so a
resolution change or a slow frame on the Pi breaks it, and it cannot say why a
click did nothing.

## Verification

- Each script passes on stock data (`~/gd-fresh`) and on the Omnitech folder
  (`~/gd-mco`), under GL and GLES builds, headless.
- A deliberately broken step (wrong button id) fails with the step named.
- Two runs of a script give identical screenshots on one machine (fixed step).
- Run once on the Pi 400 (real display) to check the input injection there.

## Open questions

- Which screens read input outside `UserInput` (direct `gos_GetKey`, SDL
  events)? Survey before building the player.
- Do button ids come from the screen `.fit` layouts consistently enough to
  name them in scripts, or do some screens need a name lookup?
- How long do the opening menus and movies take headless? Is `-uistart`
  needed from the start?
- Should the `ui` tier run in CI on every PR, or nightly only?
