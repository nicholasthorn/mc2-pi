# Selection groups: keyboard and Quest

Status: spec only, nothing built. Written 2026-11-13 from a read of `pi-port`
at `c74bc767`. Recommended design below; the Quest button choice is the open
decision for the user.

## Problem

Players want to put mechs into groups and switch between the groups quickly.
On a keyboard that is number keys. On the Quest there is no keyboard, and every
controller button is already in use, so the headset needs its own way to make
and switch groups. The user plays stock MC2 and Omnitech on the Quest, so the
controller scheme matters as much as the keyboard.

## What exists today

**The original game already has control groups** ("force groups" in the code),
ten of them, numbered 1 to 9 and 0. They are in this port unchanged.

Keyboard (`MissionInterfaceManager::update`, `code/missiongui.cpp:1789-1804`):

| Keys | Effect | Code |
|---|---|---|
| Ctrl + number | the selected units become that group (it is cleared first) | `makeForceGroup`, `missiongui.cpp:3355` |
| number | select that group, deselecting everything else | `selectForceGroup(n, true)`, `missiongui.cpp:3320` |
| Shift + number | add the group to the selection; if all of it is already selected, take it out | `selectForceGroup(n, false)` |
| double-click a unit in the world | select that unit's whole group | `missiongui.cpp:822-838` |
| double-click a unit's icon | the same, from the force group bar | `forcegroupbar.cpp:268-279` |

The number loop is not in the command table (`missiongui.cpp:170-305`), so the
groups cannot be rebound and do not show in the F1 hotkey list. It runs only
when no earlier input claimed the frame (`!bRetVal`: chat open, a click
handled), and fires once per press (`gos_GetKeyStatus != KEY_HELD`).
SDL's digit keys reach `KEY_0` to `KEY_9` in `gameos_input.cpp:93-95`.

How membership works (`code/mover.cpp:7158-7172`):

- `Mover::unitGroup` is a bit mask, but `addToUnitGroup` sets it to a single
  bit ("now you only get to be in one at a time"): **a unit is in at most one
  group**. `removeFromUnitGroup` ignores its argument and sets `-1` (no group).
- **Every unit starts in group 1** (`unitGroup = 2` in the constructor,
  `mover.cpp:2223` and `2284`). `ControlGui::addMover` takes a unit out of all
  groups if it arrives after the first three frames (`controlgui.cpp:1302`), so
  reinforcements, and units that join the force later, start in none.
- A unit that leaves the force through its icon's end animation (recovered,
  withdrawn) is taken out of its group (`mechicon.cpp:1567`).

What the player sees (`ForceGroupIcon::render`, `mechicon.cpp:951-968`): the
strip behind the pilot's name on each icon is the group's colour, from
`ForceGroupColors` (`mechicon.cpp:62`, 10 colours plus grey for no group).
Icons are sorted by group (`ForceGroupIcon::sort`, `mechicon.cpp:1455`). There
is no number on screen. Two of the colours are both red (group 4 `B70000`,
group 8 `FF0000`) and two are blue (group 1 `005392`, group 0 `0091FF`), hard to
tell apart, especially in the headset.

Dead and disabled units: selecting a group skips them (`!isDisabled()` in
`selectForceGroup`), but they stay members, so their icon keeps the colour.

Save and load: `unitGroup` is part of `MoverData` (`mover.h:669`), written by
`Mover::CopyTo` and read by `Mover::Load` (`mover.cpp:7313`, `7473`), which
`BattleMech` and `GroundVehicle` call (`mech.cpp:9162`, `gvehicl.cpp:5504`).
**Groups survive an in-mission save and load.** They do not carry over to the
next mission: units are made anew, all in group 1 again.

Multiplayer: the group lives only on the local mover and is never sent; each
player's groups are their own. Nothing to change.

Quirks found while reading (not fixed, listed for the build):

- **An empty group clears the selection.** Pressing the number of a group
  with no living members runs the deselect pass and then selects nothing.
- Alt + 1 to 3 and Alt + 4 to 0 are debug commands in the table
  (`missiongui.cpp:276-296`), and the number loop ignores Alt, so Alt + number
  also selects the group.

**Does it work in this port on desktop?** The code is reached every mission
frame and the keys are mapped, so it should; it has not been tried in a
window for this spec (no game launched), and nothing in `PROGRESS.md` or
`HARDWARE-TESTS.md` records a check. First step of the build is a person
pressing Ctrl + 2, 2, Shift + 2 and double-clicking, on desktop GL.

## What the Quest has today

`GameOS/gameos/gos_xr.cpp` turns the Touch controllers into a virtual mouse
and keys (bindings at `gos_xr.cpp:383-397`, keys in `setKeys`,
`gos_xr.cpp:599-618`); `vr_controls` holds the unit-tested stick logic.

| Input | Does | Note |
|---|---|---|
| trigger (either hand) | left click | the hand that last pulled points |
| grip (either hand) | right click | |
| left stick | pan (arrow keys) | |
| right stick | turn (x), zoom (y) | |
| A (right) | recentre the interface; held, the left stick moves and sizes it | handled in `gos_xr.cpp:1474-1486`, sends no key |
| B (right) | Ctrl, the waypoint key | held while the other hand clicks |
| X (left) | Space, run | held while the other hand clicks |
| Y (left) | J, jump | held while the other hand clicks |
| left stick click | F, force fire | held while the other hand clicks |
| right stick click | E, select visible units | a stray click changes the selection (`HARDWARE-TESTS.md`) |
| menu (left) | Escape, pause | |

**No button is free.** Every button and both stick clicks have a job, and no
key reaches the number loop: a group cannot be made in the headset today.
Selecting one already works: double-pulling the trigger on a unit, or on its
icon, selects its group (the double-click paths above).

Combinations are limited by thumbs: A, B and the right stick click share the
right thumb, and X, Y, menu and the left stick click the left one, so only a
right-hand button with the left stick, or a left-hand button with the right
stick, can be held together. Those pairs are taken (B + left stick pans; X or
Y + right stick would run or jump while turning).

## Design

Small: reuse the original groups, add two commands, one VR binding and a
digit on the icon.

### Keyboard

- Keep Ctrl + number, number, Shift + number and the double-clicks as they are.
- **New: next group and previous group** as command-table entries, so they can
  be rebound and show in the F1 list: `` ` `` (grave, `KEY_GRAVE`, unused today)
  selects the next group that has a living unit on the force bar, in the order
  1 to 9 then 0, wrapping; Shift + `` ` `` goes back. "Next" is counted from
  the last group selected by any means (number, cycle or double-click),
  remembered in `MissionInterfaceManager`; with none yet, it starts at 1.
- **New: Ctrl + `` ` `` makes a new group** from the selection: the lowest
  numbered group with no members. If all ten are in use it does nothing (an
  invalid-action sound, as other refused commands play `INVALID_GUI`).
- The new entries go **at the end** of the command table (`MAX_COMMAND`
  107 to 110): hotkeys are saved by index as `Key<n>`
  (`saveHotKeys`, `missiongui.cpp:5297`), and the chat keys are found by
  index (100 to 102, `missiongui.cpp:1755`). A prefs file from before then
  simply lacks the three new keys and keeps the defaults.
- **Empty group is a no-op** instead of clearing the selection. This changes
  shipped behaviour in a small way; it is UI, not gameplay, but it is the
  user's call (see Open questions).

### Quest

#### Press patterns: click, double click, long press

Today each Quest button has one job. A small, general layer gives a button up
to three: **click**, **double click** and **long press**. It is worth having
beyond groups (the user's suggestion, 2026-11-13), so it is specced here as
its own piece and groups are its first user.

- A pure classifier in `vr_controls` (unit-tested, no OpenXR): given a
  button's down / up state and the time each frame, it reports `CLICK`,
  `DOUBLE_CLICK` or `LONG_PRESS` events. Defaults: long press at 0.5 s held,
  double click when the second press starts within 0.3 s of the first
  release. Both are constants first, settings only if the headset says so.
- **A click fires on press** when the button has no long press, as today, and
  **on release** when it has one (so a long press never fires the click
  first; a click under 0.5 s still feels instant). A double click is the
  click followed by a second event, so it adds no wait: it suits a button
  whose click is harmless to repeat (a selection, which the double click then
  replaces). No proposed binding waits out the 0.3 s.
- **Long press** fires when the time is reached, not at release, so the
  player feels it happen; a short haptic pulse marks it (OpenXR haptic output
  action, optional in the first cut).
- **Only on tap buttons**: A, the right stick click and menu. B, X, Y and the
  left stick click are **held while the other hand clicks** (waypoints, run,
  jump, force fire); a long press there is what they already are, and a double
  tap would send their key twice. They stay as they are.
- Each event becomes a virtual key held for long enough for the 30 Hz game
  step to see it, as the other buttons do. `gos_xr.cpp` stays free of game
  knowledge: the game decides what the key means.

#### The group button

Recommended: **the right stick click**, in a mission.

| Press | Does | Sends |
|---|---|---|
| click | select visible units, **unchanged** | E |
| double click | next group (the first press's "select visible" is replaced) | `` ` `` |
| long press | new group from the selection | Ctrl + `` ` `` |

- The cycle visits the groups with a living unit, in the order 1 to 9 then 0,
  wrapping. With no groups made, every unit is in group 1, so a double click
  selects them all.
- Because the button has a long press, its click (E) is sent on release, so a
  long press makes the group from the selection the player had, not from the
  visible units.
- The stray-click problem in `HARDWARE-TESTS.md` is unchanged.

The cycle order, which groups are empty and the "new group" number live in
`MissionInterfaceManager`, where they also serve the keyboard.

Alternatives rejected:

- **A double click cycles.** A's press recentres the interface, so every
  cycle would pull the interface in front of the head. A is a fallback if the
  right stick proves too easy to click by accident.
- **Right stick click: click cycles, "all visible" as the cycle's last step**
  (this spec's first draft). Changes what one click has always done; with
  double click available there is no need.
- **Y or X held + right stick flick.** Holding J or Space puts the cursor in a
  jump or run mode, and the flick would also turn the camera.
- **A button on the interface.** Pointing at small HUD buttons is the slow
  part in a headset (the reason for the 2026-11-02 remap), and double-pulling
  an icon already does this.
- **Number keys on a virtual keypad.** Too slow mid-fight.

### What the player sees

- **A digit on each icon**: the group number drawn at the right end of the
  coloured name strip, in the icon font (`ForceGroupIcon::gosFontHandle`).
  It disambiguates the two reds and two blues, and in VR is readable where the
  colours alone are not. Units in no group show no digit.
- **Selecting** a group, by any means, highlights its units and icons as a
  selection does today; nothing else changes. The camera does not move.
- **A short line** in the game's message area ("Group 2", "New group 3") when the group is chosen by the cycle or made by Ctrl + `` ` ``, so
  a player who cannot see the icons (head turned in VR) knows what happened.
  If the message area cannot take a line without a sound or a log entry, drop
  this; the icons are enough.
- **An empty group** is never visited by the cycle, and its number key does
  nothing.

### Dead, disabled and returning units

- Dead, disabled and ejected units keep their membership (and their icon's
  colour and digit) but are never selected, as today. A group whose members
  are all dead counts as empty: the cycle skips it and Ctrl + `` ` `` may
  reuse its number (the dead keep the old number on their icons until then;
  reusing takes them out of it, since the new group is cleared first by the
  existing `makeForceGroup` logic).
- A unit that rejoins mid-mission (recovered, reinforcement) starts in no
  group, as today.

### Save and load

Nothing to do for membership: `unitGroup` is already saved. The cycle's "last
group" is not saved; after a load the cycle starts at 1. No save format change,
so `ondisksizes.cpp` is untouched.

## Affected areas

- **Game:** `code/missiongui.cpp` / `.h` (three commands, empty-group no-op,
  last-group memory, cycle order), `code/mechicon.cpp` (digit), hotkey help
  strings for the F1 list (resource strings in `res/`).
- **Platform (VR only):** the press-pattern classifier in `vr_controls`
  (unit-tested), and `GameOS/gameos/gos_xr.cpp` (which events send which
  keys, click on release for the right stick).
- **Docs:** `VR-TABLETOP.md` controls section, `HARDWARE-TESTS.md`.
- **Untouched:** renderer (GL and GLES), shaders, engine, save format, data,
  build, multiplayer.

## Verification

- Unit tests: the cycle order (skip empty and all-dead groups, wrap, start
  at 1) as a pure function over a list of group masks and alive flags.
- Desktop GL, by a person: Ctrl + 2, 2, Shift + 2, `` ` `` and Shift + `` ` ``,
  Ctrl + `` ` ``, an empty group's number, a group whose units all died, then
  save, load and press the number again. Stock and Omnitech.
- Unit tests for the classifier: click, double click, long press, a slow
  second press (two clicks), a long press then a click.
- Quest, by the user: click still selects visible units; double click cycles
  through two groups; long press makes a group;
  check that the icon digits are readable at the interface's usual distance.
- Smoke runs are unaffected (no input in `-autoexit` runs); CI as usual.

## Open questions

1. **Quest button**: right stick click with double click to cycle and long
   press to make a group (recommended), or another button? Which other tap
   buttons (A, menu) should get double click or long press, and for what?
2. **Press timings**: 0.3 s for a double click and 0.5 s for a long press, to
   be tuned in the headset.
3. **Empty group no-op**: change it (recommended), or keep the shipped
   behaviour where an empty group's number clears the selection?
4. **The message line**: wanted, or are the icons enough?
5. **`` ` `` for next group**: fine on the user's keyboard layout? On some
   layouts the key is elsewhere; it can be rebound once it is in the table.
