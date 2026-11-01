# VR: head-tracked game view, tabletop option (Meta Quest 2)

Status: feasibility study, nothing built. Written 2026-10-29 from a read of the
code on `pi-port`, revised 2026-11-01 (requirements changed: play the game's
own view in VR first; tabletop becomes an option; no whole-map rendering).
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
- **At roughly 1:100 to 1:300 ("giant")** the camera height becomes 1 to 3 m
  above the ground, so stereo depth and leaning work, and mechs are a few
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

The logic eye **follows the head**: its pose is the anchor times the centre
of the two eyes, and its FOV is widened to cover both eyes (the headset sees
about 90 to 100 degrees, the game camera 40). That keeps object culling and
appearance updates covering what the player can see. The terrain window stays
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
- **Culling:** `cosHalfFOV` and the screen-rect test in `recalcBounds` must use
  the widened logic-eye FOV.
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
  terrain window. The widened logic-eye FOV adds objects to transform. The
  XR2's CPU is much faster than a Pi 4's, but there are no numbers yet.
- Measure first: `-bench` now reports per-stage times (`<stage>_ms`, see
  `bench/README.md`) and `-benchfov` forces a headset-wide FOV.

## Phase 0 results (PC, preliminary)

Run 2026-11-01: `bench/mc2_19.fit` (3600 frames, the player's lance against 20 enemies), desktop GL
build, Mesa llvmpipe under Xvfb in WSL2, one run per FOV. Another session's
cppcheck kept the load average near 30 on 16 threads throughout, so the
wall-clock times are noisy (the 100-degree run came out faster than the
40-degree one). Read the deterministic counts and the shares within each run,
not the absolute times.

| Per frame, whole run | FOV 40 | FOV 100 |
|---|---|---|
| draws | 696 | 707 |
| triangles | 6022 | 7578 (+26%) |
| `logic_ms` / `render_ms` | 33.6 / 30.8 | 22.8 / 21.5 |
| `terrain_geom_ms` | 0.59 | 0.56 |
| `objects_ms` (of which `shape_xform_ms`) | 8.36 (3.99) | 6.12 (2.95) |
| `world_render_ms` | 20.75 | 14.72 |
| `ui_render_ms` | 8.67 | 5.83 |
| per-eye work, share of the CPU frame | 39% | 41% |

Per-eye work is `terrain_geom` + `shape_xform` + `world_render`; the CPU frame
is `logic` + `render` (`swap` here is llvmpipe rasterizing, which is GPU work
on real hardware).

- **Stereo costs roughly 1.4 to 1.5 times the mono CPU frame**: a second eye
  repeats about 40% of it, plus the logic eye's own projection (about 7%) if
  its results are not reused. Under llvmpipe `world_render` includes the
  driver's CPU work, which a hardware driver does more cheaply, so this is an
  upper bound for that part.
- Most of `logic_ms` is game logic outside the measured stages (pathing, AI,
  sensors, collisions, the mission script). It runs once, not per eye.
- A wider FOV leaves terrain cost flat: the projected window is a fixed vertex
  grid (`GameVisibleVertices`). It adds about a quarter more triangles, mostly
  objects now in view.
- **The terrain window's edge is in view at 100 degrees** from the game's
  camera height: the screenshot shows the map as a slab floating in black,
  beyond the haze (mc2_19 is a night map). Mode 1 needs a larger window
  (`terrain_geom` grows with its square, and is 0.6 ms at 60 vertices), haze
  pulled in to the window's radius, or a lower camera. The same picture shows
  the tabletop look comes almost for free: the window already reads as a
  diorama.

Still to do for the go/no-go: a quiet PC run, and the Pi (on its own display):

```bash
scripts/bench.sh --game-dir ../mc2game --build-dir build --renderers gpu --scenario bench/mc2_19.fit --label "vr-p0 fov40"
scripts/bench.sh --game-dir ../mc2game --build-dir build --renderers gpu --scenario bench/mc2_19.fit --fov 100 --label "vr-p0 fov100"
```

## Phases

| Phase | What | Exit criterion |
|---|---|---|
| 0. Measure | Frame-stage times in `-bench` output; PC and Pi runs at the normal window, with a wide FOV | Numbers in this file; go/no-go on the CPU route |
| 1. Desktop stereo, no VR | Logic-eye / render-eye split; side-by-side stereo debug mode on a normal window | Both halves correct; nightly smoke runs unchanged; no double animation steps |
| 2. OpenXR on PC (mode 1) | Swapchains, frame loop, anchor + world scale, head pose to logic and render eyes, interface on a quad layer, mouse still works | Mission playable in the headset over Link with mouse and keyboard, at 1:1 and giant scale |
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

## Open questions

- How many update-time functions mix projection with state changes? (Audit:
  `Terrain::geometry`, every `*Appearance::update`, craters, weapon effects,
  `gosFX`, weather, `FloatHelp`.)
- Does a head-following logic eye change gameplay through the active-block
  flags or camera-relative logic (it must not)? If it does, pin those to the
  un-tracked game camera and use the head pose for culling only.
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
