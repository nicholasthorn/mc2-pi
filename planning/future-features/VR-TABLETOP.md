# VR tabletop (Meta Quest 2)

Status: feasibility study, nothing built. Written 2026-10-29 from a read of the
code on `pi-port`; file and function names were checked then, line numbers were
not recorded on purpose (grep the symbol).

## Problem

Play MC2 in a VR headset with the battlefield as a 3D model on a virtual table:
the player walks around it, leans in, and commands units with tracked
controllers. The 2D interface (control panel, tac map, menus) floats on panels
beside the table. Target: standalone Quest 2 (and Quest 3/3S, same stack), with
PC VR over Quest Link as the development route.

## Scope

In: stereo rendering of the mission world as a scaled diorama, head tracking,
controller pointing and selection, the existing 800x600 interface on floating
panels, Android/Quest packaging for sideloading.

Out (for now): first-person cockpit, hand tracking, redesigned interface,
multiplayer, store distribution (the game data is under the original Microsoft
EULA; sideload only), Vulkan.

## Affected areas

Renderer (GLES and desktop GL), camera (`mclib/camera.*`), terrain
(`mclib/terrain.cpp`, `mclib/quad.cpp`), object appearances (`mclib/*actor.cpp`,
`mech3d.cpp`, `tgl.cpp`), the texture/render queue (`mclib/txmmgr.cpp`), the
mission frame (`code/mission.cpp`, `code/gamecam.cpp`), input (`missiongui.cpp`,
`GameOS/gameos/gameos_input.cpp`), platform/build (Android, OpenXR).

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

Keep the game's own camera as a hidden mono **logic eye**. It drives everything
it drives today: game update, active-block flags, culling decisions that affect
gameplay, mouse picking and the interface. Add two **render eyes** that only
draw.

Per frame:

1. Normal `Mission::update` with the logic eye (unchanged gameplay).
2. For each render eye (left, right):
   - set the camera matrices from OpenXR: `cameraOrigin` = table transform x
     eye pose, `cameraToClip` from the eye's asymmetric FOV (`XrFovf`),
     viewport = swapchain size;
   - re-run the projection-only steps: terrain vertex projection (the
     `projectZ` part of `Terrain::geometry`, **without** touching the
     active-block flags), appearance transforms (the transform part of
     `updateAppearancesOnly`, **without** advancing animation), crater/effect
     projection;
   - render the world pass into that eye's swapchain image;
   - reset the TGL pools between eyes (or size them for two passes).
3. Draw the interface once into an offscreen 800x600 texture; show it as an
   OpenXR quad layer (or textured panels) beside the table.

This needs the projection side effects separated from the state side effects in
`Terrain::geometry`, the appearance `update()` functions and the effects. The
gait double-step fix (`495f7ba`, steps at most once per `turn`) already guards
one known case; the rest need an audit.

Picking stays on the logic eye: intersect the controller ray with the table
(terrain heights via `Terrain::getTerrainElevation`), project the hit point
through the logic eye to a screen position, and feed that as the mouse. All
existing selection code (`inverseProject`, screen bounds, `PerPolySelect`) then
works unchanged. Accuracy for tall objects is approximate; refine later with a
ray test against object bounds if needed.

Rejected for a first version:

- **Moving all geometry to GPU transforms** (world-space vertex buffers, eye
  matrices in shaders, `GL_OVR_multiview2` single-pass stereo). Best
  performance, but a rewrite of terrain, effects and shadows. Keep it as the
  optimisation path (Phase 6).
- **Rendering mono and showing it on a screen in VR.** Works and is cheap, but
  it is not a tabletop.

## Tabletop specifics

- **Whole map visible.** The terrain only projects a square window of
  `Terrain::visibleVerticesPerSide` around the camera (`MapData::update`
  centres it). The tabletop needs a window covering the map, or the part of it
  on the table. Terrain cost scales with that.
- **Scale and depth.** Maps are on the order of 10,000+ world units across
  (`worldUnitsPerVertex` = 128); a table is about a metre. Build the table
  transform into `cameraOrigin`. Use a 24-bit depth swapchain (the window
  context asks for 16-bit today) and recheck near/far.
- **Things that assume a top-down camera:** sky dome (drop it), distance fog
  (rethink), letterbox/fade, compass, `cosHalfFOV` and screen-rect culling in
  `recalcBounds`, camera-relative LOD. Each needs a tabletop setting.
- **World-anchored overlays** (selection brackets, health bars, waypoints,
  `FloatHelp`) are projected with `projectZ` in `missiongui.cpp` (5 sites) and
  game objects. They must be projected per eye so they sit at the right depth,
  or be redrawn as 3D billboards.

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
  the projection steps three times (logic eye + two eyes), with a terrain
  window larger than today. The XR2's CPU is much faster than a Pi 4's, but
  there are no numbers yet.
- Measure first: the mission already times its stages with `ProfileTime`
  (`MCTimeCameraUpdate`, `MCTimeTerrainGeometry`, ...). Record those on the Pi
  and on a PC with the whole-map terrain window to estimate the stereo cost.

## Phases

| Phase | What | Exit criterion |
|---|---|---|
| 0. Measure | Frame-stage times on Pi and PC; whole-map terrain window cost | Numbers in this file; go/no-go on the CPU route |
| 1. Desktop stereo, no VR | Logic-eye / render-eye split; side-by-side stereo debug mode on a normal window | Both halves correct; nightly smoke runs unchanged; no double animation steps |
| 2. OpenXR on PC | Swapchains, frame loop, head pose to eye cameras, interface on a quad layer, mouse still works | Mission playable in the headset over Link with mouse and keyboard |
| 3. Tabletop | Table transform, whole-map terrain, sky/fog/letterbox/compass settings, 24-bit depth, grab/scale/rotate the table | Comfortable tabletop view of a full mission |
| 4. Controllers | Ray to table, logic-eye virtual mouse, button mapping, ray interaction with interface panels | Full mission playable with controllers only |
| 5. Quest standalone | Android build, OpenXR GLES binding, storage paths, packaging, data sideload, foveation, resolution tuning | Mission runs at 72 Hz on Quest 2 |
| 6. Optimise (if needed) | Late-bind GPU shape matrices at flush (`mw_` x eye view-projection), multiview, GPU terrain | Frame budget met with margin |

Phases 1 and 2 can be tested on the existing CI (phase 1 needs no headset).
Phase 0 decides whether phase 6 is needed before phase 5.

## Verification

- Phase 1 in CI: a smoke run with the stereo debug mode on, under llvmpipe,
  alongside the normal smoke runs; compare game outcomes with stereo off.
- Phases 2 to 5 need a headset; record what was run where in `PROGRESS.md`.

## Open questions

- How many update-time functions mix projection with state changes? (Audit:
  `Terrain::geometry`, every `*Appearance::update`, craters, weapon effects,
  `gosFX`, weather, `FloatHelp`.)
- Does widening the terrain window change gameplay through the active-block
  flags? (It must not: keep those on the logic eye.)
- Is the CPU route fast enough on Quest 2 at 72 Hz, or is phase 6 required?
- Comfort: table height and scale defaults, seated vs standing, snap vs smooth
  table rotation.
- Interface: one big panel with the existing 800x600 UI, or split it (tac map
  on the table edge, control panel on the wrist)? Start with one panel.
