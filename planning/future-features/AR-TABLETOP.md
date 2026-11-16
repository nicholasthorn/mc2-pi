# AR tabletop: the battle on a real table (Quest and phones)

Status: in progress (branch `ar-tabletop`, started 2026-11-16). Phase 1 on
the Quest works in a Quest 2: the battle stands on the table and reads as
strongly 3D (see "Why it looks more 3D than VR"); the phone side is not
started. Written 2026-11-15 as `AR-PHONE.md` (phones only); widened
2026-11-16 to cover the Quest the same way, scenarios for a table, and
headsets and phones sharing one table. Builds on `VR-TABLETOP.md` (mode 2, its
phase 5) and `MOBILE.md` (the phone port). ARCore API names are from its NDK C
API (`arcore_c_api.h`) and need checking against the SDK version picked.

## Problem

Show a battle on a real table: the map sits on the table at miniatures
scale, and walking round the table or leaning in moves the view. A Quest
headset sees it through passthrough, a phone through its camera (held in the
hand as a window onto it). Both are to work **the same way**: the same table,
the same scale, the same controls where they can be, so that later a headset
and phones round one table see one battle.

Who for: showing the game off, short sessions, and playing together round a
table. Long missions stay more comfortable flat or in the VR mode 1: holding a
phone up tires the arm.

The aim at this stage is to find out what is fun: build the least that puts a
battle on a table on both devices, then try it.

## One table core, two trackers

Everything that decides what the player sees on the table is shared; only
the tracker differs.

| | Quest (OpenXR) | Phone (ARCore) |
|---|---|---|
| Tracking | the headset's head and controller poses | `ArCamera_getPose` |
| The table | placed in front of the head (A), later on a marker | a tapped plane, later a marker (Augmented Image) |
| The room | `XR_FB_passthrough` layer under the eyes (Quest 2 grey, Quest 3 colour) | the camera image drawn first (`GL_TEXTURE_EXTERNAL_OES`) |
| Eyes | two | one |
| Pointer | the controller's aim ray | a ray through the tapped pixel |

The shared part:

- **`GameOS/gameos/utils/table_anchor`** (GL-free, unit tested): the table as
  a point and a heading in tracking space (both trackers use x right, y up,
  z back, metres); poses into the table's frame; settings (scale, where a
  table goes by default).
- **GameOS hands the game poses in the table's frame.** `gos_GetVREyeViews`
  and `gos_GetVRPointerRay` stay the interface; `gos_GetVRTabletop()` says
  they are on a table, and `gos_GetVRWorldScale()` is then the table's scale.
- **The game camera** (`Camera::vrBaseFor`): on the tabletop, the head is
  put on a basis at the point the game camera looks at, on the ground, with
  the camera's heading, level. So scrolling and turning the game camera move
  and turn the map across the table, as `VR-TABLETOP.md` mode 2 asks, with no
  new controls. The game camera does not follow the head (`gos_GetVRHead`).
- **No sky** on the table (`code/gamecam.cpp`).

### Scale

`MC2_TABLE_SCALE` world metres to a real one, default 600 (1:600): a 10 m mech
is under 2 cm tall and the round terrain window, the `VR-TABLETOP.md` mode 1
window with its fade, covers a dining table. The spec's range was 1:285 to
1:850; 50 to 5000 is allowed, to try. Which reads best on a 6 inch phone and in
a headset is an open question.

### Why it looks more 3D than VR

The picture is drawn the same way in both (the same eyes, terrain and
meshes); what differs is the scale of the viewer. In VR mode 1 the eyes are at
world scale 1: 6.4 cm apart, hundreds of metres above the ground, so the two
eyes' pictures barely differ and the battle reads as flat, far away. On the
table at 1:600 the eyes are, to the battle, 38 m apart and under a kilometre
from it, and the head's movements are 600 times larger too: strong stereo
depth and motion parallax, as when looking at a real model. The room round it
(passthrough) is a real reference for both. First seen in a Quest 2 on
2026-11-16 ("really compelling").

### The interface panel

In VR mode 1 the panel floats 1.5 m away, 30 degrees below the horizon. On
the table that put it over the table while further away than the table it
covered: the eyes had to cross to read it. On the tabletop it starts 8 degrees
above the horizon, as a board behind the table (A and the left stick still
move it). Not yet seen.

### The table's edge

The ground already fades out in a circle before the terrain window's edge
(`mclib/vr_edge_fade.h`) and the corners are not drawn. On the table that
fade is to the fog colour, so it shows as a misty ring over the room. First try
as it is; if it is ugly, the fade goes to transparent (the shaders' alpha) on
the tabletop.

### Passthrough on the Quest

The eyes are cleared to transparent black; after the world is drawn a
triangle at the far plane sets alpha 1 wherever something nearer was drawn
(depth test `GL_GREATER`), and the projection layer is blended by its alpha
over an `XR_FB_passthrough` layer. The manifest asks for
`com.oculus.feature.PASSTHROUGH` (not required). Without passthrough the
table stands in the fog colour, as a test.

## Scenarios for a table

The stock missions are made for a flat screen and a long campaign: big maps,
long walks, many units. On a table:

- only a patch of map is shown (the terrain window), so a mission that sends
  units across a big map means scrolling the map across the table a lot;
- the interface (control panel) covers the near edge of the table on a phone.

So a table wants **short, small scenarios**: a small map or a small area of
one, a few lances a side, an objective in view. A first list to try from the
stock and MC2X missions is to be picked by playing; later, missions made for
it (a `tabletop` flag in the mission file, or a list in the front end), and
for two or more players with `NETWORKING.md` (skirmish on a small map is the
obvious first). Not designed yet: the first step is to see what plays well.

## Playing together: shared markers

Headsets and phones round one table should see one battle in one place.
Each device tracks itself in its own space; what they need in common is the
table's pose. A **printed marker** on the table gives it:

- **Phones:** ARCore Augmented Images find a registered image of known size
  and keep tracking after it leaves the frame. The table is its centre and
  heading.
- **Quest:** third-party apps get no camera images on a Quest 2 (the Quest 3's
  Passthrough Camera API is newer and limited). So the player places the table
  on the marker: the controller's tip on two marked corners gives the centre
  and the heading. The same marker, the same table.
- **Meta's shared spatial anchors** (Quest to Quest only) and **Cloud
  Anchors** (ARCore, needs Google's service) are not cross-device and need an
  online service: rejected for this.

The game state then goes over the local network (`NETWORKING.md`; the Quest
can already join games over Wi-Fi), each device drawing its own view. The table
pose never needs to be sent: each device finds it from the marker.

## Phones: ARCore

Unchanged from the first version of this spec (`AR-PHONE.md`):

| Area | What |
|---|---|
| Build | ARCore's AAR (`com.google.ar:core`) unpacked by `scripts/android`: `classes.jar` into the dex step, `libarcore_sdk_c.so` into the APK, `arcore_c_api.h` for the compile. An `MC2_ARCORE` option, off everywhere else. |
| Android | Camera permission at runtime, `ArCoreApk_requestInstall`; manifest: the camera permission, `android.hardware.camera.ar` not required, `com.google.ar.core` meta-data "optional". |
| Platform | A `gos_ar` beside `gos_xr`: session start, pause and resume with the app lifecycle (`MOBILE.md` phase 3), one eye a frame in the table's frame (`table_anchor`), the camera texture. |
| Renderer | The camera image first: a full-screen quad from a `GL_TEXTURE_EXTERNAL_OES` texture (`samplerExternalOES`). GLES only, an exception to "every shader also builds as GLSL 4.20", kept out of the desktop list and validated as ES only. |

Each frame, on the game's GL thread: `ArSession_update`, then the camera's
pose and projection (asymmetric, matching the screen's aspect, near and far to
suit table scale), turned into the eye's four FOV angles. The eye's pose goes
through `table_anchor::toTable` with the table from a tapped plane
(`ArFrame_hitTest`, an anchor) or a marker. `ArSession_setDisplayGeometry`
follows the window; `ArFrame_transformCoordinates2d` gives the camera image's
UVs.

Touch: a tap on the world is a ray from the camera through the pixel, into the
same world picking the Quest's controllers use. Two-finger gestures move the
map across the table (the game camera's scroll, turn and zoom). A "hold view"
toggle freezes the pose so the phone can be put down and played flat.

`ArSession_update` can block until the next camera frame (30 fps): the game
may be paced at 30 Hz, as the Quest steps it at 30 Hz.

## Phases

| Phase | What | Exit criterion | Size |
|---|---|---|---|
| 1. Quest table | `table_anchor`, the camera on a table, no sky, passthrough, A places the table | mc2_01 on a real table in the headset, walking round it | S |
| 2. Phone table | ARCore build, session, camera image, one eye, a tapped plane | mc2_01 standing still on a real table through the Pixel 9 | M |
| 3. Play | Pointer and tap picking on the table, gestures, hold view, lifecycle, flat fallback; the table's edge; scale and interface tuned | A short mission played to the end on each | M |
| 4. Scenarios | Pick and mark missions that play well on a table | A list in the front end | S |
| 5. Shared table | Marker on both (Augmented Image; two-corner placement on the Quest), then multiplayer over `NETWORKING.md` | A headset and a phone see one battle on one table | M to L |
| Later | Depth occlusion, light estimation, iOS | each its own | |

## Verification

- Unit tests: `table_anchor` (test_table_anchor.cpp).
- CI: the Quest and phone APKs build (Android job); desktop and Pi builds
  unchanged (the tabletop is off unless a tracker turns it on).
- Device (`planning/HARDWARE-TESTS.md` as phases land): the Quest 2 (grey
  passthrough), then the Pixel 9; a textured and a plain table; walking round;
  frame rate.

## Open questions

- Scale: 1:600 by default; what reads best on a phone and in a headset?
- The interface over the table: the control panel covers the near edge on a
  phone; in the headset the HUD panel floats below the view as in mode 1.
- The fade at the table's edge: misty ring or transparent?
- Pacing on the phone: 30 Hz steps with the camera, or 60 with the last pose?
- One APK with ARCore optional, or an AR flavour?
- Which missions play well on a table?
