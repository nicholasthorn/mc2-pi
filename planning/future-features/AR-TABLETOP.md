# AR tabletop on a phone (ARCore)

Status: specced, not started. Written 2026-11-15 from a read of the VR code
(`gos_xr`, `gos_GetVREyeViews`, the camera's render eyes) and the phone port
(`MOBILE.md`, branch `android-flat`). Builds on `VR-TABLETOP.md` (mode 2, the
tabletop option, its phase 5) and needs the phone port's phases 0 to 3. File
and function names were checked then; grep the symbol for lines. ARCore API
names are from its NDK C API (`arcore_c_api.h`) and need checking against the
SDK version picked.

## Problem

Show a battle on a real table through a phone's camera: the map sits on the
table at miniatures scale, the phone is a window onto it, and walking round
the table or leaning in moves the view. The same idea as the Quest's tabletop
mode, with a phone (held in the hand) instead of a headset, so anyone with a
recent Android phone can see it.

Who for: showing the game off, short sessions, and later two or more phones
round one table (shared anchors, with `NETWORKING.md`). Long missions will
stay more comfortable flat: holding a phone up tires the arm.

## Scope

In:

- ARCore world tracking on Android phones and tablets that support it (the
  Pixel 9 first).
- Placing the map on a detected table (a horizontal plane), or on a printed
  mat (an ARCore Augmented Image), at a scale from about 1:285 to 1:850.
- The camera image behind the game; the world drawn from the phone's pose with
  the camera's own projection.
- Touch through a ray from the phone's camera, onto the terrain, into the
  existing picking (as the Quest controllers do). The touch layer of
  `MOBILE.md` (gestures, command bar) otherwise unchanged.
- A "hold view" toggle: freeze the camera where it is, so the phone can be
  put down or held comfortably and played flat.
- A flat fallback: no ARCore, no camera permission, or tracking lost for long:
  the game carries on as the flat phone build.

Out (for now):

- Markers of our own (OpenCV ArUco and the like): ARCore's Augmented Images
  give a marker with tracking that survives the marker leaving the frame;
  rolling our own means camera calibration, jitter filtering and no tracking
  without the marker in view.
- Occlusion by real objects (ARCore's Depth API): a later polish.
- Light estimation: later.
- Shared anchors across phones (Cloud Anchors): later, with multiplayer.
- iOS (ARKit has the same features; with `MOBILE.md`'s iOS work, later).
- The Quest's own passthrough mixed reality: a separate option of
  `VR-TABLETOP.md`.

## Affected areas

| Area | What |
|---|---|
| Build | ARCore's AAR (`com.google.ar:core`) unpacked by `scripts/android/build.sh`: `classes.jar` into the dex step, `libarcore_sdk_c.so` into the APK, `arcore_c_api.h` for the compile; a third flavour or a CMake option (`MC2_ARCORE`), off everywhere else. CI builds it (Android job). |
| Android | `MC2Activity` (or an AR subclass): camera permission at runtime, `ArCoreApk_requestInstall` (Google Play Services for AR); manifest: the camera permission, `android.hardware.camera.ar` not required, `com.google.ar.core` meta-data "optional". |
| Platform | A `gos_ar` beside `gos_xr`: session start, pause and resume with the app lifecycle (`MOBILE.md` phase 3), one eye view a frame, the camera texture. GameOS only; the game sees the same `gos_GetVREyeViews` / world scale / pointer ray interface as VR. |
| Renderer | The camera image as the first thing drawn: a full-screen quad from a `GL_TEXTURE_EXTERNAL_OES` texture (`samplerExternalOES`, `GL_OES_EGL_image_external_essl3`). GLES only: an exception to "every shader also builds as GLSL 4.20" (`shader-conventions.md`), kept out of the desktop shader list and validated as ES only. No sky, no fog or haze, depth cleared but not colour. |
| Engine | The tabletop mode of `VR-TABLETOP.md` (phase 5): anchor at table height, the map clipped to the table's edge, scroll the map across the table. Shared with the Quest. |
| Game | Mostly none: the camera already takes render eyes from GameOS (`Camera` in `mclib/camera.cpp`, `gos_GetVREyeViews`). Settings for scale and hold view. |

## Approach

### One render eye from ARCore

The Quest build already separates a **logic eye** (the game's own camera:
gameplay, culling that affects gameplay, picking, the interface) from **render
eyes** placed by the headset (`VR-TABLETOP.md`, "one logic eye, two render
eyes"; `gos_GetVREyeViews`). An AR phone is one render eye:

- Each frame, on the game's GL thread: `ArSession_update`, then the camera's
  pose (`ArCamera_getPose`) and projection (`ArCamera_getProjectionMatrix`,
  near and far to suit table scale). The projection is asymmetric and must
  match the screen's aspect exactly, or the map slides over the table as the
  phone moves; it is converted to the eye view's four FOV angles.
- Eye position = anchor transform (table pose x world scale) applied to the
  camera pose, as `gos_xr` does with the head. `gos_GetVRWorldScale` gives the
  table scale.
- Draw: camera image quad, then the world pass into the window (no swapchain:
  the phone's own framebuffer), then the interface on top as today.

`ArSession_setDisplayGeometry` follows the window's size and rotation (the
`SDL_WINDOWEVENT_SIZE_CHANGED` handling of the phone port), and
`ArFrame_transformCoordinates2d` gives the camera image's UVs for the screen.

### Placing the table

1. **Plane:** ARCore finds horizontal planes; the player taps the table, an
   anchor (`ArSession_acquireNewAnchor` / a hit test) is made there, and the
   map's centre sits on it. Rotate with two fingers before confirming.
2. **Mat (optional):** a printed image registered in an
   `ArAugmentedImageDatabase` with its real width; once seen, the anchor and
   the scale come from it (the map fits the mat). Precise and repeatable,
   handy for a demo. ARCore keeps tracking after the mat leaves the frame.

A plain white or glossy table is hard to track: ARCore needs texture. The mat
doubles as the fix.

### Touch

A tap is a ray from the camera through the tapped pixel (inverse of the eye's
projection), met with the terrain, then projected through the logic eye to a
mouse position: what the Quest's controller ray already does
(`gos_GetVRPointerRay` / `gos_SetVRWorldPointer`). Selection, orders and the
command bar then work unchanged. Taps on the interface stay screen taps.

Two-finger gestures move the **map across the table** (pan, rotate, zoom as
scale), not a camera: the camera is the phone.

### Hold view

A button freezes the eye where it is (the last pose stays) and pauses
tracking; the phone can then be held at any angle and played as a flat game of
that view. Tapping it again picks the live pose back up.

### Rejected

- **Our own marker tracking** (OpenCV ArUco, `solvePnP` on camera frames):
  calibration per phone, jitter, latency, and no pose without the marker in
  frame. ARCore does it better, including markers.
- **A Unity or Godot AR front end over the game:** the game is not a
  library; the render-eye path already exists in the engine.
- **WebXR in a browser:** no native engine there (`MOBILE.md` rejected the
  web build).
- **Rendering the whole map on the table:** as `VR-TABLETOP.md`, at
  miniatures scale the table holds about 20 x 10 terrain vertices, inside
  today's window.

## Phases

| Phase | What | Exit criterion | Size |
|---|---|---|---|
| 0. Prerequisites | Phone port phases 0 to 3; the aspect ratio right (no stretch) | A mission on the phone, correct aspect | (MOBILE.md) |
| 1. Proof of concept | ARCore in the build, session and camera background, one render eye from the camera pose, map anchored on a tapped plane at a fixed scale, no clipping or table edge | mc2_01 standing still on a real table while walking round it; frame rate and tracking noted | M |
| 2. Tabletop mode | `VR-TABLETOP.md` phase 5 (shared): table clip, edge, no sky, scale range, scroll the map across the table | A comfortable tabletop view, phone and Quest | M |
| 3. Play | Tap ray picking, gestures on the map, hold view, lifecycle (pause and resume the session), flat fallback | A mission played to the end in AR on the Pixel | M |
| 4. Mat | Augmented Image anchor and scale | The map snaps to a printed mat at its size | S |
| 5. Later | Depth occlusion, light estimation, shared anchors for multiplayer | Each its own exit | per item |

About three to five weeks with agents helping for phases 1 to 4, phase 2
counted once for both the phone and the Quest.

## Performance

The Quest 2 ran mc2_19 at 58.6 fps with one view (`VR-TABLETOP.md`, Quest 2
results); a Pixel 9 (Tensor G4, Mali-G715) is faster on the CPU. ARCore adds
its own tracking work and the camera runs at 30 fps; `ArSession_update` can
block until the next camera frame, so the game may be paced at 30 Hz (as the
Quest steps the game at 30 Hz and draws at 72). Measure with `-bench` and the
stage times in phase 1.

## Verification

- CI: the AR build compiles (Android job); the ES-only shader validates as ES.
- Desktop: nothing changes (the AR code is not built); the flat and VR builds'
  smoke and bench runs unchanged.
- Device (`planning/HARDWARE-TESTS.md` when phases land): the Pixel 9 first,
  then a Samsung (Mali or Adreno) and a tablet. Tracking on a textured and a
  plain table, the mat, walking round, the app sent to the background and
  back, frame rate.

## Open questions

- Scale: does 1:285 (35 to 42 mm mechs, 0.5 x 0.25 km of map on a dining
  table) read well on a 6 inch screen, or does a phone want 1:500 and more map?
- How much of the 800x600 interface to keep over the camera view: the control
  panel covers the table's near edge; a slimmer AR layout may be needed.
- Pacing: does the game step at the camera's 30 Hz, or draw at 60 with the
  last pose?
- One APK with AR optional (ARCore "optional" in the manifest) or a separate
  AR flavour?
