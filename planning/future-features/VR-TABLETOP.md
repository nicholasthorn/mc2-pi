# VR: head-tracked game view, tabletop option (Meta Quest 2)

Status: Phase 0 in progress. The bench instrumentation (`<stage>_ms`,
`-benchfov`) and the PC results are in; the go/no-go now waits on a
sideloaded Quest 2 test build and a PC run on a real GPU (the Pi runs were
dropped as a proxy). Phase 1 has a side-by-side `-stereo` debug view with
per-eye shadows (see "Phase 1 progress").
Written 2026-10-29 from a read of the code on
`pi-port`, revised 2026-11-01 (requirements changed: play the game's own view
in VR first; tabletop becomes an option; no whole-map rendering).
File and function names were checked then, line numbers were not recorded on
purpose (grep the symbol).

## Problem

Play MC2 in a VR headset. First goal: the game **as it is**, with the existing
camera view in stereo, the head moving the camera (look around, lean in), and
the controller sticks moving it laterally across the map. The 2D interface
(control panel, tac map, menus) floats on a panel. Later, as an option: a
**tabletop** mode where the battlefield sits on a dining table at miniatures
scale. Target: standalone Quest 2 (and Quest 3/3S, same stack), with PC VR
over Quest Link as the development route.

## Scope

In: stereo rendering of the game's current view, 6DoF head tracking on top of
the game camera, stick movement (pan, rotate, zoom), the existing 800x600
interface on a floating panel, Android/Quest packaging for sideloading. Then
the tabletop option.

Out (for now): **rendering the whole map** (too large; not needed, see the
scale estimate), first-person cockpit, hand tracking, redesigned interface,
multiplayer, store distribution (the game data is under the original Microsoft
EULA; sideload only), Vulkan.

## Affected areas

Renderer (GLES and desktop GL), camera (`mclib/camera.*`, `code/gamecam.cpp`),
terrain (`mclib/terrain.cpp`, `mclib/quad.cpp`), object appearances
(`mclib/*actor.cpp`, `mech3d.cpp`, `tgl.cpp`), the texture/render queue
(`mclib/txmmgr.cpp`), the mission frame (`code/mission.cpp`), input
(`missiongui.cpp`, `GameOS/gameos/gameos_input.cpp`), platform/build (Android,
OpenXR).

## The numbers that set the scale

From the code (`mclib/terrain.cpp`, `code/prefs.cpp`, `code/mechcmd2.cpp`,
`mclib/camera.cpp`):

| Quantity | Value |
|---|---|
| `metersPerWorldUnit` | 0.2 (so `worldUnitsPerVertex` = 128 is 25.6 m) |
| Map sizes (`realVerticesMapSide`) | 60, 80, 100 or 120 vertices: 1.5 to 3.1 km a side |
| Terrain window (`GameVisibleVertices`) | 30 + 30 x fog setting: 30 to 60 vertices, 0.77 to 1.54 km a side |
| Camera altitude | `AltitudeMinimum` 560 to `AltitudeMaximumHi` 1600 world units: 112 to 320 m |
| Camera FOV | 40 degrees default (`camera_fov`); `FOVMin`/`FOVMax` 20 to 90 |
| Far clip | `MaxClipDistance` 3000 to 5000 world units: 600 m to 1 km |

Mech height is not checked yet: BattleTech mechs are 8 to 16 m, so assume 10
to 12 m. Measure a mech's bounding box in the data before relying on it.

## Two modes, one mechanism

Both modes place the game world in the room with one **anchor transform**: a
position on the map, a rotation, and a **world scale** (real metres per world
metre). The head pose is applied on top. They differ only in scale and where
the ground sits.

### Mode 1: head-tracked game view (first target)

The game camera keeps its height and angle; the headset adds look-around and
lean. The scale matters more than it looks:

- **At 1:1** the player hovers 112 to 320 m up, like riding in a helicopter.
  The view is correct but flat: a 64 mm eye separation gives almost no stereo
  depth at a few hundred metres, and leaning 30 cm barely changes anything.
  It still feels like being there; worth trying.
- **At roughly 1:100 to 1:300 ("giant")** the camera's 112 to 320 m becomes
  1.1 to 3.2 m above the ground at 1:100, or 0.4 to 1.1 m at 1:300, so stereo depth and leaning work, and mechs are a few
  centimetres to a decimetre tall. This is effectively a floor-sized diorama
  seen from the game's usual angle.

Make the scale a setting (and a stick binding) and try both. Lateral stick
movement pans the anchor, as the edge-scroll does today; smooth motion at 1:1
risks motion sickness, less so at giant scale. Offer a comfort vignette and
snap rotation.

Terrain cost: the same window as today (30 to 60 vertices). No change.

### Mode 2: tabletop option (later)

Dining table: about 1.8 x 0.9 m (six seats), up to 2.4 x 1.0 m. Ground at table
height, about 0.75 m.

| Scale | Mech (10 to 12 m) | Ground on a 1.8 x 0.9 m table | In terrain vertices |
|---|---|---|---|
| 1:285 (6 mm BattleTech minis) | 35 to 42 mm | 513 x 256 m | 20 x 10 |
| 1:850 (today's largest window fills the table length) | 12 to 14 mm | 1.5 x 0.77 km | 60 x 30 |
| 1:1700 (a 120-vertex map fills the table length) | 6 to 7 mm | 3.1 x 1.5 km | 120 x 60 |
| 1:3400 (a 120-vertex map fits the table width) | 3 mm | whole map | 120 x 120 |

Conclusion: **at miniatures scale the table holds about 20 x 10 vertices**,
well inside the window the game already projects (30 to 60). Showing a whole
map would need a table about 11 m long at 1:285, or 3 mm mechs. So the
tabletop shows a patch of the map, and the sticks scroll the map across the
table, as panning does in mode 1. A scale range from about 1:285 to 1:850
costs no more terrain than the game does now.

## How the engine draws today (what VR has to work with)

1. `Mission::update` (`code/mission.cpp`) runs once per frame, in order:
   interface update, `eye->update()` (camera matrices), `land->update()`,
   weather, paths, `land->geometry()`, `ObjectManager->update()` (or
   `updateAppearancesOnly()` when paused), craters, `mcTextureManager->update()`.
2. **Projection happens during update, not draw.**
   - `Terrain::geometry()` calls `eye->projectZ` on every visible terrain
     vertex and stores screen coordinates (`px`, `py`, `pz`) used later by
     `TerrainQuad::draw` in `quad.cpp`. It also sets the object-block
     "active" flags.
   - Object appearances (`Mech3DAppearance::update` etc.) run
     `TransformMultiShape`, which bakes `shapeToClip` (camera included) into
     the shapes. `recalcBounds` does screen-space culling (`inView`) and
     screen-space selection boxes.
   - Effects, craters, shadows, MLR and on-screen markers also go through
     `projectZ` (about 100 call sites over 21 files).
3. `Mission::render` -> `eye->render()` -> `gamecam.cpp` enqueues sky,
   terrain, craters, objects, water, shadows, then
   `mcTextureManager->renderLists()` issues the draws; effects and weather
   follow; then `FloatHelp::renderAll()` and `missionInterface->render()` draw
   the 2D interface; then the TGL RAM pools (`vertexPool`, `colorPool`, ...)
   are reset.
4. Two geometry paths reach GL: CPU-projected screen-space `gos_VERTEX`
   (terrain, effects, interface) with an orthographic shader, and
   GPU-transformed shapes (`TG_RenderShape`: vertex/index buffers + `mvp_`, and
   the world matrix `mw_`) drawn by `ShapeRenderer` in `txmmgr.cpp`.
5. The camera is one object: `cameraOrigin` + `cameraToClip` give
   `worldToClip`; `TG_Shape::SetCameraMatrices` copies them for shapes.
   Mouse-to-world goes through `eye->inverseProject` (one site in
   `missiongui.cpp`); object picking uses screen-space bounds and
   `PerPolySelect`.

Consequence: **calling `render()` twice with two eye cameras does not work.**
Each eye needs the update-time projection steps re-run with its own camera.

## Approach: one logic eye, two render eyes

Keep the game's own camera as a mono **logic eye**. It drives everything it
drives today: game update, active-block flags, culling decisions that affect
gameplay, mouse picking and the interface. Add two **render eyes** that only
draw.

The logic eye **does not follow the head** (revised after the Phase 1 audit):
it stays the game's own camera at the game's FOV, because what it counts as
in view drives gameplay (`inView` / `windowsVisible`). The render eyes take
the head pose and the headset's FOV (about 90 to 100 degrees, against the
game's 40) and cull with their own frusta in a transform-only pass. The
terrain window stays
centred where it is today. Its edge is in view at 100 degrees (see "Phase 0
results"), so mode 1 needs a larger window, haze pulled in to the window's
radius, or a lower camera.

Per frame:

1. Normal `Mission::update` with the logic eye (unchanged gameplay).
2. For each render eye (left, right):
   - set the camera matrices from OpenXR: `cameraOrigin` = anchor x eye pose,
     `cameraToClip` from the eye's asymmetric FOV (`XrFovf`), viewport =
     swapchain size;
   - re-run the projection-only steps: terrain vertex projection (the
     `projectZ` part of `Terrain::geometry`, **without** touching the
     active-block flags), appearance transforms (the transform part of
     `updateAppearancesOnly`, **without** advancing animation), crater/effect
     projection;
   - render the world pass into that eye's swapchain image;
   - reset the TGL pools between eyes (or size them for two passes).
3. Draw the interface once into an offscreen 800x600 texture; show it as an
   OpenXR quad layer beside the view.

This needs the projection side effects separated from the state side effects in
`Terrain::geometry`, the appearance `update()` functions and the effects. The
gait double-step fix (`495f7ba`, steps at most once per `turn`) already guards
one known case; the rest need an audit.

Picking stays on the logic eye: intersect the controller ray with the terrain
(heights via `Terrain::getTerrainElevation`), project the hit point through
the logic eye to a screen position, and feed that as the mouse. All existing
selection code (`inverseProject`, screen bounds, `PerPolySelect`) then works
unchanged. Accuracy for tall objects is approximate; refine later with a ray
test against object bounds if needed. Until controllers work, the mouse keeps
working through the logic eye.

Rejected for a first version:

- **Moving all geometry to GPU transforms** (world-space vertex buffers, eye
  matrices in shaders, `GL_OVR_multiview2` single-pass stereo). Best
  performance, but a rewrite of terrain, effects and shadows. Keep it as the
  optimisation path (last phase).
- **Rendering mono onto a virtual screen.** Cheap, but no depth and no
  head-driven camera.
- **Rendering the whole map.** Not needed at any useful scale (see above).

## Things that assume a flat screen

- **Sky dome:** keep in mode 1, drop on the tabletop.
- **Distance haze:** keep in mode 1, but today's haze does not reach the
  terrain window's edge at a headset-wide FOV (Phase 0); on the tabletop it
  needs a table-edge treatment instead (see open questions).
- **Letterbox, fades, compass:** move to the interface panel or drop.
- **Culling:** `cosHalfFOV` and the screen-rect test in `recalcBounds` stay on
  the logic eye; the render eyes' transform-only pass culls against its own
  frustum.
- **World-anchored overlays** (selection brackets, health bars, waypoints,
  `FloatHelp`) are projected with `projectZ` in `missiongui.cpp` (5 sites) and
  game objects. They must be projected per eye so they sit at the right depth,
  or be redrawn as 3D billboards.
- **Depth:** use a 24-bit depth swapchain (the window context asks for 16-bit
  today) and recheck near/far for each scale.

## Controls (Quest Touch)

Starting point, to be tried in the headset:

- Left stick: pan the anchor across the map (the edge-scroll today).
- Right stick: snap rotate (left/right), zoom or scale (up/down).
- Trigger: select / command (left mouse); grip: right mouse.
- Ray from the controller for picking (logic-eye virtual mouse) and for the
  interface panel.
- A button to recentre the anchor under the head.

## Platform

- **PC first:** OpenXR (Khronos loader) with desktop GL (`XR_KHR_opengl_enable`)
  on Windows or Linux, headset over Quest Link / Air Link (Meta runtime or
  SteamVR). Fast iteration, debuggers, no packaging.
- **Quest:** Android NDK build (the arm64 build and GLES 3.1 renderer already
  exist), SDL2 Android project, OpenXR loader for Android,
  `XR_KHR_opengl_es_enable` with `XrGraphicsBindingOpenGLESAndroidKHR` (the
  EGL display/config/context from SDL's context). Meta treats GLES as legacy:
  supported, but new features are Vulkan-only. Fine for this project.
- **Frame loop:** `xrWaitFrame` / `xrBeginFrame` / `xrEndFrame` replace
  `SDL_GL_SwapWindow` (single call site in `gos_render.cpp`); the game renders
  into swapchain textures through FBOs instead of the default framebuffer.
- **Android gaps:** `HOME` / `XDG_CONFIG_HOME` are unset, so settings would
  silently not save (`gos_LoadDataFromRegistry` in `gameos.cpp`); use SDL's
  Android storage path. Game data goes on app storage (sideloaded once).
  `dlopen` of `libmc2res` works from the app's lib directory.
- **Quest 2 lifespan:** in maintenance, no announced end date (late 2026).
  Quest 3/3S run the same stack.

## Performance

- Budget: 72 Hz, two eyes, roughly 1832x1920 per eye at full resolution (render
  lower and let the compositor scale; use fixed foveation).
- CPU: the game is single-threaded. Per frame it would do the update once and
  the projection steps three times (logic eye + two eyes), over today's
  terrain window. The render eyes' wide FOV adds objects to transform. The
  XR2's CPU is much faster than a Pi 4's; PC ratios are in "Phase 0 results",
  Quest numbers come from the test build.
- Measure first: `-bench` now reports per-stage times (`<stage>_ms`, see
  `bench/README.md`) and `-benchfov` forces a headset-wide FOV.

## Phase 0 results (PC)

Run 2026-11-01: `bench/mc2_19.fit` (3600 frames, the player's lance against
20 enemies), desktop GL build, Mesa llvmpipe under Xvfb in WSL2 (16 threads).
Two runs at FOV 40 (they agree within 5%) and one quiet run at FOV 100; a
second 100-degree run collided with another session's job (load average 21,
27 fps) and is left out, as is a first pair taken under a load average near
30. llvmpipe rasterizes on the CPU, so the absolute times are not Quest or Pi
numbers; the counts and the shares within a run are what carry over.

| Per frame, whole run | FOV 40 (mean of 2) | FOV 100 |
|---|---|---|
| fps | 45.9 | 44.9 |
| draws | 696 | 707 |
| triangles | 6022 | 7578 (+26%) |
| `logic_ms` / `render_ms` / `swap_ms` | 8.62 / 6.74 / 6.42 | 9.04 / 7.50 / 5.73 |
| `camera_ms` | 0.04 | 0.04 |
| `terrain_geom_ms` | 0.18 | 0.22 |
| `objects_ms` (of which `shape_xform_ms`) | 2.29 (1.05) | 2.45 (1.15) |
| `world_render_ms` | 4.68 | 5.41 |
| `ui_render_ms` | 1.67 | 1.67 |
| per-eye work | 5.91 ms, 38% of the CPU frame | 6.78 ms, 41% |

Per-eye work is `terrain_geom` + `shape_xform` + `world_render`; the CPU frame
is `logic` + `render` (`swap` here is llvmpipe rasterizing, which is GPU work
on real hardware).

- **Stereo costs about 1.5 times the mono CPU frame.** With the logic eye at
  the game's FOV doing the update once (`logic`, 8.6 ms, including its own
  projection), the interface drawn once (1.7 ms), and two render eyes at
  100 degrees (2 x 6.8 ms), the frame is about 24 ms of CPU against 15.4 ms
  mono. Under llvmpipe `world_render` includes the driver's CPU work, which a
  hardware driver does more cheaply, so this is an upper bound for that part.
- Most of `logic_ms` (about 6 of 8.6 ms) is game logic outside the measured
  stages: pathing, AI, sensors, collisions, the mission script. It runs once,
  not per eye.
- A wider FOV costs little by itself: about 8% more CPU per frame. Terrain stays
  nearly flat because the projected window is a fixed vertex grid
  (`GameVisibleVertices`). The FOV adds about a quarter more triangles, mostly
  objects now in view.
- Budget: 72 Hz leaves 13.9 ms per frame. Whether the Quest's XR2 fits 24 ms
  of this PC's llvmpipe work into that is the go/no-go, and needs numbers
  from real GPU drivers and the real CPU: see below.
- **The terrain window's edge is in view at 100 degrees** from the game's
  camera height: the screenshot shows the map as a slab floating in black,
  beyond the haze (mc2_19 is a night map). Mode 1 needs a larger window
  (`terrain_geom` grows with its square, and is 0.2 ms at 60 vertices), haze
  pulled in to the window's radius, or a lower camera. The same picture shows
  the tabletop look comes almost for free: the window already reads as a
  diorama.

Still to do for the go/no-go (decided 2026-11-01; the Pi runs first planned
here were dropped: a Pi 4's Cortex-A72 at 1.5 to 1.8 GHz and VideoCore VI
say little about an XR2's A77-class cores at up to 2.8 GHz and its Adreno
650, and the decision only orders phases 4 and 6):

- **A sideloaded Quest 2 test build**: a flat Android app (no OpenXR yet)
  with the GLES renderer, running the mc2_19 bench scenario, mono and
  `-stereo`, and writing the frame CSV and `[BENCH]` lines for `adb pull`.
  It measures the real CPU and the real Adreno driver, and is the first part
  of Phase 4. **Run on a Quest 2, 2026-11-01** (results below).
- **A PC run on a real GPU**: takes llvmpipe's CPU cost out of
  `world_render` and gives clean per-stage CPU times to compare with the
  Quest's.

### Quest 2 results (2026-11-01)

mc2_19 bench scenario, 3600 frames, game at 800x600 in a flat window, GLES
3.1 on the Adreno 650, CPU at 1.8 to 1.86 GHz (the clocks a flat app gets):

| | fps | avg ms | p95 ms | logic ms | render ms | swap ms | draws |
|---|---|---|---|---|---|---|---|
| mono | 58.6 | 17.1 | 21.9 | 12.9 | 3.8 | 0.3 | 691 |
| `-stereo 40` | 52.1 | 19.2 | 24.4 | 12.8 | 6.0 | 0.3 | 1081 |

- The first run did 2 to 5 fps: 78 % of the CPU time was the Adreno driver
  allocating kernel memory for each `glMapBufferRange` with
  `GL_MAP_INVALIDATE_RANGE_BIT` (one per draw, about 0.4 ms each, found with
  `simpleperf`). Without the bit (the range is unused anyway) the render
  stage went from 199 ms to 3 ms; `glBufferSubData` is as slow as the
  original. llvmpipe screenshots and state checksums are unchanged by the fix.
- **Rendering is cheap:** the second eye adds 2.2 ms of CPU.
- **Game logic is the cost:** 12.9 ms against the 13.9 ms a 72 Hz frame has.
  The bench steps the game every frame; stepping it at 30 Hz and drawing the
  eyes at 72 Hz leaves about 6 to 7 ms of CPU per displayed frame.
- Not measured: GPU load at headset resolution (about 1832x1920 per eye);
  that needs the OpenXR path.

**Decision: go on the CPU route.** Phase 6 is not needed before OpenXR;
decoupling the game update from the display rate is.

## Phases

| Phase | What | Exit criterion |
|---|---|---|
| 0. Measure | Frame-stage times in `-bench` output; PC runs (llvmpipe done, real GPU to do) and a sideloaded Quest 2 test build, mono and stereo | Numbers in this file; go/no-go on the CPU route |
| 1. Desktop stereo, no VR | Logic-eye / render-eye split; side-by-side stereo debug mode on a normal window | Both halves correct; nightly smoke runs unchanged; no double animation steps |
| 2. OpenXR on PC (mode 1) | Swapchains, frame loop, anchor + world scale, head pose to the render eyes, interface on a quad layer, mouse still works | Mission playable in the headset over Link with mouse and keyboard, at 1:1 and giant scale |
| 3. Controllers | Stick pan/rotate/zoom, ray picking through the logic eye, button mapping, ray interaction with the interface panel | Full mission playable with controllers only |
| 4. Quest standalone | Android build, OpenXR GLES binding, storage paths, packaging, data sideload, foveation, resolution tuning | Mission runs at 72 Hz on Quest 2 |
| 5. Tabletop option | Table anchor at table height, scale 1:285 to 1:850, scroll the map across the table, table-edge treatment, no sky, grab/rotate the table | Comfortable tabletop view of a mission at miniatures scale |
| 6. Optimise (if needed) | Late-bind GPU shape matrices at flush (`mw_` x eye view-projection), multiview, GPU terrain | Frame budget met with margin |

Phases 0 and 1 need no headset and can be tested on the existing CI. Phase 0
decides whether phase 6 is needed before phase 4.

## Verification

- Phase 1 in CI: a smoke run with the stereo debug mode on, under llvmpipe,
  alongside the normal smoke runs; compare game outcomes (the `-bench` state
  hash) with stereo off.
- Phases 2 to 5 need a headset; record what was run where in `PROGRESS.md`.

## Phase 1 audit: projection mixed with state (2026-11-01)

A read of the update path for what each projecting function also changes. No
projecting function on the update path is projection-only, but two things
help:

- **Much of the world already projects at draw time**: craters
  (`CraterManager::render`), weapon bolts (`WeaponBolt::render`), rain
  (`Weather::render`), gosFX (the MLR clipper set up in `GameCamera::render`)
  and the GPU shape path (`TG_MultiShape::Render` rebuilds `shapeToClip` from
  `s_worldToClip`). These work per eye once the camera is swapped before
  `render()`.
- **`TransformMultiShape` caches node matrices per turn** (`calcedThisFrame ==
  turn`), so a second call in the same frame only redoes the camera part.
  Repeating it per eye is cheap and does not re-animate.

What needs work, riskiest first:

| Item | Problem | Fix |
|---|---|---|
| `*Appearance::recalcBounds` (mech, vehicle, building, tree, generic) | Interleaves the screen box and `inView` with LOD switching (deletes and recreates the shape) and view hysteresis | A new transform-only method per class that keeps the logic eye's LOD and only fills the eye's screen data |
| `inView` / `windowsVisible` | Feed gameplay: withdrawing units are destroyed off screen (`mech.cpp`), collision sounds, picking; vehicles, turrets and terrain objects animate only when in view | Keep them on the logic eye at the game's own FOV (below) |
| `appearance->update(false)` | Not repeatable: random gestures, `nodeRecycle`, flash timers, effect `Execute`, foot-poof craters (`updateAppearancesOnly` calls it) | Per-eye pass calls only the transform method, never `update` |
| `mcTextureManager->addTriangle` | Counts vertices during update; `addVertices` fills them at render into arrays of that size and drops the rest | Recount per eye (`clearArrays`, re-run the counting passes), or count for all eyes |
| `Terrain::geometry` | Also sets the object-block and vertex active flags, `eye->setInverseProject` (mouse picking), terrain light culling, the cloud update | A projection-only mode that skips those |
| Water in `TerrainQuad::setupTextures` | Projects once per frame, guarded by `calcThisFrame` bit 2 | Clear bit 2 per eye |
| `Clouds::update` | Accumulates UV scroll on every call | Split the scroll out |
| Camera statics | `cameraFrame`, the sphere clip constants, `HazeFactor`, `TG_Shape::s_worldToClip`; `GameCamera::render` sets `userInput->setViewport` | Save and restore around each eye; skip the input viewport for render eyes |
| `FloatHelp`, TGL pools | `currentFloatHelp` and the vertex/colour pools reset once per frame in `Mission::render` | Reset per eye, or size for both |
| Weather lighting | `Weather::update` writes the light colours onto `eye` | Copy them to the render eyes |

Mission state that must run once per frame and stay on the logic eye:
`GameObjectManager::update`, `MapData::update` (water phase, scrolling,
terrain window), `Weather::update`, `MissionInterfaceManager::update` (mouse
`inverseProject`), `mcTextureManager->update` (after the eye passes, so their
texture use counts).

**Answer to the old open question**: a head-following, widened logic eye does
change gameplay, through `inView` / `windowsVisible`. So the logic eye stays
the un-tracked game camera at the game's FOV; the head pose and the wide FOV
are used only by the render eyes, which cull with their own frusta in the
transform-only pass. Objects the logic eye does not see are not updated
(vehicles animate only in view); if that shows at the edges of a
headset-wide view, widen what counts as in view for animation only.

Side note, an existing quirk rather than a VR problem: `Mover::getLOSPosition`
calls `appearance->update()` when the weapon node has never been computed;
for a vehicle that advances its animation an extra frame that turn.

Phase 1 work, in order: the transform-only appearance methods and a
`GameObjectManager` pass over them; the projection-only terrain mode; per-eye
recount in the texture manager; camera save/restore; then the side-by-side
debug mode on a normal window, checked against the `-bench` state hash.
(Revised by the first slice below: most of that list turned out unnecessary
for a desktop debug view.)

## Phase 1 progress: side-by-side `-stereo` (2026-11-01)

`./mc2 -mission mc2_19 -stereo 40` draws the world twice, the left eye in the
left half of the window and the right eye in the right half, from two
cameras 40 world units (8 m) apart along the game camera's right axis. The
interface is drawn once, over the whole window. It is a debug view for
parallel ("wall-eyed") viewing; each half is squashed horizontally.

How it works, and why it needed far less than the audit's work list:

- **The engine can already draw a frame twice.** `renderLists` rewinds each
  texture's vertex and shape lists after drawing them ("comes in handy if
  gameLogic is not called"), so a second `render()` refills the same blocks.
  The counts stay valid as long as each eye draws the same triangles.
- **Meshes take the eye's camera at draw time.** `TG_MultiShape::Render`
  already rebuilt `shapeToClip` from `TG_Shape::s_worldToClip`; the GPU path
  in `TG_Shape::Render` now uses that matrix instead of the one stored during
  the update. In mono the two are the same (only `Camera::update` sets the
  camera matrices): mono screenshots are byte-identical to before.
- **Terrain is re-projected per eye** (`Terrain::projectForEye`): the land and
  water screen positions only, with each vertex's visibility (`clipInfo`)
  left as the game camera decided, so the vertex counts from the update fit.
  The height each water vertex was projected at is now stored
  (`Vertex::wElevation`) so it can be re-projected exactly.
- **Each eye is an offset applied last in `Camera::setCameraOrigin`**
  (`eyeOffset`, set by `Camera::renderWorld`): `GameCamera::render` rebuilds
  `cameraOrigin` every time, so the offset lives inside that rebuild, after
  the game camera's position and line-of-sight pitch fix are settled. Phase 2
  grows this into a full eye pose (rotation and its own projection).
- The GPU mesh path also takes its viewport at draw time.
- **GameOS points the GL viewport at one half** (`gos_SetStereoEye`): draws go
  out as issued, and screen-space and 3D draws both map through the viewport.
- Craters, weapon bolts, gosFX and weather already project while drawing.

Verified (desktop GL and GLES builds, llvmpipe, `bench/mc2_19.fit`, 3600
frames): the game-state checksum matches a mono run on every frame; the
measured disparity between the halves is 12 to 14 px, growing towards the
nearer bottom of the view, with features further right in the left image
(the eyes were swapped in the first version: the camera frame's right axis
points screen left in engine coordinates).

**Measured stereo cost** (quiet GL run, against mono on the same build):
`world_render` 3.88 to 8.31 ms, triangles 6022 to 11287, the CPU frame
(`logic` + `render`) 13.1 to 17.8 ms: **1.36 times** (a second pair of runs
after the review fixes: 10.2 to 14.2 ms, 1.40 times), below the 1.5 estimate
because no per-eye object transform is needed.

Shadows are per eye too (second slice, `vr-phase1b`): `MultiTransformShadows`
keeps computing each shadow vertex's world position during the update, and
`TG_Shape::RenderShadows` now projects those positions for the camera that
is drawing, once per vertex, as the update used to. Mono with shadows on is
byte-identical to before; with `-stereo` the state checksum is unchanged and
every screenshot differs from the first slice's. (Checked with `Shadows` and
`UseLocalShadows` on: the bench's minimum settings turn shadows off, and
mc2_19 is a night map, whose shadows come from point and spot lights.) The
cloud layer needs nothing: `Clouds::update` never sets `renderClouds`, so it
is not drawn in this build.

World-anchored overlays are per eye too (third slice, `vr-overlays`).
Selection brackets, health bars, icons and select boxes were already drawn
inside each eye's pass, but from the screen bounds `recalcBounds` computes
with the game camera, so they had no parallax: both halves showed them at
the same x, in the screen plane. Floating text (`FloatHelp`: unit names,
sensor labels) was queued and drawn once over the whole window, matching
neither half. Now:

- An eye differs from the game camera by a sideways shift only, so a point's
  screen x moves by a constant times its 1/depth (`screenPos.w`).
  `Camera::renderWorld` measures that constant per eye on the camera's
  look-at point (`Camera::eyeParallax`); `Camera::overlayShiftX` turns it
  into a shift for a given `screenPos.w`, which the `Appearance` draw
  functions and `getEyeScreenPos()` (sensor labels) add. `getScreenPos()`
  stays the game camera's position.
  It is 0 outside an eye's pass, so picking, drag-select and culling, which
  read the bounds during the update, are unchanged.
- Floating text added during an eye's pass is drawn in that eye's half and
  dropped (`FloatHelp::renderAndResetFrom`); text set during the update (HUD
  text) is still drawn once over the whole window.

Measured on mc2_19 screenshots (green bars and brackets only): the shift
between the halves went from 0 px to -15 px (engage) and -30 px (close-up),
matching the scene's -12 to -32 px. Mono is byte-identical, the state
checksum unchanged. The whole box shifts by its centre's parallax, a close
enough fit for a box a few metres deep.

Still full-window: mouse picking and the cursor, and the interface panels.
`-stereo` is for looking at the rendering, not for playing; Phase 2 puts the
interface on its own layer and picks with a controller ray.

Not covered by an automated check yet: the eye order. It rests on one
measured sign in `setCameraOrigin`; a flip passes the unit tests and the
state checksum. A smoke check could measure the disparity between the halves
of a `-stereo` screenshot (it should be negative: features further left in
the right half).

Objects and terrain the game camera culls are missing at the edges of an
eye's view; with eyes 8 m apart that is a sliver, with a headset-wide FOV it
is not (Phase 2 needs the render eyes to cull for themselves, which means
sizing the texture manager's counts for every eye).

Deferred to Phase 2 (decided 2026-11-01): the in-view results are gameplay
state as well as draw state. `inView` / `canBeSeen()` decide which vehicles
animate, sensor and fire-range checks in `mech.cpp`, and `windowsVisible`;
the terrain's `clipInfo` also sets the active object blocks. Widening them
for the eyes would make stereo gameplay drift from mono. The fix is a second,
draw-only result ("drawn by some eye") next to the game camera's exact one,
through `Terrain::geometry`, each appearance's `recalcBounds` and the draw
lists; it belongs with the OpenXR eyes, where the gap is large, rather than
the -stereo debug view, where it is a sliver.

Each half keeps the full window's aspect ratio, so the eyes are squashed 2:1
in this view; Phase 2 gives each eye its own projection and viewport size.

## Phase 2 progress: OpenXR on the Quest (2026-11-01)

Done straight on the Quest 2 (the adb loop already worked; PC over Link can
follow, the code is platform-neutral). Branch `vr-openxr`.

- **Session:** `GameOS/gameos/gos_xr.{h,cpp}` (`MC2_OPENXR`, on for
  Android): the Khronos loader (Maven Central, SHA-256 pinned), an instance
  and session on SDL's EGL context, a LOCAL reference space. The manifest
  makes it an immersive app with optional hand tracking (without that the
  headset refuses to launch unless a controller is awake).
- **Frame:** `gos_xr::beginFrame()` (in `draw_screen`) waits for the
  headset's frame and locates the eyes; `Camera::renderWorld` draws each eye
  into its own swapchain image (1440x1584 on a Quest 2, 4x MSAA rendered to
  texture). The window gets no world, only the interface drawn after it;
  `present()` sends the eyes as a projection layer and the window's frame
  as a see-through HUD quad (alpha-blended, transparent where there is no
  interface) 30 degrees below the horizon, where the player looks at the
  battle. `MC2_XR_EYES=0` shows the whole game frame on a big screen ahead
  instead.
- **Eyes:** `utils/vr_eye.cpp` (GL-free, unit-tested). The engine camera's
  axes are x screen left, y screen down, z forward (as `projectZ` maps them,
  whatever `UP_AXIS` says), a point reflection of OpenXR's. The eye is the
  game camera's yaw, then the head; by default the horizon stays level with
  the real one and the player looks down at the battle (with the game's
  pitch kept, `MC2_VR_GAME_PITCH=1`, turning the head swung the world about
  a tilted axis). Head movement is level with the ground, times
  `worldUnitsPerMeter` and `MC2_VR_SCALE`. The frustum is the eye's own,
  asymmetric field of view.
- **Meshes:** the CPU path projects shape vertices once, during the update,
  for the game camera, so in the eyes they were head-locked. In VR the GPU
  path (alariq's, off by default) is used while `gos_IsVR()`, alpha-textured
  shapes included (alpha-tested); shapes it cannot take (spotlights, windows,
  fading) are left out of the eyes. Its textured lit shader did not link on
  the Adreno driver: `calc_light` copied a whole light record (sixteen
  `mat4`s) into a local; it now reads the fields in place.
- **Overlays:** brackets, bars and floating text from the world are left
  out (screen positions from the game camera do not survive a turned head);
  they need drawing in world space. The interface's own text is in the HUD.
- **Textures:** in VR, 8-bit textures get mipmaps and trilinear, 4x
  anisotropic minification (object textures asked for none, and the
  renderer never made any): distant mechs shimmered.

Verified in the headset (mc2_19 bench battle): world fixed as the head
turns and leans, stereo fuses, up/down and side to side right, the HUD
frames the battle. The flat game is unchanged (llvmpipe screenshots and
state checksums against the previous branch). Debugging switches: `MC2_XR_POSE=yaw,pitch` fixes the head
rotation, `MC2_XR_SHOT=n` saves the left eye and the window frame.

Known gaps:

- **67.3 fps, nearly 72:** the game now steps at 30 Hz (`MC2_XR_LOGIC_HZ`)
  while the eyes are drawn every headset frame from the new head pose; each
  step gets the time since the last step, so game speed is unchanged. The
  median frame is 13.8 ms (on budget); frames with a game step take 21 ms
  or more (step 11 ms plus two eyes 10 ms). Next: the step on its own
  thread, or cheaper eyes. (Before: 47.6 fps stepping every frame; a third
  world pass for a tablet cost 1 ms more and was dropped for the HUD.)
- **Culling, terrain done:** in VR `Terrain::geometry` projects and draws
  every vertex in the terrain window (while `gos_IsVR()`), while
  the gameplay decision (which object blocks are active) keeps the game
  camera's view; no frame-time cost (67.0 fps), flat game byte-identical.
- **Culling, objects done:** in VR `recalcBounds` (mechs, vehicles,
  generic objects, buildings, trees) culls by distance only
  (`Camera::drawsAll`); widening it changes nothing in the game (with
  `MC2_DRAW_ALL=1` in a flat game the state checksums match on every frame
  of the mc2_19 bench). Terrain objects (trees, buildings) are reached only
  through active terrain blocks, which stay the game camera's: the blocks
  and vertices the eyes may see get their own draw-only flags
  (`objBlockInfo[].drawn`, `objVertexDrawn`), render and renderShadows use
  those in VR, and `GameObjectManager::updateDrawnAppearances` updates only
  the appearance of objects drawn but outside the game camera's view.
  Quest 2: objects everywhere in the window, 66.0 fps (from 67.0).
- **Geometry behind the camera:** the GPU mesh shader divided by `w` itself
  and kept `abs()`, so geometry behind the camera was drawn mirrored (huge
  dark wedges once objects behind the game camera were drawn; the likely
  bomber artifact). The engine's `w` is negative in front of the camera; the
  shader now keeps `-rhw`, and the GPU clips what is behind.
- **To do: the terrain window's edge** is visible at a headset's field of
  view (a hard border, then the cloud layer as a grey wall): a larger window
  for VR, or haze pulled in to the window's radius.
- Some shimmer remains on distant mechs; spotlights are missing in the
  eyes; the sky dome shows as a grey ring at the horizon; artifacts were seen
  when the bench camera passes through the bombers.
- No controllers yet (Phase 3): the mouse and keyboard do not reach the
  headset build.
- The Guardian (boundary) takes the headset to passthrough when it thinks
  it is outside its play area; the session then stays VISIBLE, not
  FOCUSED.

## Open questions

- (Answered by the Phase 1 audit: projection is mixed with state in every
  update-time projecting function, and a head-following logic eye would change
  gameplay, so the logic eye stays un-tracked.)
- Picking through an un-tracked logic eye only reaches what the game camera
  sees. If the player looks or points outside it, either recentre the game
  camera on the pointed-at spot first, or pick against object bounds in world
  space (the refinement noted under "Approach").
- Is the CPU route fast enough on Quest 2 at 72 Hz, or is phase 6 required?
- Mode 1 scale: is 1:1 comfortable with smooth panning, and which giant scale
  feels best? Seated or standing?
- Tabletop edge: the terrain window is a square larger than the table. Clip at
  the table rectangle with stencil, `gl_ClipDistance` (GLES needs
  `GL_EXT_clip_cull_distance`), or shrink the window to the table patch?
- Real mech size in world units (bounding boxes in the data) for the tabletop
  scale table.
- Interface: one big panel with the existing 800x600 UI, or split it (tac map
  on the table edge, control panel on the wrist)? Start with one panel.
