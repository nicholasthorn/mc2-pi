# VR: the game camera follows the head

Status: built on branch `vr-head-camera`, run on a desktop with a fixed head
pose and once on a Quest 2 (2026-11-02). Follows `VR-FOV.md`. Line numbers are
not recorded on purpose: grep the symbol.

## Problem

After #86 the game camera (`eye`) is as wide as the headset but still looks
where the game points it, pitched down at the game's yaw, while the eyes look
where the head does. Two gaps remain from that:

- **Far ground off the game's screen.** The eyes keep a level horizon while the
  game camera is pitched down, so at a tilt of about 65 degrees far ground
  straight ahead projects off its screen and cannot be picked.
- **What is behind the camera runs appearance only.** `Terrain::geometry` keeps
  `gameOnScreen` for active blocks and needs `isInFront`, so turning round shows
  terrain objects with `refreshAppearanceOnly`.

## What was built

The game camera's *matrices* follow the head, outside the eyes' passes:

- `Camera::setCameraOrigin`, when `vrFollowsHead` and not in an eye's pass, turns
  `cameraOrigin` with `vr_eye::followHead`, and so `worldToClip` and `clipToWorld`,
  and sets `lookVector` and the static `cameraFrame` to match: everything that
  asks "is it in view" (`Terrain::geometry`, `bdactor.cpp`, the arm pieces,
  `artlry.cpp`, `clouds.cpp`, `projectZ`, picking, `seesForGame`) reads those.
- **`cameraRotation` and `projectionAngle` are not touched.** So the sticks and
  keys (`Camera::moveLeft` and friends), the sound (`soundsys.cpp`), ABL's
  `getRotation`, the camera position and saves all stay the game's. The eyes and
  the pointer's ray start from the game's own view (`vrGameOrigin` in
  `vrRayToWorld`; the eyes rebuild it in `setCameraOrigin`), or they would turn
  twice.
- The camera stays where it is: only the rotation follows
  (`vr_eye::followHead`, no head movement). `physicalPos` is unchanged.
- `Mission::update` takes `gos_GetVRHead` every game step, **after**
  `missionInterface->update`. The frames since the last step projected the pick
  through the camera as it was; the interface reads that pick against the
  terrain and object bounds of the same camera. Turn it before the interface and
  the two disagree.
- `gos_GetVRHead` (GameOS): the left eye's orientation; kept while there are no
  eyes (system menu), and **while the trigger is pulled or a button of the virtual
  mouse is held** (`gos_xr::pointerPulled`). The pointer holds still on the game's
  screen through a click (`vr_controls::holdForClick`); if the screen turned under
  it the world point would change and the game would take the press for a drag
  (`dragStart != wPos`). It also gives the pointer's alpha reading, which comes a
  frame or two late, time to catch up (`kAlphaNear`).
- `vrPickWorld` tests `isInFront`: a hand can now point more than 90 degrees from
  where the head looks, and `projectZ` would mirror that onto the screen.
- `Camera::inverseProjectZ`, in VR, meets the ground with
  `vr_eye::screenToGround` for the minimap's wedge (below).

Switches: `MC2_XR_FOLLOW=0` keeps the game's own view (#86's behaviour), for
comparing in the headset. `MC2_VR_PATHS=1 MC2_XR_POSE=yaw,pitch` is the desktop
test: the window then shows what the game camera sees. Without `MC2_XR_POSE`,
`MC2_VR_PATHS=1` does not follow, there being no head.

The flat game is gated out: `gos_GetVRHead` and `inverseProjectZ`'s new branch
need `gos_IsVR()`, `vrPickWorld` gets no ray.

## Why it holds up against the Phase 1 audit

`VR-TABLETOP.md` decided the logic eye stays un-tracked because `inView` and
`windowsVisible` feed gameplay. Since #79 that no longer holds in VR:

- Movers cull by distance only (`Camera::drawsAll`), so `inView` and
  `windowsVisible` for mechs and vehicles do not depend on where the camera looks.
- **Turrets update every step whatever the view**: `GameObjectManager::update`
  walks `turrets[]` outside the active blocks. Special buildings and gates too. So
  looking away does not switch off a turret that is firing at you.
- What does depend on the view: the terrain objects in `objList`, that is ordinary
  buildings and trees, through `objBlockInfo[].active` and `objVertexActive`. Those
  the head has left get `refreshAppearanceOnly`, as off-screen ones always did. In
  the flat game scrolling away does the same.

So the gameplay change is small: which trees and ordinary buildings take the full
update. Cost: the same number of vertices or fewer (mc2_01: about 640 active at
the game's own view, about 570 with a level head), plus one rotation per step.

## What was measured (desktop GL, llvmpipe, `MC2_VR_PATHS=1`, mc2_01)

- With the head at rest and the game's pitch kept (`MC2_XR_POSE=0,0
  MC2_VR_GAME_PITCH=1`) the followed camera is the game's: `lookVector`,
  `cameraFrame` and the active vertices match `MC2_VR_PATHS=1` alone (641, 632).
  This run found a sign error in the first `worldFrame` that the unit test had
  copied, since `fabs` hides it in the culling tests. The test now holds rows read
  from the game.
- `MC2_XR_POSE=180,0`: `lookVector` is the reverse of the game's heading and level.
  Active vertices fall to about 120: the terrain window is centred on where the
  game camera looks, so little of it lies behind the camera.
- Pick round trip: `inverseProject` of screen (400, 380), then `projectZ`, lands
  within a few pixels at poses 0,0 and 0,-65 (the cell snap); 0,0 and 0,-65 find
  tiles, 180,0 finds none (past the window, so the point stays the last found).
- Minimap wedge, screen corners on the ground: see below.
- The flat game: mc2_19, 1500 frames, `-deterministic`, gives `[STATEHASH] c8c9a701`
  from both a build of `pi-port` and this branch. The hash covers movers only; the
  rest rests on the gating above.
- **Not measured:** the frame rate in the headset, picking with a real hand.

## First run on a Quest 2 (2026-11-02, mc2_01, about 2 minutes, build 87f7d7a5)

The wearer's verdict: "think it's good". From the log:

- 27 presses, 25 valid; hits up to 571 m away were valid, most presses near the
  middle of the game's screen, as expected with the camera on the gaze. No GL or
  OpenXR errors, and taking the headset off and on twice changed nothing.
- 84 sampled picks: 70 on screen, 7 no hit, 7 off screen. Of the off-screen ones
  several are just below the bottom edge (below).
- **The window was 3664x1920, not 800x600** (`options.cfg` on the headset, written
  10:57 that day). At that aspect the eyes want about 132 degrees, the limit gives
  120, and the game camera covers only about 42 degrees up and down against the
  eyes' 48 and 50, so the outer band of the view is off its screen. Sums elsewhere
  in this file and in `VR-FOV.md` that say 115.6 assume 800x600.
- **Frame rate: 42 to 61 fps in most seconds of the 57 logged (`VrApi`), 72 in only
  a few**, with CPU level 5 and GPU level 3 to 4. Not explained; see the bench below,
  which does not reproduce it.

## Frame rate: mc2_19 bench on a Quest 2 (2026-11-02, build 805b4004)

One run of each, 3600 frames, fixed head (`MC2_XR_POSE=0,0`), nobody moving:

| Window setting | Following | fps | p95 ms | p99 ms | `logic_ms` | `objects_ms` | `world_render_ms` |
|---|---|---|---|---|---|---|---|
| 3664x1920 (camera 120 degrees) | on | 69.0 | 18.18 | 27.94 | 1.49 | 0.75 | 9.21 |
| 3664x1920 | off | 68.7 | 18.79 | 27.99 | 1.49 | 0.75 | 9.39 |
| 800x600 in `options.cfg` (camera 115.6) | on | 68.5 | 19.24 | 27.80 | 1.54 | 0.75 | 9.21 |
| 800x600 | off | 68.5 | 18.81 | 27.86 | 1.53 | 0.75 | 9.25 |

So neither following nor the window setting costs anything measurable here, and all
four match #86's 69.5 or so. The low rate of the first run is therefore something this
bench does not do: a moving head, the pointer picking, or `mc2_01` played by hand. To
find it: `mc2_01` worn and played the same way, default against `MC2_XR_FOLLOW=0`.
`bench.sh`'s rule holds: one run each, so differences of a few tenths mean nothing.

## The minimap's wedge

`gametacmap.cpp` draws it from `inverseProjectZ` of the screen corners, with a
depth guessed linearly from the screen's y. At #86's 115.6 degrees that was
already wrong: corners thousands of units from the camera, on the wrong side.
With a level head it would be worse. In VR it now uses
`vr_eye::screenToGround`: the ray through the corner meets the ground at the
focus's height, or stops at `Camera::MaxClipDistance` when it runs above the
horizon or further than that. The wedge turns with the head. It covers the game
camera's screen down to the top of the panel, a little more than the eyes see.

## Gotchas

0. **Code that steers the game by the screen must not use the followed camera.**
   `BenchDirector`'s framing projects the fight's centre and moves the camera until it
   sits at a screen position. Through a followed camera it dragged the camera far
   from the fight (seen in the headset; on a desktop mc2_19 at 400 frames gave
   `[STATEHASH] 8374e563` against 866a73f7). It now calls `Camera::dropVRHead`
   first: the hash matches with following off, level, and at pose 120,-20. It is the
   only such code found: `projectZ`'s other callers on the update path only draw or
   pick.
1. **The game's 2D panel moves with the gaze.** The window's bottom third is the
   panel, and a world pick landing there would click it, so it is refused
   (`gos_xr`, the alpha test). That band is now 32 to 50 degrees below where the
   head looks, rather than a fixed part of the ground, so any ground can be clicked
   by looking at it, but not while looking well above it.
2. **Screen positions change with the head.** The drag box (window-space, shown on
   the HUD quad) and anything else drawn in the window from the game camera's
   screen moves with the head. A drag holds the head, so its box does not.
3. **30 Hz.** The camera turns once per game step, so a fast head turn (300 degrees
   a second is 10 degrees a step) leads it. Nothing is drawn with it: objects at the
   very edge run appearance only for a step, and `overlayToEye` works from the same
   camera that made the anchor.
4. **`cameraFrame` is static.** The eyes' passes reset it to the game's view; it is
   only read during the update, and `setEye(-1)` restores the followed one.
5. **Not the head's position.** `followHead` drops it. At the default scale the
   head moves a few world units from a camera over 600 high.

## Still open

- Headset checks: `HARDWARE-TESTS.md`.
- **The pointer while the head turns.** Each game step moves the pick on the game's
  screen, and past about 40 degrees a second that is more than `kAlphaNear`, so the
  pointer is invalid until the late alpha reading catches up. A fast trigger pull
  then can lose the click. Widening `kAlphaNear` risks clicking the panel near its
  edge, so it waits for what the headset shows.
- **Clouds.** `Clouds::render` draws from the game camera's projection in the eyes
  (it did before too, fixed to the game's view). It now turns with the head, once a
  step. Not looked at in the headset.
- **Tiles under a level head.** Vertices near the camera's up axis skip the screen
  test (`CLIP_THRESHOLD_DISTANCE`), so a tile straddling the view plane could take a
  click. The same geometry exists at the game's pitch, and the desktop round trip
  came back right; a click landing under the camera would be this.
- **Comparing runs.** What is active depends on the head, so a bench comparison in
  the headset wants `MC2_XR_POSE` or `MC2_XR_FOLLOW=0`.
- `gos_GetVRHead` takes the left eye's orientation: right for a Quest, whose eyes
  share one; a headset with canted displays would want the average. Its hold logic
  has no unit test, being a few lines beside GameOS state.
- A trigger held for a long time holds what counts as in view too; harmless, but it
  is a difference.
