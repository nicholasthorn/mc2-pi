# VR: widen the game camera's field of view

Status: built on branch `vr-fov`, PR #86, run on a Quest 2 (planned and built
2026-11-02, from `pi-port` with #77, #79 and #83). Results and what is still
open are below. Resolves the to-do "the game camera's field of view in VR" in
`VR-TABLETOP.md` (Phase 3 progress). Line numbers are not recorded on
purpose: grep the symbol.

## Problem

In VR the player sees about 100 degrees, but the logic eye (the game camera,
`eye`) still has `camera_fov = 40`. The headset's eyes render with their own
frusta (`vr_eye::clipEdges` from `XrFovf`), so the 40 degrees is invisible as
a picture; it shows as what the game does with the view:

- **Picking stops at the game camera's screen.** `vrPickWorld` finds the
  world point under the controller's ray, projects it through the game camera
  (`projectZ`) and feeds the virtual mouse with it. A point outside the game
  camera's 800x600 projects off the screen (one hit landed at 1733, 1257), so
  the pointer is dropped. `Camera::seesForGame` keeps `findObjectByMouse` and
  drag selection to the same screen on purpose (objects behind the camera
  project mirrored). Both widen by themselves when `camera_fov` does, because
  both go through `worldToClip` and `projectZ`.
- **Terrain objects outside the cone run no game logic.** `Terrain::geometry`
  keeps the game camera's own answer (`gameOnScreen`, from the sphere-clip
  constants `verticalSphereClipConstant` / `horizontalSphereClipConstant`,
  which are `tan(... camera_fov ...)`) to set `objBlockInfo[].active` and
  `objVertexActive`. `GameObjectManager::update` walks active blocks only
  (special buildings and gates always update). Outside, #79's
  `updateDrawnAppearances` refreshes the appearance only
  (`refreshAppearanceOnly`): buildings do not change state, nothing there
  advances as a game object. This is the "animation" gap at the sides, and it
  is the original game's off-screen behaviour made visible. (Corrected in
  `VR-HEAD-CAMERA.md`: turrets are not among these, `GameObjectManager::update`
  runs them every step, so checks below that look for a turret stopping find
  nothing.)
- **Movers already pass.** Since #79 `recalcBounds` in VR culls by distance
  only (`Camera::drawsAll`): the view-cone test is
  `(cosine > eye->cosHalfFOV) || eye->drawsAll()` and the screen-rect test
  `eye->drawsAll() || on screen` in `mech3d.cpp`, `gvactor.cpp`,
  `genactor.cpp`, `bdactor.cpp`. So movers animate, set `windowsVisible` and
  carry screen bounds everywhere within `MaxClipDistance`. The to-do's
  "units between the two do not animate" is not true for mechs and vehicles
  after #79; check an ordinary building, not a mech, when looking for the gap.
- **Cone tests #79 did not touch** still use `cosHalfFOV` alone: the mech arm
  pieces (`Mech3DAppearance::leftArmRecalc` / `rightArmRecalc`), artillery
  markers (`artlry.cpp`), clouds (`clouds.cpp`). `GameObject::onScreen`
  (lights, `light.cpp`) uses `projectZ`'s screen test.

Widening `camera_fov` to the headset's view while in VR closes all of these
with one number, which is why it was chosen over fixing each site. Accepted
cost: VR gameplay can differ from a flat run of the same mission (more
objects update per step). The flat game must stay byte-identical.

## What `camera_fov` feeds

Know these before changing it; the gotchas below come from them.

| Use | Where | Updated when |
|---|---|---|
| `cameraToClip` (the projection): `horizontal_fov = camera_fov` in radians, vertical from `Environment.screenHeight / screenWidth` | `Camera::setOrthogonal` | every `calculateProjectionConstants` (every `Camera::update`) |
| Sphere-clip constants for terrain vertices and buildings | `Camera::calculateProjectionConstants` | same |
| `cosHalfFOV`, the view cone for objects | `Camera::init` (`cos(fov * 0.7071 * DEG)`), `Zoom*` (`cos(fov * DEG)`, a different formula, `SimpleCamera` only), the `-benchfov` block in `Mission::update` | **not** by `setFieldOfView`: a stale cone is the first thing to get wrong |
| `goalFOV` / `goalFOVTime` | `Camera::updateGoalFOV`, from `Camera::update`; set by ABL `setcamerafov` / goal (cinematics) | every update while a goal runs |
| `camera_fov = 40` reset | `Camera::init` (every mission), `Zoom*` | mission start, so a one-off set does not survive |

`camera_fov` is **horizontal**, at the window's aspect. The VR window is the
800x600 interface, so vertical = 2 atan(0.75 tan(h / 2)): 40 gives 30.5,
100 gives 83.6, 110 gives 92.6. `-benchfov` (`gos_GetBenchFOV`,
`Mission::update`) already forces a wide FOV the right way: `setFieldOfView`,
`setFieldOfViewGoal(fov, 0)` to cancel scripted goals, and `cosHalfFOV` with
the 0.7071 factor. Phase 0 measured it: about 8% more CPU per frame at 100,
a quarter more triangles (`VR-TABLETOP.md`, Phase 0 results).

## Plan

### 1. The number: a pure function, test first

`vr_eye::gameFieldOfView(const float leftFov[4], const float rightFov[4],
float heightOverWidth)` in `GameOS/gameos/utils/vr_eye.{h,cpp}`
(GL-free, already unit tested), returning the horizontal FOV in degrees for
the game camera:

- half width: the largest of either eye's left and right angles, because the
  game camera is symmetric (inputs in OpenXR's `XrFovf` order: left, right, up,
  down, radians, left and down negative, as `gosVREyeView::fov`);
- half height: the same over the up and down angles; the game camera must cover
  it too, so convert: `atan(tan(half height) / heightOverWidth)`;
- result: twice the larger of the two, clamped to `[kMinGameFov, kMaxGameFov]`,
  20 and 120.

A Quest 2 gives 115.6 (eyes -49 to 49 by 48 to -50; the vertical decides). Tests in `tests/unit/test_vr_eye.cpp`: symmetric eyes give the
union; a tall FOV drives the answer through the aspect; the clamp; and a
degenerate (zero) FOV gives the minimum, not NaN.

### 2. One way to force the FOV on a `Camera`

`Camera::setFixedFieldOfView(float degrees)` in `mclib/camera.{h,cpp}`:
`camera_fov = degrees; goalFOV = degrees; goalFOVTime = 0; cosHalfFOV =
cos(degrees * 0.7071f * DEGREES_TO_RADS)`. Move the `-benchfov` block in
`Mission::update` onto it (same behaviour, one place). The sphere-clip
constants follow at the next `calculateProjectionConstants`, which
`eye->update()` runs right after.

### 3. GameOS: the headset's view as one number

`float gos_GetVRFieldOfView()` in `gameos.hpp` / `gameos_graphics.cpp`,
backed by `gos_xr`:

- `MC2_XR_FOV=<degrees>` (environment, like `MC2_XR_HUD`) overrides: 20 to 120,
  or 0 to keep the game's own (`vr_eye::parseGameFov`); anything else prints a
  line and is ignored;
- otherwise, while `gos_IsVR()`: `gameFieldOfView` of the **first** eyes located
  (`gos_xr::eyeViews`), whose angles are kept, so the session losing focus
  (system menu, Guardian) changes nothing, never back to 40; the aspect is the
  window's now, as it can change during a mission. Before any eyes were located,
  100;
- not in VR: 0 (off). `MC2_VR_PATHS=1` on a desktop makes `gos_IsVR()` true
  with no eye views, so it gets the 100 default or `MC2_XR_FOV`: that is the
  desktop test.

Logged when the value changes: `[VR] eyes: left ..., right ... degrees; game
camera N degrees`.

### 4. Apply it every game step

In `Mission::update`, in the `-benchfov` block before `eye->update()`:
`forcedFOV = gos_GetBenchFOV() > 0 ? gos_GetBenchFOV() : gos_GetVRFieldOfView();
if (forcedFOV > 0) eye->setFixedFieldOfView(forcedFOV);`.
Every step, because `Camera::init` resets to 40 at each mission start and
ABL cinematics set their own FOV. `-benchfov` keeps priority so bench runs
stay comparable.

Nothing changes in `vrPickWorld`, `seesForGame`, `findObjectByMouse` or the
pointer: they widen with the projection. The to-do's worry, "widen the
picking guard with it", is satisfied by construction; check it, do not
re-implement it.

### 5. Left out

Nothing shows this gap yet. If something does: add `|| eye->drawsAll()` to the two arm cone tests in `mech3d.cpp` so a
blown-off arm is drawn wherever the head looks even if the clamp ever cuts
the cone short; it matches what #79 did to the body test. Leave `artlry.cpp`
and `clouds.cpp` alone unless the headset shows a gap.

### 6. Docs

`VR-TABLETOP.md`: the to-do becomes a Phase 3 progress entry pointing here;
`HARDWARE-TESTS.md`: the headset checks below; `PROGRESS.md`: the dated
line; `android/README.md` or the VR plan's controls section: `MC2_XR_FOV`.

### Order of work

1. Test, then `gameFieldOfView` (step 1). `scripts/check.sh`.
2. `setFixedFieldOfView` and the `-benchfov` move (step 2); a `-bench` run
   with `-benchfov 100` before and after gives the same `[STATEHASH]`.
3. `gos_GetVRFieldOfView` and the `Mission::update` hook (steps 3, 4).
4. Desktop: `MC2_VR_PATHS=1 ./mc2 -mission mc2_01 -autoexit 600` (the window
   shows the 100 degree view), then with
   `MC2_XR_FOV=60` and `MC2_XR_FOV=200` (clamp message).
5. Flat game unchanged: `scripts/smoke.sh --deterministic` on one mission
   before and after, same `[STATEHASH]`.
6. Cost: `scripts/bench.sh ... --scenario bench/mc2_19.fit --fov 110` against
   `--fov 40`: `objects_ms`, `logic_ms`.
7. Headset (`android/README.md`, `MC2_XR_PICK_LOG=1` in `mc2args.txt`):
   the log line's numbers; a world click near the edge of the view picks
   (the log says "screen on"); selection brackets and health bars look the same size as before;
   `mc2.log`'s frame rate against the 66 fps of #79.

## Gotchas

1. **`setFieldOfView` leaves `cosHalfFOV` stale.** The cone is set by hand in
   three places with two formulas (`Camera::init` uses `fov * 0.7071`, the
   `Zoom*` methods `fov`). Use `setFixedFieldOfView` for the VR path and do
   not "fix" the `Zoom*` formula: they are only reached from `SimpleCamera`
   (the logistics mech viewer) and changing them changes the flat game.
2. **Horizontal, at a 4:3 window.** `camera_fov` is the horizontal angle;
   the headset's per-eye FOV is taller than it is wide. Take the number from
   the vertical when that is the larger demand (step 1) or the top and
   bottom of the view keep the gap. Do not read the aspect from the eye
   image (1832x1920); the game camera projects to `Environment.screenWidth`
   by `screenHeight`.
3. **Clamp at 120, not `-benchfov`'s 170.** The cone test is `cosine >
   cos(fov * 0.7071)`: at 127.3 degrees that cosine is 0 and above it is
   negative, so objects behind the camera pass. The sphere-clip constants are
   `tan(fov * 0.7071 * res / |res|)` and run away towards 159. 120 covers any
   headset the engine will meet.
4. **Only in VR.** The force is gated on `gos_IsVR()` (through
   `gos_GetVRFieldOfView` returning 0). A flat run must give the same
   `[STATEHASH]` as before. The hash covers only movers' position, rotation
   and status (`stateHash` in `benchdirector.cpp`), so it says little about
   this change: mc2_19's battle with `MC2_VR_PATHS=1` gave the same hash at
   40 and 100 degrees, and a turret that fires differently need not move it.
   The nightly smoke sets neither.
5. **Views come and go.** `gos_xr::eyeViews` returns 0 whenever
   `g.viewsValid` is false (`xrLocateViews` not run or failed, session not
   focused). Without the latch the FOV would snap to 40 and back each time
   the system menu opens, flipping every active block and `windowsVisible`.
   Latch; the game does not need the eyes' exact FOV this frame, only a
   stable one.
6. **Set it every step.** `Camera::init` restores 40 at every mission start; ABL `setcamerafov` / `setcamerafovgoal` run in cinematics.
   In VR the scripts' FOV has no visual effect anyway (the eyes have their
   own frusta), so overriding them loses nothing; say so in the docs.
7. **Overlays should keep their size; check that they do.** Only an anchor
   goes through `Appearance::toEye` / `overlayToEye`; bars, brackets and text
   then add their sizes in eye pixels (`drawBars`, `drawSelectBox`,
   `drawTextHelp`), scaled by `getScaleFactor`, not the FOV. The anchors are
   projected corners of the box (`boxCoords` in each `recalcBounds`), which
   map back to the same world point at any FOV. An object whose box stays the
   coarse one (gotcha 8) keeps an offset in game pixels, which `overlayToEye`
   scales up by the clip ratio, about 3.9 times more than at 40 degrees: look
   for a bracket too far from its object.
8. **Coarse bounds use `getScaleFactor`, not the FOV.** Every `recalcBounds`
   first builds `upperLeft` / `lowerRight` as `screenPos +
   bounds * eye->getScaleFactor()` (altitude-based, FOV-blind), uses that
   as the on-screen gate (bypassed by `drawsAll` in VR), then replaces it with
   the projected box. Harmless in VR, but do not use the coarse box for
   anything new.
9. **Mirrored projections stay.** `projectZ` has no near-plane test: a point
   behind the camera (clip `w > 0`) lands mirrored on the screen. The wider
   cone makes it rarer, not impossible (head turned round). Keep
   `seesForGame`'s `w < 0` guard and `vrPickWorld`'s `onScreen` test.
10. **More blocks active means more game objects updating.** Ordinary
    buildings and trees in the newly active blocks take the full
    `GameObject::update`, not `refreshAppearanceOnly`; the render side does
    not change (it already draws the whole window). Phase 0's 8% was measured
    on the PC at 100; measure at 110 on the headset, where the 30 Hz step is
    the budget (`gos_xr::logicStep`, `MC2_XR_LOGIC_HZ`). If it hurts, the
    knob is `MC2_XR_FOV`, not a lower clamp.
11. **The terrain window is the real edge.** Active blocks can now reach the
    window's edge (`GameVisibleVertices`, 30 to 60 vertices, the fog
    setting): objects there pop as the window moves. Drawing already shows
    the edge in VR (the open to-do in `VR-TABLETOP.md`); this makes it a
    logic edge too. Not new, but it will be blamed on this change.
12. **`-stereo` and `SimpleCamera` are untouched.** The side-by-side debug
    view is not `gos_IsVR()`; the logistics viewer is another `Camera`
    object; the force applies to `eye` in `Mission::update` only.
13. **The HUD is separate.** The 2D interface is a quad layer; the pointer
    margin (`kPointerMargin`, 12 game pixels) and the HUD hit test do not
    care about the world FOV. One game pixel now covers more world, so the
    virtual mouse's resolution over the world drops by the same 3.9; tile
    and unit picking tolerate it (boxes and tiles are many pixels), but
    check that a small distant vehicle can still be clicked.
14. **Pitch and the sky.** The game camera is pitched down
    (`projectionAngle`); with a vertical of about 100 degrees the top of its frustum is
    above the horizon, so `gameOnScreen` marks sky-side vertices and blocks
    as active too. Nothing is drawn by the game camera in VR, so this only
    adds a few active blocks; it is why the vertical should not exceed what
    the headset needs.
15. **`camera_fov` is not saved** (`Camera::save` writes `ProjectionAngle`,
    `FOVMin`, `FOVMax`), so a game saved in VR loads flat at 40 and in VR
    forced again. Good; do not add it.

## Results so far (2026-11-02)

Desktop GL, Mesa llvmpipe under Xvfb, `MC2_VR_PATHS=1` (so 100 degrees, there
being no eyes to ask):

- mc2_19's bench scenario, 1800 frames, at `MC2_XR_FOV=0` and 100: both ran
  clean, same `[STATEHASH]`, and the screenshots show the wider view, the
  minimap's view cone included. Draws and triangles barely move (908 against
  903 draws), since the VR paths already draw the whole window. Timings are
  not comparable: other sessions were loading the machine.
- With a wide view on a flat window the terrain window's edge and the cloud
  layer show, the open to-do in `VR-TABLETOP.md`; not caused by this change,
  and the headset already shows it.
- Quest 2, first run ("much much better", more testing wanted): eyes -49 to
  49 by 48 to -50 degrees, so 115.6 for the game camera; of 55 sampled pointer
  hits 53 were on the game screen, the other two during the mission's opening
  camera move. Turrets, overlay sizes and the frame rate are not yet checked.
- That run also logged `XR_ERROR_POSE_INVALID` 18 times, each a dropped frame,
  never seen before: picks now reach far enough for a beam hundreds of metres
  long, and `vr_controls::beamPose` lost precision working from its middle
  (0.86% off unit in a test). Fixed and pinned by a unit test; the headset has
  to confirm that this was the cause. The next run, about 50 seconds: none.
- Frame rate in that run (`VrApi` in logcat), quiet mc2_01: 72 fps in nearly
  every second after 9 seconds of startup, under 1% stale frames, GPU 71% on
  average, CPU and GPU 14.3 ms against a 13.9 ms budget. No 40-degree run to
  compare with, and no battle: see the mc2_19 bench below.

- Quest 2, mc2_19 bench, 3600 frames, build 235feba7, three runs at
  `MC2_XR_FOV=0` and two at the default (115.6), alternating: 69.6 to 70.0 fps
  against 69.4 to 69.5; the game step, frames with `logic_ms` over 2, 4.8 ms
  against 4.95 ms; `objects_ms` 0.86 against 0.88; p99 about 27.8 ms in both.
  Both sit a little under 72 in this battle. The wider view costs about
  0.1 ms a step here, and few turrets are near this fight. The state hash
  differs between runs at the same setting, so it cannot compare them.
- One earlier 50-second mc2_01 run held about 66 fps with the CPU clock lower
  and no more load; not explained, and not seen in these five runs, where the
  clocks were higher.
- Turrets update every step whatever the view, so there is no turret to check
  (`VR-HEAD-CAMERA.md`).

## Found in review (2026-11-02)

- **Picks beyond the terrain window.** With the window's edge on the game's
  screen the ray could meet ground nothing draws, and `inverseProject`, finding
  no tile, projected the whole map every step and could settle on a mirrored
  point behind the camera. The ground now stops at the window
  (`Terrain::isInWindow`), and in VR that search is skipped: the point stays the
  last one found. Units past the window are drawn, so they can still be picked.
- **A mirror image of the view behind the camera.** `Terrain::geometry`'s test
  takes `fabs(1 / y)`, so it passes vertices behind the camera. At 40 degrees
  that is a sliver; at 115.6 most of the window, so what was behind the player
  ran full game logic. In VR those vertices no longer count as in the game's
  view (`Camera::isInFront`). So turning the head right round shows objects
  appearance only, as at 40 degrees; a game camera following the head's yaw
  (below) is the answer if that shows.
- `MC2_XR_FOV=0` keeps the game's 40 degrees, but not quite the game's own view:
  the in-front test applies in VR at any field of view, so the sliver behind the
  camera is dropped there too, and the 40-degree bench runs above had that fix.
- Still open:
  - Ground under the game's 2D panel cannot be clicked, about 32 to 50 degrees
    below where the game camera looks (`gos_xr`'s alpha test). Now below where
    the head looks: look at it to click it (`VR-HEAD-CAMERA.md`).
  - The sum assumes the eyes look where the game camera looks, but they keep a
    level horizon while it is pitched down: at a tilt of about 65 degrees, far
    ground straight ahead is off the game's screen. Tilts of 35 to 50 are fine.
    Closed by the game camera following the head (`VR-HEAD-CAMERA.md`).
  - In VR a click can use the last point found if the pick leaves the window
    before the game step reads it (the map scrolling); it should be rare.
  - The minimap's wedge was wrong at this field of view; fixed in
    `VR-HEAD-CAMERA.md`.
  - A script's field of view lasts until the next game step.
  - A ray entering the window from outside can report a hit at its edge (the
    camera outside the window at full zoom-out, the ray low); it should be rare.
  - No unit test for `Terrain::isInWindow` or for keeping the first eyes in
    `gos_GetVRFieldOfView`.

## Alternatives considered

- **Pick in world space, skip the screen.** `vrPickWorld` already has the
  world point; feeding orders straight to the game would need a new path
  through `MissionInterfaceManager`, and fixes picking only.
- **Let the logic eye follow the head's yaw.** Keeps a 40 degree cone where
  the player looks; cheaper per step, but turning the head would change which
  terrain objects are alive. Added on top of this change, at the headset's
  field of view and following pitch too: `VR-HEAD-CAMERA.md`.
- **A second, draw-only active-block set for logic.** Already exists for
  drawing (`objBlockInfo[].drawn`); using it for logic is the same gameplay
  change as widening the FOV with more code.
